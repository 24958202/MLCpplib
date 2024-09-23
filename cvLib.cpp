/*
    c++20 lib for using opencv
*/
#include <opencv2/opencv.hpp>  
#include <opencv2/features2d.hpp> 
#include <opencv2/video.hpp> 
#include <tesseract/baseapi.h>  
#include <tesseract/publictypes.h>  
#include "authorinfo/author_info.h" 
#include <vector>  
#include <iostream>  
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cmath> // For std::abs
#include <map>
#include <set>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <ranges> //std::views
#include <cstdint> 
#include <functional>
#include <cstdlib>
#include <unordered_set>
#include <iterator> 
#include <utility>        // For std::pair  
#include <execution> // for parallel execution policies (C++17)
#include <boost/functional/hash.hpp> // For boost::hash 
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "cvLib.h"
class subfunctions{
    struct pair_hash {  
        template <class T>  
        std::size_t operator() (const std::pair<T, T>& pair) const {  
            auto hash1 = std::hash<T>{}(pair.first);  
            auto hash2 = std::hash<T>{}(pair.second);  
            return hash1 ^ hash2; // Combine the two hash values  
        }  
    };  
    using PairSet = std::unordered_set<std::pair<int, int>, pair_hash>;  
    public:
        /*
            Update std::unordered_map<std::string, std::vector<uint32_t>> value, if the first key exists, append data to second value
            otherwise,create a new key.
        */
        void updateMap(std::unordered_map<std::string, std::vector<uint32_t>>&, const std::string&, const std::vector<uint32_t>&);
        void convertToBlackAndWhite(cv::Mat&, std::vector<std::vector<RGB>>&);
        // Function to convert a dataset to cv::Mat  
        cv::Mat convertDatasetToMat(const std::vector<std::vector<RGB>>&);
        void markVideo(cv::Mat&, const cv::Scalar&,const cv::Scalar&);
        // Function to check if a point is inside a polygon  
        //para1:x, para2:y , para3: polygon
        bool isPointInPolygon(int, int, const std::vector<std::pair<int, int>>&);
        // Function to get all pixels inside the object defined by A  
        std::vector<std::vector<RGB>> getPixelsInsideObject(const std::vector<std::vector<RGB>>&, const std::vector<std::pair<int, int>>&); 
        cv::Mat getObjectsInVideo(const cv::Mat&);
        void saveModel(const std::unordered_map<std::string, std::vector<cv::Mat>>&, const std::string&);
        void saveModel_keypoint(const std::unordered_map<std::string, std::vector<cv::KeyPoint>>&, const std::string&);
        void merge_without_duplicates(std::vector<uint32_t>&, const std::vector<uint32_t>&);
};
void subfunctions::updateMap(std::unordered_map<std::string, std::vector<uint32_t>>& myMap, const std::string& key, const std::vector<uint32_t>& get_img_uint) {
    // Use find to check if the key exists
    auto it = myMap.find(key);
    if (it != myMap.end()) {
        // Key exists, append the vector
        it->second.insert(it->second.end(), get_img_uint.begin(), get_img_uint.end());
    } else {
        // Key does not exist, insert a new key-value pair
        myMap[key] = get_img_uint;
    }
}
void subfunctions::convertToBlackAndWhite(cv::Mat& image, std::vector<std::vector<RGB>>& datasets) {
    if(datasets.empty() || datasets[C_0].empty()){
        std::cerr << "Error: datasets is empty!" << std::endl;
        return;
    }
    // Ensure datasets has the same dimensions as the image
    if (datasets.size() != image.rows) {
        std::cerr << "Error: The dataset's row count does not match the image row count.\n";
        return;
    }
    for (unsigned int i = 0; i < image.rows; ++i) {
        if (datasets[i].size() != image.cols) {
            std::cerr << "Error: The dataset's column count does not match the image column count.\n";
            return;
        }
        for (unsigned int j = 0; j < image.cols; ++j) {
            // Access the pixel and its RGB values
            cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);
            uint32_t r = pixel[C_2];
            uint32_t g = pixel[C_1];
            uint32_t b = pixel[C_0];
            // Calculate the grayscale value using the luminance method
            uint32_t grayValue = static_cast<uint32_t>(0.299 * r + 0.587 * g + 0.114 * b);
            // Determine the binary value with thresholding
            uint32_t bwValue = (grayValue < 128) ? 0 : 255;
            // Assign the calculated binary value to the image
            pixel[C_0] = bwValue;
            pixel[C_1] = bwValue;
            pixel[C_2] = bwValue;
            // Update the corresponding dataset
            datasets[i][j] = {bwValue, bwValue, bwValue};
        }
    }
}
cv::Mat subfunctions::convertDatasetToMat(const std::vector<std::vector<RGB>>& dataset) { 
    if (dataset.empty() || dataset[C_0].empty()) {  
        throw std::runtime_error("Dataset is empty or has no columns.");  
    }   
    unsigned int rows = dataset.size();  
    unsigned int cols = dataset[0].size();  
    cv::Mat image(rows, cols, CV_8UC3); // Create a Mat with 3 channels (BGR)  
    for (unsigned int i = 0; i < rows; ++i) {  
        for (unsigned int j = 0; j < cols; ++j) {  
            // Access the RGB values from the dataset  
            const RGB& rgb = dataset[i][j];  
            // Set the pixel in the cv::Mat  
            image.at<cv::Vec3b>(i, j) = cv::Vec3b(rgb.b, rgb.g, rgb.r); // OpenCV uses BGR format  
        }  
    }  
    return image;  
}  
void subfunctions::markVideo(cv::Mat& frame,const cv::Scalar& brush_color, const cv::Scalar& bg_color){
    if(frame.empty()){
        return;
    }
    cvLib cvl_j;
    std::vector<std::vector<RGB>> frame_to_mark = cvl_j.cv_mat_to_dataset(frame);
    if(!frame_to_mark.empty()){
        std::vector<std::pair<int, int>> outliers_found = cvl_j.findOutlierEdges(frame_to_mark, 9);
        if(!outliers_found.empty()){
            cvl_j.markOutliers(frame_to_mark,outliers_found,brush_color);
            // Create a blank image with a black background  
            cv::Mat outputImage(frame.rows, frame.cols, CV_8UC3, bg_color); // Black background  
            outputImage = this->convertDatasetToMat(frame_to_mark);
            outputImage.copyTo(frame); 
        }
    }
}
// Function to check if a point is inside a polygon  
bool subfunctions::isPointInPolygon(int x, int y, const std::vector<std::pair<int, int>>& polygon) {  
    bool inside = false;  
    int n = polygon.size();  
    for (int i = 0, j = n - 1; i < n; j = i++) {  
        if ((polygon[i].second > y) != (polygon[j].second > y) &&  
            (x < (polygon[j].first - polygon[i].first) * (y - polygon[i].second) / (polygon[j].second - polygon[i].second) + polygon[i].first)) {  
            inside = !inside;  
        }  
    }  
    return inside;  
}  
std::vector<std::vector<RGB>> subfunctions::getPixelsInsideObject(const std::vector<std::vector<RGB>>& image_rgb, const std::vector<std::pair<int, int>>& objEdges) {  
    if(image_rgb.empty() || image_rgb[C_0].empty() || objEdges.empty()) {  
        return {}; // Return an empty image if no data or edges
    }
    std::vector<std::vector<RGB>> output_objs = image_rgb;
    std::unordered_set<std::pair<int, int>, boost::hash<std::pair<int, int>>> objSet(objEdges.begin(), objEdges.end());
    for (int x = 0; x < output_objs.size(); ++x) {  
        for (int y = 0; y < output_objs[x].size(); ++y) {  
            if (objSet.find({x, y}) == objSet.end()) {  
                output_objs[x][y] = {255, 255, 255}; // Set to white
            }
        }
    }
    return output_objs;
}
cv::Mat subfunctions::getObjectsInVideo(const cv::Mat& inVideo){
    cvLib cv_j;
    std::vector<std::vector<RGB>> objects_detect;
    std::vector<std::vector<RGB>> image_rgb = cv_j.cv_mat_to_dataset_color(inVideo);  
    if (!image_rgb.empty()) {  
        // Find outliers (edges)  
        auto outliers = cv_j.findOutlierEdges(image_rgb, 5);   
        objects_detect = this->getPixelsInsideObject(image_rgb, outliers);  
    }  
    cv::Mat finalV = this->convertDatasetToMat(objects_detect);
    return finalV;  
}
void subfunctions::saveModel(const std::unordered_map<std::string, std::vector<cv::Mat>>& featureMap, const std::string& filename){
    if(filename.empty()){
        return;
    }
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Unable to open file for writing.");
    }
    size_t mapSize = featureMap.size();
    ofs.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
    for (const auto& [className, features] : featureMap) {
        size_t keySize = className.size();
        ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
        ofs.write(className.c_str(), keySize);
        size_t featureCount = features.size();
        ofs.write(reinterpret_cast<const char*>(&featureCount), sizeof(featureCount));
        for (const auto& desc : features) {
            int rows = desc.rows;
            int cols = desc.cols;
            int type = desc.type();
            ofs.write(reinterpret_cast<const char*>(&rows), sizeof(int));
            ofs.write(reinterpret_cast<const char*>(&cols), sizeof(int));
            ofs.write(reinterpret_cast<const char*>(&type), sizeof(int));
            ofs.write(reinterpret_cast<const char*>(desc.data), desc.elemSize() * rows * cols);
        }
    }
    ofs.close();
}
void subfunctions::saveModel_keypoint(const std::unordered_map<std::string, std::vector<cv::KeyPoint>>& featureMap, const std::string& filename) {
    if (filename.empty()) {
        return;
    }
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Unable to open file for writing.");
    }
    try {
        size_t mapSize = featureMap.size();
        ofs.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
        for (const auto& [className, keypoints] : featureMap) {
            size_t keySize = className.size();
            ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
            ofs.write(className.data(), keySize);
            size_t keypointCount = keypoints.size();
            ofs.write(reinterpret_cast<const char*>(&keypointCount), sizeof(keypointCount));
            for (const auto& kp : keypoints) {
                ofs.write(reinterpret_cast<const char*>(&kp.pt.x), sizeof(kp.pt.x));
                ofs.write(reinterpret_cast<const char*>(&kp.pt.y), sizeof(kp.pt.y));
                ofs.write(reinterpret_cast<const char*>(&kp.size), sizeof(kp.size));
                ofs.write(reinterpret_cast<const char*>(&kp.angle), sizeof(kp.angle));
                ofs.write(reinterpret_cast<const char*>(&kp.response), sizeof(kp.response));
                ofs.write(reinterpret_cast<const char*>(&kp.octave), sizeof(kp.octave));
                ofs.write(reinterpret_cast<const char*>(&kp.class_id), sizeof(kp.class_id));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error writing to file: " << e.what() << std::endl;
    }
    ofs.close();
}
void subfunctions::merge_without_duplicates(std::vector<uint32_t>& data_main, const std::vector<uint32_t>& data_append) {  
    if(data_main.empty() || data_append.empty()){
        return;
    }
    std::map<uint32_t,unsigned int> mNoDup;
    for (const auto& key : data_main) {
        mNoDup[key] = 0;
    }
    for (const auto& subItem : data_append) {
        // Check if subItem already exists in the map
        if (mNoDup.find(subItem) != mNoDup.end()) {
            // If it exists, increment the value by 1
            mNoDup[subItem]++;
        } else {
            // If it does not exist, add it to the map with a value of 0
            mNoDup[subItem] = 0;
        }
    }
    // Create a vector of pairs from the map
    std::vector<std::pair<uint32_t, unsigned int>> vec(mNoDup.begin(), mNoDup.end());
    // Sort the vector by the values in descending order
    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    data_main.clear();
    for(const auto& item : vec){
        data_main.push_back(item.first);
    }
}  
/*
    Start cvLib -----------------------------------------------------------------------------------------------------
*/
cv::Mat cvLib::preprocessImage(const std::string& imgPath, const outputImgMode& img_mode) {
    // Step 1: Open the image using cv::imread
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Error: Could not open or find the image." << std::endl;
        return cv::Mat();
    }
    // Step 2: Resize the image to 120x120 pixels
    cv::Mat resizedImage;
    cv::resize(image, resizedImage, cv::Size(120, 120));
    if(img_mode == outputImgMode::Color){
        return resizedImage;
    }
    else if(img_mode == outputImgMode::Gray){
        cv::Mat gray_image;
        cv::cvtColor(resizedImage, gray_image, cv::COLOR_BGR2GRAY);
        return gray_image;
    }
    // Step 3: Convert the image to CV_32F and normalize
    // cv::Mat floatImage;
    // resizedImage.convertTo(floatImage, CV_32F);
    // floatImage /= 255.0; // Normalize to [0, 1] for machine learning applications
}
std::vector<std::vector<RGB>> cvLib::get_img_120_gray_for_ML(const std::string& imgPath) {  
    if(imgPath.empty()){
        return {};
    }
    std::vector<std::vector<RGB>> datasets;  
    cv::Mat image = this->preprocessImage(imgPath,outputImgMode::Gray); 
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image,gray_image;
    cv::resize(image, resized_image, cv::Size(120, 120));
    cv::cvtColor(resized_image, gray_image, cv::COLOR_BGR2GRAY);
    // Assume cv_mat_to_dataset_color(resized_image) is implemented correctly
    datasets = this->cv_mat_to_dataset_color(gray_image);
    return datasets;  
}
unsigned int cvLib::count_occurrences(const std::vector<uint32_t>& main_data, const std::vector<uint32_t>& sub_data) {  
    if (sub_data.empty()) return 0;  // Return zero if sub_data is empty  
    // Use std::search and std::ranges to count occurrences  
    unsigned int count = 0;  
    auto it = std::search(main_data.begin(), main_data.end(),   
                          sub_data.begin(), sub_data.end());  
    while (it != main_data.end()) {  
        ++count;  // Found one occurrence  
        // Search for the next occurrence  
        it = std::search(std::next(it), main_data.end(), sub_data.begin(), sub_data.end());  
    }  
    return count;  
}  
// Function to convert std::vector<uint32_t> to std::vector<std::vector<RGB>>  
std::vector<std::vector<RGB>> cvLib::convertToRGB(const std::vector<uint32_t>& pixels, unsigned int width, unsigned int height) {  
    std::vector<std::vector<RGB>> image(height, std::vector<RGB>(width));  
    for (unsigned int y = 0; y < height; ++y) {  
        for (unsigned int x = 0; x < width; ++x) {  
            uint32_t packedPixel = pixels[y * width + x];  
            uint32_t r = (packedPixel >> 16) & 0xFF; // Extract red  
            uint32_t g = (packedPixel >> 8) & 0xFF;  // Extract green  
            uint32_t b = packedPixel & 0xFF;         // Extract blue  
            image[y][x] = RGB(r, g, b); // Assign to 2D vector  
        }  
    }  
    return image; // Return the resulting 2D RGB vector  
}  
// Function to convert std::vector<std::vector<RGB>> back to std::vector<uint32_t>  
std::vector<uint32_t> cvLib::convertToPacked(const std::vector<std::vector<RGB>>& image) {  
    unsigned int height = image.size();  
    unsigned int width = (height > 0) ? image[0].size() : 0;  
    std::vector<uint32_t> pixels(height * width);  
    for (unsigned int y = 0; y < height; ++y) {  
        for (unsigned int x = 0; x < width; ++x) {  
            const RGB& rgb = image[y][x];  
            // Pack the RGB into a uint32_t  
            pixels[y * width + x] = (static_cast<uint32_t>(rgb.r) << 16) |  
                                     (static_cast<uint32_t>(rgb.g) << 8) |  
                                     (static_cast<uint32_t>(rgb.b));  
        }  
    }  
    return pixels; // Return the resulting packed pixel vector  
}  
// Function to convert std::vector<uint32_t> to cv::Mat  
cv::Mat cvLib::vectorToImage(const std::vector<uint32_t>& pixels, unsigned int width, unsigned int height) {  
    // Create a cv::Mat object with the specified dimensions and type (CV_8UC3 for BGR)  
    cv::Mat image(height, width, CV_8UC3);  
    // Iterate through each pixel in the vector  
    for (unsigned int y = 0; y < height; ++y) {  
        for (unsigned int x = 0; x < width; ++x) {  
            // Get the packed pixel from the vector  
            uint32_t packedPixel = pixels[y * width + x];  
            // Unpack the pixel components  
            uint32_t b = (packedPixel & 0xFF);            // Blue component  
            uint32_t g = (packedPixel >> 8) & 0xFF;       // Green component  
            uint32_t r = (packedPixel >> 16) & 0xFF;      // Red component  
            // uint32_t a = (packedPixel >> 24) & 0xFF;    // Alpha component (if needed)  
            // Set the pixel value in the cv::Mat (OpenCV uses BGR format)  
            image.at<cv::Vec3b>(y, x) = cv::Vec3b(b, g, r);  
        }  
    }  
    return image;  // Return the created image  
}  
/*
    This function to convert an cv::Mat into a std::vector<std::vector<RGB>> dataset
*/
std::vector<std::vector<RGB>> cvLib::cv_mat_to_dataset(const cv::Mat& genImg){
    std::vector<std::vector<RGB>> datasets(genImg.rows,std::vector<RGB>(genImg.cols));
    for (unsigned int i = 0; i < genImg.rows; ++i) {  //rows
        for (unsigned int j = 0; j < genImg.cols; ++j) {  //cols
            // Get the intensity value  
            uchar intensity = genImg.at<uchar>(i, j);  
            // Populate the RGB struct for grayscale  
            datasets[i][j] = {static_cast<uint32_t>(intensity), static_cast<uint32_t>(intensity), static_cast<uint32_t>(intensity)};  
        }  
    }  
    return datasets;
}
std::vector<std::vector<RGB>> cvLib::cv_mat_to_dataset_color(const cv::Mat& genImg) {  
    // Create a copy of the input image to apply noise reduction
    cv::Mat processedImg;
    // Noise reduction step using Gaussian blur
    cv::GaussianBlur(genImg, processedImg, cv::Size(5, 5), 0);
    // Or alternatively, use median blur (uncomment if you want to use this instead)
    // cv::medianBlur(genImg, processedImg, 5);
    // Initialize the dataset with the processed image
    std::vector<std::vector<RGB>> datasets(processedImg.rows, std::vector<RGB>(processedImg.cols));  
    for (unsigned int i = 0; i < processedImg.rows; ++i) {  // rows  
        for (unsigned int j = 0; j < processedImg.cols; ++j) {  // cols  
            // Check if the image is still in color:  
            if (processedImg.channels() == 3) {  
                cv::Vec3b bgr = processedImg.at<cv::Vec3b>(i, j);  
                // Populate the RGB struct  
                datasets[i][j] = {static_cast<uint32_t>(bgr[C_2]), static_cast<uint32_t>(bgr[C_1]), static_cast<uint32_t>(bgr[C_0])}; // Convert BGR to RGB  
            } else if (processedImg.channels() == 1) {  
                // Handle grayscale images  
                uchar intensity = processedImg.at<uchar>(i, j);  
                datasets[i][j] = {static_cast<uint32_t>(intensity), static_cast<uint32_t>(intensity), static_cast<uint32_t>(intensity)}; // Grayscale to RGB  
            }   
        }  
    }  
    return datasets;  
}
imgSize cvLib::get_image_size(const std::string& imgPath) {  
    imgSize im_s = {0, 0}; // Initialize width and height to 0  
    if (imgPath.empty()) {  
        std::cerr << "Error: Image path is empty." << std::endl;  
        return im_s; // Return default initialized size  
    }  
    cv::Mat img = cv::imread(imgPath);  
    if (img.empty()) { // Check if the image was loaded successfully  
        std::cerr << "Error: Could not open or find the image at " << imgPath << std::endl;  
        return im_s; // Return default initialized size  
    }  
    im_s.width = img.cols;  
    im_s.height = img.rows;  
    return im_s;   
}
/*
    cv::Scalar markerColor(0,255,0);
    cv::Scalar txtColor(255, 255, 255);
    rec_thickness = 2;
*/
// Function to draw a green rectangle on an image  
void cvLib::drawRectangleWithText(cv::Mat& image, int x, int y, unsigned int width, unsigned int height, const std::string& text, unsigned int rec_thickness, const cv::Scalar& markerColor, const cv::Scalar& txtColor) {  
    // Define rectangle vertices  
    cv::Point top_left(x, y);  
    cv::Point bottom_right(x + width, y + height);  
    // Draw the rectangle  
    cv::rectangle(image, top_left, bottom_right, markerColor, rec_thickness); // Rectangle with specified thickness and color  
    // Define text position at the upper-left corner of the rectangle  
    cv::Point text_position(x + 5, y - 15); // Adjust as necessary, '5' from left and '20' from top for padding  
    // Add text  
    cv::putText(image, text, text_position, cv::FONT_HERSHEY_SIMPLEX, 0.6, txtColor, 1); // Text positioning based on updated text_position  
}
void cvLib::savePPM(const cv::Mat& image, const std::string& filename) {  
    std::ofstream ofs(filename, std::ios::binary);  
    if (!ofs) {  
        std::cerr << "Error opening file for writing: " << filename << std::endl;  
        return;  
    }  
    ofs << "P6\n" << image.cols << ' ' << image.rows << "\n255\n";  
    ofs.write(reinterpret_cast<const char*>(image.data), image.total() * image.elemSize());  
}  
void cvLib::saveVectorRGB(const std::vector<std::vector<RGB>>& img, unsigned int width, unsigned int height, const std::string& filename){
    std::ofstream out(filename);  
    if (!out) {  
        std::cerr << "Error opening file for writing: " << filename << std::endl;  
        return;  
    }  
    // Write the PPM header  
    out << "P3\n"; // PPM magic number  
    out << "# Created by saveImage function\n"; // Comment line  
    out << width << " " << height << "\n"; // Image dimensions  
    out << "255\n"; // Maximum color value  
    // Write pixel data  
    for (const auto& row : img) {  
        for (const auto& item : row) {  
            out << item.r << " " << item.g << " " << item.b << "\n";  
        }  
    }  
    out.close();  
}
std::vector<std::vector<RGB>> cvLib::get_img_matrix(const std::string& imgPath, unsigned int img_rows, unsigned int img_cols) {  
    if(imgPath.empty()){
        return {};
    }
    std::vector<std::vector<RGB>> datasets(img_rows, std::vector<RGB>(img_cols));  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image, gray_image;  
    cv::resize(image, resized_image, cv::Size(img_cols, img_rows));
    cv::cvtColor(resized_image, gray_image, cv::COLOR_BGR2GRAY);
    // Assume cv_mat_to_dataset(gray_image) is implemented correctly
    datasets = this->cv_mat_to_dataset(gray_image);
    return datasets;  
}
std::vector<std::vector<RGB>> cvLib::get_img_matrix_color(const std::string& imgPath, unsigned int img_rows, unsigned int img_cols) {  
    if(imgPath.empty()){
        return {};
    }
    std::vector<std::vector<RGB>> datasets(img_rows, std::vector<RGB>(img_cols));  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image;
    cv::resize(image, resized_image, cv::Size(img_cols, img_rows));
    // Assume cv_mat_to_dataset_color(resized_image) is implemented correctly
    datasets = this->cv_mat_to_dataset_color(resized_image);
    return datasets;  
}
std::multimap<std::string, std::vector<RGB>> cvLib::read_images(std::string& folderPath){  
    std::multimap<std::string, std::vector<RGB>> datasets;  
    if(folderPath.empty()){  
        return datasets;  
    }  
    if(folderPath.back() != '/'){
        folderPath.append("/");
    }
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {  
        if (entry.is_regular_file()) {  
            std::string imgPath = entry.path().filename().string();
            std::string nuts_key = folderPath + imgPath;
            imgSize img_size;
            img_size = this->get_image_size(nuts_key);
            std::vector<std::vector<RGB>> image_rgb = this->get_img_matrix(nuts_key,img_size.width,img_size.height);  
            std::vector<RGB> one_d_image_rgb;
            for(const auto& mg : image_rgb){
                for(const auto& m : mg){
                    RGB data_add = m;
                    one_d_image_rgb.push_back(data_add);
                }
            }
            datasets.insert({nuts_key,one_d_image_rgb});
        }  
    }  
    return datasets;  
}
// Function to find outlier edges using a simple gradient method  
std::vector<std::pair<int, int>> cvLib::findOutlierEdges(const std::vector<std::vector<RGB>>& data, unsigned int gradientMagnitude_threshold) {
    std::vector<std::pair<int, int>> outliers;
    if (data.empty() || data[C_0].empty()) {
        return outliers;
    }
    unsigned int height = data.size();
    unsigned int width = data[C_0].size();
    for (unsigned int i = 1; i < height - 1; ++i) {
        for (unsigned int j = 1; j < width - 1; ++j) {
            int gx = -data[i-1][j-1].r + data[i+1][j+1].r +  
                     -2 * data[i][j-1].r + 2 * data[i][j+1].r +  
                     -data[i-1][j+1].r + data[i+1][j-1].r;  
            int gy = data[i-1][j-1].r + 2 * data[i-1][j].r + data[i-1][j+1].r -  
                     (data[i+1][j-1].r + 2 * data[i+1][j].r + data[i+1][j+1].r);  
            double gradientMagnitude = std::sqrt(gx * gx + gy * gy);
            if (gradientMagnitude > gradientMagnitude_threshold) {
                outliers.emplace_back(i, j);
            }
        }
    }
    return outliers;
}
bool cvLib::saveImage(const std::vector<std::vector<RGB>>& data, const std::string& filename){ 
    if (data.empty() || data[C_0].empty()) {  
        std::cerr << "Error: Image data is empty." << std::endl;  
        return false;  
    }  
    std::ofstream file(filename);  
    if (!file) {  
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;  
        return false;  
    }  
    unsigned int width = data[0].size();  
    unsigned int height = data.size();  
    file << "P3\n" << width << " " << height << "\n255\n"; // PPM header  
    try {  
        for (unsigned int i = 0; i < height; ++i) {  
            for (unsigned int j = 0; j < width; ++j) {  
                const RGB& rgb = data.at(i).at(j);  // Use at() for safety  
                // Ensure RGB values are within valid range  
                if (rgb.r > 255 || rgb.g > 255 || rgb.b > 255) {  
                    throw std::runtime_error("RGB values must be in the range [0, 255]");  
                }  
                file << static_cast<uint32_t>(rgb.r) << " " << static_cast<uint32_t>(rgb.g) << " " << static_cast<uint32_t>(rgb.b) << "\n";  
            }  
        }  
    } catch (const std::exception& e) {  
        std::cerr << "Error: " << e.what() << std::endl;  
        return false;  
    }  
    file.close();  
    std::cout << "Image saved as " << filename << std::endl;  
    return true; // Return true if saving was successful  
}
void cvLib::markOutliers(std::vector<std::vector<RGB>>& data, const std::vector<std::pair<int, int>>& outliers, const cv::Scalar& markerColor) {
        if (data.empty() || data[C_0].empty()) {  
            return;  
        }  
        if (outliers.empty()) return;  

        // Ensure markerColor checks correctly correspond to RGB
        bool isEmpty = (markerColor[C_0] == 0 && markerColor[C_1] == 0 && markerColor[C_2] == 0);
        if (isEmpty) return;
        int minX = std::numeric_limits<int>::max();  
        int minY = std::numeric_limits<int>::max();  
        int maxX = std::numeric_limits<int>::min();  
        int maxY = std::numeric_limits<int>::min();  
        for (const auto& outlier : outliers) {
            int x = outlier.first;  
            int y = outlier.second;  
            // Ensure x and y are within bounds
            if (x >= 0 && x < data.size() && y >= 0 && y < data[x].size()) {
                // Update bounding box calculations
                minX = std::min(minX, x);  
                minY = std::min(minY, y);  
                maxX = std::max(maxX, x);  
                maxY = std::max(maxY, y);
                // Mark the outlier as specified by markerColor
                data[x][y] = {static_cast<uint32_t>(markerColor[C_0]), static_cast<uint32_t>(markerColor[C_1]), static_cast<uint32_t>(markerColor[C_2])};
            }
        }
        // [Optional] Use minX, minY, maxX, maxY for further bounding box logic
}
void cvLib::createOutlierImage(const std::vector<std::vector<RGB>>& originalData, const std::vector<std::pair<int, int>>& outliers, const std::string& outImgPath, const cv::Scalar& bgColor) {
    if (originalData.empty() || originalData[C_0].empty()) {
        std::cerr << "Original data is empty, nothing to process." << std::endl;
        return;  
    }
    if (outliers.empty()) {  
        std::cerr << "No outliers provided." << std::endl;  
        return;  
    }  
    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int maxY = std::numeric_limits<int>::min();
    for (const auto& outlier : outliers) {  
        minX = std::min(minX, outlier.first);  
        minY = std::min(minY, outlier.second);  
        maxX = std::max(maxX, outlier.first);  
        maxY = std::max(maxY, outlier.second);  
    }  
    int outputWidth = maxY - minY + 1;  
    int outputHeight = maxX - minX + 1;  
    cv::Mat outputImage(outputHeight, outputWidth, CV_8UC3, cv::Scalar(bgColor[C_0], bgColor[C_1], bgColor[C_2]));
    for (const auto& outlier : outliers) {  
        int outlierX = outlier.first;  
        int outlierY = outlier.second;  
        if (outlierX < originalData.size() && outlierY < originalData[C_0].size()) {  
            int newX = outlierY - minY;  
            int newY = outlierX - minX;  
            RGB pixel = originalData[outlierX][outlierY];  
            outputImage.at<cv::Vec3b>(newY, newX) = cv::Vec3b(pixel.b, pixel.g, pixel.r);
        }
    }  
    cv::imwrite(outImgPath, outputImage);
    // Example usage for saving the output or processing as expected within defined functions in cvLib:
    // this->savePPM(outputImage, outImgPath); 
    // this->saveImage(originalData, outImgPath + "_orig.ppm");
    std::cout << "Outlier image saved successfully." << std::endl;
}
void cvLib::read_image_detect_edges(const std::string& imagePath,unsigned int gradientMagnitude_threshold,const std::string& outImgPath,const brushColor& markerColor, const brushColor& bgColor){
    if(imagePath.empty()){
        return;
    }
    std::vector<RGB> pixelToPaint;
    imgSize img_size = this->get_image_size(imagePath);
    std::vector<std::vector<RGB>> image_rgb = this->get_img_matrix(imagePath,img_size.width,img_size.height);  
    // Find outliers (edges)  
    auto outliers = this->findOutlierEdges(image_rgb,gradientMagnitude_threshold); 
    cv::Scalar brushMarkerColor;  
    switch (markerColor) {  
        case brushColor::Green:  
            brushMarkerColor = cv::Scalar(0, 255, 0);  
            break;  
        case brushColor::Red:  
            brushMarkerColor = cv::Scalar(255, 0, 0);  
            break;  
        case brushColor::White:  
            brushMarkerColor = cv::Scalar(255, 255, 255);  
            break;  
        case brushColor::Black:  
            brushMarkerColor = cv::Scalar(0, 0, 0);  
            break;  
        default:  
            std::cerr << "Error: Unexpected marker color" << std::endl;  
            return; // or handle an error  
    }    
    this->markOutliers(image_rgb, outliers, brushMarkerColor);  
    // //Save the modified image to a file  
    std::string ppmFile = outImgPath;
    //save selection images
    cv::Scalar brushbgColor;  
    switch (bgColor) {  
        case brushColor::Green:  
            brushbgColor = cv::Scalar(0, 255, 0);  
            break;  
        case brushColor::Red:  
            brushbgColor = cv::Scalar(255, 0, 0);  
            break;  
        case brushColor::White:  
            brushbgColor = cv::Scalar(255, 255, 255);  
            break;  
        case brushColor::Black:  
            brushbgColor = cv::Scalar(0, 0, 0);  
            break;  
        default:  
            std::cerr << "Error: Unexpected marker color" << std::endl;  
            return; // or handle an error  
    }    
    this->createOutlierImage(image_rgb, outliers,outImgPath,brushbgColor);
}
void cvLib::convertToBlackAndWhite(cv::Mat& image, std::vector<std::vector<RGB>>& datasets) {
        // Ensure datasets has the same dimensions as the image
        if (datasets.size() != image.rows) {
            std::cerr << "Error: The dataset's row count does not match the image row count.\n";
            return;
        }
        for (unsigned int i = 0; i < image.rows; ++i) {
            if (datasets[i].size() != image.cols) {
                std::cerr << "Error: The dataset's column count does not match the image column count.\n";
                return;
            }
            for (unsigned int j = 0; j < image.cols; ++j) {
                // Access the pixel and its RGB values
                cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);
                uint32_t r = pixel[C_2];
                uint32_t g = pixel[C_1];
                uint32_t b = pixel[C_0];
                // Calculate the grayscale value using the luminance method
                uint32_t grayValue = static_cast<uint32_t>(0.299 * r + 0.587 * g + 0.114 * b);
                // Determine the binary value with thresholding
                uint32_t bwValue = (grayValue < 128) ? 0 : 255;
                // Assign the calculated binary value to the image
                pixel[C_0] = bwValue;
                pixel[C_1] = bwValue;
                pixel[C_2] = bwValue;
                // Update the corresponding dataset
                datasets[i][j] = {bwValue, bwValue, bwValue};
            }
        }
}
bool cvLib::read_image_detect_objs(const std::string& img1, const std::string& img2, unsigned int featureCount, float ratioThresh, unsigned int de_threshold) {  
    if (img1.empty() || img2.empty()) {  
        std::cerr << "Image paths are empty." << std::endl;  
        return false;  
    }  
    cv::Mat m_img1 = cv::imread(img1);  
    cv::Mat m_img2 = cv::imread(img2); 
    if (m_img1.empty() || m_img2.empty()) {  
        std::cerr << "Failed to read one or both images." << std::endl;  
        return false;  
    }  
    // Convert to grayscale
    cv::Mat gray1, gray2;  
    if (m_img1.channels() > 1) {  
        cv::cvtColor(m_img1, gray1, cv::COLOR_BGR2GRAY);  
    } else {  
        gray1 = m_img1;  
    }  
    if (m_img2.channels() > 1) {  
        cv::cvtColor(m_img2, gray2, cv::COLOR_BGR2GRAY);  
    } else {  
        gray2 = m_img2;  
    }  
    // Use ORB for keypoint detection and description
    cv::Ptr<cv::ORB> detector = cv::ORB::create(featureCount);  
    std::vector<cv::KeyPoint> keypoints1, keypoints2;  
    cv::Mat descriptors1, descriptors2;  
    std::cout << "Start processing..." << std::endl;  
    auto start = std::chrono::high_resolution_clock::now();  
    detector->detectAndCompute(gray1, cv::noArray(), keypoints1, descriptors1);  
    detector->detectAndCompute(gray2, cv::noArray(), keypoints2, descriptors2);  
    cv::BFMatcher matcher(cv::NORM_HAMMING);  
    std::vector<std::vector<cv::DMatch>> knnMatches;  
    matcher.knnMatch(descriptors1, descriptors2, knnMatches, 2);  
    // Apply the ratio test
    std::vector<cv::DMatch> goodMatches;  
    for (size_t i = 0; i < knnMatches.size(); i++) {  
        if (knnMatches[i].size() > 1 && knnMatches[i][C_0].distance < ratioThresh * knnMatches[i][C_1].distance) {  
            goodMatches.push_back(knnMatches[i][C_0]);  
        }  
    }  
    // Calculate the duration  
    auto end = std::chrono::high_resolution_clock::now();  
    std::chrono::duration<double> duration = end - start;  
    std::cout << "Execution time: " << duration.count() << " seconds\n";  
    std::cout << img1 << " score: " << goodMatches.size() << std::endl;  
    if (goodMatches.size() < de_threshold) {  
        return false;  
    } else {  
        std::vector<cv::Point2f> img1Points;  
        std::vector<cv::Point2f> img2Points;  
        for (size_t i = 0; i < goodMatches.size(); i++) {  
            img1Points.push_back(keypoints1[goodMatches[i].queryIdx].pt);  
            img2Points.push_back(keypoints2[goodMatches[i].trainIdx].pt);  
        }  
        if (img1Points.size() < 4 || img2Points.size() < 4) {  
            std::cerr << "Error: Not enough points to calculate homography. Need at least 4 pairs of points." << std::endl;  
            return false;  
        }  
        cv::Mat H;  
        try {  
            H = cv::findHomography(img1Points, img2Points, cv::RANSAC, 5.0);  
            if (H.empty()) {
                std::cerr << "Homography could not be calculated." << std::endl;
                return false;
            }
        } catch (const cv::Exception& e) {  
            std::cerr << "OpenCV error: " << e.what() << std::endl;  
            return false;  
        } catch (const std::exception& e) {  
            std::cerr << "Standard exception: " << e.what() << std::endl;  
            return false;  
        } catch (...) {  
            std::cerr << "Unknown exception occurred." << std::endl;  
            return false;  
        }  
        // Optionally visualize matches  
        cv::Mat img_matches;  
        cv::drawMatches(m_img1, keypoints1, m_img2, keypoints2, goodMatches, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);  
        cv::imshow("Matches", img_matches);
        std::string strimgout = img1;
        strimgout.append("_output.jpg");
        cv::imwrite(strimgout, img_matches);
        //cv::waitKey(0); // Uncomment this line to display the window interactively.
        return true;  
    }  
    return false;  
}
bool cvLib::isObjectInImage(const std::string& img1, const std::string& img2, unsigned int featureCount, float ratioThresh, unsigned int deThreshold) {  
    if (img1.empty() || img2.empty()) {  
        std::cerr << "Image paths are empty." << std::endl;  
        return false;  
    }  
    imgSize img1_size = this->get_image_size(img1);
    imgSize img2_size = this->get_image_size(img2);
    cv::Mat m_img1 = cv::imread(img1);  
    cv::Mat m_img2 = cv::imread(img2);  
    if (m_img1.empty() || m_img2.empty()) {  
        std::cerr << "Failed to read one or both images." << std::endl;  
        return false;  
    }  
    std::vector<std::vector<RGB>> dataset_img1(m_img1.rows, std::vector<RGB>(m_img1.cols));  
    std::vector<std::vector<RGB>> dataset_img2(m_img2.rows, std::vector<RGB>(m_img2.cols));  
    subfunctions sub_j;
    sub_j.convertToBlackAndWhite(m_img1, dataset_img1);
    sub_j.convertToBlackAndWhite(m_img2, dataset_img2);
    cv::Mat gray1 = sub_j.convertDatasetToMat(dataset_img1);
    cv::Mat gray2 = sub_j.convertDatasetToMat(dataset_img2);
    // Use ORB for keypoint detection and description  
    cv::Ptr<cv::ORB> detector = cv::ORB::create(featureCount);  
    std::vector<cv::KeyPoint> keypoints1, keypoints2;  
    cv::Mat descriptors1, descriptors2;  
    std::cout << "Start processing..." << std::endl;  
    auto start = std::chrono::high_resolution_clock::now();  
    detector->detectAndCompute(gray1, cv::noArray(), keypoints1, descriptors1);  
    detector->detectAndCompute(gray2, cv::noArray(), keypoints2, descriptors2);  
    cv::BFMatcher matcher(cv::NORM_HAMMING);  
    std::vector<std::vector<cv::DMatch>> knnMatches;  
    matcher.knnMatch(descriptors1, descriptors2, knnMatches, 2);  
    // Apply the ratio test as per Lowe's paper  
    std::vector<cv::DMatch> goodMatches;  
    for (size_t i = 0; i < knnMatches.size(); i++) {  
        if (knnMatches[i][0].distance < ratioThresh * knnMatches[i][1].distance) {  
            goodMatches.push_back(knnMatches[i][0]);  
        }  
    }  
    // Calculate the duration  
    auto end = std::chrono::high_resolution_clock::now();  
    std::chrono::duration<double> duration = end - start;  
    std::cout << "Execution time: " << duration.count() << " seconds\n";  
    std::cout << img1 << " score: " << goodMatches.size() << std::endl;  
    if (goodMatches.size() < deThreshold) {  
        return false;  
    } else {  
        std::vector<cv::Point2f> img1Points;  
        std::vector<cv::Point2f> img2Points;  
        for (size_t i = 0; i < goodMatches.size(); i++) {  
            img1Points.push_back(keypoints1[goodMatches[i].queryIdx].pt);  
            img2Points.push_back(keypoints2[goodMatches[i].trainIdx].pt);  
        }  
        if (img1Points.size() < 4 || img2Points.size() < 4) {  
            std::cerr << "Error: Not enough points to calculate homography. Need at least 4 pairs of points." << std::endl;  
            return false;  
        }  
        cv::Mat H;  
        try {  
            H = cv::findHomography(img1Points, img2Points, cv::RANSAC, 5.0);  
        } catch (const cv::Exception& e) {  
            std::cerr << "OpenCV error: " << e.what() << std::endl;  
            return false;  
        } catch (const std::exception& e) {  
            std::cerr << "Standard exception: " << e.what() << std::endl;  
            return false;  
        } catch (...) {  
            std::cerr << "Unknown exception occurred." << std::endl;  
            return false;  
        }  
        if (!H.empty()) {  
            // Optionally visualize matches  
            cv::Mat img_matches;  
            cv::drawMatches(m_img1, keypoints1, m_img2, keypoints2, goodMatches, img_matches, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);  
            cv::imshow("Matches", img_matches);  
            std::string strimgout = img1;  
            strimgout.append("_output.jpg");  
            cv::imwrite(strimgout, img_matches);  
            //cv::waitKey(0);  
            return true;  
        }  
    }  
    return false;  
}  
std::vector<std::vector<RGB>> cvLib::objectsInImage(const std::string& imgPath, unsigned int gradientMagnitude_threshold, const outputImgMode& out_mode) {  
    if (imgPath.empty()) {
        std::cerr << "Error: Image path is empty." << std::endl;
        return {};
    }
    subfunctions subfun;  
    imgSize img_size = this->get_image_size(imgPath);  
    std::vector<std::vector<RGB>> image_rgb;
    if(out_mode == outputImgMode::Color){
        image_rgb = this->get_img_matrix_color(imgPath, img_size.height, img_size.width);  
    }
    else if(out_mode == outputImgMode::Gray){
        image_rgb = this->get_img_matrix(imgPath, img_size.height, img_size.width);
    }
    if (!image_rgb.empty()) {
        auto outliers = this->findOutlierEdges(image_rgb, gradientMagnitude_threshold);
        return subfun.getPixelsInsideObject(image_rgb, outliers);
    }
    return {};
}
char* cvLib::read_image_detect_text(const std::string& imgPath){
    if(imgPath.empty()){
        return nullptr;
    }
    // Load the image  
    cv::Mat image = cv::imread(imgPath);  
    if (image.empty()) {  
        std::cerr << "Could not open or find the image!" << std::endl;  
        return nullptr; // Return nullptr to indicate failure  
    }  
    // Preprocessing: convert to grayscale  
    cv::Mat gray;  
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);  
    // Initialize Tesseract  
    tesseract::TessBaseAPI *ocr = new tesseract::TessBaseAPI();  
    if (ocr->Init(NULL, "eng+chi_sim")) { // Initialize for both English and Simplified Chinese  
        fprintf(stderr, "Could not initialize tesseract.\n");  
        return nullptr; // Return nullptr to indicate failure  
    }  
    // Set the image for recognition  
    ocr->SetImage(gray.data, gray.cols, gray.rows, 1, gray.step[0]);  
    // Get recognized text  
    char* text = ocr->GetUTF8Text();  
    ocr->End(); // Cleanup Tesseract object  
    return text; // Return the recognized text  
}
void cvLib::StartWebCam(unsigned int webcame_index,const std::string& winTitle,const std::vector<std::string>& imageListToFind,const cv::Scalar& brush_color,std::function<void(cv::Mat&)> callback){
    // Open the default camera (usually the first camera, index 0)  
    cv::VideoCapture cap(webcame_index);  
    // Check if the camera opened successfully  
    if (!cap.isOpened()) {  
        std::cerr << "Error: Could not open the webcam." << std::endl;  
        return;  
    }  
    // Create a window to display the video  
    //cv::namedWindow(winTitle, cv::WINDOW_AUTOSIZE); 
    //cv::resizeWindow(winTitle, 1024, 768);   
    //cv::Mat frame, gray, blurred, thresh;  // To hold each frame captured from the webcam  
    //cv::Ptr<cv::BackgroundSubtractor> pBackSub = cv::createBackgroundSubtractorMOG2(); 
    cv::Mat frame, thresh;  
    cv::Ptr<cv::BackgroundSubtractor> pBackSub = cv::createBackgroundSubtractorMOG2(500, 16, true); 
    std::vector<cv::Rect> detectedBoxes;  
    // Capture frames in a loop 
    subfunctions sub_j; 
    while (true) {  
        // Capture a new frame from the webcam  
        cap >> frame; // Alternatively, you can use cap.read(frame);  
        // Check if the frame is empty  
        if (frame.empty()) break;
        /*
            start marking on the input frame
        */
        //cv::Scalar bgColor(0,0,0);
        // sub_j.markVideo(frame,brush_color,bgColor);
        // cv::Mat oframe = sub_j.getObjectsInVideo(frame);
        // // Apply background subtraction  
        pBackSub->apply(frame, thresh);  
        // Find contours  
        std::vector<std::vector<cv::Point>> contours;  
        cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);  
        detectedBoxes.clear(); // Clear previous detections  
        for (const auto& contour : contours) {  
            double area = cv::contourArea(contour);  
            if (area > 1000) { // Minimum area threshold  
                cv::Rect boundingBox = cv::boundingRect(contour);  
                detectedBoxes.push_back(boundingBox);  
            }  
        }  
        // Merge overlapping rectangles  
        std::vector<cv::Rect> mergedBoxes;  
        for (const auto& box : detectedBoxes) {  
            bool merged = false;  
            for (auto& mergedBox : mergedBoxes) {  
                if ((mergedBox & box).area() > 0) { // Check for overlap  
                    mergedBox = mergedBox | box; // Merge boxes  
                    merged = true;  
                    break;  
                }  
            }  
            if (!merged) {  
                mergedBoxes.push_back(box);  
            }  
        }  
        // Draw merged rectangles  
        for (const auto& box : mergedBoxes) {  
            cv::rectangle(frame, box, cv::Scalar(0, 255, 0), 2); // Draw rectangle  
        }  
        /*
            end marking video
        */
        // Invoke the callback with the captured frame  
        if(callback){
            callback(frame);  
        }
        // Display the frame in the created window  
        //cv::imshow(winTitle, frame);  
        // Exit the loop if the user presses the 'q' key  
        char key = (char)cv::waitKey(30);  
        if (key == 'q') {  
            break; // Allow exit if 'q' is pressed  
        }  
    }  
    // Release the camera and close the window  
    cap.release();  
    cv::destroyAllWindows();  
}
std::vector<uint32_t> cvLib::get_one_image(const std::string& image_path) {
        std::vector<uint32_t> img_matrix;
        if (image_path.empty()) {
            return {};  // Return an empty vector
        }
        cv::Mat img = this->preprocessImage(image_path,outputImgMode::Gray);//this->get_img_120_gray_for_ML(imgFolderPath);
        cv::Mat gray_image;
        cv::cvtColor(img, gray_image, cv::COLOR_BGR2GRAY);
        if (gray_image.empty()) {
            throw std::runtime_error("Failed to load image: " + image_path);
        }
        img_matrix.reserve(gray_image.rows * gray_image.cols * gray_image.channels());
        for (int y = 0; y < gray_image.rows; ++y) {
            for (int x = 0; x < gray_image.cols; ++x) {
                if (gray_image.channels() == 4) {
                    cv::Vec4b pixel = gray_image.at<cv::Vec4b>(y, x); // BGRA
                    img_matrix.push_back(pixel[C_0]); // Blue
                    img_matrix.push_back(pixel[C_1]); // Green
                    img_matrix.push_back(pixel[C_2]); // Red
                    img_matrix.push_back(pixel[C_3]); // Alpha
                } else if (gray_image.channels() == 3) {
                    cv::Vec3b pixel = gray_image.at<cv::Vec3b>(y, x); // BGR
                    img_matrix.push_back(pixel[C_0]); // Blue
                    img_matrix.push_back(pixel[C_1]); // Green
                    img_matrix.push_back(pixel[C_2]); // Red
                    // Optionally: img_matrix.push_back(255); // Add Alpha if needed (assumed fully opaque)
                } else if (gray_image.channels() == 1) { // Handle grayscale
                    uint32_t pixel = gray_image.at<uint32_t>(y, x);
                    img_matrix.push_back(pixel); // Red as grayscale
                    img_matrix.push_back(pixel); // Green as grayscale
                    img_matrix.push_back(pixel); // Blue as grayscale
                    // Optionally: img_matrix.push_back(255); // Add Alpha if needed
                } else {
                    std::cerr << "Unsupported image format: " << gray_image.channels() << " channels." << std::endl;
                    throw std::runtime_error("Unsupported image format.");
                }
            }
        }
        return img_matrix;
}
std::vector<cv::KeyPoint> cvLib::extractORBFeatures(const cv::Mat& img, cv::Mat& descriptors) {
    cv::Ptr<cv::ORB> orb = cv::ORB::create();
    std::vector<cv::KeyPoint> keypoints;
    orb->detectAndCompute(img, cv::noArray(), keypoints, descriptors);
    return keypoints;
}
void cvLib::train_img_occurrences(const std::string& images_folder_path, const std::string& model_output_path, const std::string& model_output_key_path){
    std::unordered_map<std::string, std::vector<cv::Mat>> dataset; 
    std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>> dataset_keypoint;  
    if (images_folder_path.empty()) {  
        return;  
    }  
    subfunctions sub_j;
    try {  
        for (const auto& entryMainFolder : std::filesystem::directory_iterator(images_folder_path)) {  
            if (entryMainFolder.is_directory()) { // Check if the entry is a directory  
                std::string sub_folder_name = entryMainFolder.path().filename().string();  
                std::string sub_folder_path = entryMainFolder.path().string();  
                std::vector<cv::Mat> sub_folder_all_images;  
                std::vector<std::vector<cv::KeyPoint>> sub_folder_keypoints;
                // Accumulate pixel count for memory reservation  
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file()) {  
                        std::string imgFilePath = entrySubFolder.path().string();  
                        cv::Mat get_img = this->preprocessImage(imgFilePath,outputImgMode::Color);
                        if(!get_img.empty()){
                            cv::Mat descriptors;
                            std::vector<cv::KeyPoint> sub_key = this->extractORBFeatures(get_img,descriptors);
                            sub_folder_all_images.push_back(descriptors);
                            sub_folder_keypoints.push_back(sub_key);
                        }
                        else{
                            std::cout << imgFilePath << " (get_img is empty)!" << std::endl;
                        } 
                    }  
                }  
                dataset[sub_folder_name] = sub_folder_all_images;
                dataset_keypoint[sub_folder_name] = sub_folder_keypoints;
                std::cout << sub_folder_name << " is done!" << std::endl;  
            }  
        }  
    } catch (const std::filesystem::filesystem_error& e) {  
        std::cerr << "Filesystem error: " << e.what() << std::endl;  
        return; // Return an empty dataset in case of filesystem error  
    }  
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
    /*
        Summarize the dataset
    */
    std::cout << "Machine learning..." << std::endl;
    std::unordered_map<std::string, cv::Mat> summarizedDataset;
    for (const auto& [category, descriptors] : dataset) {
        if (descriptors.empty()) continue;
        cv::Mat sum = descriptors[C_0].clone();
        for (size_t i = 1; i < descriptors.size(); ++i) {
            sum += descriptors[i];
        }
        cv::Mat average = sum / static_cast<double>(descriptors.size());
        summarizedDataset[category] = average;
    }
    std::unordered_map<std::string, std::vector<cv::KeyPoint>> summarizedKeypoints;
    for (const auto& [category, keypointSets] : dataset_keypoint) {
        if (keypointSets.empty()) continue;
        // Accumulators for keypoint properties
        std::vector<double> xSum, ySum, sizeSum, angleSum;
        for (const auto& keypoints : keypointSets) {
            for (size_t i = 0; i < keypoints.size(); ++i) {
                if (i >= xSum.size()) {
                    // Initialize
                    xSum.push_back(0);
                    ySum.push_back(0);
                    sizeSum.push_back(0);
                    angleSum.push_back(0);
                }
                xSum[i] += keypoints[i].pt.x;
                ySum[i] += keypoints[i].pt.y;
                sizeSum[i] += keypoints[i].size;
                angleSum[i] += keypoints[i].angle;
            }
        }
        // Compute averages
        std::vector<cv::KeyPoint> averagedKeypoints;
        for (size_t i = 0; i < xSum.size(); ++i) {
            cv::KeyPoint kp;
            kp.pt.x = xSum[i] / keypointSets.size();
            kp.pt.y = ySum[i] / keypointSets.size();
            kp.size = sizeSum[i] / keypointSets.size();
            kp.angle = angleSum[i] / keypointSets.size();
            averagedKeypoints.push_back(kp);
        }
        summarizedKeypoints[category] = averagedKeypoints;  
    }
    std::cout << "Successfully saved the images into the dataset, all jobs are done!" << std::endl;  
    sub_j.saveModel(dataset, model_output_path); 
    sub_j.saveModel_keypoint(summarizedKeypoints,model_output_key_path); 
}
void cvLib::loadModel(std::unordered_map<std::string, std::vector<cv::Mat>>& featureMap, const std::string& filename){
   if(filename.empty()){
        return;
   }
   std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error: Unable to open file for reading." << std::endl;
        return;
    }
    size_t mapSize;
    ifs.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
    for (size_t i = 0; i < mapSize; ++i) {
        size_t keySize;
        ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
        std::string className(keySize, ' ');
        ifs.read(&className[C_0], keySize);
        size_t featureCount;
        ifs.read(reinterpret_cast<char*>(&featureCount), sizeof(featureCount));
        std::vector<cv::Mat> features;
        for (size_t j = 0; j < featureCount; ++j) {
            int rows, cols, type;
            ifs.read(reinterpret_cast<char*>(&rows), sizeof(rows));
            ifs.read(reinterpret_cast<char*>(&cols), sizeof(cols));
            ifs.read(reinterpret_cast<char*>(&type), sizeof(type));
            cv::Mat desc(rows, cols, type);
            ifs.read(reinterpret_cast<char*>(desc.data), desc.elemSize() * rows * cols);
            features.push_back(desc);
        }
        featureMap[className] = features;
    }
    ifs.close();
}  
void cvLib::loadModel_keypoint(std::unordered_map<std::string, std::vector<cv::KeyPoint>>& featureMap, const std::string& filename) {
    if (filename.empty()) {
        return;
    }
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error: Unable to open file for reading." << std::endl;
        return;
    }
    try {
        size_t mapSize;
        ifs.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
        for (size_t i = 0; i < mapSize; ++i) {
            size_t keySize;
            ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
            std::string className(keySize, '\0');  // Use '\0' to ensure proper string initialization
            ifs.read(&className[C_0], keySize);
            size_t keypointCount;
            ifs.read(reinterpret_cast<char*>(&keypointCount), sizeof(keypointCount));
            std::vector<cv::KeyPoint> keypoints(keypointCount);
            for (size_t j = 0; j < keypointCount; ++j) {
                cv::KeyPoint kp;
                ifs.read(reinterpret_cast<char*>(&kp.pt.x), sizeof(kp.pt.x));
                ifs.read(reinterpret_cast<char*>(&kp.pt.y), sizeof(kp.pt.y));
                ifs.read(reinterpret_cast<char*>(&kp.size), sizeof(kp.size));
                ifs.read(reinterpret_cast<char*>(&kp.angle), sizeof(kp.angle));
                ifs.read(reinterpret_cast<char*>(&kp.response), sizeof(kp.response));
                ifs.read(reinterpret_cast<char*>(&kp.octave), sizeof(kp.octave));
                ifs.read(reinterpret_cast<char*>(&kp.class_id), sizeof(kp.class_id));
                keypoints[j] = kp;
            }
            featureMap[className] = keypoints;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading from file: " << e.what() << std::endl;
    }
    ifs.close();
}


