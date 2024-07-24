#include <opencv2/opencv.hpp>  
#include <Eigen/Dense>  
#include <vector>  
#include <iostream>  
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>
#include <thread>
#include <chrono>
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
    for (int i = 0; i < gray_image.rows; ++i) {  
        for (int j = 0; j < gray_image.cols; ++j) {  
            datasets[i][j] = pixels[index];  
            index++;  
        }  
    }  
    return datasets;  
}
std::multimap<std::string, std::vector<std::vector<RGB>>> read_images(const std::string& folderPath){  
    std::multimap<std::string, std::vector<std::vector<RGB>>> datasets;  
    if(folderPath.empty()){  
        return datasets;  
    }  
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {  
        if (entry.is_regular_file()) {  
            std::string imgPath = entry.path().filename().string();
            std::string nuts_key = folderPath + "/" + imgPath;
            std::vector<std::vector<RGB>> image_rgb = get_img_matrix(nuts_key);  
            datasets.insert({nuts_key,image_rgb});
        }  
    }  
    return datasets;  
}
void outResult(const std::string& test_image_path, const std::string& train_image_path){
    if(test_image_path.empty() || train_image_path.empty()){
        return;
    }
    std::ofstream ofile("/home/ronnieji/lib/images/Pistachio_Image_Dataset/output/result.txt",std::ios::app);
    if(!ofile.is_open()){
        ofile.open("/home/ronnieji/lib/images/Pistachio_Image_Dataset/output/result.txt",std::ios::app);
    }
    ofile << test_image_path + " | " + train_image_path << '\n';
    ofile.close();
}
std::string findMaxKey(const std::map<std::string, unsigned int>& result) {  
    // Initialize max key and max value  
    std::string maxKey;  
    unsigned int maxValue = 0;  
    for (const auto& [key, value] : result) {  
        if (value > maxValue) {  
            maxValue = value;  
            maxKey = key;  
        }  
    }  
    return maxKey;  
}  
int main() {   
        std::multimap<std::string, std::vector<std::vector<RGB>>> get_train_datasets = read_images("/home/ronnieji/lib/images/Pistachio_Image_Dataset/train_nuts");  
        std::multimap<std::string, std::vector<std::vector<RGB>>> get_test_datasets = read_images("/home/ronnieji/lib/images/Pistachio_Image_Dataset/test_nuts");  
        for (const auto& test_image : get_test_datasets) {  
            std::map<std::string, unsigned int> result;  
            std::cout << "Test image: " << test_image.first << std::endl;         
            for (unsigned j = 0; j < test_image.second.size(); ++j) {  
                std::vector<RGB> test_line = test_image.second[j];        
                for (const auto& test_pixel : test_line) {  
                    size_t test_pixel_onezero = 0;  
                    if (test_pixel.r == 0 && test_pixel.g == 0 && test_pixel.b == 0) {  
                        test_pixel_onezero = 0;  
                    } else {  
                        test_pixel_onezero = 1;  
                    }             
                    for (const auto& gtrain : get_train_datasets) {  
                        std::vector<RGB> train_line = gtrain.second[j];                 
                        for (const auto& train_pixel : train_line) {  
                            size_t train_pixel_onezero = 0;  
                            if (train_pixel.r == 0 && train_pixel.g == 0 && train_pixel.b == 0) {  
                                train_pixel_onezero = 0;  
                            } else {  
                                train_pixel_onezero = 1;  
                            }  
                            if ((train_pixel_onezero == 0 && test_pixel_onezero == 0) || (train_pixel_onezero != 0 && test_pixel_onezero != 0)) {  
                                result[gtrain.first]++;  
                            }  
                        }  
                    }  
                }  
            }  
            std::string bestMatch = findMaxKey(result);
            outResult(test_image.first,bestMatch);
        }
    return 0;  
}