/*
    Program to detect faces in a video and save unique face images.
*/

#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>

const unsigned int MAX_FEATURES = 1000; // Maximum number of features to detect
const float RATIO_THRESH = 0.75f;       // Ratio threshold for matching
const unsigned int DE_THRESHOLD = 10;   // Minimum matches to consider a face as existing
unsigned int faceCount = 0;              // Counter for faces detected

// Function to check for existing face images using features
bool checkExistingFace(const std::string& faces_folder_path, const cv::Mat& img_input) {
    if (!std::filesystem::exists(faces_folder_path)) {
        std::cerr << "The folder does not exist" << std::endl;
        return false;
    }
    cv::Ptr<cv::ORB> detector = cv::ORB::create(MAX_FEATURES);
    std::vector<cv::KeyPoint> keypoints_input;
    cv::Mat descriptors_input;
    // Compute features for the input image
    detector->detectAndCompute(img_input, cv::noArray(), keypoints_input, descriptors_input);
    for (const auto& entry : std::filesystem::directory_iterator(faces_folder_path)) {
        if (entry.is_regular_file()) {
            cv::Mat existing_face = cv::imread(entry.path().string());
            if (existing_face.empty()) continue; // Skip if image cannot be read
            std::vector<cv::KeyPoint> keypoints_existing;
            cv::Mat descriptors_existing;
            // Compute features for the existing image
            detector->detectAndCompute(existing_face, cv::noArray(), keypoints_existing, descriptors_existing);
            cv::BFMatcher matcher(cv::NORM_HAMMING);
            std::vector<std::vector<cv::DMatch>> knnMatches;
            matcher.knnMatch(descriptors_existing, descriptors_input, knnMatches, 2);
            std::vector<cv::DMatch> goodMatches;
            // Apply the ratio test to filter good matches
            for (const auto& match : knnMatches) {
                if (match.size() > 1 && match[0].distance < RATIO_THRESH * match[1].distance) {
                    goodMatches.push_back(match[0]);
                }
            }
            std::cout << "Matching score: " << goodMatches.size() << std::endl;
            if (goodMatches.size() > DE_THRESHOLD) {
                std::cout << "The face image already exists." << std::endl;
                return true; // The face already exists
            }
        }
    }
    return false; // Face does not exist
}
// Callback function to handle events after faces are detected
void onFacesDetected(const std::vector<cv::Rect>& faces, cv::Mat& frame, const std::string& face_folder) {
    if (faces.empty()) return;
    // Draw rectangles around detected faces
    for (size_t i = 0; i < faces.size(); ++i) {
        cv::rectangle(frame, faces[i], cv::Scalar(0, 255, 0), 2); // Green rectangles
        std::string text = (i < 9) ? "0" + std::to_string(i + 1) : std::to_string(i + 1);
        cv::putText(frame, text, cv::Point(faces[i].x, faces[i].y - 10), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
    }
    // Save the first detected face as a snapshot
    cv::Rect faceROI = faces[0]; // Assuming we save the first detected face
    cv::Mat faceImage = frame(faceROI).clone(); // Extract and clone the region of interest
    // Check for existing face images
    if (checkExistingFace(face_folder, faceImage)) {
        return; // Exit if the face already exists
    }
    // Save the unique face
    std::string fileName = face_folder + "/face_" + std::to_string(faceCount++) + ".jpg"; // Create a filename
    cv::imwrite(fileName, faceImage); // Save the image with the extracted face
    std::cout << "Saved snapshot: " << fileName << std::endl;
}
void startRecording() {
    // Load the Haar Cascade model for face detection
    cv::CascadeClassifier faceCascade;
    if (!faceCascade.load("/Users/dengfengji/ronnieji/MLCpplib-main/haarcascade_frontalface_default.xml")) {
        std::cerr << "Error: Could not load Haar Cascade model." << std::endl;
        return;
    }
    // Open the default camera
    cv::VideoCapture cap(0); // Change to 0 for primary camera
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video stream." << std::endl;
        return;
    }
    cv::Mat frame;
    while (true) {
        // Capture each frame from the video stream
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Error: Empty frame." << std::endl;
            break;
        }
        // Convert the frame to grayscale
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        // Detect faces in the frame
        std::vector<cv::Rect> faces;
        faceCascade.detectMultiScale(gray, faces, 1.1, 10, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
        // Invoke the callback if faces are detected
        if (!faces.empty()) {
            onFacesDetected(faces, frame, "faces"); // Call the callback
        }
        // Display the resulting frame
        cv::imshow("Face Detection", frame);
        // Exit if 'q' is pressed
        char key = cv::waitKey(30);
        if (key == 'q') {
            break; // Exit if 'q' is pressed
        }
    }
    cap.release();
    cv::destroyAllWindows();
}
int main() {
    startRecording();
    return 0;
}
