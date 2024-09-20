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
        void saveModel(const std::unordered_map<std::string, std::vector<uint32_t>>&, const std::string&);
        unsigned int count_occurrences(const std::vector<uint32_t>&, const std::vector<uint32_t>&);
        void merge_without_duplicates(std::vector<uint32_t>&, const std::vector<uint32_t>&);
};
void subfunctions::convertToBlackAndWhite(cv::Mat& image, std::vector<std::vector<RGB>>& datasets) {   
    for (int i = 0; i < image.rows; ++i) {  
        for (int j = 0; j < image.cols; ++j) {  
            // Access the pixel and its RGB values  
            cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);  
            int r = pixel[2];  
            int g = pixel[1];  
            int b = pixel[0];  
            // Calculate the grayscale value using the luminance method  
            int grayValue = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);  
            // Determine the binary value with thresholding  
            int bwValue = (grayValue < 128) ? 0 : 255;  
            // Assign the calculated binary value to the image  
            pixel[0] = bwValue;  
            pixel[1] = bwValue;  
            pixel[2] = bwValue;  
            // Update the corresponding dataset  
            datasets[i][j] = {bwValue, bwValue, bwValue};  
        }  
    }  
}  
cv::Mat subfunctions::convertDatasetToMat(const std::vector<std::vector<RGB>>& dataset) { 
    if (dataset.empty() || dataset[0].empty()) {  
        throw std::runtime_error("Dataset is empty or has no columns.");  
    }   
    int rows = dataset.size();  
    int cols = dataset[0].size();  
    cv::Mat image(rows, cols, CV_8UC3); // Create a Mat with 3 channels (BGR)  
    for (int i = 0; i < rows; ++i) {  
        for (int j = 0; j < cols; ++j) {  
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
        std::vector<std::pair<int, int>> outliers_found = cvl_j.findOutlierEdges(frame_to_mark, 90);
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
    std::vector<std::vector<RGB>> output_objs = image_rgb; // Start with a copy of the original image  
    if (output_objs.empty() || output_objs[0].empty() || objEdges.empty()) {  
        return output_objs; // Return the original image if data or edges are empty  
    }  
    // Create a set of object coordinates for quick lookup  
    std::unordered_set<std::pair<int, int>, boost::hash<std::pair<int, int>>> objSet(objEdges.begin(), objEdges.end());  
    // Iterate through the image and modify pixels  
    for (int x = 0; x < output_objs.size(); ++x) {  
        for (int y = 0; y < output_objs[x].size(); ++y) {  
            // If the current pixel is not in the object edges, set it to white  
            if (objSet.find({x, y}) == objSet.end()) {  
                output_objs[x][y] = {255, 255, 255}; // Set to white  
            }  
        }  
    }  
    return output_objs; // Return the modified image  
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
void subfunctions::saveModel(const std::unordered_map<std::string, std::vector<uint32_t>>& content_in_imgs, const std::string& filename){
    std::ofstream ofs(filename, std::ios::binary);  
    if (!ofs) {  
        std::cerr << "Error opening file for writing: " << filename << std::endl;  
        return;  
    }  
    // Write the size of the unordered_map  
    size_t mapSize = content_in_imgs.size();  
    ofs.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));  
    // Write each key-value pair  
    for (const auto& pair : content_in_imgs) {  
        // Write the size of the string key  
        size_t keySize = pair.first.size();  
        ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));  
        ofs.write(pair.first.c_str(), keySize);  
        // Write the size of the vector  
        size_t vecSize = pair.second.size();  
        ofs.write(reinterpret_cast<const char*>(&vecSize), sizeof(vecSize));  
        ofs.write(reinterpret_cast<const char*>(pair.second.data()), vecSize * sizeof(uint32_t));  
    }  
    ofs.close();  
}
unsigned int subfunctions::count_occurrences(const std::vector<uint32_t>& main_data, const std::vector<uint32_t>& sub_data) {  
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
void subfunctions::merge_without_duplicates(std::vector<uint32_t>& data_main, const std::vector<uint32_t>& data_append) {  
    // Create a set to track unique elements  
    std::unordered_set<uint32_t> unique_elements(data_main.begin(), data_main.end());  
    // Iterate through data_append and add to data_main if not already present  
    for (const auto& item : data_append) {  
        // Attempt to insert the item into the set  
        if (unique_elements.insert(item).second) { // Only if it was not already in the set  
            data_main.push_back(item); // Append the unique item to data_main  
        }  
    }  
}  
/*
    Start cvLib -----------------------------------------------------------------------------------------------------
*/
// Function to convert std::vector<uint32_t> to std::vector<std::vector<RGB>>  
std::vector<std::vector<RGB>> cvLib::convertToRGB(const std::vector<uint32_t>& pixels, int width, int height) {  
    std::vector<std::vector<RGB>> image(height, std::vector<RGB>(width));  
    for (int y = 0; y < height; ++y) {  
        for (int x = 0; x < width; ++x) {  
            uint32_t packedPixel = pixels[y * width + x];  
            uint8_t r = (packedPixel >> 16) & 0xFF; // Extract red  
            uint8_t g = (packedPixel >> 8) & 0xFF;  // Extract green  
            uint8_t b = packedPixel & 0xFF;         // Extract blue  
            image[y][x] = RGB(r, g, b); // Assign to 2D vector  
        }  
    }  
    return image; // Return the resulting 2D RGB vector  
}  
// Function to convert std::vector<std::vector<RGB>> back to std::vector<uint32_t>  
std::vector<uint32_t> cvLib::convertToPacked(const std::vector<std::vector<RGB>>& image) {  
    int height = image.size();  
    int width = (height > 0) ? image[0].size() : 0;  
    std::vector<uint32_t> pixels(height * width);  
    for (int y = 0; y < height; ++y) {  
        for (int x = 0; x < width; ++x) {  
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
cv::Mat cvLib::vectorToImage(const std::vector<uint32_t>& pixels, int width, int height) {  
    // Create a cv::Mat object with the specified dimensions and type (CV_8UC3 for BGR)  
    cv::Mat image(height, width, CV_8UC3);  
    // Iterate through each pixel in the vector  
    for (int y = 0; y < height; ++y) {  
        for (int x = 0; x < width; ++x) {  
            // Get the packed pixel from the vector  
            uint32_t packedPixel = pixels[y * width + x];  
            // Unpack the pixel components  
            uint8_t b = (packedPixel & 0xFF);            // Blue component  
            uint8_t g = (packedPixel >> 8) & 0xFF;       // Green component  
            uint8_t r = (packedPixel >> 16) & 0xFF;      // Red component  
            // uint8_t a = (packedPixel >> 24) & 0xFF;    // Alpha component (if needed)  
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
    for (int i = 0; i < genImg.rows; ++i) {  //rows
        for (int j = 0; j < genImg.cols; ++j) {  //cols
            // Get the intensity value  
            uchar intensity = genImg.at<uchar>(i, j);  
            // Populate the RGB struct for grayscale  
            datasets[i][j] = {static_cast<int>(intensity), static_cast<int>(intensity), static_cast<int>(intensity)};  
        }  
    }  
    return datasets;
}
std::vector<std::vector<RGB>> cvLib::cv_mat_to_dataset_color(const cv::Mat& genImg) {  
    std::vector<std::vector<RGB>> datasets(genImg.rows, std::vector<RGB>(genImg.cols));  
    for (int i = 0; i < genImg.rows; ++i) {  // rows  
        for (int j = 0; j < genImg.cols; ++j) {  // cols  
            // Check if the image is still in color:  
            if (genImg.channels() == 3) {  
                cv::Vec3b bgr = genImg.at<cv::Vec3b>(i, j);  
                // Populate the RGB struct  
                datasets[i][j] = {static_cast<int>(bgr[2]), static_cast<int>(bgr[1]), static_cast<int>(bgr[0])}; // Convert BGR to RGB  
            } else if (genImg.channels() == 1) {  
                // Handle grayscale images  
                uchar intensity = genImg.at<uchar>(i, j);  
                datasets[i][j] = {static_cast<int>(intensity), static_cast<int>(intensity), static_cast<int>(intensity)}; // Grayscale to RGB  
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
void cvLib::drawRectangleWithText(cv::Mat& image, int x, int y, int width, int height, const std::string& text, int rec_thickness, const cv::Scalar& markerColor, const cv::Scalar& txtColor) {  
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
void cvLib::saveVectorRGB(const std::vector<std::vector<RGB>>& img, int width, int height, const std::string& filename){
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
std::vector<std::vector<RGB>> cvLib::get_img_matrix(const std::string& imgPath, int img_rows, int img_cols) {  
    std::vector<std::vector<RGB>> datasets(img_rows, std::vector<RGB>(img_cols)); // Create vector of vectors for RGB values  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(img_cols, img_rows)); // Corrected dimensions  
    cv::Mat gray_image;  
    cv::cvtColor(resized_image, gray_image, cv::COLOR_BGR2GRAY);   
    datasets = this->cv_mat_to_dataset(gray_image); // Ensure cv_mat_to_dataset handles this correctly  
    return datasets;  
}
std::vector<std::vector<RGB>> cvLib::get_img_matrix_color(const std::string& imgPath, int img_rows, int img_cols) {  
    std::vector<std::vector<RGB>> datasets(img_rows, std::vector<RGB>(img_cols)); // Create vector of vectors for RGB values  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(img_cols, img_rows)); // Corrected dimensions  
    datasets = this->cv_mat_to_dataset_color(image); // Ensure cv_mat_to_dataset handles this correctly  
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
std::vector<std::pair<int, int>> cvLib::findOutlierEdges(const std::vector<std::vector<RGB>>& data, int gradientMagnitude_threshold) {  
    std::vector<std::pair<int, int>> outliers;  
    if (data.empty() || data[0].empty()) { // Check for empty data  
        return outliers; // Return early if no data  
    }  
    int height = data.size();  
    int width = data[0].size();  
    for (int i = 1; i < height - 1; ++i) {  
        for (int j = 1; j < width - 1; ++j) {  
            // Calculate gradient using all channels  
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
    if (data.empty() || data[0].empty()) {  
        std::cerr << "Error: Image data is empty." << std::endl;  
        return false;  
    }  
    std::ofstream file(filename);  
    if (!file) {  
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;  
        return false;  
    }  
    int width = data[0].size();  
    int height = data.size();  
    file << "P3\n" << width << " " << height << "\n255\n"; // PPM header  
    try {  
        for (int i = 0; i < height; ++i) {  
            for (int j = 0; j < width; ++j) {  
                const RGB& rgb = data.at(i).at(j);  // Use at() for safety  
                // Ensure RGB values are within valid range  
                if (rgb.r > 255 || rgb.g > 255 || rgb.b > 255) {  
                    throw std::runtime_error("RGB values must be in the range [0, 255]");  
                }  
                file << static_cast<int>(rgb.r) << " " << static_cast<int>(rgb.g) << " " << static_cast<int>(rgb.b) << "\n";  
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
    if (data.empty() || data[0].empty()) {  
        return;  
    }  
    // Find the bounding rectangle for drawing  
    if (outliers.empty()) return;  
    bool isEmpty = (markerColor[0]==0 && markerColor[1]==0 && markerColor[2]==0);
    if (isEmpty) return;
    int minX = std::numeric_limits<int>::max();  
    int minY = std::numeric_limits<int>::max();  
    int maxX = std::numeric_limits<int>::min();  
    int maxY = std::numeric_limits<int>::min();  
    for (const auto& outlier : outliers) {  
        /*
            //Mark the green line around the edge of the object
            int x = outlier.first;  
            int y = outlier.second;  
            data[x][y] = {0, 255, 0}; // Mark in green  
        */
        /*
            Check if the outlier is within the selections rectangle
            draw a rectangle around the object
        */
        int x = outlier.first;  
        int y = outlier.second;  
        minX = std::min(minX, x);  
        minY = std::min(minY, y);  
        maxX = std::max(maxX, x);  
        maxY = std::max(maxY, y);  
        // Mark the outlier in green  
        data[x][y] = {static_cast<int>(markerColor[0]), static_cast<int>(markerColor[1]), static_cast<int>(markerColor[2])}; // Mark in green   
    }  
}
void cvLib::createOutlierImage(const std::vector<std::vector<RGB>>& originalData, const std::vector<std::pair<int, int>>& outliers, const std::string& outImgPath, const cv::Scalar& bgColor){
    // Calculate the minimum and maximum coordinates of the outliers  
    if (outliers.empty()) {  
        std::cerr << "No outliers provided." << std::endl;  
        return;  
    }  
    int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;  
    for (const auto& outlier : outliers) {  
        minX = std::min(minX, outlier.first);  
        minY = std::min(minY, outlier.second);  
        maxX = std::max(maxX, outlier.first);  
        maxY = std::max(maxY, outlier.second);  
    }  
    // Calculate width and height based on outliers  
    int outputWidth = maxY - minY + 1;  // Width of the rectangle  
    int outputHeight = maxX - minX + 1; // Height of the rectangle  
    // Create a blank image with a black background  
    cv::Mat outputImage(outputHeight, outputWidth, CV_8UC3, bgColor); // Black background  
    // Draw outliers in the output image  
    for (const auto& outlier : outliers) {  
        int outlierX = outlier.first; // original image x-coordinate  
        int outlierY = outlier.second; // original image y-coordinate  
        // Ensure the outlier is within the bounds of the original image  
        if (outlierX < originalData.size() && outlierY < originalData[0].size()) {  
            // Calculate position in the output image  
            int newX = outlierY - minY; // Adjusted X for output image  
            int newY = outlierX - minX; // Adjusted Y for output image  
            // Get the RGB values from the original data  
            RGB pixel = originalData[outlierX][outlierY];  
            outputImage.at<cv::Vec3b>(newY, newX) = cv::Vec3b(pixel.b, pixel.g, pixel.r); // BGR format for OpenCV  
        }  
    }  
    // Save the output image in various formats  
    this->savePPM(outputImage, outImgPath); 
    this->saveImage(originalData,outImgPath + "_orig.ppm");
    cv::imwrite(outImgPath, outputImage);  
    // cv::imwrite("/home/ronnieji/lib/images/output/outlier_uncompressed_image.png", outputImage, {cv::IMWRITE_PNG_COMPRESSION, 0});  
    // cv::imwrite("/home/ronnieji/lib/images/output/outlier_compressed9_image.png", outputImage, {cv::IMWRITE_PNG_COMPRESSION, 9});  
    // cv::imwrite("/home/ronnieji/lib/images/output/outlier_image.bmp", outputImage);  
    std::cout << "Outlier image saved successfully." << std::endl;  
}
void cvLib::read_image_detect_edges(const std::string& imagePath,int gradientMagnitude_threshold,const std::string& outImgPath,const brushColor& markerColor, const brushColor& bgColor){
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
void cvLib::convertToBlackAndWhite(const std::string& filename, std::vector<std::vector<RGB>>& datasets, int width, int height) {   
    if (filename.empty()) {  
        return;  
    }   
    cv::Mat image = cv::imread(filename, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return;   
    }  
    for (int i = 0; i < image.rows; ++i) {  
        for (int j = 0; j < image.cols; ++j) {  
            // Access the pixel and its RGB values  
            cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);  
            int r = pixel[2];  
            int g = pixel[1];  
            int b = pixel[0];  
            // Calculate the grayscale value using the luminance method  
            int grayValue = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);       
            // Determine the binary value with thresholding  
            int bwValue = (grayValue < 128) ? 0 : 255;   
            // Assign the calculated binary value to the image  
            pixel[0] = bwValue;  
            pixel[1] = bwValue;  
            pixel[2] = bwValue;  
            // Update the corresponding dataset  
            datasets[i][j] = {bwValue, bwValue, bwValue};  
        }  
    }  
}  
bool cvLib::read_image_detect_objs(const std::string& img1, const std::string& img2, int featureCount, float ratioThresh, int de_threshold) {  
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
    if (m_img1.channels() == 3) {  
        cv::cvtColor(m_img1, gray1, cv::COLOR_BGR2GRAY);  
    } else {  
        gray1 = m_img1;  
    }  
    if (m_img2.channels() == 3) {  
        cv::cvtColor(m_img2, gray2, cv::COLOR_BGR2GRAY);  
    } else {  
        gray2 = m_img2;  
    }  
    // Use ORB for keypoint detection and description  
    cv::Ptr<cv::ORB> detector = cv::ORB::create(featureCount); // Adjust number of features as needed  
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
    const float ratio_thresh = ratioThresh;  
    std::vector<cv::DMatch> goodMatches;  
    for (size_t i = 0; i < knnMatches.size(); i++) {  
        if (knnMatches[i][0].distance < ratio_thresh * knnMatches[i][1].distance) {  
            goodMatches.push_back(knnMatches[i][0]);  
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
bool cvLib::isObjectInImage(const std::string& img1, const std::string& img2, int featureCount, float ratioThresh, int deThreshold) {  
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
std::vector<std::vector<RGB>> cvLib::objectsInImage(const std::string& imgPath, int gradientMagnitude_threshold) {  
    std::vector<std::vector<RGB>> objects_detect;  
    if (imgPath.empty()) {  
        return objects_detect;  
    }  
    subfunctions subfun;  
    imgSize img_size = this->get_image_size(imgPath);  
    std::vector<std::vector<RGB>> image_rgb = this->get_img_matrix_color(imgPath, img_size.height, img_size.width);  
    if (!image_rgb.empty()) {  
        // Find outliers (edges)  
        auto outliers = this->findOutlierEdges(image_rgb, gradientMagnitude_threshold);   
        objects_detect = subfun.getPixelsInsideObject(image_rgb, outliers);  
    }  
    return objects_detect;  
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
void cvLib::StartWebCam(int webcame_index,const std::string& winTitle,const std::vector<std::string>& imageListToFind,const cv::Scalar& brush_color,std::function<void(cv::Mat&)> callback){
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
        //sub_j.markVideo(frame,brush_color,bgColor);
        // Apply background subtraction  
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
        //cv::Mat oframe = sub_j.getObjectsInVideo(frame);

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
        return {};  // Return an empty map instead of img_matrix  
    }  
    cv::Mat img = cv::imread(image_path, cv::IMREAD_UNCHANGED);  
    if (img.empty()) {  
        throw std::runtime_error("Failed to load image: " + image_path);  
    }  
    // Prepare the vector to store the pixel data  
    img_matrix.resize(img.rows * img.cols); // Allocate enough space once  
    uint32_t* ptr = img_matrix.data(); // Pointer to write directly  
    // Read the image data  
    for (int y = 0; y < img.rows; ++y) {  
        for (int x = 0; x < img.cols; ++x) {  
            uint32_t packedPixel = 0;  
            if (img.channels() == 4) {  
                cv::Vec4b pixel = img.at<cv::Vec4b>(y, x); // BGRA  
                packedPixel = (pixel[2] << 24) |  // Red  
                              (pixel[1] << 16) |  // Green  
                              (pixel[0] << 8) |  // Blue  
                              (pixel[3]);         // Alpha  
            } else if (img.channels() == 3) {  
                cv::Vec3b pixel = img.at<cv::Vec3b>(y, x); // BGR  
                packedPixel = (pixel[2] << 24) |  // Red  
                              (pixel[1] << 16) |  // Green  
                              (pixel[0] << 8) |  // Blue  
                              0xFF;               // Alpha (set to 255)  
            } else {  
                std::cerr << "Unsupported image format: " << img.channels() << " channels." << std::endl;  
                throw std::runtime_error("Unsupported image format.");  
            }  

            ptr[y * img.cols + x] = packedPixel; // Assign directly in resized vector  
        }  
    }  
    return img_matrix;
}   
std::unordered_map<std::string, std::vector<uint32_t>> cvLib::get_img_in_folder(const std::string& folder_path) {  
    std::unordered_map<std::string, std::vector<uint32_t>> dataset;  
    if (folder_path.empty()) {  
        return dataset;  
    }  
    try {  
        for (const auto& entryMainFolder : std::filesystem::directory_iterator(folder_path)) {  
            if (entryMainFolder.is_directory()) { // Check if the entry is a directory  
                std::string sub_folder_name = entryMainFolder.path().filename().string();  
                std::string sub_folder_path = entryMainFolder.path();  
                std::vector<uint32_t> sub_folder_all_images;  
                // Accumulate pixel count for memory reservation  
                size_t totalPixels = 0;  
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file()) {  
                        std::string imgFolderPath = entrySubFolder.path();  
                        // Load the image using OpenCV  
                        cv::Mat img = cv::imread(imgFolderPath, cv::IMREAD_UNCHANGED);  
                        if (img.empty()) {  
                            throw std::runtime_error("Failed to load image: " + imgFolderPath);  
                        }  
                        totalPixels += img.rows * img.cols; // Calculate the total number of pixels  
                    }  
                }  
                // Reserve space for pixel data  
                sub_folder_all_images.reserve(totalPixels);  
                // Reload the images to fill pixel data  
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file()) {  
                        std::string imgFolderPath = entrySubFolder.path();  
                        cv::Mat img = cv::imread(imgFolderPath, cv::IMREAD_UNCHANGED);  
                        if (img.empty()) {  
                            throw std::runtime_error("Failed to load image: " + imgFolderPath);  
                        }  
                        // Read the image data  
                        for (int y = 0; y < img.rows; ++y) {  
                            for (int x = 0; x < img.cols; ++x) {  
                                uint32_t packedPixel = 0;  
                                if (img.channels() == 4) {  
                                    cv::Vec4b pixel = img.at<cv::Vec4b>(y, x); // BGRA  
                                    packedPixel = (pixel[2] << 24) |  // Red  
                                                  (pixel[1] << 16) |  // Green  
                                                  (pixel[0] << 8) |  // Blue  
                                                  (pixel[3]);         // Alpha  
                                } else if (img.channels() == 3) {  
                                    cv::Vec3b pixel = img.at<cv::Vec3b>(y, x); // BGR  
                                    packedPixel = (pixel[2] << 24) |  // Red  
                                                  (pixel[1] << 16) |  // Green  
                                                  (pixel[0] << 8) |  // Blue  
                                                  0xFF;               // Alpha (set to 255)  
                                } else {  
                                    std::cerr << "Unsupported image format: " << img.channels() << " channels." << std::endl;  
                                    throw std::runtime_error("Unsupported image format.");  
                                }  
                                sub_folder_all_images.push_back(packedPixel);  
                            }  
                        }  
                    }  
                }  
                dataset[sub_folder_name] = sub_folder_all_images;  
                std::cout << sub_folder_name << " is done!" << std::endl;  
            }  
        }  
    } catch (const std::filesystem::filesystem_error& e) {  
        std::cerr << "Filesystem error: " << e.what() << std::endl;  
        return dataset; // Return an empty dataset in case of filesystem error  
    }  
    std::cout << "Successfully saved the images into the dataset, all jobs are done!" << std::endl;  
    return dataset;  
}  
std::unordered_map<std::string, std::vector<uint32_t>> cvLib::train_img_in_folder(const std::unordered_map<std::string, std::vector<uint32_t>>& img_dataset, const std::string& model_output_path) {  
    std::unordered_map<std::string, std::vector<uint32_t>> content_in_imgs;  
    // Check for empty dataset or model path  
    if (img_dataset.empty() || model_output_path.empty()) {  
        return content_in_imgs;  
    }  
    std::cout << "Start training..." << std::endl;  
    for (const auto& item : img_dataset) {  
        std::string img_label = item.first;  
        std::vector<uint32_t> imgs_data = item.second;  
        // Create an unordered_map with the custom hash function  
        std::unordered_map<std::vector<uint32_t>, unsigned int, VectorHash> imgCount;   
        for (unsigned int i = 0; i < imgs_data.size(); i++) {  
            unsigned int imgs_score = 0;  
            // Check if the next element exists  
            if (i + 1 < imgs_data.size()) {  
                for (unsigned int j = 0; j < imgs_data.size(); j++) {  
                    if (j != i && j + 1 < imgs_data.size()) {  
                        // Compare adjacent elements  
                        if (imgs_data[j] == imgs_data[i] && imgs_data[j + 1] == imgs_data[i + 1]) {  
                            imgs_score++;  
                        }  
                    }  
                }  
                // Use the custom hashable pair as a key  
                imgCount[{imgs_data[i], imgs_data[i + 1]}] = imgs_score;  
                std::cout << imgs_data[i] << " and " << imgs_data[i + 1] << " count: " << imgs_score << std::endl;  
            }  
        }  
        // Sort imgCount into a vector of pairs  
        std::vector<std::pair<std::vector<uint32_t>, unsigned int>> sortedImgCount(imgCount.begin(), imgCount.end());  
        std::sort(sortedImgCount.begin(), sortedImgCount.end(), [](const auto& a, const auto& b) {  
            return a.second > b.second;  
        });  
        // Reorganize the results into content_in_imgs  
        for (const auto& entry : sortedImgCount) {  
            const auto& vec = entry.first;  
            // Since we are using img_label as key and we want the latest vector of uint32_t for that label  
            content_in_imgs[img_label] = vec; // Store the latest sorted vector  
        }  
        std::cout << img_label << " training is done!" << std::endl;  
    }  
    // Save the model using subfunctions  
    subfunctions sub_j;  
    sub_j.saveModel(content_in_imgs, model_output_path);  
    return content_in_imgs;  
}  
std::unordered_map<std::string, std::vector<uint32_t>> cvLib::train_img_occurrences(const std::string& images_folder_path, const std::string& model_output_path){
    std::unordered_map<std::string, std::vector<uint32_t>> dataset;  
    if (images_folder_path.empty()) {  
        return dataset;  
    }  
    subfunctions sub_j;
    try {  
        for (const auto& entryMainFolder : std::filesystem::directory_iterator(images_folder_path)) {  
            if (entryMainFolder.is_directory()) { // Check if the entry is a directory  
                std::string sub_folder_name = entryMainFolder.path().filename().string();  
                std::string sub_folder_path = entryMainFolder.path();  
                std::vector<uint32_t> sub_folder_all_images;  
                // Accumulate pixel count for memory reservation  
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file()) {  
                        std::string imgFolderPath = entrySubFolder.path();  
                        std::vector<std::vector<RGB>> get_img = this->objectsInImage(imgFolderPath,1);
                        if(!get_img.empty()){
                            std::vector<uint32_t> get_img_uint = this->convertToPacked(get_img);
                            if(sub_folder_all_images.empty()){//if it's empty insert get_img_uint
                                sub_folder_all_images.insert(sub_folder_all_images.end(),get_img_uint.begin(),get_img_uint.end());
                            }
                            else{// merge get_img_uint
                                sub_j.merge_without_duplicates(sub_folder_all_images,get_img_uint);
                            }
                        }
                    }  
                }  
             
                dataset[sub_folder_name] = sub_folder_all_images;  
                std::cout << sub_folder_name << " is done!" << std::endl;  
            }  
        }  
    } catch (const std::filesystem::filesystem_error& e) {  
        std::cerr << "Filesystem error: " << e.what() << std::endl;  
        return dataset; // Return an empty dataset in case of filesystem error  
    }  
    std::cout << "Successfully saved the images into the dataset, all jobs are done!" << std::endl;  
    sub_j.saveModel(dataset, model_output_path);  
    return dataset;  
}
void cvLib::loadModel(std::unordered_map<std::string, std::vector<uint32_t>>& content_in_imgs, const std::string& filename){
    std::ifstream ifs(filename, std::ios::binary);  
    if (!ifs) {  
        std::cerr << "Error opening file for reading: " << filename << std::endl;  
        return;  
    }  
    // Read the size of the unordered_map  
    size_t mapSize;  
    ifs.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));  
    // Read each key-value pair  
    for (size_t i = 0; i < mapSize; ++i) {  
        // Read the size of the string key  
        size_t keySize;  
        ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));  
        std::string key(keySize, '\0');  
        ifs.read(&key[0], keySize);  
        // Read the size of the vector  
        size_t vecSize;  
        ifs.read(reinterpret_cast<char*>(&vecSize), sizeof(vecSize));  
        std::vector<uint32_t> values(vecSize);  
        ifs.read(reinterpret_cast<char*>(values.data()), vecSize * sizeof(uint32_t));  
        content_in_imgs[key] = std::move(values);  
    }  
    ifs.close();  
}  

