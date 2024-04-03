#include <opencv2/opencv.hpp>
#include <chrono>
#include <iomanip>

std::string getCurrentDateTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");
    return ss.str();
}

int main() {
    cv::VideoCapture cap(0); // Open the default camera
    if (!cap.isOpened()) {
        std::cerr << "Error opening the webcam" << std::endl;
        return -1;
    }

    cv::CascadeClassifier face_cascade;
    if (!face_cascade.load("/home/ronnieji/lib/webcam/haarcascade_frontalface_default.xml")) {
        std::cerr << "Error loading face cascade file" << std::endl;
        return -1;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame; // Capture frame from webcam

        std::vector<cv::Rect> faces;
        //face_cascade.detectMultiScale(frame, faces, 1.1, 2, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
        face_cascade.detectMultiScale(frame, faces, 1.3, 5, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

        for (const auto& face : faces) {
            cv::rectangle(frame, face, cv::Scalar(255, 0, 0), 2); // Draw rectangle around detected face
        }

        cv::imshow("Face Detection", frame);

        if (cv::waitKey(1) == 27) { // Press 'Esc' key to exit
            break;
        }

        if (!faces.empty()) {
            std::string filename = "/home/ronnieji/lib/webcam/capture/" + getCurrentDateTime() + ".jpg";
            cv::imwrite(filename, frame);
            std::cout << "Alert: Human face detected! Full image saved as " << filename << std::endl;
        }
    }

    cap.release();
    cv::destroyAllWindows();

    return 0;
}
