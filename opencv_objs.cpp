/*
    The following code can quickly picking up the edge of objects in a image.
*/
#include <opencv2/opencv.hpp>  
#include <Eigen/Dense>  
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

struct RGB {  
    size_t r;  
    size_t g;  
    size_t b;  
};  
std::vector<std::vector<RGB>> get_img_matrix(const std::string& imgPath){
    std::vector<std::vector<RGB>> datasets(120, std::vector<RGB>(120)); // Create a vector of vectors for RGB values  
    // Read the image using imread function  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if(image.empty()){  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(120, 120));  
    cv::Mat gray_image;  
    cv::cvtColor(resized_image,gray_image,cv::COLOR_BGR2GRAY); 
    /*---------------------------------------------------------*/
    // std::string output_folder = "/home/ronnieji/lib/images/output/grayscale_image.jpg";
    // cv::imwrite(output_folder,gray_image);
    /*---------------------------------------------------------*/ 
    for (int i = 0; i < gray_image.rows; ++i) {  
        for (int j = 0; j < gray_image.cols; ++j) {  
            // Get the intensity value  
            uchar intensity = gray_image.at<uchar>(i, j);  
            // Populate the RGB struct for grayscale  
            datasets[i][j] = {static_cast<size_t>(intensity), static_cast<size_t>(intensity), static_cast<size_t>(intensity)};  
        }  
    }  
    return datasets;  
}
std::multimap<std::string, std::vector<RGB>> read_images(const std::string& folderPath){  
    std::multimap<std::string, std::vector<RGB>> datasets;  
    if(folderPath.empty()){  
        return datasets;  
    }  
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {  
        if (entry.is_regular_file()) {  
            std::string imgPath = entry.path().filename().string();
            std::string nuts_key = folderPath + "/" + imgPath;
            std::vector<std::vector<RGB>> image_rgb = get_img_matrix(nuts_key);  
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
std::vector<std::pair<int, int>> findOutlierEdges(const std::vector<std::vector<RGB>>& data) {  
    std::vector<std::pair<int, int>> outliers;  
    int width = data.size();  
    int height = data[0].size();  
    for (int i = 1; i < width - 1; ++i) {  
        for (int j = 1; j < height - 1; ++j) {  
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
            if (gradientMagnitude > 100) { // Adjust the threshold as needed  
                outliers.emplace_back(i, j);  
            }  
        }  
    }  
    return outliers;  
}  
// Function to mark outliers in the image data  
void markOutliers(std::vector<std::vector<RGB>>& data, const std::vector<std::pair<int, int>>& outliers) {  
    for (const auto& outlier : outliers) {  
        int x = outlier.first;  
        int y = outlier.second;  
        data[x][y] = {0, 255, 0}; // Mark in green  
    }  
}  
// Function to save the image to a PPM file  
bool saveImage(const std::vector<std::vector<RGB>>& data, const std::string& filename) {  
   std::ofstream file(filename);  
    if (!file.is_open()) {  
        //file.open(filename);
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;  
        return false; // Return false if the file couldn't be opened  
    }  
    int width = data.size();  
    int height = data[0].size();   
    // PPM file header  
    file << "P3\n" << width << " " << height << "\n255\n"; // PPM header  
    for (int i = 0; i < width; ++i) {  
        for (int j = 0; j < height; ++j) {  
            const RGB& rgb = data[i][j];  
            file << rgb.r << " " << rgb.g << " " << rgb.b << "\n";  
        }  
    }   
    file.close();  
    std::cout << "Image saved as " << filename << std::endl;  
    return true; // Return true if saving was successful  
}  
void read_image_detect_edges(const std::string& imagePath){
    if(imagePath.empty()){
        return;
    }
    std::vector<RGB> pixelToPaint;
    std::vector<std::vector<RGB>> image_rgb = get_img_matrix(imagePath);  
    // Find outliers (edges)  
    auto outliers = findOutlierEdges(image_rgb);  
    // Mark outliers in green  
    markOutliers(image_rgb, outliers);  
    // //Save the modified image to a file  
    if (!saveImage(image_rgb, "/home/ronnieji/lib/images/output/sample2.ppm")) {  
        std::cout << "Failed to save outliers_image." << std::endl; // Exit if failed to save the image  
    }  
}
int main(){
    read_image_detect_edges("/home/ronnieji/lib/images/sample2.jpg");
    return 0;
}