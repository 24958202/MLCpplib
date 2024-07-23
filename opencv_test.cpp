#include <opencv2/opencv.hpp>  
#include <Eigen/Dense>  
#include <vector>  
#include <iostream>  
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include<boost/algorithm/string/replace.hpp>

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
    std::vector<RGB> pixels;  
    for (int i = 0; i < gray_image.rows; ++i) {  
        for (int j = 0; j < gray_image.cols; ++j) {  
            cv::Vec3b intensity = gray_image.at<cv::Vec3b>(i, j);  
            RGB pixel;  
            pixel.r = intensity[2];  // Red channel  
            pixel.g = intensity[1];  // Green channel  
            pixel.b = intensity[0];  // Blue channel  
            pixels.push_back(pixel);  
        }  
    }  
    // Populate the datasets matrix with RGB values  
    int index = 0;  
    for (int i = 0; i < 120; ++i) {  
        for (int j = 0; j < 120; ++j) {  
            datasets[i][j] = pixels[index];  
            index++;  
        }  
    }  
    return datasets;  
}
std::unordered_map<std::string, std::vector<std::vector<RGB>>> read_images(const std::string& folderPath){  
    std::unordered_map<std::string, std::vector<std::vector<RGB>>> datasets;  
    if(folderPath.empty()){  
        return datasets;  
    }  
    unsigned int nuts_cout = 0;
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {  
        if (entry.is_regular_file()) {  
            std::string imgPath = entry.path().filename().string();
            std::cout << "Opening image: " << imgPath << std::endl;  
            std::vector<std::vector<RGB>> image_rgb = get_img_matrix(folderPath + "/" + imgPath);  
            nuts_cout++;
            std::string nuts_key = "nuts" + std::to_string(nuts_cout);
            datasets[nuts_key] = image_rgb;
        }  
    }  
    return datasets;  
}
int main() {   
    std::unordered_map<std::string, std::vector<std::vector<RGB>>> get_datasets = read_images("/home/ronnieji/lib/images/Pistachio_Image_Dataset/nuts");
    if(!get_datasets.empty()){
        for(const auto& gd : get_datasets){
            std::cout << gd.first;
            for(const auto& imgrgb : gd.second){
                for(const auto& rgbv : imgrgb){
                    RGB i_rgb = rgbv;
                    std::cout << " R:" << i_rgb.r << " G:" << i_rgb.g << " B:" << i_rgb.b;
                }
            }
            std::cout << std::endl;
        }
    }
    return 0;  
}