/**
 * Face Detection Motion Capture - C++20 Implementation
 * Uses OpenCV 4.x for face detection and motion tracking
 */

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <string>
#include <iomanip>
#include <thread>
#include <format>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std::chrono;

class FaceMotionDetector {
public:
    FaceMotionDetector(const std::string& output_dir, int min_motion_area = 2000,
                       int cooldown_seconds = 3, int consecutive_frames = 3)
        : output_dir_(output_dir)
        , min_motion_area_(min_motion_area)
        , cooldown_seconds_(cooldown_seconds)
        , consecutive_frames_required_(consecutive_frames)
        , consecutive_face_frames_(0)
        , last_capture_time_(0)
        , total_frames_(0)
        , motion_frames_(0)
        , face_frames_(0)
        , captures_(0)
    {
        // Create output directory
        fs::create_directories(output_dir_);

        // Load Haar cascade for face detection
        std::string cascade_path = "/opt/homebrew/opt/opencv/share/opencv4/haarcascades/";
        face_cascade_.load(cascade_path + "haarcascade_frontalface_default.xml");
        
        if (face_cascade_.empty()) {
            throw std::runtime_error("Failed to load face cascade classifier");
        }

        // Initialize background subtractor
        bg_subtractor_ = cv::createBackgroundSubtractorMOG2(
            700,    // history
            80.0,   // varThreshold
            false   // detectShadows
        );

        std::cout << "Face Motion Detector initialized\n";
        std::cout << "Output: " << output_dir_ << "\n";
    }

    auto get_timestamp() -> std::string {
        auto now = system_clock::now();
        auto time_t = system_clock::to_time_t(now);
        auto ms = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;
        
        std::tm tm = *std::localtime(&time_t);
        return std::format("{:04}{:02}{:02}_{:02}{:02}{:02}_{:06}",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count());
    }

    auto detectFaces(const cv::Mat& frame) -> std::vector<cv::Rect> {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);

        std::vector<cv::Rect> faces;
        face_cascade_.detectMultiScale(
            gray,
            faces,
            1.05,    // scaleFactor
            8,       // minNeighbors (strict)
            0,
            cv::Size(60, 60),  // minSize
            cv::Size(500, 500) // maxSize
        );

        return faces;
    }

    auto getMotionMask(const cv::Mat& frame) -> std::pair<cv::Mat, double> {
        cv::Mat fg_mask;
        bg_subtractor_->apply(frame, fg_mask);
        
        // Morphological operations
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, {7, 7});
        cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(fg_mask, fg_mask, cv::MORPH_CLOSE, kernel);
        cv::dilate(fg_mask, fg_mask, kernel, {}, 2);

        // Calculate motion area
        double motion_area = cv::countNonZero(fg_mask);

        return {fg_mask, motion_area};
    }

    auto verifyMotionInFace(const cv::Mat& motion_mask, const cv::Rect& face) -> bool {
        int margin = static_cast<int>(face.width * 0.3);
        cv::Rect roi(
            std::max(0, face.x - margin),
            std::max(0, face.y - margin),
            std::min(motion_mask.cols, face.x + face.width + margin) - std::max(0, face.x - margin),
            std::min(motion_mask.rows, face.y + face.height + margin) - std::max(0, face.y - margin)
        );

        if (roi.width <= 0 || roi.height <= 0) return false;

        cv::Mat face_region = motion_mask(roi);
        double motion_pixels = cv::countNonZero(face_region);
        double total_pixels = face_region.rows * face_region.cols;

        return (motion_pixels / total_pixels) > 0.05;
    }

    auto captureImage(const cv::Mat& frame) -> std::string {
        std::string timestamp = get_timestamp();
        std::string filename = std::format("face_capture_{}.jpg", timestamp);
        std::string filepath = output_dir_ + "/" + filename;

        cv::Mat display = frame.clone();
        
        // Add timestamp
        std::string time_str = get_timestamp();
        cv::putText(display, time_str, {10, 30},
            cv::FONT_HERSHEY_SIMPLEX, 0.7, {0, 255, 0}, 2);
        
        cv::imwrite(filepath, display);
        return filepath;
    }

    auto isInCooldown() -> bool {
        auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        return (now - last_capture_time_) < (cooldown_seconds_ * 1000);
    }

    auto drawDetections(cv::Mat& display, const cv::Mat& motion_mask,
                        const std::vector<cv::Rect>& faces,
                        const std::vector<cv::Rect>& verified) -> void {
        // Draw motion regions faintly
        if (!motion_mask.empty()) {
            cv::Mat mask_colored;
            cv::cvtColor(motion_mask, mask_colored, cv::COLOR_GRAY2BGR);
            cv::addWeighted(display, 0.85, mask_colored, 0.15, 0, display);
        }

        // Draw face boxes
        for (const auto& face : faces) {
            bool is_verified = std::find(verified.begin(), verified.end(), face) != verified.end();
            cv::Scalar color = is_verified ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 165, 255);
            cv::rectangle(display, face, color, 2);
            cv::putText(display, is_verified ? "VERIFIED" : "DETECTED",
                {face.x, face.y - 10}, cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
        }
    }

    auto processFrame(const cv::Mat& frame) -> std::tuple<bool, bool, std::vector<cv::Rect>, std::vector<cv::Rect>> {
        total_frames_++;

        auto [motion_mask, motion_area] = getMotionMask(frame);
        bool has_motion = motion_area > min_motion_area_;

        if (has_motion) {
            motion_frames_++;
        }

        std::vector<cv::Rect> all_faces;
        std::vector<cv::Rect> verified_faces;
        bool has_verified = false;

        if (has_motion) {
            all_faces = detectFaces(frame);
            
            if (!all_faces.empty()) {
                face_frames_++;
            }

            // Verify motion overlaps with face
            for (const auto& face : all_faces) {
                if (verifyMotionInFace(motion_mask, face)) {
                    verified_faces.push_back(face);
                }
            }

            has_verified = !verified_faces.empty();

            if (has_verified) {
                consecutive_face_frames_++;
            } else {
                consecutive_face_frames_ = 0;
            }

            // Capture if verified
            if (has_verified && consecutive_face_frames_ >= consecutive_frames_required_ && !isInCooldown()) {
                auto filepath = captureImage(frame);
                last_capture_time_ = duration_cast<milliseconds>(
                    system_clock::now().time_since_epoch()).count();
                captures_++;
                consecutive_face_frames_ = 0;
                
                std::cout << "[CAPTURE] " << fs::path(filepath).filename() << "\n";
                std::cout << "  -> Verified faces: " << verified_faces.size() << "\n";
            }
        } else {
            consecutive_face_frames_ = 0;
        }

        return {has_motion, has_verified, all_faces, verified_faces};
    }

    auto run(int camera_index = 0) -> void {
        cv::VideoCapture cap;
        
        std::cout << "[INFO] Opening camera...\n";
        
        // Open with AVFoundation and set device type
        cap.open(camera_index, cv::CAP_AVFOUNDATION);
        
        // Configure for high quality capture
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
        cap.set(cv::CAP_PROP_FPS, 30);
        
        // Set buffer size to 1 for low latency
        cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
        
        // Wait for camera to initialize
        std::cout << "[INFO] Waiting for camera initialization...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!cap.isOpened()) {
            throw std::runtime_error(
                "Cannot open camera.\n"
                "Please check:\n"
                "  1. Close FaceTime, Photo Booth, Zoom, etc.\n"
                "  2. System Settings > Privacy & Security > Camera\n"
                "  3. Grant camera access"
            );
        }
        
        std::cout << "[INFO] Camera opened successfully!\n";
        std::cout << "[INFO] Frame size: " 
                  << cap.get(cv::CAP_PROP_FRAME_WIDTH) << "x"
                  << cap.get(cv::CAP_PROP_FRAME_HEIGHT) << "\n";

        // Configure camera
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
        cap.set(cv::CAP_PROP_FPS, 30);

        std::cout << "=" << std::string(60, '=') << "\n";
        std::cout << "Face Detection Motion Capture (C++20)\n";
        std::cout << "=" << std::string(60, '=') << "\n";
        std::cout << "Output: " << output_dir_ << "\n";
        std::cout << "Motion threshold: " << min_motion_area_ << " px\n";
        std::cout << "Cooldown: " << cooldown_seconds_ << "s\n";
        std::cout << "Verification: " << consecutive_frames_required_ << " consecutive frames\n";
        std::cout << "=" << std::string(60, '=') << "\n";
        std::cout << "Controls: Q=Quit, S=Manual capture, P=Pause\n";
        std::cout << "=" << std::string(60, '=') << "\n";

        cv::namedWindow("Face Detection", cv::WINDOW_NORMAL);
        
        cv::Mat frame;
        bool paused = false;

        while (true) {
            if (!paused) {
                cap >> frame;
                if (frame.empty()) {
                    std::cout << "[WARNING] Empty frame, retrying...\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }

                auto [has_motion, has_verified, faces, verified] = processFrame(frame);
                auto [motion_mask, _] = getMotionMask(frame);

                cv::Mat display = frame.clone();
                drawDetections(display, motion_mask, faces, verified);

                // Status bar
                std::string status = paused ? "PAUSED" : "LIVE";
                cv::Scalar status_color = paused ? cv::Scalar(255, 255, 0) : cv::Scalar(0, 255, 0);
                
                std::string info = std::format("{} | Motion: {} | Faces: {} [VER: {}] | Consecutive: {}/{}",
                    status, has_motion ? "YES" : "NO", faces.size(), verified.size(),
                    consecutive_face_frames_, consecutive_frames_required_);

                cv::rectangle(display, {0, display.rows - 35}, {display.cols, display.rows},
                    {0, 0, 0}, -1);
                cv::putText(display, info, {10, display.rows - 12},
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, status_color, 1);

                cv::imshow("Face Detection", display);
            }

            int key = cv::waitKey(1) & 0xFF;
            if (key == 'q' || key == 'Q' || key == 27) break;
            else if (key == 's' || key == 'S') {
                if (!frame.empty()) {
                    auto filepath = captureImage(frame);
                    std::cout << "[MANUAL] " << fs::path(filepath).filename() << "\n";
                }
            }
            else if (key == 'p' || key == 'P') {
                paused = !paused;
                std::cout << "[" << (paused ? "PAUSED" : "RESUMED") << "]\n";
            }
        }

        cap.release();
        cv::destroyAllWindows();

        std::cout << "\n" << "=" << std::string(60, '=') << "\n";
        std::cout << "Session Statistics:\n";
        std::cout << "  Frames: " << total_frames_ << "\n";
        std::cout << "  Motion: " << motion_frames_ << "\n";
        std::cout << "  Face detected: " << face_frames_ << "\n";
        std::cout << "  Captures: " << captures_ << "\n";
        std::cout << "  Saved to: " << output_dir_ << "\n";
        std::cout << "=" << std::string(60, '=') << "\n";
    }

private:
    std::string output_dir_;
    int min_motion_area_;
    int cooldown_seconds_;
    int consecutive_frames_required_;
    int consecutive_face_frames_;
    long long last_capture_time_;
    
    cv::CascadeClassifier face_cascade_;
    cv::Ptr<cv::BackgroundSubtractorMOG2> bg_subtractor_;
    
    int64_t total_frames_;
    int64_t motion_frames_;
    int64_t face_frames_;
    int64_t captures_;
};

auto main(int argc, char* argv[]) -> int {
    std::string output_dir = "./Captured";
    int min_motion_area = 2000;
    int cooldown = 3;
    int consecutive_frames = 3;

    // Simple argument parsing
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) output_dir = argv[++i];
        else if (arg == "-t" && i + 1 < argc) min_motion_area = std::stoi(argv[++i]);
        else if (arg == "-c" && i + 1 < argc) cooldown = std::stoi(argv[++i]);
        else if (arg == "-f" && i + 1 < argc) consecutive_frames = std::stoi(argv[++i]);
        else if (arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "  -o <dir>     Output directory (default: ./Captured)\n";
            std::cout << "  -t <pixels>  Motion threshold (default: 2000)\n";
            std::cout << "  -c <seconds> Cooldown between captures (default: 3)\n";
            std::cout << "  -f <frames>  Consecutive frames required (default: 3)\n";
            return 0;
        }
    }

    try {
        FaceMotionDetector detector(output_dir, min_motion_area, cooldown, consecutive_frames);
        detector.run();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
