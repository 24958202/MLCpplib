/*
    c++20 lib for using opencv
*/
#include <opencv2/opencv.hpp>  
#include <Eigen/Dense> 
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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "cvLib.h"
/*
    This function to convert an cv::Mat into a std::vector<std::vector<RGB>> dataset
*/
std::vector<std::vector<RGB>> cvLib::cv_mat_to_dataset(const cv::Mat& genImg){
    std::vector<std::vector<RGB>> datasets(genImg.rows,std::vector<RGB>(genImg.cols));
    for (int i = 0; i < genImg.rows; ++i) {  
        for (int j = 0; j < genImg.cols; ++j) {  
            // Get the intensity value  
            uchar intensity = genImg.at<uchar>(i, j);  
            // Populate the RGB struct for grayscale  
            datasets[i][j] = {static_cast<int>(intensity), static_cast<int>(intensity), static_cast<int>(intensity)};  
        }  
    }  
    return datasets;
}
imgSize cvLib::get_image_size(const std::string& imgPath){
    imgSize im_s;
    if(imgPath.empty()){
        return im_s;
    }
    cv::Mat img = cv::imread(imgPath);
    im_s.width = img.cols;
    im_s.height = img.rows;
    img.release();
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
std::vector<std::vector<RGB>> cvLib::get_img_matrix(const std::string& imgPath, int img_rows, int img_cols){
    std::vector<std::vector<RGB>> datasets(img_rows, std::vector<RGB>(img_cols)); // Create a vector of vectors for RGB values  
    // Read the image using imread function  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if(image.empty()){  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(img_rows, img_cols));  
    cv::Mat gray_image;  
    cv::cvtColor(resized_image,gray_image,cv::COLOR_BGR2GRAY); 
    /*---------------------------------------------------------*/
    // std::string output_folder = "/home/ronnieji/lib/images/output/grayscale_image.jpg";
    // cv::imwrite(output_folder,gray_image);
    /*---------------------------------------------------------*/ 
    datasets = cv_mat_to_dataset(gray_image);
    image.release();
    resized_image.release();
    gray_image.release();
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
            std::vector<std::vector<RGB>> image_rgb = get_img_matrix(nuts_key,img_size.width,img_size.height);  
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
    int height = data.size();  
    int width = data[0].size();  
    for (int i = 1; i < height - 1; ++i) {  
        for (int j = 1; j < width - 1; ++j) {  
            // Get pixel values  
            const RGB& pixel = data[i][j];  
            // Calculate the gradient  
            int gx = -data[i-1][j-1].r + data[i+1][j+1].r +  
                     -2 * data[i][j-1].r + 2 * data[i][j+1].r +  
                     -data[i-1][j+1].r + data[i+1][j-1].r;  
            int gy = data[i-1][j-1].r + 2 * data[i-1][j].r + data[i-1][j+1].r -  
                     (data[i+1][j-1].r + 2 * data[i+1][j].r + data[i+1][j+1].r);  
            // Calculate the gradient magnitude  
            double gradientMagnitude = std::sqrt(gx * gx + gy * gy);  
            // Define a threshold for detecting edges  
            if (gradientMagnitude > gradientMagnitude_threshold) { // Adjust threshold as needed  
                outliers.emplace_back(i, j);  
            }  
        }  
    }  
    return outliers;  
}
bool cvLib::saveImage(const std::vector<std::vector<RGB>>& data, const std::string& filename){ 
     if (data.empty() || data[0].empty()){   
        std::cerr << "Error: Image data is empty." << std::endl;  
        return false;  
    }
    std::ofstream file(filename);  
    if (!file.is_open()){   
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;  
        return false; // Return false if the file couldn't be opened  
    }
    int width = data[0].size(); // Corrected width and height calculation  
    int height = data.size();   
    file << "P3\n" << width << " " << height << "\n255\n"; // PPM header  
    for (int i = 0; i < height; ++i){   
        for (int j = 0; j < width; ++j){   
            const RGB& rgb = data[i][j];  
            file << rgb.r << " " << rgb.g << " " << rgb.b << "\n";  
        }
    }
    file.close();  
    std::cout << "Image saved as " << filename << std::endl;  
    return true; // Return true if saving was successful
}
void cvLib::markOutliers(std::vector<std::vector<RGB>>& data, const std::vector<std::pair<int, int>>& outliers, const cv::Scalar& markerColor) {  
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
        data[x][y] = {markerColor[0], markerColor[1], markerColor[2]}; // Mark in green  
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
    outputImage.release();
    std::cout << "Outlier image saved successfully." << std::endl;  
}
void cvLib::read_image_detect_edges(const std::string& imagePath,int gradientMagnitude_threshold,const std::string& outImgPath){
    if(imagePath.empty()){
        return;
    }
    std::vector<RGB> pixelToPaint;
    imgSize img_size = get_image_size(imagePath);
    std::vector<std::vector<RGB>> image_rgb = this->get_img_matrix(imagePath,img_size.width,img_size.height);  
    // Find outliers (edges)  
    auto outliers = this->findOutlierEdges(image_rgb,gradientMagnitude_threshold);  
    // Mark outliers in green  
    cv::Scalar markColor(0,255,0);
    this->markOutliers(image_rgb, outliers, markColor);  
    // //Save the modified image to a file  
    std::string ppmFile = outImgPath;
    //save selection images
    cv::Scalar bgColor(0,0,0);
    this->createOutlierImage(image_rgb, outliers,outImgPath,bgColor);
}