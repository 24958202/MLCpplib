#include <iostream>  
#include <vector>  
#include <unordered_map>  
#include <string>  
#include <fstream>  
#include <opencv2/opencv.hpp>  
#include <opencv2/features2d.hpp>  

const unsigned int MAX_FEATURES = 1000;  

// Function to save the feature map (serialization)  
void saveModel_keypoint(const std::unordered_map<std::string, std::vector<cv::Mat>>& featureMap, const std::string& filename) {  
    if (filename.empty()) {  
        throw std::runtime_error("Error: Filename is empty.");  
    }  
    std::ofstream ofs(filename, std::ios::binary);  
    if (!ofs.is_open()) {  
        throw std::runtime_error("Unable to open file for writing.");  
    }  
    try {  
        size_t mapSize = featureMap.size();  
        ofs.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));  
        for (const auto& [className, images] : featureMap) {  
            size_t keySize = className.size();  
            ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));  
            ofs.write(className.data(), keySize);  
            size_t imageCount = images.size();  
            ofs.write(reinterpret_cast<const char*>(&imageCount), sizeof(imageCount));  
            for (const auto& img : images) {  
                if (img.empty()) {  
                    continue; // Skip empty images  
                }  
                // Write image dimensions and type  
                int rows = img.rows;  
                int cols = img.cols;  
                int type = img.type();  
                ofs.write(reinterpret_cast<const char*>(&rows), sizeof(rows));  
                ofs.write(reinterpret_cast<const char*>(&cols), sizeof(cols));  
                ofs.write(reinterpret_cast<const char*>(&type), sizeof(type));  
                // Write image data  
                size_t dataSize = img.total() * img.elemSize();  
                ofs.write(reinterpret_cast<const char*>(img.data), dataSize);  

                // Debugging output  
                std::cout << "Serialized matrix: rows=" << rows << ", cols=" << cols << ", type=" << type << ", dataSize=" << dataSize << std::endl;  
            }  
        }  
    } catch (const std::exception& e) {  
        std::cerr << "Error writing to file: " << e.what() << std::endl;  
        throw; // Rethrow the exception after logging  
    }  
    ofs.close();  
}  

// Function to load the feature map (deserialization)  
void loadModel_keypoint(std::unordered_map<std::string, std::vector<cv::Mat>>& featureMap, const std::string& filename) {  
    if (filename.empty()) {  
        std::cerr << "Error: Filename is empty." << std::endl;  
        return;  
    }  
    std::ifstream ifs(filename, std::ios::binary);  
    if (!ifs.is_open()) {  
        std::cerr << "Error: Unable to open file for reading." << std::endl;  
        return;  
    }  
    try {  
        size_t mapSize;  
        if (!ifs.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize))) {  
            std::cerr << "Error: Failed to read map size." << std::endl;  
            return;  
        }  
        for (size_t i = 0; i < mapSize; ++i) {  
            size_t keySize;  
            if (!ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize))) {  
                std::cerr << "Error: Failed to read key size." << std::endl;  
                return;  
            }  
            std::string className(keySize, '\0');  
            if (!ifs.read(&className[0], keySize)) {  
                std::cerr << "Error: Failed to read class name." << std::endl;  
                return;  
            }  
            size_t imageCount;  
            if (!ifs.read(reinterpret_cast<char*>(&imageCount), sizeof(imageCount))) {  
                std::cerr << "Error: Failed to read image count." << std::endl;  
                return;  
            }  
            std::vector<cv::Mat> images;  
            images.reserve(imageCount);  
            for (size_t j = 0; j < imageCount; ++j) {  
                int rows, cols, type;  
                if (!ifs.read(reinterpret_cast<char*>(&rows), sizeof(rows)) ||  
                    !ifs.read(reinterpret_cast<char*>(&cols), sizeof(cols)) ||  
                    !ifs.read(reinterpret_cast<char*>(&type), sizeof(type))) {  
                    std::cerr << "Error: Failed to read image dimensions." << std::endl;  
                    return;  
                }  

                // Validate matrix dimensions and type  
                if (rows <= 0 || cols <= 0 || type < 0) {  
                    std::cerr << "Error: Invalid matrix dimensions or type. rows=" << rows << ", cols=" << cols << ", type=" << type << std::endl;  
                    continue; // Skip invalid images  
                }  

                cv::Mat img(rows, cols, type);  
                size_t dataSize = img.total() * img.elemSize();  

                // Read image data  
                if (!ifs.read(reinterpret_cast<char*>(img.data), dataSize)) {  
                    std::cerr << "Error: Failed to read image data. Expected " << dataSize << " bytes." << std::endl;  
                    return;  
                }  

                // Debugging output  
                std::cout << "Deserialized matrix: rows=" << rows << ", cols=" << cols << ", type=" << type << ", dataSize=" << dataSize << std::endl;  

                images.push_back(img);  
            }  
            featureMap[className] = images;  
        }  
    } catch (const std::exception& e) {  
        std::cerr << "Error reading from file: " << e.what() << std::endl;  
    }  
    ifs.close();  
}  

// Function to preprocess an image into slices  
void preprocessImg(const std::string& img_path, const unsigned int ReSIZE_IMG_WIDTH, const unsigned int ReSIZE_IMG_HEIGHT, std::vector<cv::Mat>& outImg) {  
    if (img_path.empty()) {  
        std::cerr << "Error: img_path is empty." << std::endl;  
        return;  
    }  
    if (ReSIZE_IMG_WIDTH == 0 || ReSIZE_IMG_HEIGHT == 0) {  
        std::cerr << "Error: Invalid resize dimensions." << std::endl;  
        return;  
    }  
    try {  
        outImg.clear();  
        // Load the image  
        cv::Mat img = cv::imread(img_path, cv::IMREAD_COLOR);  
        if (img.empty()) {  
            std::cerr << "Error: Image not loaded correctly from path: " << img_path << std::endl;  
            return; // Handle the error appropriately  
        }  
        // Resize the image  
        cv::Mat resizeImg;  
        cv::resize(img, resizeImg, cv::Size(ReSIZE_IMG_WIDTH, ReSIZE_IMG_HEIGHT));  
        // Convert to grayscale  
        cv::Mat img_gray;  
        cv::cvtColor(resizeImg, img_gray, cv::COLOR_BGR2GRAY);  
        // Define the size of slices  
        const int sliceWidth = 64;  
        const int sliceHeight = 64;  
        // Iterate and slice the image into 64x64 pieces  
        for (int y = 0; y <= img_gray.rows - sliceHeight; y += sliceHeight) {  
            for (int x = 0; x <= img_gray.cols - sliceWidth; x += sliceWidth) {  
                cv::Rect sliceArea(x, y, sliceWidth, sliceHeight);  
                cv::Mat slice = img_gray(sliceArea).clone(); // Use clone to ensure a deep copy  
                outImg.push_back(slice);  
            }  
        }  
    } catch (const cv::Exception& ex) {  
        std::cerr << "OpenCV Error: " << ex.what() << std::endl;  
    } catch (const std::exception& ex) {  
        std::cerr << "Standard Error: " << ex.what() << std::endl;  
    } catch (...) {  
        std::cerr << "Unknown error in preprocessImg." << std::endl;  
    }  
}  

// Function to extract SIFT features  
std::vector<cv::KeyPoint> extractSIFTFeatures(const cv::Mat& img, cv::Mat& descriptors, const unsigned int MAX_FEATURES) {  
    std::vector<cv::KeyPoint> keypoints_input;  
    if (img.empty()) {  
        return keypoints_input;  
    }  
    try {  
        cv::Ptr<cv::SIFT> detector = cv::SIFT::create(MAX_FEATURES);  
        detector->detectAndCompute(img, cv::noArray(), keypoints_input, descriptors);  
    } catch (const std::exception& ex) {  
        std::cerr << "Error in SIFT feature extraction: " << ex.what() << std::endl;  
    } catch (...) {  
        std::cerr << "Unknown error in extractSIFTFeatures." << std::endl;  
    }  
    return keypoints_input;  
}  

// Test function  
void test_cvMat() {  
    std::vector<cv::Mat> test_img;  
    preprocessImg("/home/ronnieji/ronnieji/kaggle/train/banana/Image_9.jpg", 448, 448, test_img); // Update with your image path  
    if (!test_img.empty()) {  
        for (unsigned int i = 0; i < test_img.size(); ++i) {  
            cv::Mat eval_descriptors;  
            auto trained_key = extractSIFTFeatures(test_img[i], eval_descriptors, MAX_FEATURES);  
            if (!eval_descriptors.empty()) {  
                std::cout << "index: " << i << " has descriptors." << std::endl;  
            } else {  
                std::cout << "index: " << i << " has no descriptors." << std::endl;  
            }  
        }  
    }  
    std::unordered_map<std::string, std::vector<cv::Mat>> test_slices;  
    test_slices["test1"] = test_img;  

    // Save the model  
    saveModel_keypoint(test_slices, "/home/ronnieji/ronnieji/sliceImgRecog-main/main/test.dat");  

    // Load the model  
    std::unordered_map<std::string, std::vector<cv::Mat>> featureMap;  
    loadModel_keypoint(featureMap, "/home/ronnieji/ronnieji/sliceImgRecog-main/main/test.dat");  
    
    if (!featureMap.empty()) {  
        for (const auto& item : featureMap) {  
            auto cvMat_vec = item.second;  
            if (!cvMat_vec.empty()) {  
                for (unsigned int i = 0; i < cvMat_vec.size(); ++i) {  
                    cv::Mat eval_descriptors;  
                    auto trained_key = extractSIFTFeatures(cvMat_vec[i], eval_descriptors, MAX_FEATURES);  
                    
                    // Check pixel data integrity  
                    if (i < test_img.size()) { // Ensure we don't go out of bounds  
                        cv::Mat original_slice = test_img[i];  
                        double diff = cv::norm(original_slice, cvMat_vec[i], cv::NORM_INF);  
                        if (diff > 0) {  
                            std::cout << "Warning: Slice " << i << " data differs from original!" << std::endl;  
                        } else {  
                            std::cout << "Slice " << i << " data matches original." << std::endl;  
                        }  
                    }  

                    if (!eval_descriptors.empty()) {  
                        std::cout << "Decoded index: " << i << " has descriptors." << std::endl;  
                    } else {  
                        std::cout << "Decoded index: " << i << " has no descriptors." << std::endl;  
                    }  
                }  
            }  
        }  
    }  
}  
void img_comparing() {  
    // Define the target size  
    cv::Size targetSize(448, 448);  
    // Load images in grayscale  
    cv::Mat img1 = cv::imread("/home/ronnieji/ronnieji/kaggle/train/banana/Image_9.jpg", cv::IMREAD_GRAYSCALE);  
    cv::Mat img2 = cv::imread("/home/ronnieji/ronnieji/kaggle/train/banana/Image_73.jpg", cv::IMREAD_GRAYSCALE);  
    // Check if images are loaded successfully  
    if (img1.empty() || img2.empty()) {  
        std::cerr << "Could not open one or both images: Image_9 or Image_73!" << std::endl;  
        return;  
    }  
    // Resize images to 448x448  
    cv::resize(img1, img1, targetSize);  
    cv::resize(img2, img2, targetSize);  
    // Calculate L1 Norm  
    double l1Norm = cv::norm(img1, img2, cv::NORM_L1);  
    std::cout << "L1 Norm of absolute pixel differences: " << l1Norm << std::endl;  
    // Load next pair of images  
    cv::Mat img3 = cv::imread("/home/ronnieji/ronnieji/kaggle/train/banana/Image_4.jpg", cv::IMREAD_GRAYSCALE);  
    cv::Mat img4 = cv::imread("/home/ronnieji/ronnieji/kaggle/train/banana/Image_5.jpg", cv::IMREAD_GRAYSCALE);  
    // Check if images are loaded successfully  
    if (img3.empty() || img4.empty()) {  
        std::cerr << "Could not open one or both images: Image_4 or Image_5!" << std::endl;  
        return;  
    }  
    // Resize images to 448x448  
    cv::resize(img3, img3, targetSize);  
    cv::resize(img4, img4, targetSize);  
    // Calculate L2 Norm  
    double l2Norm = cv::norm(img3, img4, cv::NORM_L2);  
    std::cout << "L2 Norm (Euclidean norm) of error: " << l2Norm << std::endl;  
    // Load last pair of images  
    cv::Mat img5 = cv::imread("/home/ronnieji/ronnieji/kaggle/train/banana/Image_3.jpg", cv::IMREAD_GRAYSCALE);  
    cv::Mat img6 = cv::imread("/home/ronnieji/ronnieji/kaggle/train/banana/Image_28.jpg", cv::IMREAD_GRAYSCALE);  
    // Check if images are loaded successfully  
    if (img5.empty() || img6.empty()) {  
        std::cerr << "Could not open one or both images: Image_3 or Image_28!" << std::endl;  
        return;  
    }  
    // Resize images to 448x448  
    cv::resize(img5, img5, targetSize);  
    cv::resize(img6, img6, targetSize);  
    // Calculate L3 Norm (Infinity Norm)  
    double l3Norm = cv::norm(img5, img6, cv::NORM_INF);  
    std::cout << "L3 Infinity Norm (maximum absolute error): " << l3Norm << std::endl;  
}  
int main() {  
    //test_cvMat(); 
	img_comparing(); 
    return 0;  
}