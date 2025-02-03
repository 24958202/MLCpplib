#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <mutex>
#include <chrono>

std::mutex faceMutex;

// Function to calculate cosine similarity between two embeddings
double cosineSimilarity(const cv::Mat& vec1, const cv::Mat& vec2) {
    double dot = vec1.dot(vec2);
    double norm1 = cv::norm(vec1);
    double norm2 = cv::norm(vec2);
    return (norm1 * norm2) > 0 ? dot / (norm1 * norm2) : 0.0;
}

// Check if a face is a duplicate based on embeddings
bool isDuplicateFace(const cv::Mat& newEmbedding, const std::vector<cv::Mat>& existingEmbeddings, double threshold = 0.6) {
    for (const auto& embedding : existingEmbeddings) {
        if (cosineSimilarity(newEmbedding, embedding) > threshold) {
            return true;
        }
    }
    return false;
}

// Extract face embeddings using a pre-trained model
cv::Mat getFaceEmbedding(cv::dnn::Net& embedder, const cv::Mat& face) {
    cv::Mat blob = cv::dnn::blobFromImage(face, 1.0 / 255.0, cv::Size(96, 96), cv::Scalar(0, 0, 0), true, false);
    embedder.setInput(blob);
    return embedder.forward().clone(); // Clone to ensure deep copy
}

// Load existing face embeddings from storage folder
void loadExistingFaces(const std::string& folder, cv::dnn::Net& embedder, std::vector<cv::Mat>& embeddings) {
    if (!std::filesystem::exists(folder)) return;

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.path().extension() == ".jpg" || entry.path().extension() == ".png") {
            cv::Mat face = cv::imread(entry.path().string());
            if (!face.empty()) {
                cv::Mat embedding = getFaceEmbedding(embedder, face);
                embeddings.push_back(embedding);
            }
        }
    }
}

// Save a unique face image to the folder
void saveUniqueFace(const cv::Mat& face, const std::string& folder, cv::dnn::Net& embedder, std::vector<cv::Mat>& faceEmbeddings) {
    cv::Mat embedding = getFaceEmbedding(embedder, face);

    if (isDuplicateFace(embedding, faceEmbeddings)) {  
        std::cout << "Duplicate face detected. Skipping save." << std::endl;  
        return;  
    }  

    if (!std::filesystem::exists(folder)) {  
        std::filesystem::create_directories(folder);  
    }  

    std::lock_guard<std::mutex> lock(faceMutex);
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    std::string filename = folder + "/face_" + std::to_string(timestamp) + ".jpg";    
    bool success = cv::imwrite(filename, face);  
    if (!success) {  
        std::cerr << "Failed to save image: " << filename << std::endl;  
    } else {  
        std::cout << "Saved new face: " << filename << std::endl;  
        faceEmbeddings.push_back(embedding);  
    }
}

// Process each frame to detect and save unique faces
void processFrame(cv::Mat& frame, const std::string& face_folder, 
                 cv::dnn::Net& faceNet, cv::dnn::Net& embedder, 
                 std::vector<cv::Mat>& faceEmbeddings) {
    try {
        cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300),
                                            cv::Scalar(104.0, 177.0, 123.0), false, false);
        faceNet.setInput(blob);
        cv::Mat detections = faceNet.forward();

        std::vector<cv::Rect> faces;
        std::vector<float> confidences;
        float* data = (float*)detections.ptr<float>(0);
        for (int i = 0; i < detections.size[2]; i++) {
            float confidence = data[i * 7 + 2];

            if (confidence > 0.8) {
                int x1 = static_cast<int>(data[i * 7 + 3] * frame.cols);
                int y1 = static_cast<int>(data[i * 7 + 4] * frame.rows);
                int x2 = static_cast<int>(data[i * 7 + 5] * frame.cols);
                int y2 = static_cast<int>(data[i * 7 + 6] * frame.rows);

                x1 = std::max(0, std::min(x1, frame.cols - 1));
                y1 = std::max(0, std::min(y1, frame.rows - 1));
                x2 = std::max(0, std::min(x2, frame.cols - 1));
                y2 = std::max(0, std::min(y2, frame.rows - 1));

                if (x2 > x1 && y2 > y1) {
                    faces.emplace_back(cv::Point(x1, y1), cv::Point(x2, y2));
                    confidences.push_back(confidence);
                }
            }
        }

        std::vector<int> indices;
        cv::dnn::NMSBoxes(faces, confidences, 0.7, 0.3, indices);

        for (int idx : indices) {
            cv::Rect face = faces[idx];
            cv::rectangle(frame, face, cv::Scalar(0, 255, 0), 2);

            cv::Mat faceImg = frame(face).clone();
            if (faceImg.empty() || faceImg.rows < 50 || faceImg.cols < 50) {
                continue;
            }

            saveUniqueFace(faceImg, face_folder, embedder, faceEmbeddings);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in processFrame: " << e.what() << std::endl;
    }
}

void startRecording(const std::string& modelConfig, const std::string& modelWeights,
                   const std::string& embedderModel, const std::string& faces_img_folder) {
    cv::dnn::Net faceNet = cv::dnn::readNetFromCaffe(modelConfig, modelWeights);
    cv::dnn::Net embedder = cv::dnn::readNetFromTorch(embedderModel);

    if (faceNet.empty() || embedder.empty()) {
        std::cerr << "Error: Failed to load models." << std::endl;
        return;
    }

    std::vector<cv::Mat> faceEmbeddings;
    loadExistingFaces(faces_img_folder, embedder, faceEmbeddings);
    std::cout << "Loaded " << faceEmbeddings.size() << " existing face embeddings" << std::endl;

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open video stream." << std::endl;
        return;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) break;

        processFrame(frame, faces_img_folder, faceNet, embedder, faceEmbeddings);

        cv::imshow("Face Detection", frame);
        if (cv::waitKey(30) == 'q') break;
    }

    cap.release();
    cv::destroyAllWindows();
}

int main() {
    // Verify model files exist
    auto check_file = [](const std::string& path) {
        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: Missing model file - " << path << std::endl;
            exit(1);
        }
    };

    std::string modelConfig = "/home/ronnieji/ronnieji/lib/MLCpplib-main/models/deploy.prototxt.txt";
    std::string modelWeights = "/home/ronnieji/ronnieji/lib/MLCpplib-main/models/res10_300x300_ssd_iter_140000.caffemodel";
    std::string embedderModel = "/home/ronnieji/ronnieji/lib/MLCpplib-main/models/openface_nn4.small2.v1.t7";
    std::string faces_img_storage_folder_path = "/home/ronnieji/ronnieji/lib/MLCpplib-main/capture";

    check_file(modelConfig);
    check_file(modelWeights);
    check_file(embedderModel);

    startRecording(modelConfig, modelWeights, embedderModel, faces_img_storage_folder_path);
    return 0;
}