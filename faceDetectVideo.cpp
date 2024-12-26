/*
    Program to detect faces in a video and save unique face images.
	-I/usr/include/x86_64-linux-gnu/curl -lcurl -I/usr/include -lboost_system -lboost_filesystem -lboost_thread -lssl -lcrypto 
*/
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>  
#include <stdexcept>
#include <chrono>
#include <thread>
#include <curl/curl.h> 
const unsigned int MAX_FEATURES = 1000;   // Max number of features to detect
const float RATIO_THRESH = 0.95f;          // Ratio threshold for matching
const unsigned int DE_THRESHOLD = 10;      // Min matches to consider a face as existing
unsigned int faceCount = 0;                 // Counter for faces detected
// Function to read a file into a string  
std::string readFile(const std::string& filePath) {  
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);  
    if (!file) {  
        throw std::runtime_error("Failed to open file: " + filePath);  
    }  
    std::streamsize size = file.tellg();  
    file.seekg(0, std::ios::beg);  
    std::string buffer(size, '\0');  
    if (!file.read(buffer.data(), size)) {  
        throw std::runtime_error("Failed to read file: " + filePath);  
    }  
    return buffer;  
}  
// Function to send an email with an image attachment using libcurl  
void sendEmailWithAttachment(const std::string& smtpServer, int port,  
                             const std::string& username, const std::string& password,  
                             const std::string& from, const std::string& to,  
                             const std::string& subject, const std::string& body,  
                             const std::string& attachmentPath) {  
    CURL* curl = curl_easy_init();  
    if (!curl){  
        throw std::runtime_error("Failed to initialize CURL");  
    }  
    struct curl_slist* recipients = nullptr;  
    struct curl_mime* mime = nullptr;  
    struct curl_mimepart* part = nullptr;  
    try {  
        // Set the SMTP server and port  
        std::string smtpUrl = "smtp://" + smtpServer + ":" + std::to_string(port);  
        curl_easy_setopt(curl, CURLOPT_URL, smtpUrl.c_str());  
        // Enable TLS  
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);  
        // Set authentication details  
        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());  
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());  
        // Set the sender  
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());  
        // Set the recipient  
        recipients = curl_slist_append(recipients, to.c_str());  
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);  
        // Create the MIME message  
        mime = curl_mime_init(curl);  
        // Add the email body  
        part = curl_mime_addpart(mime);  
        curl_mime_data(part, body.c_str(), CURL_ZERO_TERMINATED);  
        curl_mime_type(part, "text/plain");  
        // Add the image attachment  
        part = curl_mime_addpart(mime);  
        curl_mime_filedata(part, attachmentPath.c_str());  
        curl_mime_type(part, "image/jpeg"); // Change this to the correct MIME type if needed  
        curl_mime_filename(part, "image.jpg");  
        // Set the MIME message  
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);  
        // Enable verbose output for debugging  
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);  
        // Perform the request  
        CURLcode res = curl_easy_perform(curl);  
        if (res != CURLE_OK) {  
            throw std::runtime_error("CURL error: " + std::string(curl_easy_strerror(res)));  
        }  
        std::cout << "Email sent successfully!" << std::endl;  
    } catch (const std::exception& ex) {  
        std::cerr << "Error: " << ex.what() << std::endl;  
    }  
    // Clean up  
    if (mime) curl_mime_free(mime);  
    if (recipients) curl_slist_free_all(recipients);  
    curl_easy_cleanup(curl);  
}  
bool checkExistingFace(const std::string& faces_folder_path, const cv::Mat& img_input) {
    if (!std::filesystem::exists(faces_folder_path) || !std::filesystem::is_directory(faces_folder_path)) {
        std::cerr << "The folder does not exist or cannot be accessed." << std::endl;
        return false;
    }
    cv::Mat img_gray;
    cv::cvtColor(img_input, img_gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(img_gray, img_gray, cv::Size(5, 5), 0);
    cv::Ptr<cv::SIFT> detector = cv::SIFT::create(MAX_FEATURES);
    std::vector<cv::KeyPoint> keypoints_input;
    cv::Mat descriptors_input;
    detector->detectAndCompute(img_gray, cv::noArray(), keypoints_input, descriptors_input);
    if (descriptors_input.empty()) {
        std::cerr << "Input image has no descriptors." << std::endl;
        return false;
    }
    for (const auto& entry : std::filesystem::directory_iterator(faces_folder_path)) {
        if (entry.is_regular_file()) {
            cv::Mat existing_face = cv::imread(entry.path().string());
            if (existing_face.empty()) continue;
            cv::Mat existing_face_gray;
            cv::cvtColor(existing_face, existing_face_gray, cv::COLOR_BGR2GRAY);
            std::vector<cv::KeyPoint> keypoints_existing;
            cv::Mat descriptors_existing;
            detector->detectAndCompute(existing_face_gray, cv::noArray(), keypoints_existing, descriptors_existing);
            if (descriptors_existing.empty()) {
                std::cerr << "Existing face has no descriptors." << std::endl;
                continue;
            }
            cv::BFMatcher matcher(cv::NORM_L2);
            std::vector<std::vector<cv::DMatch>> knnMatches;
            matcher.knnMatch(descriptors_existing, descriptors_input, knnMatches, 2);
            std::vector<cv::DMatch> goodMatches;
            for (const auto& match : knnMatches) {
                if (match.size() > 1 && match[0].distance < RATIO_THRESH * match[1].distance) {
                    goodMatches.push_back(match[0]);
                }
            }
            if (goodMatches.size() > DE_THRESHOLD) {
                std::cout << "The face image already exists." << std::endl;
                return true;
            }
        }
    }
    return false;
}
void onFacesDetected(const std::vector<cv::Rect>& faces, cv::Mat& frame, const std::string& face_folder) {
    if (faces.empty()) return;
    for (size_t i = 0; i < faces.size(); ++i) {
        cv::rectangle(frame, faces[i], cv::Scalar(0, 255, 0), 2);
        std::string text = (i < 9) ? "0" + std::to_string(i + 1) : std::to_string(i + 1);
        cv::putText(frame, text, cv::Point(faces[i].x, faces[i].y - 10), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
    }
    cv::Rect faceROI = faces[0]; // Save the first detected face
    cv::Mat faceImage = frame(faceROI).clone();
    if (checkExistingFace(face_folder, faceImage)) {
        return;
    }
    std::string fileName = face_folder + "/face_" + std::to_string(faceCount++) + ".jpg";
    cv::imwrite(fileName, faceImage);
    std::cout << "Saved snapshot: " << fileName << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	/*
	 * send email
	 */
	try {  
        // SMTP server configuration  
        const std::string smtpServer = "smtp.office365.com";  
        const int port = 587;  
        const std::string username = "ronniedengfengji@hotmail.com";  
        const std::string password = "";  
        // Email details  
        const std::string from = "ronniedengfengji@hotmail.com";  
        const std::string to = "dengfeng@hotmail.com";  
        const std::string subject = "dengfeng Inc. work mail reminder: New faces";  
        const std::string body = "New faces detected.";  
        const std::string attachmentPath = fileName;  
        // Send the email  
        sendEmailWithAttachment(smtpServer, port, username, password, from, to, subject, body, attachmentPath);  
    } catch (const std::exception& e) {  
        std::cerr << "Error: " << e.what() << std::endl;  
    }  
}
void startRecording(const std::string& facial_model, const std::string& faces_img_folder) {
    cv::CascadeClassifier faceCascade;
    if (!faceCascade.load(facial_model)) {
        std::cerr << "Error: Could not load Haar Cascade model." << std::endl;
        return;
    }
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video stream." << std::endl;
        return;
    }
    cv::Mat frame;
	try{
		while (true) {
			cap >> frame;
			if (frame.empty()) {
				std::cerr << "Error: Empty frame." << std::endl;
				break;
			}
			cv::Mat gray;
			cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
			std::vector<cv::Rect> faces;
			faceCascade.detectMultiScale(gray, faces, 1.1, 10, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
			if (!faces.empty()) {
				onFacesDetected(faces, frame, faces_img_folder);
			}
			cv::imshow("Face Detection", frame);
			char key = cv::waitKey(30);
			if (key == 'q') {
				break;
			}
		}
		cap.release();
		cv::destroyAllWindows();
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors" << std::endl;
	}
}
int main() {
	std::string facial_model_file = "/home/ronnieji/ronnieji/lib/MLCpplib-main/haarcascade_frontalface_default.xml";
	std::string faces_img_storage_folder_path = "/home/ronnieji/ronnieji/lib/MLCpplib-main/faces";
    startRecording(facial_model_file,faces_img_storage_folder_path);
    return 0;
}
