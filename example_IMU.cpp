#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <Eigen/Dense>

// Dear ImGui and SDL3 headers
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

namespace {

constexpr double kEpsilon = 1e-12;

Eigen::Vector3d safeNormalized(const Eigen::Vector3d& v) {
    const double norm = v.norm();
    if (norm <= kEpsilon || !std::isfinite(norm)) {
        return Eigen::Vector3d::Zero();
    }
    return v / norm;
}

// Converts a unit quaternion [q0, q1, q2, q3] to a 3x3 rotation matrix.
Eigen::Matrix3d rotate(const Eigen::Vector4d& quaternion) {
    Eigen::Vector4d q = quaternion;
    const double norm = q.norm();
    if (norm <= kEpsilon || !std::isfinite(norm)) {
        return Eigen::Matrix3d::Identity();
    }
    q /= norm;

    const double q0 = q(0);
    const double q1 = q(1);
    const double q2 = q(2);
    const double q3 = q(3);

    Eigen::Matrix3d rotation;
    rotation(0, 0) = 1.0 - 2.0 * (q2 * q2 + q3 * q3);
    rotation(0, 1) = 2.0 * (q1 * q2 - q0 * q3);
    rotation(0, 2) = 2.0 * (q1 * q3 + q0 * q2);

    rotation(1, 0) = 2.0 * (q1 * q2 + q0 * q3);
    rotation(1, 1) = 1.0 - 2.0 * (q1 * q1 + q3 * q3);
    rotation(1, 2) = 2.0 * (q2 * q3 - q0 * q1);

    rotation(2, 0) = 2.0 * (q1 * q3 - q0 * q2);
    rotation(2, 1) = 2.0 * (q2 * q3 + q0 * q1);
    rotation(2, 2) = 1.0 - 2.0 * (q1 * q1 + q2 * q2);
    return rotation;
}

// State transition matrix for quaternion kinematics.
Eigen::Matrix4d stateTransition(const Eigen::Vector3d& angularVelocity,
                                double dt) {
    const double wx = angularVelocity(0);
    const double wy = angularVelocity(1);
    const double wz = angularVelocity(2);

    Eigen::Matrix4d omega;
    omega << 0.0, -wx, -wy, -wz,
             wx, 0.0, wz, -wy,
             wy, -wz, 0.0, wx,
             wz, wy, -wx, 0.0;

    return Eigen::Matrix4d::Identity() + 0.5 * dt * omega;
}

// Noise-input matrix.
Eigen::Matrix<double, 4, 3> noiseInput(const Eigen::Vector4d& q) {
    const double q0 = q(0);
    const double q1 = q(1);
    const double q2 = q(2);
    const double q3 = q(3);

    Eigen::Matrix<double, 4, 3> matrix;
    matrix << -q1, -q2, -q3,
               q0, -q3,  q2,
               q3,  q0, -q1,
              -q2,  q1,  q0;
    return 0.5 * matrix;
}

// Numerical measurement Jacobian for accelerometer and magnetometer.
Eigen::Matrix<double, 6, 4> measurementJacobian(
    const Eigen::Vector4d& q,
    const Eigen::Vector3d& gravityReference,
    const Eigen::Vector3d& magneticReference) {

    constexpr double epsilon = 1e-6;
    Eigen::Matrix<double, 6, 4> jacobian =
        Eigen::Matrix<double, 6, 4>::Zero();

    const Eigen::Vector3d accelBase =
        safeNormalized(-rotate(q) * gravityReference);
    const Eigen::Vector3d magBase =
        safeNormalized(rotate(q) * magneticReference);

    for (int i = 0; i < 4; ++i) {
        Eigen::Vector4d perturbed = q;
        perturbed(i) += epsilon;
        perturbed.normalize();

        const Eigen::Vector3d accelPerturbed =
            safeNormalized(-rotate(perturbed) * gravityReference);
        const Eigen::Vector3d magPerturbed =
            safeNormalized(rotate(perturbed) * magneticReference);

        jacobian.block<3, 1>(0, i) =
            (accelPerturbed - accelBase) / epsilon;
        jacobian.block<3, 1>(3, i) =
            (magPerturbed - magBase) / epsilon;
    }

    return jacobian;
}

class IMUTracker {
public:
    struct InitData {
        Eigen::Vector3d gravityReference = Eigen::Vector3d::Zero();
        double gravityMagnitude = 0.0;
        Eigen::Vector3d magneticReference = Eigen::Vector3d::Zero();
        double gyroNoise = 0.0;
        double gyroBiasNorm = 0.0;
        double accelerometerNoise = 0.0;
        double magnetometerNoise = 0.0;
        Eigen::Vector3d gyroBias = Eigen::Vector3d::Zero();
    };

    explicit IMUTracker(double samplingRate = 100.0,
                        int gyroOrder = 1,
                        int accelerometerOrder = 2,
                        int magnetometerOrder = 3)
        : sampling_(samplingRate),
          dt_(samplingRate > 0.0 ? 1.0 / samplingRate : 0.0),
          gyroStart_(columnStart(gyroOrder)),
          accelerometerStart_(columnStart(accelerometerOrder)),
          magnetometerStart_(columnStart(magnetometerOrder)) {
        if (samplingRate <= 0.0) {
            throw std::invalid_argument("Sampling rate must be positive.");
        }
    }

    InitData initialize(const Eigen::MatrixXd& data,
                        double gyroNoiseScale = 100.0,
                        double accelerometerNoiseScale = 100.0,
                        double magnetometerNoiseScale = 10.0) const {
        validateData(data);
        if (data.rows() == 0) {
            throw std::invalid_argument("Initialization data is empty.");
        }

        const Eigen::MatrixXd gyro =
            data.block(0, gyroStart_, data.rows(), 3);
        const Eigen::MatrixXd accelerometer =
            data.block(0, accelerometerStart_, data.rows(), 3);
        const Eigen::MatrixXd magnetometer =
            data.block(0, magnetometerStart_, data.rows(), 3);

        const Eigen::Vector3d accelMean = accelerometer.colwise().mean();
        const Eigen::Vector3d magMean = magnetometer.colwise().mean();
        const Eigen::Vector3d gyroMean = gyro.colwise().mean();

        InitData init;
        init.gravityReference = -accelMean;
        init.gravityMagnitude = init.gravityReference.norm();
        init.magneticReference = safeNormalized(magMean);
        init.gyroBias = gyroMean;
        init.gyroBiasNorm = gyroMean.norm();

        if (init.gravityMagnitude <= kEpsilon) {
            throw std::runtime_error(
                "Cannot initialize: mean accelerometer magnitude is zero.");
        }
        if (init.magneticReference.isZero(kEpsilon)) {
            throw std::runtime_error(
                "Cannot initialize: mean magnetometer magnitude is zero.");
        }

        const Eigen::Vector3d accelVariance =
            (accelerometer.rowwise() - accelMean.transpose())
                .colwise()
                .squaredNorm() /
            static_cast<double>(accelerometer.rows());
        const Eigen::Vector3d gyroVariance =
            (gyro.rowwise() - gyroMean.transpose())
                .colwise()
                .squaredNorm() /
            static_cast<double>(gyro.rows());
        const Eigen::Vector3d magVariance =
            (magnetometer.rowwise() - magMean.transpose())
                .colwise()
                .squaredNorm() /
            static_cast<double>(magnetometer.rows());

        init.gyroNoise = gyroNoiseScale * gyroVariance.norm();
        init.accelerometerNoise =
            accelerometerNoiseScale * accelVariance.norm();
        init.magnetometerNoise =
            magnetometerNoiseScale * magVariance.norm();

        // Keep covariance matrices nonsingular even for perfectly constant input.
        init.gyroNoise = std::max(init.gyroNoise, 1e-9);
        init.accelerometerNoise = std::max(init.accelerometerNoise, 1e-9);
        init.magnetometerNoise = std::max(init.magnetometerNoise, 1e-9);
        return init;
    }

    std::tuple<Eigen::MatrixXd, std::vector<Eigen::Vector3d>> attitudeTrack(
        const Eigen::MatrixXd& data,
        const InitData& init) const {
        validateData(data);

        const Eigen::Index samples = data.rows();
        Eigen::MatrixXd gyro =
            data.block(0, gyroStart_, samples, 3).rowwise() -
            init.gyroBias.transpose();
        const Eigen::MatrixXd accelerometer =
            data.block(0, accelerometerStart_, samples, 3);
        const Eigen::MatrixXd magnetometer =
            data.block(0, magnetometerStart_, samples, 3);

        Eigen::MatrixXd navigationAcceleration =
            Eigen::MatrixXd::Zero(samples, 3);
        std::vector<Eigen::Vector3d> orientations;
        orientations.reserve(static_cast<std::size_t>(samples));

        Eigen::Matrix4d covariance = Eigen::Matrix4d::Identity() * 1e-10;
        Eigen::Vector4d q(1.0, 0.0, 0.0, 0.0);

        for (Eigen::Index t = 0; t < samples; ++t) {
            const Eigen::Vector3d angularVelocity = gyro.row(t).transpose();
            const Eigen::Vector3d acceleration =
                accelerometer.row(t).transpose();
            const Eigen::Vector3d magneticField =
                magnetometer.row(t).transpose();

            // (1) Propagation
            const Eigen::Matrix4d transition =
                stateTransition(angularVelocity, dt_);
            const Eigen::Matrix<double, 4, 3> input = noiseInput(q);
            const Eigen::Matrix4d processNoise =
                std::pow(init.gyroNoise * dt_, 2) *
                (input * input.transpose());

            q = transition * q;
            if (q.norm() <= kEpsilon || !q.allFinite()) {
                throw std::runtime_error(
                    "Quaternion became invalid during propagation.");
            }
            q.normalize();
            covariance =
                transition * covariance * transition.transpose() + processNoise;

            const double accelNorm = acceleration.norm();
            const double magNorm = magneticField.norm();

            // (2) Measurement update. Skip it if either sensor vector is invalid.
            if (accelNorm > kEpsilon && magNorm > kEpsilon &&
                std::isfinite(accelNorm) && std::isfinite(magNorm)) {
                const Eigen::Vector3d measuredAcceleration =
                    acceleration / accelNorm;
                const Eigen::Vector3d measuredMagneticField =
                    magneticField / magNorm;
                const Eigen::Vector3d predictedAcceleration =
                    safeNormalized(-rotate(q) * init.gravityReference);
                const Eigen::Vector3d predictedMagneticField =
                    safeNormalized(rotate(q) * init.magneticReference);

                Eigen::Matrix<double, 6, 1> residual;
                residual << measuredAcceleration - predictedAcceleration,
                            measuredMagneticField - predictedMagneticField;

                const double accelVariance =
                    std::pow(init.accelerometerNoise / accelNorm, 2) +
                    std::pow(1.0 - init.gravityMagnitude / accelNorm, 2);
                const double magVariance =
                    std::pow(init.magnetometerNoise, 2);

                Eigen::Matrix<double, 6, 6> measurementNoise =
                    Eigen::Matrix<double, 6, 6>::Zero();
                measurementNoise.diagonal() <<
                    accelVariance, accelVariance, accelVariance,
                    magVariance, magVariance, magVariance;

                const Eigen::Matrix<double, 6, 4> jacobian =
                    measurementJacobian(
                        q, init.gravityReference, init.magneticReference);
                const Eigen::Matrix<double, 6, 6> innovationCovariance =
                    jacobian * covariance * jacobian.transpose() +
                    measurementNoise;

                Eigen::LDLT<Eigen::Matrix<double, 6, 6>> solver(
                    innovationCovariance);
                if (solver.info() == Eigen::Success) {
                    const Eigen::Matrix<double, 4, 6> covarianceTimesH =
                        covariance * jacobian.transpose();
                    const Eigen::Matrix<double, 4, 6> kalmanGain =
                        solver.solve(covarianceTimesH.transpose()).transpose();

                    q += kalmanGain * residual;
                    if (q.norm() <= kEpsilon || !q.allFinite()) {
                        throw std::runtime_error(
                            "Quaternion became invalid during correction.");
                    }
                    q.normalize();

                    // Joseph form is more numerically stable than P - KHP.
                    const Eigen::Matrix4d identity =
                        Eigen::Matrix4d::Identity();
                    const Eigen::Matrix4d correction =
                        identity - kalmanGain * jacobian;
                    covariance =
                        correction * covariance * correction.transpose() +
                        kalmanGain * measurementNoise * kalmanGain.transpose();
                }
            }

            covariance = 0.5 * (covariance + covariance.transpose());

            // (3) Save navigation-frame acceleration and X-axis orientation.
            Eigen::Vector4d conjugate = q;
            conjugate.tail<3>() *= -1.0;
            const Eigen::Matrix3d bodyToNavigation = rotate(conjugate);
            const Eigen::Vector3d accelerationNavigation =
                bodyToNavigation * acceleration + init.gravityReference;

            navigationAcceleration.row(t) = accelerationNavigation.transpose();
            orientations.push_back(
                bodyToNavigation * Eigen::Vector3d::UnitX());
        }

        return {navigationAcceleration, orientations};
    }

    Eigen::MatrixXd removeAccelerationError(
        Eigen::MatrixXd navigationAcceleration,
        double threshold = 0.2) const {
        const Eigen::Index samples = navigationAcceleration.rows();
        if (samples == 0) {
            return navigationAcceleration;
        }

        Eigen::Index movementStart = -1;
        for (Eigen::Index t = 0; t < samples; ++t) {
            if (navigationAcceleration.row(t).norm() > threshold) {
                movementStart = t;
                break;
            }
        }

        Eigen::Index movementEnd = -1;
        const Eigen::Vector3d finalAcceleration =
            navigationAcceleration.row(samples - 1).transpose();
        for (Eigen::Index t = samples - 1; t >= 0; --t) {
            if ((navigationAcceleration.row(t).transpose() - finalAcceleration)
                    .norm() > threshold) {
                movementEnd = t;
                break;
            }
        }

        if (movementStart < 0 || movementEnd <= movementStart ||
            movementEnd >= samples) {
            return navigationAcceleration;
        }

        const Eigen::Index tailLength = samples - movementEnd;
        if (tailLength <= 0) {
            return navigationAcceleration;
        }

        const Eigen::Vector3d drift =
            navigationAcceleration
                .block(movementEnd, 0, tailLength, 3)
                .colwise()
                .mean();
        const Eigen::Index movementLength = movementEnd - movementStart;
        const Eigen::Vector3d driftRate =
            drift / static_cast<double>(movementLength);

        for (Eigen::Index i = 0; i < movementLength; ++i) {
            navigationAcceleration.row(movementStart + i) -=
                ((static_cast<double>(i) + 1.0) * driftRate).transpose();
        }
        for (Eigen::Index i = movementEnd; i < samples; ++i) {
            navigationAcceleration.row(i) -= drift.transpose();
        }

        return navigationAcceleration;
    }

    Eigen::MatrixXd zeroVelocityUpdate(
        const Eigen::MatrixXd& navigationAcceleration,
        double threshold) const {
        const Eigen::Index samples = navigationAcceleration.rows();
        Eigen::MatrixXd velocities = Eigen::MatrixXd::Zero(samples, 3);

        Eigen::Index previousStationarySample = -1;
        bool stationary = false;
        Eigen::Vector3d velocity = Eigen::Vector3d::Zero();

        for (Eigen::Index t = 0; t < samples; ++t) {
            const Eigen::Vector3d acceleration =
                navigationAcceleration.row(t).transpose();

            if (acceleration.norm() < threshold) {
                if (!stationary) {
                    const Eigen::Index interval = t - previousStationarySample;
                    if (interval > 0) {
                        const Eigen::Vector3d predictedVelocity =
                            velocity + acceleration * dt_;
                        const Eigen::Vector3d driftRate =
                            predictedVelocity / static_cast<double>(interval);

                        for (Eigen::Index i = 0; i < interval - 1; ++i) {
                            const Eigen::Index row =
                                previousStationarySample + 1 + i;
                            if (row >= 0 && row < samples) {
                                velocities.row(row) -=
                                    ((static_cast<double>(i) + 1.0) * driftRate)
                                        .transpose();
                            }
                        }
                    }
                }

                velocity.setZero();
                previousStationarySample = t;
                stationary = true;
            } else {
                velocity += acceleration * dt_;
                stationary = false;
            }

            velocities.row(t) = velocity.transpose();
        }

        return velocities;
    }

    Eigen::MatrixXd positionTrack(
        const Eigen::MatrixXd& navigationAcceleration,
        const Eigen::MatrixXd& velocities) const {
        if (navigationAcceleration.rows() != velocities.rows() ||
            navigationAcceleration.cols() != 3 || velocities.cols() != 3) {
            throw std::invalid_argument(
                "Acceleration and velocity matrices must be N x 3 with equal N.");
        }

        const Eigen::Index samples = navigationAcceleration.rows();
        Eigen::MatrixXd positions = Eigen::MatrixXd::Zero(samples, 3);
        Eigen::Vector3d position = Eigen::Vector3d::Zero();

        for (Eigen::Index t = 0; t < samples; ++t) {
            const Eigen::Vector3d velocity = velocities.row(t).transpose();
            const Eigen::Vector3d acceleration =
                navigationAcceleration.row(t).transpose();
            position += velocity * dt_ +
                        0.5 * acceleration * (dt_ * dt_);
            positions.row(t) = position.transpose();
        }

        return positions;
    }

private:
    static int columnStart(int order) {
        switch (order) {
            case 1:
                return 0;
            case 2:
                return 3;
            case 3:
                return 6;
            default:
                throw std::invalid_argument(
                    "Sensor order must be 1, 2, or 3.");
        }
    }

    void validateData(const Eigen::MatrixXd& data) const {
        const int requiredColumns =
            std::max({gyroStart_, accelerometerStart_, magnetometerStart_}) + 3;
        if (data.cols() < requiredColumns) {
            throw std::invalid_argument(
                "IMU data does not contain all required sensor columns.");
        }
        if (!data.allFinite()) {
            throw std::invalid_argument("IMU data contains NaN or infinity.");
        }
    }

    double sampling_;
    double dt_;
    int gyroStart_;
    int accelerometerStart_;
    int magnetometerStart_;
};

Eigen::MatrixXd receiveData(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Cannot open data file: " + filename);
    }

    std::vector<std::vector<double>> rows;
    std::string line;
    std::size_t expectedColumns = 0;
    std::size_t lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty()) {
            continue;
        }

        std::vector<double> row;
        std::stringstream stream(line);
        std::string value;

        while (std::getline(stream, value, ',')) {
            try {
                std::size_t parsedCharacters = 0;
                const double number = std::stod(value, &parsedCharacters);
                if (value.find_first_not_of(" \t\r", parsedCharacters) !=
                    std::string::npos) {
                    throw std::invalid_argument("Trailing characters");
                }
                if (!std::isfinite(number)) {
                    throw std::invalid_argument("Non-finite value");
                }
                row.push_back(number);
            } catch (const std::exception&) {
                throw std::runtime_error(
                    "Invalid numeric value on line " +
                    std::to_string(lineNumber) + ": " + value);
            }
        }

        if (row.empty()) {
            continue;
        }
        if (expectedColumns == 0) {
            expectedColumns = row.size();
        } else if (row.size() != expectedColumns) {
            throw std::runtime_error(
                "Inconsistent column count on line " +
                std::to_string(lineNumber) + ".");
        }
        if (row.size() < 9) {
            throw std::runtime_error(
                "Line " + std::to_string(lineNumber) +
                " has fewer than 9 values.");
        }

        rows.push_back(std::move(row));
    }

    if (rows.empty()) {
        throw std::runtime_error("The data file is empty: " + filename);
    }

    Eigen::MatrixXd data(
        static_cast<Eigen::Index>(rows.size()),
        static_cast<Eigen::Index>(expectedColumns));
    for (Eigen::Index i = 0; i < data.rows(); ++i) {
        for (Eigen::Index j = 0; j < data.cols(); ++j) {
            data(i, j) = rows[static_cast<std::size_t>(i)]
                              [static_cast<std::size_t>(j)];
        }
    }

    return data;
}

}  // namespace

int main() {
    try {
        // (1) Process IMU data before creating the UI.
        IMUTracker tracker(100.0);
        const Eigen::MatrixXd data =
            receiveData("/Users/jidengfeng/data.txt");

        if (data.rows() <= 30 || data.cols() < 9) {
            std::cerr << "Need at least 31 rows and 9 columns of IMU data.\n";
            return 1;
        }

        const IMUTracker::InitData initialization =
            tracker.initialize(data.block(5, 0, 25, 9));
        const auto [navigationAcceleration, orientations] =
            tracker.attitudeTrack(
                data.block(30, 0, data.rows() - 30, 9), initialization);
        (void)orientations;

        const Eigen::MatrixXd filteredAcceleration =
            tracker.removeAccelerationError(navigationAcceleration);
        const Eigen::MatrixXd velocities =
            tracker.zeroVelocityUpdate(filteredAcceleration, 0.2);
        const Eigen::MatrixXd positions =
            tracker.positionTrack(filteredAcceleration, velocities);

        std::cout << "Tracking samples: " << positions.rows() << '\n'
                  << "Position range X: " << positions.col(0).minCoeff()
                  << " to " << positions.col(0).maxCoeff() << '\n'
                  << "Position range Y: " << positions.col(1).minCoeff()
                  << " to " << positions.col(1).maxCoeff() << '\n'
                  << "Position range Z: " << positions.col(2).minCoeff()
                  << " to " << positions.col(2).maxCoeff() << '\n';

        if (positions.rows() == 0) {
            std::cerr << "No tracking samples were produced.\n";
            return 1;
        }

        // (2) Initialize SDL3. SDL3 returns true on success and false on failure.
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
            return 1;
        }

        // macOS supports OpenGL 3.2 Core with GLSL 1.50.
        if (!SDL_GL_SetAttribute(
                SDL_GL_CONTEXT_FLAGS,
                SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG) ||
            !SDL_GL_SetAttribute(
                SDL_GL_CONTEXT_PROFILE_MASK,
                SDL_GL_CONTEXT_PROFILE_CORE) ||
            !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) ||
            !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2)) {
            std::cerr << "SDL_GL_SetAttribute failed: " << SDL_GetError()
                      << '\n';
            SDL_Quit();
            return 1;
        }

        const SDL_WindowFlags windowFlags =
            static_cast<SDL_WindowFlags>(
                SDL_WINDOW_OPENGL |
                SDL_WINDOW_RESIZABLE |
                SDL_WINDOW_HIGH_PIXEL_DENSITY);

        SDL_Window* window = SDL_CreateWindow(
            "IMU Tracker Simulation", 1280, 720, windowFlags);
        if (window == nullptr) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError()
                      << '\n';
            SDL_Quit();
            return 1;
        }

        SDL_GLContext glContext = SDL_GL_CreateContext(window);
        if (glContext == nullptr) {
            std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError()
                      << '\n';
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        if (!SDL_GL_MakeCurrent(window, glContext)) {
            std::cerr << "SDL_GL_MakeCurrent failed: " << SDL_GetError()
                      << '\n';
            SDL_GL_DestroyContext(glContext);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        if (!SDL_GL_SetSwapInterval(1)) {
            std::cerr << "Warning: could not enable VSync: "
                      << SDL_GetError() << '\n';
        }

        // (3) Initialize Dear ImGui.
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForOpenGL(window, glContext)) {
            std::cerr << "Failed to initialize the ImGui SDL3 backend.\n";
            ImGui::DestroyContext();
            SDL_GL_DestroyContext(glContext);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        if (!ImGui_ImplOpenGL3_Init("#version 150")) {
            std::cerr << "Failed to initialize the ImGui OpenGL backend.\n";
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            SDL_GL_DestroyContext(glContext);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        // (4) Prepare UI buffers.
        const int totalSamples = static_cast<int>(positions.rows());
        std::vector<float> positionX(static_cast<std::size_t>(totalSamples));
        std::vector<float> positionY(static_cast<std::size_t>(totalSamples));
        std::vector<float> positionZ(static_cast<std::size_t>(totalSamples));

        for (int i = 0; i < totalSamples; ++i) {
            positionX[static_cast<std::size_t>(i)] =
                static_cast<float>(positions(i, 0));
            positionY[static_cast<std::size_t>(i)] =
                static_cast<float>(positions(i, 1));
            positionZ[static_cast<std::size_t>(i)] =
                static_cast<float>(positions(i, 2));
        }

        int currentSample = 0;
        bool isPlaying = false;
        Uint64 lastTime = SDL_GetTicks();
        bool done = false;

        // (5) Main UI loop.
        while (!done) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT) {
                    done = true;
                }
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                    event.window.windowID == SDL_GetWindowID(window)) {
                    done = true;
                }
            }

            const Uint64 currentTime = SDL_GetTicks();
            if (isPlaying && currentTime - lastTime >= 10) {
                // Advance by all elapsed 10 ms intervals, not just one sample.
                const Uint64 elapsedSteps = (currentTime - lastTime) / 10;
                const int remaining = totalSamples - 1 - currentSample;
                const int advance = static_cast<int>(std::min<Uint64>(
                    elapsedSteps, static_cast<Uint64>(std::max(remaining, 0))));
                currentSample += advance;
                lastTime += elapsedSteps * 10;

                if (currentSample >= totalSamples - 1) {
                    currentSample = totalSamples - 1;
                    isPlaying = false;
                }
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowPos(
                ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(
                ImVec2(600.0f, 620.0f), ImGuiCond_FirstUseEver);
            ImGui::Begin("IMU Data Stream Simulator");

            if (ImGui::Button(isPlaying ? "Pause" : "Play")) {
                isPlaying = !isPlaying;
                lastTime = SDL_GetTicks();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                currentSample = 0;
                isPlaying = false;
                lastTime = SDL_GetTicks();
            }

            ImGui::SliderInt(
                "Timeline", &currentSample, 0, totalSamples - 1);

            ImGui::SeparatorText("Current Position (Navigation Frame)");
            ImGui::Text(
                "X: %8.4f m",
                positionX[static_cast<std::size_t>(currentSample)]);
            ImGui::Text(
                "Y: %8.4f m",
                positionY[static_cast<std::size_t>(currentSample)]);
            ImGui::Text(
                "Z: %8.4f m",
                positionZ[static_cast<std::size_t>(currentSample)]);

            ImGui::SeparatorText("Live Trajectory Graphs");

            // Fixed limits of [-2, 2] can make a valid trajectory appear flat
            // when its values are much smaller or much larger. Scale each
            // graph to the currently visible data instead.
            auto plotAxis = [](const char* label,
                               const std::vector<float>& values,
                               int count) {
                float minimum = values[0];
                float maximum = values[0];
                for (int i = 1; i < count; ++i) {
                    minimum = std::min(minimum, values[static_cast<std::size_t>(i)]);
                    maximum = std::max(maximum, values[static_cast<std::size_t>(i)]);
                }

                float margin = (maximum - minimum) * 0.10f;
                if (margin < 0.001f) {
                    margin = 0.001f;
                }
                if (maximum - minimum < 1e-6f) {
                    minimum -= 0.01f;
                    maximum += 0.01f;
                } else {
                    minimum -= margin;
                    maximum += margin;
                }

                ImGui::Text("%s range: %.5f to %.5f m", label, minimum, maximum);
                ImGui::PlotLines(label, values.data(), count, 0, nullptr,
                                 minimum, maximum, ImVec2(0.0f, 100.0f));
            };

            plotAxis("X Axis", positionX, currentSample + 1);
            plotAxis("Y Axis", positionY, currentSample + 1);
            plotAxis("Z Axis", positionZ, currentSample + 1);

            ImGui::End();

            ImGui::Render();
            int framebufferWidth = 0;
            int framebufferHeight = 0;
            if (!SDL_GetWindowSizeInPixels(
                    window, &framebufferWidth, &framebufferHeight)) {
                framebufferWidth = static_cast<int>(io.DisplaySize.x);
                framebufferHeight = static_cast<int>(io.DisplaySize.y);
            }

            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }

        // (6) Cleanup.
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
