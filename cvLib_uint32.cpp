/*
    c++20 lib for using opencv
*/
#include <opencv2/opencv.hpp>  
#include <opencv2/features2d.hpp> 
#include <opencv2/video.hpp> 
#include <boost/functional/hash.hpp> // Include for hashing pairs
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
#include <stdexcept>
#include <boost/functional/hash.hpp> // For boost::hash 
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "cvLib_uint32.h"
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
        void convertToPackedPixel(const cv::Mat&, std::vector<uint32_t>&);
        cv::Mat convertPackedToMat(const std::vector<uint32_t>&, int, int);
        void convertToBlackAndWhite(cv::Mat&, std::vector<uint32_t>&);
        // Function to convert a dataset to cv::Mat  
        cv::Mat convertDatasetToMat(const std::vector<uint32_t>&, int, int);
        void markVideo(cv::Mat&, const cv::Scalar&, const cv::Scalar&);
        // Function to check if a point is inside a polygon  
        //para1:x, para2:y , para3: polygon
        bool isPointInPolygon(int, int, const std::vector<std::pair<int, int>>&);
        // Function to get all pixels inside the object defined by A   
        std::vector<uint32_t> getPixelsInsideObject(const std::vector<uint32_t>&, const std::vector<std::pair<int, int>>&, int, int);
        cv::Mat getObjectsInVideo(const cv::Mat&);
        void saveModel(const std::unordered_map<std::string, std::vector<uint32_t>>&, const std::string&);
};
void subfunctions::convertToPackedPixel(const cv::Mat& img, std::vector<uint32_t>& packedData) {  
    packedData.resize(img.rows * img.cols);  
    for (int y = 0; y < img.rows; ++y) {  
        for (int x = 0; x < img.cols; ++x) {  
            cv::Vec3b pixel = img.at<cv::Vec3b>(y, x); // Assuming BGR format  
            packedData[y * img.cols + x] = (static_cast<uint32_t>(pixel[2]) << 16) |  // R  
                                             (static_cast<uint32_t>(pixel[1]) << 8)  |  // G  
                                             (static_cast<uint32_t>(pixel[0]));       // B  
        }  
    }  
}
cv::Mat subfunctions::convertPackedToMat(const std::vector<uint32_t>& packedData, int rows, int cols) {  
    cv::Mat img(rows, cols, CV_8UC3);  
    for (int y = 0; y < rows; ++y) {  
        for (int x = 0; x < cols; ++x) {  
            uint32_t packedPixel = packedData[y * cols + x];  
            img.at<cv::Vec3b>(y, x)[0] = packedPixel & 0xFF;               // B  
            img.at<cv::Vec3b>(y, x)[1] = (packedPixel >> 8) & 0xFF;       // G  
            img.at<cv::Vec3b>(y, x)[2] = (packedPixel >> 16) & 0xFF;      // R  
        }  
    }  
    return img;  
}
void subfunctions::convertToBlackAndWhite(cv::Mat& image, std::vector<uint32_t>& datasets) {  
        for (int i = 0; i < image.rows; ++i) {  
            for (int j = 0; j < image.cols; ++j) {  
                // Access the pixel from packed data  
                uint32_t packedPixel = datasets[i * image.cols + j];  
                // Unpack the pixel  
                int b = (packedPixel & 0xFF);           // Blue  
                int g = (packedPixel >> 8) & 0xFF;      // Green  
                int r = (packedPixel >> 16) & 0xFF;     // Red  
                // Calculate the grayscale value using the luminance method  
                int grayValue = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);  
                // Determine the binary value with thresholding  
                int bwValue = (grayValue < 128) ? 0 : 255;  
                // Pack the pixel back as grayscale (use the same value for all channels)  
                uint32_t newPackedPixel = (bwValue << 24) | (bwValue << 16) | (bwValue << 8) | bwValue;  
                datasets[i * image.cols + j] = newPackedPixel; // Update the packed pixel data  
                // Update the original image with the new pixel value  
                image.at<cv::Vec3b>(i, j) = cv::Vec3b(bwValue, bwValue, bwValue); // Set the grayscale value  
            }  
        }  
}  
// Function to convert packed pixel data to cv::Mat  
cv::Mat subfunctions::convertDatasetToMat(const std::vector<uint32_t>& dataset, int rows, int cols) {  
    if (dataset.empty() || dataset.size() != rows * cols) {  
        throw std::runtime_error("Dataset is empty or does not match specified dimensions.");  
    }  
    cv::Mat image(rows, cols, CV_8UC3); // Create a Mat with 3 channels (BGR)  
    for (int i = 0; i < rows; ++i) {  
        for (int j = 0; j < cols; ++j) {  
            // Access the packed pixel values from the dataset  
            uint32_t packedPixel = dataset[i * cols + j];  
            // Extract BGR values  
            uint8_t b = static_cast<uint8_t>(packedPixel & 0xFF);          // Blue  
            uint8_t g = static_cast<uint8_t>((packedPixel >> 8) & 0xFF);   // Green  
            uint8_t r = static_cast<uint8_t>((packedPixel >> 16) & 0xFF);  // Red  
            // Set the pixel in the cv::Mat  
            image.at<cv::Vec3b>(i, j) = cv::Vec3b(b, g, r); // OpenCV uses BGR format  
        }  
    }  
    return image;  
}  
// Function to mark video frames using packed pixel data  
void subfunctions::markVideo(cv::Mat& frame, const cv::Scalar& brush_color, const cv::Scalar& bg_color) {  
    if (frame.empty()) {  
        return;  
    }  
    cvLib cvl_j;  
    std::vector<uint32_t> frame_to_mark(frame.rows * frame.cols); // Create the packed pixel data vector  
    // Read frame into packed data (assumed to be filled here from frame)  
    // [Filling packed pixel data not shown for brevity; should follow the same approach as previous conversion]  
    if (!frame_to_mark.empty()) {  
        std::vector<std::pair<int, int>> outliers_found = cvl_j.findOutlierEdges(frame_to_mark,frame.cols,frame.rows,90);  
        if (!outliers_found.empty()) {  
            cvl_j.markOutliers(frame_to_mark, outliers_found, brush_color,frame.cols);  
            // Create a blank image with the specified background color  
            cv::Mat outputImage(frame.rows, frame.cols, CV_8UC3, bg_color); // Background  
            outputImage = convertDatasetToMat(frame_to_mark, frame.rows, frame.cols);  
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
std::vector<uint32_t> subfunctions::getPixelsInsideObject(const std::vector<uint32_t>& image_pixels, const std::vector<std::pair<int, int>>& objEdges, int width, int height) {  
        std::vector<uint32_t> output_pixels = image_pixels; // Start with a copy of the original pixels  
        if (output_pixels.empty() || objEdges.empty()) {  
            return output_pixels; // Return the original pixel data if edges are empty  
        }  
        // Create a set of object coordinates for quick lookup  
        std::unordered_set<std::pair<int, int>, boost::hash<std::pair<int, int>>> objSet(objEdges.begin(), objEdges.end());  
        // Iterate through the image and modify pixels  
        for (int y = 0; y < height; ++y) {  
            for (int x = 0; x < width; ++x) {  
                // If the current pixel is not in the object edges, set it to white  
                if (objSet.find({y, x}) == objSet.end()) {  
                    output_pixels[y * width + x] = 0xFFFFFFFF; // Set to white (0xFFFFFFFF) in packed format  
                }  
            }  
        }  
        return output_pixels; // Return the modified pixel data  
}  
// Function to fetch objects in video using packed pixel data  
cv::Mat subfunctions::getObjectsInVideo(const cv::Mat& inVideo) {  
    cvLib cv_j;  
    std::vector<uint32_t> objects_detect;  
    // Convert input video to packed pixel data  
    std::vector<uint32_t> image_rgb(inVideo.rows * inVideo.cols); // Create packed data vector  
    // [Code to fill image_rgb with packed data from inVideo not shown for brevity]  
    if (!image_rgb.empty()) {  
        // Find outliers (edges)  
        auto outliers = cv_j.findOutlierEdges(image_rgb,inVideo.cols,inVideo.rows,5);  
        objects_detect = this->getPixelsInsideObject(image_rgb, outliers,inVideo.cols,inVideo.rows);  
    }  
    cv::Mat finalV = convertDatasetToMat(objects_detect, inVideo.rows, inVideo.cols);  
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
// Converts a grayscale cv::Mat to a packed pixel dataset  
std::vector<uint32_t> cvLib::cv_mat_to_dataset(const cv::Mat& genImg) {  
    std::vector<uint32_t> datasets(genImg.rows * genImg.cols);  
    for (int i = 0; i < genImg.rows; ++i) {  // rows  
        for (int j = 0; j < genImg.cols; ++j) {  // cols  
            // Get the intensity value  
            uchar intensity = genImg.at<uchar>(i, j);  
            // Pack the grayscale value into a 32-bit integer (ARGB)  
            datasets[i * genImg.cols + j] = (intensity << 24) | (intensity << 16) | (intensity << 8) | intensity;  
        }  
    }  
    return datasets;  
}   
// Converts a color cv::Mat to a packed pixel dataset  
std::vector<uint32_t> cvLib::cv_mat_to_dataset_color(const cv::Mat& genImg) {  
    std::vector<uint32_t> datasets(genImg.rows * genImg.cols);  
    for (int i = 0; i < genImg.rows; ++i) {  // rows  
        for (int j = 0; j < genImg.cols; ++j) {  // cols  
            // Check if the image is in color:  
            if (genImg.channels() == 3) {  
                cv::Vec3b bgr = genImg.at<cv::Vec3b>(i, j);  
                // Pack the RGB values into a 32-bit integer (ARGB)  
                datasets[i * genImg.cols + j] = (bgr[2] << 24) | (bgr[1] << 16) | (bgr[0] << 8) | 255; // Assuming full alpha  
            } else if (genImg.channels() == 1) {  
                // Handle grayscale images  
                uchar intensity = genImg.at<uchar>(i, j);  
                datasets[i * genImg.cols + j] = (intensity << 24) | (intensity << 16) | (intensity << 8) | intensity; // Grayscale to packed format  
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
void cvLib::saveVectorRGB(const std::vector<uint32_t>& img, int width, int height, const std::string& filename) {  
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
    for (const auto& packedPixel : img) {  
        uint8_t r = (packedPixel >> 24) & 0xFF; // Extract red  
        uint8_t g = (packedPixel >> 16) & 0xFF; // Extract green  
        uint8_t b = (packedPixel >> 8) & 0xFF;  // Extract blue  
        out << static_cast<int>(r) << " " << static_cast<int>(g) << " " << static_cast<int>(b) << "\n";  
    }  
    out.close();  
}  
// Get grayscale image as packed pixel data from a file  
std::vector<uint32_t> cvLib::get_img_matrix(const std::string& imgPath, int img_rows, int img_cols) {  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_GRAYSCALE);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return std::vector<uint32_t>(img_rows * img_cols);   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(img_cols, img_rows)); // Resize to correct dimensions  
    return this->cv_mat_to_dataset(resized_image); // Return as packed data  
}  
// Get color image as packed pixel data from a file  
std::vector<uint32_t> cvLib::get_img_matrix_color(const std::string& imgPath, int img_rows, int img_cols) {  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return std::vector<uint32_t>(img_rows * img_cols);   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(img_cols, img_rows)); // Resize to correct dimensions  
    return cv_mat_to_dataset_color(resized_image); // Return as packed data  
}  
// Read images from a folder into a multimap with packed pixel data  
std::multimap<std::string, std::vector<uint32_t>> cvLib::read_images(const std::string& folderPath) {  
    std::multimap<std::string, std::vector<uint32_t>> datasets;  
    if (folderPath.empty()) {  
        return datasets;  
    }  
    std::string adjustedPath = folderPath;  
    if (adjustedPath.back() != '/') {  
        adjustedPath.append("/");  
    }  
    for (const auto& entry : std::filesystem::directory_iterator(adjustedPath)) {  
        if (entry.is_regular_file()) {  
            std::string imgPath = entry.path().filename().string();  
            std::string fullPath = adjustedPath + imgPath;  
            int img_rows, img_cols; // Assume these are defined or fetched from the image metadata  
            std::vector<uint32_t> image_rgb = get_img_matrix_color(fullPath, img_rows, img_cols);  
            datasets.insert({fullPath, image_rgb});  
        }  
    }  
    return datasets;  
}  
// Function to find outlier edges using a simple gradient method  
std::vector<std::pair<int, int>> cvLib::findOutlierEdges(const std::vector<uint32_t>& pixels, int width, int height, int gradientMagnitude_threshold) {  
        std::vector<std::pair<int, int>> outliers;  
        if (pixels.empty()) // Check for empty data  
            return outliers; // Return early if no data  
        // Iterate over the image, skipping border pixels  
        for (int i = 1; i < height - 1; ++i) {  
            for (int j = 1; j < width - 1; ++j) {  
                // Function to extract the red channel from a packed pixel  
                auto getRed = [&pixels, width](int y,int x) -> uint8_t {  
                    uint32_t packedPixel = pixels[y * width + x];  
                    return static_cast<uint8_t>((packedPixel >> 24) & 0xFF); // Extract Red channel  
                };  
                // Calculate gradients using the red channel  
                int gx = -getRed(i - 1, j - 1) + getRed(i + 1, j + 1) +  
                         -2 * getRed(i, j - 1) + 2 * getRed(i, j + 1) +  
                         -getRed(i - 1, j + 1) + getRed(i + 1, j - 1);  
                int gy = getRed(i - 1, j - 1) + 2 * getRed(i - 1, j) + getRed(i - 1, j + 1) -  
                         (getRed(i + 1, j - 1) + 2 * getRed(i + 1, j) + getRed(i + 1, j + 1));  
                double gradientMagnitude = std::sqrt(gx * gx + gy * gy);  
                if (gradientMagnitude > gradientMagnitude_threshold) {  
                    outliers.emplace_back(i, j);  
                }  
            }  
        }  
        return outliers;  
}  
// Function to save an image using packed pixel data  
bool cvLib::saveImage(const std::vector<uint32_t>& data, const std::string& filename, int width, int height) {   
    if (data.empty()) {  
        std::cerr << "Error: Image data is empty." << std::endl;  
        return false;  
    }  
    std::ofstream file(filename);  
    if (!file) {  
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;  
        return false;  
    }  
    file << "P3\n" << width << " " << height << "\n255\n"; // PPM header  
    try {  
        for (int i = 0; i < height; ++i) {  
            for (int j = 0; j < width; ++j) {  
                uint32_t pixel = data.at(i * width + j); // Access packed pixel  
                // Unpack pixel components  
                uint8_t r = (pixel >> 16) & 0xFF; // Red component  
                uint8_t g = (pixel >> 8) & 0xFF;  // Green component  
                uint8_t b = pixel & 0xFF;         // Blue component  
                // Ensure RGB values are within valid range  
                if (r > 255 || g > 255 || b > 255) {  
                    throw std::runtime_error("RGB values must be in the range [0, 255]");  
                }  
                file << static_cast<int>(r) << " " << static_cast<int>(g) << " " << static_cast<int>(b) << "\n";  
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
// Function to mark outliers in an image using packed pixel data  
void cvLib::markOutliers(std::vector<uint32_t>& data, const std::vector<std::pair<int, int>>& outliers, const cv::Scalar& markerColor, int width) {  
    if (data.empty()) {  
        return;  
    }  
    if (outliers.empty() || (markerColor[0] == 0 && markerColor[1] == 0 && markerColor[2] == 0)) {  
        return;  
    }  
    for (const auto& outlier : outliers) {  
        int x = outlier.first;  
        int y = outlier.second;  
        // Update pixel for outlier with marker color  
        if (x < 0 || y < 0 || x >= width || y >= data.size() / width) {  // Correct y check  
            continue; // Ensure in bounds  
        }  
        // Pack the marker pixel in the format 0xRRGGBB  
        uint32_t markerPixel = (static_cast<uint32_t>(static_cast<uint8_t>(markerColor[2])) << 16) | // R  
                                (static_cast<uint32_t>(static_cast<uint8_t>(markerColor[1])) << 8)  | // G  
                                (static_cast<uint32_t>(static_cast<uint8_t>(markerColor[0])));       // B  
        data[y * width + x] = markerPixel; // Update the packed pixel at the correct index  
    }  
}  
// Function to create an outlier image using packed pixel data  
void cvLib::createOutlierImage(const std::vector<uint32_t>& originalData, const std::vector<std::pair<int, int>>& outliers, const std::string& outImgPath, const cv::Scalar& bgColor, int width) {  
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
    // Calculate image dimensions based on outliers  
    int outputWidth = maxY - minY + 1;  // Width of the rectangle  
    int outputHeight = maxX - minX + 1; // Height of the rectangle  
    // Create a blank image with the background color  
    cv::Mat outputImage(outputHeight, outputWidth, CV_8UC3, bgColor); // Background color  
    // Draw outliers in the output image  
    for (const auto& outlier : outliers) {  
        int outlierX = outlier.first; // original image x-coordinate  
        int outlierY = outlier.second; // original image y-coordinate     
        // Ensure the outlier is within the bounds of the original image  
        if (outlierX < 0 || outlierX >= originalData.size() / width || outlierY < 0 || outlierY >= width) {  
            continue;  
        } 
        // Calculate position in the output image  
        int newX = outlierY - minY; // Adjusted X for output image  
        int newY = outlierX - minX; // Adjusted Y for output image  
        // Get the packed pixel data from the original data  
        uint32_t pixel = originalData[outlierX * width + outlierY];  
        // Unpack RGB values  
        uint8_t r = (pixel >> 16) & 0xFF; // Red  
        uint8_t g = (pixel >> 8) & 0xFF;  // Green  
        uint8_t b = pixel & 0xFF;         // Blue  
        // Set the pixel value in the output image (BGR format for OpenCV)  
        outputImage.at<cv::Vec3b>(newY, newX) = cv::Vec3b(b, g, r);  
    }  
    // Save the output image to the specified path  
    cv::imwrite(outImgPath, outputImage);  
    std::cout << "Outlier image saved successfully." << std::endl;  
}  
// Function to read an image, detect edges, and save an outlier image  
void cvLib::read_image_detect_edges(const std::string& imagePath, int gradientMagnitude_threshold, const std::string& outImgPath, const brushColor& markerColor, const brushColor& bgColor) {  
    if (imagePath.empty()) {  
        return;  
    }  
    imgSize img_size = this->get_image_size(imagePath);  
    std::vector<uint32_t> imageData = this->get_img_matrix_color(imagePath, img_size.width, img_size.height); // Assuming this retrieves packed pixel data  
    // Find outliers (edges)  
    auto outliers = this->findOutlierEdges(imageData, img_size.width,img_size.height, gradientMagnitude_threshold);  
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
    // Mark outliers in the image  
    this->markOutliers(imageData, outliers, brushMarkerColor, img_size.width);  
    // Create outlier image  
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
            std::cerr << "Error: Unexpected background color" << std::endl;  
            return; // or handle an error  
    }  
    this->createOutlierImage(imageData, outliers, outImgPath, brushbgColor, img_size.width);  
}  
// Function to convert an image to black and white using packed pixel data  
void cvLib::convertToBlackAndWhite(const std::string& filename, std::vector<uint32_t>& packedData, int width, int height) {  
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
            cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);  
            int r = pixel[2];  
            int g = pixel[1];  
            int b = pixel[0];  

            // Calculate the grayscale value using the luminance method  
            int grayValue = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);  

            // Determine the binary value with thresholding  
            uint8_t bwValue = (grayValue < 128) ? 0 : 255;  

            // Update the packed dataset  
            packedData[i * width + j] = (bwValue << 16) | (bwValue << 8) | bwValue; // Convert to packed pixel format  
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
    // Read images  
    cv::Mat m_img1 = cv::imread(img1);  
    cv::Mat m_img2 = cv::imread(img2);  
    if (m_img1.empty() || m_img2.empty()) {  
        std::cerr << "Failed to read one or both images." << std::endl;  
        return false;  
    }  
    // Create packed pixel data vectors  
    std::vector<uint32_t> dataset_img1(m_img1.rows * m_img1.cols);  
    std::vector<uint32_t> dataset_img2(m_img2.rows * m_img2.cols);  
    // Convert to packed pixel data (assumed to be filled here)  
    subfunctions sub_j;  
    sub_j.convertToPackedPixel(m_img1, dataset_img1); // New function to convert to packed pixel  
    sub_j.convertToPackedPixel(m_img2, dataset_img2); // New function to convert to packed pixel  
    // Convert packed pixel data back to cv::Mat for processing  
    cv::Mat gray1 = sub_j.convertPackedToMat(dataset_img1, m_img1.rows, m_img1.cols); // New function to convert back  
    cv::Mat gray2 = sub_j.convertPackedToMat(dataset_img2, m_img2.rows, m_img2.cols); // New function to convert back  
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
            return true;  
        }  
    }  
    return false;  
} 
// Function to detect objects in an image using packed pixel data  
std::vector<uint32_t> cvLib::objectsInImage(const std::string& imgPath, int gradientMagnitude_threshold) {  
    std::vector<uint32_t> objects_detect; // Change return type to vector of packed pixels  
    if (imgPath.empty()) {  
        return objects_detect;  
    }  
    subfunctions subfun;  
    imgSize img_size = this->get_image_size(imgPath);  
    std::vector<uint32_t> image_rgb = this->get_img_matrix_color(imgPath, img_size.height, img_size.width); // Assuming this retrieves packed pixel data  
    if (!image_rgb.empty()) {  
        // Find outliers (edges)  
        auto outliers = this->findOutlierEdges(image_rgb, img_size.width, img_size.height,gradientMagnitude_threshold);  
        objects_detect = subfun.getPixelsInsideObject(image_rgb, outliers, img_size.width,img_size.height); // Adjust to accept packed data  
    }  
    return objects_detect; // Return the packed pixel data  
}  
// Function to read an image and detect text using Tesseract  
char* cvLib::read_image_detect_text(const std::string& imgPath) {  
    if (imgPath.empty()) {  
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
        return img_matrix;  
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
    std::map<std::vector<uint32_t>, unsigned int> imgCount;  
    for (unsigned int i = 0; i < img_matrix.size(); i++) {  
        unsigned int imgs_score = 0;  
        if (i + 1 < img_matrix.size()) {  
            for (unsigned int j = 0; j < img_matrix.size() - 1; j++) { // Avoid out-of-bounds  
                if (j != i && img_matrix[j] == img_matrix[i] && img_matrix[j + 1] == img_matrix[i + 1]) {  
                    imgs_score++;  
                }  
            }  
            imgCount[{img_matrix[i], img_matrix[i + 1]}] = imgs_score;  
            std::cout << "Test image: " << img_matrix[i] << " and " << img_matrix[i + 1] << " count: " << imgs_score << std::endl;  
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
