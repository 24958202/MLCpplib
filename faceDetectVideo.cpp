/*
    Program to detect faces in a video
*/
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>

const uint8_t C_0 = 0;
const uint8_t C_1 = 1;
const uint8_t C_2 = 2;
const uint8_t C_3 = 3;
unsigned int faceCount = 0; // Counter for faces detected
// Function to check for existing face images using features
bool read_image_detect_objs(const std::string& faces_folder_path, cv::Mat& img_input, unsigned int featureCount = 1000, float ratioThresh = 0.75f, unsigned int de_threshold = 10) {  
    if (!std::filesystem::exists(faces_folder_path)) {
        std::cerr << "The folder does not exist" << std::endl;
        return false;
    }
    cv::Ptr<cv::ORB> detector = cv::ORB::create(featureCount);
    std::vector<cv::KeyPoint> keypoints_input;
    cv::Mat descriptors_input;
    // Compute features for the input image
    detector->detectAndCompute(img_input, cv::noArray(), keypoints_input, descriptors_input);
    for (const auto& entry : std::filesystem::directory_iterator(faces_folder_path)) {  
        if (entry.is_regular_file()) {
            cv::Mat m_img1 = cv::imread(entry.path().string()); 
            std::vector<cv::KeyPoint> keypoints1;
            cv::Mat descriptors1; 
            // Compute features for the existing image  
            detector->detectAndCompute(m_img1, cv::noArray(), keypoints1, descriptors1);  
            cv::BFMatcher matcher(cv::NORM_HAMMING);  
            std::vector<std::vector<cv::DMatch>> knnMatches;  
            matcher.knnMatch(descriptors1, descriptors_input, knnMatches, 2);
            // Apply the ratio test to filter good matches
            std::vector<cv::DMatch> goodMatches;
            for (const auto& match : knnMatches) {  
                if (match.size() > 1 && match[C_0].distance < ratioThresh * match[C_1].distance) {
                    goodMatches.push_back(match[C_0]);  
                }
            }
            std::cout << "Matching score: " << goodMatches.size() << std::endl;  
            if (goodMatches.size() > de_threshold) {
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
    // Draw rectangles around detected faces and mark them
    for (size_t i = 0; i < faces.size(); ++i) {
        cv::rectangle(frame, faces[i], cv::Scalar(0, 255, 0), 2); // Green rectangles
        std::string text = (i < 9) ? "0" + std::to_string(i + 1) : std::to_string(i + 1);
        cv::putText(frame, text, cv::Point(faces[i].x, faces[i].y - 10), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
    }
    // Save the first detected face as a snapshot
    cv::Rect faceROI = faces[0]; // Assuming we save the first detected face
    cv::Mat faceImage = frame(faceROI).clone(); // Extract the region of interest and clone it to avoid issues
    if (read_image_detect_objs(face_folder, faceImage)) {
        return; // Exit if the face already exists
    }
    // Save the new face
    std::string fileName = face_folder + "/face_" + std::to_string(faceCount++) + ".jpg"; // Create a filename
    cv::imwrite(fileName, faceImage); // Save the image with the extracted face
    std::cout << "Saved snapshot: " << fileName << std::endl;
}
void start_recording() {
    // Load the Haar Cascade model for face detection
    cv::CascadeClassifier faceCascade;
    if (!faceCascade.load("/Users/dengfengji/ronnieji/MLCpplib-main/haarcascade_frontalface_default.xml")) {
        std::cerr << "Error: Could not load Haar Cascade model." << std::endl;
        return;
    }
    // Open the default camera
    cv::VideoCapture cap(1);
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
            onFacesDetected(faces, frame, "/Users/dengfengji/ronnieji/MLCpplib-main/faces"); // Call the callback
        }
        // Display the resulting frame
        cv::imshow("Face Detection", frame);
        // Check for key press
        char key = cv::waitKey(30);
        if (key == 'q') {
            break; // Exit if 'q' is pressed
        }
    }
    cap.release();
    cv::destroyAllWindows();
}
int main() {
    start_recording();
    return 0;
}
