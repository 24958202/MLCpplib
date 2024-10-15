/*
    c++20 lib for using opencv
    Dependencies:
    opencv,tesseract,sdl2,sdl_image,boost
*/
#include <opencv2/opencv.hpp>  
#include <opencv2/features2d.hpp> 
#include <opencv2/video.hpp> 
#include <tesseract/baseapi.h>  
#include <tesseract/publictypes.h>  
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "authorinfo/author_info.h" 
#include <vector> 
#include <tuple> 
#include <queue>
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
#include <numeric>
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
// Define the functions for the nested subfunctions class
//void cvLib::subfunctions::visual_recognize_obj(std::vector<std::pair<cvLib::the_obj_in_an_image,double>>& test_img_data,const double& check_record_numbers, const std::unordered_map<std::string, std::vector<pubstructs::RGB>>& traineddataMap, cvLib::the_obj_in_an_image& objInfo);
void cvLib::subfunctions::updateMap(std::unordered_map<std::string, std::vector<uint8_t>>& myMap, const std::string& key, const std::vector<uint8_t>& get_img_uint) {
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
void cvLib::subfunctions::convertToBlackAndWhite(cv::Mat& image, std::vector<std::vector<pubstructs::RGB>>& datasets) {
    if(datasets.empty() || datasets[pubstructs::C_0].empty()){
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
            // Access the pixel and its pubstructs::RGB values
            cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);
            uint8_t r = pixel[pubstructs::C_2];
            uint8_t g = pixel[pubstructs::C_1];
            uint8_t b = pixel[pubstructs::C_0];
            // Calculate the grayscale value using the luminance method
            uint8_t grayValue = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
            // Determine the binary value with thresholding
            uint8_t bwValue = (grayValue < 128) ? 0 : 255;
            // Assign the calculated binary value to the image
            pixel[pubstructs::C_0] = bwValue;
            pixel[pubstructs::C_1] = bwValue;
            pixel[pubstructs::C_2] = bwValue;
            // Update the corresponding dataset
            datasets[i][j] = {bwValue, bwValue, bwValue};
        }
    }
}
void cvLib::subfunctions::move_single_objs_to_center(std::vector<std::pair<std::vector<unsigned int>, double>>& imageData,unsigned int imageWidth, unsigned int imageHeight){
    if (imageData.empty()) {
        return;
    }
    // Calculate the centroid of the keypoints
    double centroidX = 0.0;
    double centroidY = 0.0;
    for (const auto& dataPair : imageData) {
        centroidX += dataPair.first[pubstructs::C_0];
        centroidY += dataPair.first[pubstructs::C_1];
    }
    centroidX /= imageData.size();
    centroidY /= imageData.size();
    // Calculate the center of the image
    unsigned int imageCenterX = imageWidth / 2;
    unsigned int imageCenterY = imageHeight / 2;
    // Calculate the shift needed
    int shiftX = imageCenterX - static_cast<int>(centroidX);
    int shiftY = imageCenterY - static_cast<int>(centroidY);
    // Shift each keypoint position
    for (auto& dataPair : imageData) {
        dataPair.first[pubstructs::C_0] = static_cast<unsigned int>(std::max(0, std::min(static_cast<int>(dataPair.first[pubstructs::C_0]) + shiftX, static_cast<int>(imageWidth - 1))));
        dataPair.first[pubstructs::C_1] = static_cast<unsigned int>(std::max(0, std::min(static_cast<int>(dataPair.first[pubstructs::C_1]) + shiftY, static_cast<int>(imageHeight - 1))));
    }
}
void cvLib::subfunctions::move_objs_to_center(std::unordered_map<std::string, std::vector<std::pair<unsigned int, unsigned int>>>& objectPixelsMap, unsigned int imageWidth, unsigned int imageHeight){
    if (objectPixelsMap.empty()) {
        return;
    }
    // Go through each vector of object pixels within the map
    for (auto& entry : objectPixelsMap) {
        std::vector<std::pair<unsigned int, unsigned int>>& objectPixels = entry.second;
        if (objectPixels.empty()) {
            continue;
        }
        // Calculate the centroid of the object
        double centroidX = 0.0;
        double centroidY = 0.0;
        for (const auto& pixel : objectPixels) {
            centroidX += pixel.first;
            centroidY += pixel.second;
        }
        centroidX /= objectPixels.size();
        centroidY /= objectPixels.size();
        // Calculate the center of the image
        unsigned int imageCenterX = imageWidth / 2;
        unsigned int imageCenterY = imageHeight / 2;
        // Calculate the shift needed
        int shiftX = imageCenterX - static_cast<int>(centroidX);
        int shiftY = imageCenterY - static_cast<int>(centroidY);
        // Shift each pixel position
        for (auto& pixel : objectPixels) {
            // Ensure new positions are within image bounds
            pixel.first = static_cast<unsigned int>(std::max(0, std::min(static_cast<int>(pixel.first) + shiftX, static_cast<int>(imageWidth - 1))));
            pixel.second = static_cast<unsigned int>(std::max(0, std::min(static_cast<int>(pixel.second) + shiftY, static_cast<int>(imageHeight - 1))));
        }
    }
}
//sigma = 0.33
void cvLib::subfunctions::automaticCanny(const cv::Mat& gray, cv::Mat& edges, double sigma){
    // Calculate the median of the gradient magnitudes
    if (gray.empty()) {
        throw std::invalid_argument("The input image is empty");
    }
    // Compute gradients using Sobel
    cv::Mat grad_x, grad_y;
    cv::Sobel(gray, grad_x, CV_32F, 1, 0, 3);  // Use CV_32F
    cv::Sobel(gray, grad_y, CV_32F, 0, 1, 3);  // Use CV_32F
    // Compute the magnitude of gradients
    cv::Mat grad;
    cv::magnitude(grad_x, grad_y, grad);
    // Flatten gradient values into a vector for median calculation
    std::vector<float> gradientList(grad.begin<float>(), grad.end<float>());
    // Compute the median of the gradient magnitudes
    std::nth_element(gradientList.begin(), gradientList.begin() + gradientList.size() / 2, gradientList.end());
    float medianGrad = gradientList[gradientList.size() / 2];
    // Calculate thresholds based on the median
    float lowerThresholdFloat = std::max(0.0f, (1.0f - static_cast<float>(sigma)) * medianGrad);
    float upperThresholdFloat = std::min(255.0f, (1.0f + static_cast<float>(sigma)) * medianGrad);
    int lowerThreshold = static_cast<int>(lowerThresholdFloat);
    int upperThreshold = static_cast<int>(upperThresholdFloat);
    // Perform Canny edge detection
    cv::Canny(gray, edges, lowerThreshold, upperThreshold);
}
void cvLib::subfunctions::visual_recognize_obj(const std::vector<std::pair<cvLib::the_obj_in_an_image,double>>& test_img_data,const double& check_record_numbers, const std::unordered_map<std::string, std::vector<pubstructs::RGB>>& traineddataMap, cvLib::the_obj_in_an_image& objInfo){
    if(test_img_data.empty()){
        std::cerr << "cvLib::subfunctions::visual_recognize_obj : test_img_data is empty!" << std::endl;
        return;
    }
    /*
        start recognizing...
        trained_img_data
        std::unordered_map<std::string, std::vector<std::pair<unsigned int, unsigned int>>> _loaddataMap;
    */
    try{
        /*
            rule 1 starts
        */
        // double check_record_numbers = 0.2;//0.2
        // const unsigned int checkRange = static_cast<unsigned int>(check_record_numbers * test_img_data.size());
        // std::vector<cvLib::the_obj_in_an_image> test_pick;
        // if (checkRange > 0) {
        //     for (unsigned int j = 0; j < checkRange; ++j) {
        //         test_pick.push_back(test_img_data[j].first);
        //     }
        // }
        // if (!test_pick.empty() && !_loaddataMap.empty()) {
        //     for (const auto& train_item : _loaddataMap) {
        //         bool matchFound = false;
        //         std::vector<pubstructs::RGB> train_unit = train_item.second;
        //         if (train_unit.size() < test_pick.size()) {
        //             continue;
        //         }
        //         // Try every possible starting position in train_unit
        //         for (unsigned int i = 0; i <= train_unit.size() - test_pick.size(); ++i) {
        //             bool match = false;
        //             // Compare test_pick with the current slice of train_unit
        //             for (unsigned int k = 0; k < test_pick.size(); ++k) {
        //                 if(k+7 < test_pick.size()){
        //                     if(test_pick[k].rgb == train_unit[i] &&
        //                        test_pick[k+1].rgb == train_unit[i+1] &&
        //                        test_pick[k+2].rgb == train_unit[i+2] &&
        //                        test_pick[k+3].rgb == train_unit[i+3] &&
        //                        test_pick[k+4].rgb == train_unit[i+4] &&
        //                        test_pick[k+5].rgb == train_unit[i+5] &&
        //                        test_pick[k+6].rgb == train_unit[i+6]){
        //                        match = true;
        //                        str_result.x = test_pick[k].x;
        //                        str_result.y = test_pick[k].y;
        //                        str_result.rec_topLeft = test_pick[k].rec_topLeft;
        //                        str_result.rec_bottomRight = test_pick[k].rec_bottomRight;
        //                        break;
        //                     }
        //                 }
        //             }
        //             if (match) {
        //                 matchFound = true;
        //                 str_result.objName = train_item.first;
        //                 break; // Exit the loop as a match is found
        //             }
        //         }
        //         if (matchFound) {
        //             break; // Exit the outer loop as a match is found
        //         }
        //     }
        // }
        /*
            rule 1 end
        */
        /*
            rule 2 starts here
        */
        //double check_record_numbers = 0.2;//0.2
        std::unordered_map<std::string,unsigned int> temp_res;
        const unsigned int checkRange = static_cast<unsigned int>(check_record_numbers * test_img_data.size());
        std::vector<cvLib::the_obj_in_an_image> test_pick;
        if (checkRange > 0) {
            for (unsigned int j = 0; j < checkRange; ++j) {
                test_pick.push_back(test_img_data[j].first);
            }
        }
        if (!test_pick.empty() && !traineddataMap.empty()){
            unsigned int matchCount = 0;
            // Compare test_pick with the current slice of train_unit
            /*
                //apply to str_result
                str_result.objName = train_item.first;
                str_result.x = test_pick[k].x;
                str_result.y = test_pick[k].y;
                str_result.rec_topLeft = test_pick[k].rec_topLeft;
                str_result.rec_bottomRight = test_pick[k].rec_bottomRight;
            */
            for (unsigned int k = 0; k < test_pick.size(); ++k) {
                if(k+7 < test_pick.size()){
                    objInfo.x = test_pick[k].x;
                    objInfo.y = test_pick[k].y;
                    objInfo.rec_topLeft = test_pick[k].rec_topLeft;
                    objInfo.rec_bottomRight = test_pick[k].rec_bottomRight;
                    for (const auto& train_item : traineddataMap) {
                        std::vector<pubstructs::RGB> train_unit = train_item.second;
                        if (train_unit.size() < test_pick.size()) {
                            continue;
                        }
                        for (unsigned int i = 0; i <= train_unit.size() - test_pick.size(); ++i){
                            if(test_pick[k].rgb == train_unit[i] &&
                            test_pick[k+1].rgb == train_unit[i+1] &&
                            test_pick[k+2].rgb == train_unit[i+2] &&
                            test_pick[k+3].rgb == train_unit[i+3] &&
                            test_pick[k+4].rgb == train_unit[i+4] &&
                            test_pick[k+5].rgb == train_unit[i+5] &&
                            test_pick[k+6].rgb == train_unit[i+6]){
                                temp_res[train_item.first]++;
                            }
                        }
                    }
                }
            }
            /*
                sort the result
            */
            if(!temp_res.empty()){
                std::vector<std::pair<std::string, unsigned int>> sorted_score_counting(temp_res.begin(), temp_res.end());
                // Sort the vector of pairs
                std::sort(sorted_score_counting.begin(), sorted_score_counting.end(), [](const auto& a, const auto& b) {
                    return a.second > b.second;
                });
                auto it = sorted_score_counting.begin();
                objInfo.objName = it->first;
            }
        }
        /*
            rule 2 ends here
        */
        
    }
    catch (const std::filesystem::filesystem_error& e) {  
        std::cerr << "Filesystem error: " << e.what() << std::endl;  
    }  
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {  
        std::cerr << "Unknown exception occurred." << std::endl;   
    }
}
cvLib::the_obj_in_an_image cvLib::subfunctions::getObj_in_an_image(const cvLib& cvl_j,const cv::Mat& getImg,const cvLib::mark_font_info& markerInfo){
    /*
        preprocess image
    */
    cvLib::the_obj_in_an_image str_result;
    if(getImg.empty()){
        return str_result;
    }
    str_result.fontface = markerInfo.fontface;
    str_result.fontScale = markerInfo.fontScale;
    str_result.thickness = markerInfo.thickness;
    str_result.fontcolor = markerInfo.fontcolor;
    str_result.text_position = markerInfo.text_position;
    std::chrono::time_point<std::chrono::high_resolution_clock> t_count_start;
    std::chrono::time_point<std::chrono::high_resolution_clock> t_count_end;
    std::unordered_map<std::string, std::vector<pubstructs::RGB>> _loaddataMap;
    unsigned int _gradientMagnitude_threshold = 33; 
    bool display_time = false;
    unsigned int distance_bias = 2;
    display_time = cvl_j.get_display_time();
    distance_bias = cvl_j.get_distance_bias();
    _gradientMagnitude_threshold = cvl_j.get_gradientMagnitude_threshold();
    _loaddataMap = cvl_j.get_loaddataMap();
    double percent_check_sub = cvl_j.get_percent_to_check();
    if(display_time){
        t_count_start = std::chrono::high_resolution_clock::now(); // Initialize start time 
    }
    if(!getImg.empty()){
        cv::Mat desc;
        std::vector<cv::KeyPoint> testKey = cvl_j.extractORBFeatures(getImg,desc);
        if(!testKey.empty()){
            std::vector<std::pair<cvLib::the_obj_in_an_image,double>> test_img_data;
            for (size_t i = 0; i < testKey.size(); ++i) {
                const cv::KeyPoint& kp = testKey[i]; 
                unsigned int x = static_cast<unsigned int>(kp.pt.x);
                unsigned int y = static_cast<unsigned int>(kp.pt.y);
                // Ensure the coordinates are within the image bounds
                if (x < static_cast<unsigned int>(getImg.cols) && y < static_cast<unsigned int>(getImg.rows)) {
                    // Get the pubstructs::RGB pixel value at (x, y)
                    cv::Vec3b pixel = getImg.at<cv::Vec3b>(cv::Point(x, y));
                    uint8_t blue = pixel[pubstructs::C_0];
                    uint8_t green = pixel[pubstructs::C_1];
                    uint8_t red = pixel[pubstructs::C_2];
                    // Create an pubstructs::RGB object and add it to the vector
                    pubstructs::RGB color(red, green, blue);
                    cvLib::the_obj_in_an_image temp_obj;
                    temp_obj.rgb = color;
                    temp_obj.x = static_cast<int>(kp.pt.x);
                    temp_obj.y = static_cast<int>(kp.pt.y);
                    temp_obj.rec_topLeft = cv::Point(kp.pt.x - kp.size / 2, kp.pt.y - kp.size / 2);
                    temp_obj.rec_bottomRight = cv::Point(kp.pt.x + kp.size / 2, kp.pt.y + kp.size / 2);
                    test_img_data.push_back(std::make_pair(temp_obj,static_cast<double>(kp.response)));
                } else {
                    std::cerr << "Coordinate (" << x << ", " << y << ") is out of bounds" << std::endl;
                }
            }
            if(!test_img_data.empty()){
                std::sort(test_img_data.begin(),test_img_data.end(),[](const auto& a, const auto& b){
                    return a.second > b.second;
                });
                this->visual_recognize_obj(test_img_data,percent_check_sub,_loaddataMap,str_result);
            }
            else{
                std::cerr << "cvLib::subfunctions::getObj_in_an_image(line:460): test_img is empty!" << std::endl;
            }
        }
    }
    if(display_time){
        t_count_end = std::chrono::high_resolution_clock::now();   
        std::chrono::duration<double> duration = t_count_end - t_count_start;  
        //std::cout << "Execution time: " << duration.count() << " seconds\n"; 
        str_result.timespent = duration;
    }
    return str_result;
}
cv::Mat cvLib::subfunctions::convertDatasetToMat(const std::vector<std::vector<pubstructs::RGB>>& dataset) { 
   if (dataset.empty() || dataset[pubstructs::C_0].empty()) {
        throw std::runtime_error("Dataset is empty or has no columns.");
    }
    int rows = dataset.size();
    int cols = dataset[pubstructs::C_0].size();
    cv::Mat image(rows, cols, CV_8UC3);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            const pubstructs::RGB& rgb = dataset[i][j];
            image.at<cv::Vec3b>(i, j) = cv::Vec3b(rgb.b, rgb.g, rgb.r);
        }
    }
    return image;
}  
void cvLib::subfunctions::markVideo(cv::Mat& frame,const cv::Scalar& brush_color, const cv::Scalar& bg_color){
    if(frame.empty()){
        return;
    }
    cvLib cvl_j;
    std::vector<std::vector<pubstructs::RGB>> frame_to_mark = cvl_j.cv_mat_to_dataset(frame);
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
bool cvLib::subfunctions::isPointInPolygon(int x, int y, const std::vector<std::pair<int, int>>& polygon) {  
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
std::vector<std::vector<pubstructs::RGB>> cvLib::subfunctions::getPixelsInsideObject(const std::vector<std::vector<pubstructs::RGB>>& image_rgb, const std::vector<std::pair<int, int>>& objEdges) {  
    if(image_rgb.empty() || image_rgb[pubstructs::C_0].empty() || objEdges.empty()) {  
        return {}; // Return an empty image if no data or edges
    }
    std::vector<std::vector<pubstructs::RGB>> output_objs = image_rgb;
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
cv::Mat cvLib::subfunctions::getObjectsInVideo(const cv::Mat& inVideo){
    cvLib cv_j;
    std::vector<std::vector<pubstructs::RGB>> objects_detect;
    std::vector<std::vector<pubstructs::RGB>> image_rgb = cv_j.cv_mat_to_dataset_color(inVideo);  
    if (!image_rgb.empty()) {  
        // Find outliers (edges)  
        auto outliers = cv_j.findOutlierEdges(image_rgb, 5);   
        objects_detect = this->getPixelsInsideObject(image_rgb, outliers);  
    }  
    cv::Mat finalV = this->convertDatasetToMat(objects_detect);
    return finalV;  
}
void cvLib::subfunctions::saveModel(const std::unordered_map<std::string, std::vector<cv::Mat>>& featureMap, const std::string& filename){
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
void cvLib::subfunctions::saveModel_keypoint(const std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>>& featureMap, const std::string& filename) {
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
        for (const auto& [className, keypointSets] : featureMap) {
            size_t keySize = className.size();
            ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
            ofs.write(className.data(), keySize);
            size_t setCount = keypointSets.size();
            ofs.write(reinterpret_cast<const char*>(&setCount), sizeof(setCount));
            for (const auto& keypoints : keypointSets) {
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
        }
    } catch (const std::exception& e) {
        std::cerr << "Error writing to file: " << e.what() << std::endl;
    }
    ofs.close();
}
void cvLib::subfunctions::merge_without_duplicates(std::vector<uint8_t>& data_main, const std::vector<uint8_t>& data_append) {  
    if(data_main.empty() || data_append.empty()){
        return;
    }
    std::map<uint8_t,unsigned int> mNoDup;
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
    std::vector<std::pair<uint8_t, unsigned int>> vec(mNoDup.begin(), mNoDup.end());
    // Sort the vector by the values in descending order
    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    data_main.clear();
    for(const auto& item : vec){
        data_main.push_back(item.first);
    }
}  
bool cvLib::subfunctions::isSimilar(const pubstructs::RGB& a, const pubstructs::RGB& b, const unsigned int& threshold) {
    // Example similarity threshold comparison, adjust as necessary
    return (std::abs(static_cast<int>(a.r) - static_cast<int>(b.r)) < threshold &&
            std::abs(static_cast<int>(a.g) - static_cast<int>(b.g)) < threshold &&
            std::abs(static_cast<int>(a.b) - static_cast<int>(b.b)) < threshold);
}
std::tuple<double, double, double> cvLib::subfunctions::rgbToHsv(unsigned int r, unsigned int g, unsigned int b){
    double rd = r / 255.0;
    double gd = g / 255.0;
    double bd = b / 255.0;
    double cmax = std::max({rd, gd, bd});
    double cmin = std::min({rd, gd, bd});
    double delta = cmax - cmin;
    double h = 0.0;
    double s = (cmax == 0) ? 0 : (delta / cmax);
    double v = cmax;
    if (delta > 0) {
        if (cmax == rd) {
            h = 60 * (fmod(((gd - bd) / delta), 6));
        } else if (cmax == gd) {
            h = 60 * (((bd - rd) / delta) + 2);
        } else if (cmax == bd) {
            h = 60 * (((rd - gd) / delta) + 4);
        }
    }
    if (h < 0) {
        h += 360;
    }
    return {h, s * 100, v * 100}; // Return H, S, V
}
// Convert your pubstructs::RGB struct to OpenCV's Vec3b (BGR format)
cv::Vec3b cvLib::subfunctions::rgbToVec3b(const pubstructs::RGB& color) {
    // Directly use the color components since they are already uint8_t
    return cv::Vec3b(color.b, color.g, color.r);
}
// Convert OpenCV's Vec3b (BGR format) to your pubstructs::RGB struct
pubstructs::RGB cvLib::subfunctions::vec3bToRgb(const cv::Vec3b& color){
    return pubstructs::RGB(color[pubstructs::C_2], color[pubstructs::C_1], color[pubstructs::C_0]); // Convert OpenCV's BGR to pubstructs::RGB
}
std::pair<cv::Scalar, cv::Scalar> cvLib::subfunctions::determineHSVRange(const cv::Mat& hsv, double percentile) const{
    cv::Mat hist;
    // Compute histograms for H, S, and V channels
    int histSize[] = {180, 256, 256}; // HistSize for H, S, V channels
    float hranges[] = {0, 180};
    float sranges[] = {0, 256};
    float vranges[] = {0, 256};
    const float* ranges[] = {hranges, sranges, vranges};
    int channels[] = {0, 1, 2};
    cv::calcHist(&hsv, 1, channels, cv::Mat(), hist, 3, histSize, ranges);
    // Calculate the lower and upper HSV values based on histogram analysis or specific logic.
    cv::Scalar lowH, highH;
    // A simple strategy with the percentile (e.g., lower and upper fraction of pixel values)
    double minHue = (double)histSize[pubstructs::C_0] * percentile;
    double maxHue = (double)histSize[pubstructs::C_0] * (1.0 - percentile);
    // Example initialization
    lowH = cv::Scalar(minHue, 100, 100);
    highH = cv::Scalar(maxHue, 255, 255);
    return std::make_pair(lowH, highH);
}
// Function to read a WebP image and convert it to a vector of tuples
std::vector<std::tuple<double, double, double>> cvLib::subfunctions::webpToTupleVector(const std::string& webpImagePath) {
    if(webpImagePath.empty()){
        return {};
    }
    cv::Mat image = cv::imread(webpImagePath, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Error: Could not read the WebP image." << std::endl;
        return {};
    }
    std::vector<std::tuple<double, double, double>> dataset;
    for (int i = 0; i < image.rows; ++i) {
        for (int j = 0; j < image.cols; ++j) {
            cv::Vec3b pixel = image.at<cv::Vec3b>(i, j);
            double b = pixel[pubstructs::C_0] / 255.0;
            double g = pixel[pubstructs::C_1] / 255.0;
            double r = pixel[pubstructs::C_2] / 255.0;
            dataset.emplace_back(r, g, b);
        }
    }
    return dataset;
}
// Function to convert the tuple vector back to an pubstructs::RGB 2D vector
std::vector<std::vector<pubstructs::RGB>> cvLib::subfunctions::tupleVectorToRGBVector(
    const std::vector<std::tuple<double, double, double>>& dataset, int width, int height) {
    if (dataset.empty() || width <= 0 || height <= 0 || dataset.size() != static_cast<size_t>(width * height)) {
        throw std::runtime_error("Dataset is empty or dimensions are incorrect.");
        return {};
    }
    std::vector<std::vector<pubstructs::RGB>> imageRGB(height, std::vector<pubstructs::RGB>(width));
    size_t index = 0;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            auto [r, g, b] = dataset[index++];
            imageRGB[i][j] = pubstructs::RGB(static_cast<uint8_t>(r * 255), 
                                 static_cast<uint8_t>(g * 255), 
                                 static_cast<uint8_t>(b * 255));
        }
    }
    return imageRGB;
}
// Assuming the contours and other data have been prepared accordingly
std::vector<cv::Mat> cvLib::subfunctions::extractAndTransformContours(
    const cv::Mat& inputImage, double lowerCannyThreshold, double upperCannyThreshold, int minContourArea){
    // Step 1: Preprocess the image
    cv::Mat grayImage, blurredImage, edges, processedEdges;
    cv::cvtColor(inputImage, grayImage, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(grayImage, blurredImage, cv::Size(5, 5), 0);
    // Step 2: Apply edge detection (Canny)
    cv::Canny(blurredImage, edges, lowerCannyThreshold, upperCannyThreshold);
    // Step 3: Apply morphological transformations
    cv::dilate(edges, processedEdges, cv::Mat(), cv::Point(-1, -1), 2);
    cv::erode(processedEdges, processedEdges, cv::Mat(), cv::Point(-1, -1), 1);
    // Step 4: Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processedEdges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    // Step 5: Filter and process each contour
    std::vector<cv::Mat> contourImages;
    const int canvasSize = 800;
    const double targetScale = 0.80; // Target 80% of the image size
    for (const auto& contour : contours) {
        // Filter contours by minimum area
        if (cv::contourArea(contour) < minContourArea) {
            continue;
        }
        try{
            // Calculate bounding box of the contour
            cv::Rect boundingBox = cv::boundingRect(contour);
            // Create a mask
            cv::Mat mask = cv::Mat::zeros(inputImage.size(), CV_8UC1);
            cv::drawContours(mask, std::vector<std::vector<cv::Point>>{contour}, -1, cv::Scalar(255), cv::FILLED);
            // Extract the colored image using mask
            cv::Mat coloredRegion;
            inputImage.copyTo(coloredRegion, mask);
            // Extract the ROI based on bounding box and apply scaling
            cv::Mat roi = coloredRegion(boundingBox);
            // Calculate the scale to make the bounding box take up 80% of the canvas
            double scaleX = (canvasSize * targetScale) / boundingBox.width;
            double scaleY = (canvasSize * targetScale) / boundingBox.height;
            double scale = std::min(scaleX, scaleY); // Maintain aspect ratio
            // Resize the ROI
            cv::Mat scaledROI;
            cv::resize(roi, scaledROI, cv::Size(), scale, scale);
            // Create a white background and place the scaled ROI in the center
            cv::Mat whiteBackground = cv::Mat::ones(canvasSize, canvasSize, CV_8UC3) * 255;
            int offsetX = (canvasSize - scaledROI.cols) / 2;
            int offsetY = (canvasSize - scaledROI.rows) / 2;
            scaledROI.copyTo(whiteBackground(cv::Rect(offsetX, offsetY, scaledROI.cols, scaledROI.rows)));
            // Store the resulting image
            contourImages.push_back(whiteBackground);
        }
        catch(const std::exception& e){
            std::cerr << e.what() << " error ignored." << std::endl;
        }
    }
    return contourImages;
}
/*
    end subfunctions------------------------------------------------------
*/
/*
    start sub functions sdl2
*/
void cvLib::subsdl2::cleanup(SDL_Window* window, SDL_Renderer* renderer){
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}
SDL_Texture* cvLib::subsdl2::loadTexture(const std::string& path, SDL_Renderer* renderer){
    SDL_Texture* texture = nullptr;
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
    }
    return texture;
}
bool cvLib::subsdl2::saveTextureToFile(SDL_Renderer* renderer, SDL_Texture* texture, const std::string& filename) {
    if(filename.empty()){
        return false;
    }
    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        std::cerr << "Unable to create surface: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetRenderTarget(renderer, texture);
    SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch);
    // Using SDL2_image to write the surface to a file
    if (IMG_SavePNG(surface, filename.c_str()) != 0) {
        std::cerr << "Failed to save PNG: " << IMG_GetError() << std::endl;
    }
    SDL_FreeSurface(surface);
    SDL_SetRenderTarget(renderer, nullptr);
    return true;
}
std::vector<std::vector<pubstructs::RGB>> cvLib::subsdl2::convertSurfaceToVector(SDL_Surface* surface){
    std::vector<std::vector<pubstructs::RGB>> imageData;
    if (surface->format->BytesPerPixel != 4) {
        std::cerr << "Surface does not have 4 bytes per pixel" << std::endl;
        return imageData;
    }
    auto* pixels = static_cast<Uint32*>(surface->pixels);
    int width = surface->w;
    int height = surface->h;
    int pitch = surface->pitch / 4; // pitch is in bytes, divided by 4 for number of pixels
    imageData.resize(height, std::vector<pubstructs::RGB>(width));
    SDL_PixelFormat* fmt = surface->format;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Uint32 pixel = pixels[y * pitch + x];
            uint8_t r, g, b;
            SDL_GetRGB(pixel, fmt, &r, &g, &b);
            imageData[y][x] = pubstructs::RGB(r, g, b);
        }
    }
    return imageData;
}
cv::Mat cvLib::subsdl2::put_img2_in_img1(
    cvLib& parent,
    const std::string& img1, 
    const std::string& img2, 
    const int img2Width,
    const int img2Height,
    const int img2Left,
    const int img2Top,
    const uint8_t img2alpha,
    const std::string& output_img, 
    const unsigned int in_img_width, 
    const unsigned int in_img_height, 
    const unsigned int out_img_width, 
    const unsigned int out_img_height, 
    const unsigned int inDepth){
    if(img1.empty() || img2.empty() || output_img.empty()){
        return cv::Mat();
    }
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return cv::Mat();
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return cv::Mat();
    }
    SDL_Window* window = SDL_CreateWindow("SDL2 Image Output", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, in_img_width, in_img_height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        IMG_Quit();
        SDL_Quit();
        return cv::Mat();
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        cleanup(window, nullptr);
        return cv::Mat();
    }
    SDL_Texture* baseTexture = loadTexture(img1, renderer);
    SDL_Texture* overlayTexture = loadTexture(img2, renderer);
    if (!baseTexture || !overlayTexture) {
        cleanup(window, renderer);
        return cv::Mat();
    }
    SDL_SetTextureBlendMode(overlayTexture, SDL_BLENDMODE_BLEND);
    SDL_Texture* targetTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, out_img_width, out_img_height);
    SDL_SetRenderTarget(renderer, targetTexture);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, baseTexture, nullptr, nullptr);
    SDL_Rect overlayRect = {img2Left, img2Top, img2Width, img2Height};
    SDL_SetTextureAlphaMod(overlayTexture, img2alpha);
    SDL_RenderCopy(renderer, overlayTexture, nullptr, &overlayRect);
    SDL_SetRenderTarget(renderer, nullptr);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, out_img_width, out_img_height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!surface) {
        std::cerr << "Unable to create surface: " << SDL_GetError() << std::endl;
        cleanup(window, renderer);
        return {};
    }
    SDL_SetRenderTarget(renderer, targetTexture);
    if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA8888, surface->pixels, surface->pitch) != 0) {
        std::cerr << "SDL_RenderReadPixels error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        cleanup(window, renderer);
        return {};
    }
    auto imageData = convertSurfaceToVector(surface);
    cv::Mat finalImg = parent.subf_j.convertDatasetToMat(imageData);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(baseTexture);
    SDL_DestroyTexture(overlayTexture);
    SDL_DestroyTexture(targetTexture);
    cleanup(window, renderer);
    return finalImg;
}
/*
    end sdl2 sub library
*/
/*
    Start cvLib -----------------------------------------------------------------------------------------------------
*/
/*
        std::unordered_map<std::string, std::vector<std::pair<unsigned int, unsigned int>>> _loaddataMap;
        unsigned int _gradientMagnitude_threshold = 33; 
        bool display_time = false;
        unsigned int distance_bias = 2;
*/
void cvLib::set_distance_bias(const unsigned int& db){
    this->distance_bias = db;
}
unsigned int cvLib::get_distance_bias() const{
    return distance_bias;
}
void cvLib::set_display_time(const bool& dt){
    this->display_time = dt;
}
bool cvLib::get_display_time() const{
    return display_time;
}
void cvLib::set_learing_rate(const double& lr){
    this->learning_rate = lr;
}
double cvLib::get_learning_rate() const{
    return learning_rate;
}
void cvLib::set_gradientMagnitude_threshold(const unsigned int& gmt){
    this->_gradientMagnitude_threshold = gmt;
}
unsigned int cvLib::get_gradientMagnitude_threshold() const{
    return _gradientMagnitude_threshold;
}
void cvLib::set_loaddataMap(const std::unordered_map<std::string, std::vector<pubstructs::RGB>>& lmap){
    this->_loaddataMap = lmap;
}
std::unordered_map<std::string, std::vector<pubstructs::RGB>> cvLib::get_loaddataMap() const{
    return _loaddataMap;
}
void cvLib::set_percent_to_check(const double& peCheck){
    this->percent_to_check = peCheck;
}
double cvLib::get_percent_to_check() const{
    return percent_to_check;
}
std::vector<std::string> cvLib::splitString(const std::string& input, char delimiter){
    std::vector<std::string> result;
    if(input.empty() || delimiter == '\0'){
        std::cerr << "Jsonlib::splitString input empty" << '\n';
    	return result;
    }
    std::stringstream ss(input);
    std::string token;
    while(std::getline(ss,token,delimiter)){
        result.push_back(token);
    }
    return result;
}
cv::Mat cvLib::convert_to_256_8bit(const cv::Mat& inputImage){
    if (inputImage.empty()) {
        std::cerr << "Error: Input image is empty!" << std::endl;
        return cv::Mat();
    }
    try{
        cv::Mat resizedImage;
        cv::resize(inputImage, resizedImage, cv::Size(800, 800));
        cv::Mat img;
        cv::cvtColor(resizedImage, img, cv::COLOR_BGR2RGB); // Convert to RGB if needed
        // Reshape the image to a 2D matrix of pixels
        cv::Mat data = img.reshape(1, img.total());
        data.convertTo(data, CV_32F); // Convert to floating point for K-means
        // Define criteria: (Type, max_iter, epsilon)
        //cv::TermCriteria criteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 10, 1.0);
        cv::TermCriteria criteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 5, 1.0);
        // Apply K-means clustering to reduce colors
        int k = 256; // We want a palette of 256 colors
        cv::Mat labels, centers;
        cv::kmeans(data, k, labels, criteria, 3, cv::KMEANS_PP_CENTERS, centers);
        // Map the centers back to original image
        cv::Mat quantizedImage(data.size(), data.type());
        for (size_t i = 0; i < data.rows; ++i) {
            quantizedImage.at<cv::Vec3f>(i) = centers.at<cv::Vec3f>(labels.at<int>(i));
        }
        // Reshape back to the original image shape
        quantizedImage = quantizedImage.reshape(3, img.rows);
        quantizedImage.convertTo(quantizedImage, CV_8UC3);
        cv::cvtColor(quantizedImage, quantizedImage, cv::COLOR_RGB2BGR); // Convert back to BGR if needed
        return quantizedImage;
    }
    catch(const std::exception& ex){
        std::cerr << "cvLib::convert_to_256_8bit " << ex.what() << std::endl;
    }
    return cv::Mat();
}
cv::Mat cvLib::placeOnTransparentBackground(const cv::Mat& inImg, unsigned int img_width, unsigned int img_height) {
    if (inImg.empty()) {
        return cv::Mat();
    }
    cv::Mat alphaImg;
    // Ensure the image has an alpha channel
    if (inImg.channels() == 3) {
        cv::cvtColor(inImg, alphaImg, cv::COLOR_BGR2BGRA);
    } else {
        alphaImg = inImg;
    }
    // Step 2: Resize the image to 80% of the dimension
    int resized_img_width = static_cast<int>(img_width * 0.8);
    int resized_img_height = static_cast<int>(img_height * 0.8);
    cv::Size newSize(resized_img_width, resized_img_height);
    cv::Mat resizedImage;
    cv::resize(alphaImg, resizedImage, newSize);
    // Step 3: Create a transparent background
    cv::Mat background(cv::Size(img_width, img_height), CV_8UC4, cv::Scalar(0, 0, 0, 0)); // Transparent
    // Step 4: Calculate positioning to center the resized image on the background
    int xOffset = (background.cols - resizedImage.cols) / 2;
    int yOffset = (background.rows - resizedImage.rows) / 2;
    // Step 5: Place the resized image on the transparent background centered
    cv::Rect roi(xOffset, yOffset, resizedImage.cols, resizedImage.rows);
    resizedImage.copyTo(background(roi), resizedImage);
    return background;
}
// Function to convert std::vector<uint8_t> to std::vector<std::vector<pubstructs::RGB>>  
std::vector<std::vector<pubstructs::RGB>> cvLib::convertToRGB(const std::vector<uint8_t>& pixels, unsigned int width, unsigned int height) {  
    std::vector<std::vector<pubstructs::RGB>> image(height, std::vector<pubstructs::RGB>(width));  
    for (unsigned int y = 0; y < height; ++y) {  
        for (unsigned int x = 0; x < width; ++x) {  
            uint8_t packedPixel = pixels[y * width + x];  
            uint8_t r = (packedPixel >> 16) & 0xFF; // Extract red  
            uint8_t g = (packedPixel >> 8) & 0xFF;  // Extract green  
            uint8_t b = packedPixel & 0xFF;         // Extract blue  
            image[y][x] = pubstructs::RGB(r, g, b); // Assign to 2D vector  
        }  
    }  
    return image; // Return the resulting 2D pubstructs::RGB vector  
}  
// Function to convert std::vector<std::vector<pubstructs::RGB>> back to std::vector<uint8_t>  
std::vector<uint8_t> cvLib::convertToPacked(const std::vector<std::vector<pubstructs::RGB>>& image) {  
    unsigned int height = image.size();  
    unsigned int width = (height > 0) ? image[0].size() : 0;  
    std::vector<uint8_t> pixels(height * width);  
    for (unsigned int y = 0; y < height; ++y) {  
        for (unsigned int x = 0; x < width; ++x) {  
            const pubstructs::RGB& rgb = image[y][x];  
            // Pack the RGB into a uint8_t  
            pixels[y * width + x] = (static_cast<uint8_t>(rgb.r) << 16) |  
                                     (static_cast<uint8_t>(rgb.g) << 8) |  
                                     (static_cast<uint8_t>(rgb.b));  
        }  
    }  
    return pixels; // Return the resulting packed pixel vector  
}  
// Function to convert std::vector<uint8_t> to cv::Mat  
cv::Mat cvLib::vectorToImage(const std::vector<std::vector<pubstructs::RGB>>& pixels, unsigned int width, unsigned int height) {  
    if(pixels.empty()){
        return cv::Mat();
    }
    // Create a cv::Mat object with the specified dimensions and type (CV_8UC3 for BGR).
    cv::Mat image(height, width, CV_8UC3);
    // Verify that the height and width match the input vector
    assert(pixels.size() == height && (height == 0 || pixels[pubstructs::C_0].size() == width));
    // Iterate through each pixel in the vector
    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            // Get the RGB pixel from the vector
            const pubstructs::RGB& pixel = pixels[y][x];
            // Set the pixel value in the cv::Mat (OpenCV uses BGR format)
            image.at<cv::Vec3b>(y, x) = cv::Vec3b(pixel.b, pixel.g, pixel.r);
        }
    }
    return image;  // Return the created image
}  
/*
    Function to detect keypoints by clusters
    para1: input image 
    para2: clusters cv::Rect(0, 0, 100, 100), cv::Rect(100, 100, 100, 100)
*/
std::unordered_map<std::string, std::vector<cv::KeyPoint>> detectKeypointsByClusters(const cv::Mat& image, const std::vector<cv::Rect>& regions) {
    // Create an ORB detector
    auto orb = cv::ORB::create();
    // Map to store keypoints with cluster identifiers
    std::unordered_map<std::string, std::vector<cv::KeyPoint>> keypointsMap;
    for (size_t i = 0; i < regions.size(); ++i) {
        // Extract the region of interest
        cv::Mat region = image(regions[i]);
        // Detect keypoints in the specified region
        std::vector<cv::KeyPoint> keypoints;
        orb->detect(region, keypoints);
        // Formulate the cluster key (e.g., cluster1, cluster2, ...)
        std::string clusterKey = "cluster" + std::to_string(i + 1);
        // Store the detected keypoints in the map
        keypointsMap[clusterKey] = keypoints;
    }
    return keypointsMap;
}
/*
    This function to convert an cv::Mat into a std::vector<std::vector<pubstructs::RGB>> dataset
*/
std::vector<std::vector<pubstructs::RGB>> cvLib::cv_mat_to_dataset(const cv::Mat& genImg) {
    // Create a copy of the input image to avoid modifying original
    cv::Mat processedImg;
    cv::GaussianBlur(genImg, processedImg, cv::Size(5, 5), 0);
    // Check if input has a single channel, convert if not
    if (processedImg.channels() != 1) {
        cv::cvtColor(processedImg, processedImg, cv::COLOR_BGR2GRAY);
    }
    std::vector<std::vector<pubstructs::RGB>> datasets(processedImg.rows, std::vector<pubstructs::RGB>(processedImg.cols));
    for (unsigned int i = 0; i < processedImg.rows; ++i) {
        for (unsigned int j = 0; j < processedImg.cols; ++j) {
            uchar intensity = processedImg.at<uchar>(i, j);
            datasets[i][j] = {static_cast<uint8_t>(intensity), static_cast<uint8_t>(intensity), static_cast<uint8_t>(intensity)};
        }
    }
    return datasets;
}
std::vector<std::vector<pubstructs::RGB>> cvLib::cv_mat_to_dataset_color(const cv::Mat& genImg) {  
    // Create a copy of the input image to apply noise reduction
    cv::Mat processedImg;
    // Noise reduction step using Gaussian blur
    cv::GaussianBlur(genImg, processedImg, cv::Size(5, 5), 0);
    // Or alternatively, use median blur (uncomment if you want to use this instead)
    // cv::medianBlur(genImg, processedImg, 5);
    // Initialize the dataset with the processed image
    std::vector<std::vector<pubstructs::RGB>> datasets(processedImg.rows, std::vector<pubstructs::RGB>(processedImg.cols));  
    for (unsigned int i = 0; i < processedImg.rows; ++i) {  // rows  
        for (unsigned int j = 0; j < processedImg.cols; ++j) {  // cols  
            // Check if the image is still in color:  
            if (processedImg.channels() == 3) {  
                cv::Vec3b bgr = processedImg.at<cv::Vec3b>(i, j);  
                // Populate the pubstructs::RGB struct  
                datasets[i][j] = {static_cast<uint8_t>(bgr[pubstructs::C_2]), static_cast<uint8_t>(bgr[pubstructs::C_1]), static_cast<uint8_t>(bgr[pubstructs::C_0])}; // Convert BGR to pubstructs::RGB  
            } else if (processedImg.channels() == 1) {  
                // Handle grayscale images  
                uchar intensity = processedImg.at<uchar>(i, j);  
                datasets[i][j] = {static_cast<uint8_t>(intensity), static_cast<uint8_t>(intensity), static_cast<uint8_t>(intensity)}; // Grayscale to pubstructs::RGB  
            }   
        }  
    }  
    return datasets;  
}
void cvLib::convertAndSaveAsWebP(const std::string& inputImagePath, const std::string& outputWebPPath){
    if(inputImagePath.empty() || outputWebPPath.empty()){
        return;
    }
    cv::Mat image = cv::imread(inputImagePath, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Error: Could not read the input image." << std::endl;
        return;
    }
    std::vector<int> params = {cv::IMWRITE_WEBP_QUALITY, 90};
    if (!cv::imwrite(outputWebPPath, image, params)) {
        std::cerr << "Error: Could not write the image as WebP." << std::endl;
    } else {
        std::cout << "Image successfully saved as WebP: " << outputWebPPath << std::endl;
    }
}
cvLib::imgSize cvLib::get_image_size(const std::string& imgPath) {  
    imgSize im_s = {0, 0}; // Initialize width and height to 0  
    if (imgPath.empty()) {  
        std::cerr << "Error: Image path is empty." << std::endl;  
        return im_s; // Return default initialized size  
    }  
    cv::Mat img = cv::imread(imgPath);  
    if (img.empty()) { // Check if the image was loaded successfully  
        std::cerr << "get_image_size Error: Could not open or find the image at " << imgPath << std::endl;  
        return im_s; // Return default initialized size  
    }  
    im_s.width = img.cols;  
    im_s.height = img.rows;  
    return im_s;   
}
std::unordered_map<std::string, std::vector<cv::KeyPoint>> cvLib::extractContoursAsKeyPoints(const cv::Mat& edges) {
    if (edges.empty()) {
        return {};
    }
    cv::Mat smoothedEdges;
    // Apply Gaussian Blur to reduce noise and improve contour detection
    cv::GaussianBlur(edges, smoothedEdges, cv::Size(5, 5), 0);
    // Apply a binary threshold to further suppress noise
    cv::Mat binaryEdges;
    cv::threshold(smoothedEdges, binaryEdges, 50, 255, cv::THRESH_BINARY);
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    // Find external contours to avoid nested ones
    cv::findContours(binaryEdges, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    // Filter contours by area to eliminate noise
    const double areaThreshold = 100.0; // Define a threshold based on expected object sizes
    std::vector<std::vector<cv::Point>> filteredContours;
    for (const auto& contour : contours) {
        if (cv::contourArea(contour) > areaThreshold) {
            filteredContours.push_back(contour);
        }
    }
    std::unordered_map<std::string, std::vector<cv::KeyPoint>> contourKeyPoints;
    for (size_t i = 0; i < filteredContours.size(); ++i) {
        std::vector<cv::KeyPoint> keypoints;
        // Approximate contours to reduce number of points for keypoint conversion
        std::vector<cv::Point> approxContour;
        double epsilon = 0.02 * cv::arcLength(filteredContours[i], true); // Adjust the scaling factor if necessary
        cv::approxPolyDP(filteredContours[i], approxContour, epsilon, true);
        for (const auto& point : approxContour) {
            // Convert each point to a keypoint
            keypoints.emplace_back(cv::KeyPoint(
                static_cast<float>(point.x),
                static_cast<float>(point.y),
                1.0f // Use a default size, adjust if needed
            ));
        }
        // Construct key name based on contour index
        std::string key = "Contour_" + std::to_string(i);
        contourKeyPoints[key] = keypoints;
    }
    return contourKeyPoints;
}
void cvLib::sub_find_contours_in_an_image(const std::string& imagePath, std::vector<std::pair<std::string, cv::Mat>>& imgclusters) {
    if (imagePath.empty()) {
        std::cerr << "Error: Image path is empty." << std::endl;
        return;
    }
    try {
        // Read the input image
        cv::Mat image = cv::imread(imagePath);
        if (image.empty()) {
            throw std::runtime_error("Error: Could not open or find the image!");
        }
        // Resize the image while maintaining the aspect ratio
        cv::Mat imageResized;
        float aspectRatio = static_cast<float>(image.cols) / image.rows;
        if (aspectRatio > 1.0) {
            cv::resize(image, imageResized, cv::Size(800, int(800 / aspectRatio)));
        } else {
            cv::resize(image, imageResized, cv::Size(int(800 * aspectRatio), 800));
        }
        // Convert to grayscale
        cv::Mat gray;
        cv::cvtColor(imageResized, gray, cv::COLOR_BGR2GRAY);
        // Use Canny for edge detection
        cv::Mat edges;
        subf_j.automaticCanny(gray, edges, 0.33);
        // Find contours
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (contours.empty()) {
            std::cerr << "Warning: No contours found in the image!" << std::endl;
            return;
        }
        // Iterate over contours and extract images
        for (size_t i = 0; i < contours.size(); i++) {
            // Create a mask for each contour
            cv::Mat mask = cv::Mat::zeros(imageResized.size(), CV_8UC1);
            cv::drawContours(mask, contours, static_cast<int>(i), cv::Scalar(255), cv::FILLED);
            // Use mask to extract contour area from the resized image
            cv::Mat contourContent;
            imageResized.copyTo(contourContent, mask);
            // Add the extracted contour content to the output list
            std::string strK = "contour" + std::to_string(i);
            imgclusters.emplace_back(strK, contourContent);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
/*
    cv::Scalar markerColor(0,255,0);
    cv::Scalar txtColor(255, 255, 255);
    rec_thickness = 2;
*/
// Function to draw a green rectangle on an image 
void cvLib::drawRectangleWithText(cv::Mat& image, int x, int y, unsigned int width, unsigned int height, const std::string& text, unsigned int rec_thickness, const cv::Scalar& markerColor, const cv::Scalar& txtColor, double fontScale, int fontface) {
    // Define rectangle vertices
    cv::Point top_left(x, y);
    cv::Point bottom_right(x + width, y + height);
    // Draw the rectangle
    cv::rectangle(image, top_left, bottom_right, markerColor, rec_thickness);  // Rectangle with specified thickness and color
    // Define text position at the upper-left corner of the rectangle
    cv::Point text_position(x + 5, y - 15);  // Adjust as necessary, '5' from left and '20' from top for padding
    // Add text
    cv::putText(image, text, text_position, fontface, fontScale, txtColor, 1);  // Use fontScale and fontface
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
void cvLib::saveVectorRGB(const std::vector<std::vector<pubstructs::RGB>>& img, unsigned int width, unsigned int height, const std::string& filename){
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
std::vector<std::vector<pubstructs::RGB>> cvLib::get_img_matrix(const std::string& imgPath, unsigned int img_rows, unsigned int img_cols, const pubstructs::inputImgMode& img_mode) {  
    if(imgPath.empty()){
        return {};
    }
    std::vector<std::vector<pubstructs::RGB>> datasets(img_rows, std::vector<pubstructs::RGB>(img_cols));  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "get_img_matrix Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image, gray_image;  
    cv::resize(image, resized_image, cv::Size(img_cols, img_rows));
    if(img_mode == pubstructs::inputImgMode::Gray){
        cv::cvtColor(resized_image, gray_image, cv::COLOR_BGR2GRAY);
        // Assume cv_mat_to_dataset(gray_image) is implemented correctly
        datasets = this->cv_mat_to_dataset(gray_image);
    }
    else{
        datasets = this->cv_mat_to_dataset(resized_image);
    }
    return datasets;  
}
std::multimap<std::string, std::vector<pubstructs::RGB>> cvLib::read_images(std::string& folderPath){  
    std::multimap<std::string, std::vector<pubstructs::RGB>> datasets;  
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
            std::vector<std::vector<pubstructs::RGB>> image_rgb = this->get_img_matrix(nuts_key,img_size.width,img_size.height, pubstructs::inputImgMode::Gray);  
            std::vector<pubstructs::RGB> one_d_image_rgb;
            for(const auto& mg : image_rgb){
                for(const auto& m : mg){
                    pubstructs::RGB data_add = m;
                    one_d_image_rgb.push_back(data_add);
                }
            }
            datasets.insert({nuts_key,one_d_image_rgb});
        }  
    }  
    return datasets;  
}
// Function to find outlier edges using a simple gradient method  
std::vector<std::pair<int, int>> cvLib::findOutlierEdges(const std::vector<std::vector<pubstructs::RGB>>& data, unsigned int gradientMagnitude_threshold) {
    std::vector<std::pair<int, int>> outliers;
    if (data.empty() || data[pubstructs::C_0].empty()) {
        return outliers;
    }
    unsigned int height = data.size();
    unsigned int width = data[pubstructs::C_0].size();
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
bool cvLib::saveImage(const std::vector<std::vector<pubstructs::RGB>>& data, const std::string& filename){ 
    if (data.empty() || data[pubstructs::C_0].empty()) {  
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
                const pubstructs::RGB& rgb = data.at(i).at(j);  // Use at() for safety  
                // Ensure pubstructs::RGB values are within valid range  
                if (rgb.r > 255 || rgb.g > 255 || rgb.b > 255) {  
                    throw std::runtime_error("RGB values must be in the range [0, 255]");  
                }  
                file << static_cast<uint8_t>(rgb.r) << " " << static_cast<uint8_t>(rgb.g) << " " << static_cast<uint8_t>(rgb.b) << "\n";  
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
void cvLib::markOutliers(std::vector<std::vector<pubstructs::RGB>>& data, const std::vector<std::pair<int, int>>& outliers, const cv::Scalar& markerColor) {
        if (data.empty() || data[pubstructs::C_0].empty()) {  
            return;  
        }  
        if (outliers.empty()) return;  

        // Ensure markerColor checks correctly correspond to RGB
        bool isEmpty = (markerColor[pubstructs::C_0] == 0 && markerColor[pubstructs::C_1] == 0 && markerColor[pubstructs::C_2] == 0);
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
                data[x][y] = {static_cast<uint8_t>(markerColor[pubstructs::C_0]), static_cast<uint8_t>(markerColor[pubstructs::C_1]), static_cast<uint8_t>(markerColor[pubstructs::C_2])};
            }
        }
        // [Optional] Use minX, minY, maxX, maxY for further bounding box logic
}
void cvLib::createOutlierImage(const std::vector<std::vector<pubstructs::RGB>>& originalData, const std::vector<std::pair<int, int>>& outliers, const std::string& outImgPath, const cv::Scalar& bgColor) {
    if (originalData.empty() || originalData[pubstructs::C_0].empty()) {
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
    cv::Mat outputImage(outputHeight, outputWidth, CV_8UC3, cv::Scalar(bgColor[pubstructs::C_0], bgColor[pubstructs::C_1], bgColor[pubstructs::C_2]));
    for (const auto& outlier : outliers) {  
        int outlierX = outlier.first;  
        int outlierY = outlier.second;  
        if (outlierX < originalData.size() && outlierY < originalData[pubstructs::C_0].size()) {  
            int newX = outlierY - minY;  
            int newY = outlierX - minX;  
            pubstructs::RGB pixel = originalData[outlierX][outlierY];  
            outputImage.at<cv::Vec3b>(newY, newX) = cv::Vec3b(pixel.b, pixel.g, pixel.r);
        }
    }  
    cv::imwrite(outImgPath, outputImage);
    // Example usage for saving the output or processing as expected within defined functions in cvLib:
    // this->savePPM(outputImage, outImgPath); 
    // this->saveImage(originalData, outImgPath + "_orig.ppm");
    std::cout << "Outlier image saved successfully." << std::endl;
}
std::vector<cv::Mat> cvLib::extractAndProcessObjects(const std::string& imagePath, unsigned int cannyThreshold1, unsigned int cannyThreshold2, unsigned int gradientMagnitude, unsigned int outputWidth, unsigned int outputHeight, double scaleFactor){
    // Initialize a vector to store processed objects
    std::vector<cv::Mat> objectImages;
    if(imagePath.empty()){
        return objectImages;
    }
    if (!std::filesystem::exists(imagePath) || !std::filesystem::is_regular_file(imagePath)) {
        std::cerr << "File path is invalid or file not found: " << imagePath << std::endl;
        return objectImages;
    }
    try{
        // Load the image
        cv::Mat originalImage = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (originalImage.empty()) {
            std::cerr << "cvLib::extractAndProcessObjects Error loading image!" << std::endl;
            return objectImages;
        }
        // Convert to grayscale using OpenCV
        cv::Mat grayImage;
        cv::cvtColor(originalImage, grayImage, cv::COLOR_BGR2GRAY);
        // Apply Gaussian Blur
        cv::Mat blurredImage;
        cv::GaussianBlur(grayImage, blurredImage, cv::Size(5, 5), 0);
        // Apply Canny Edge Detection
        cv::Mat edges;
        cv::Canny(blurredImage, edges, cannyThreshold1, cannyThreshold2);
        // Optionally enhance edges with morphology
        cv::Mat morphEdges;
        cv::morphologyEx(edges, morphEdges, cv::MORPH_CLOSE, cv::Mat(), cv::Point(-1, -1), 2);
        // Find contours on enhanced edges
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(morphEdges, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        // Optional: Uncomment and consider Sobel if needed for additional analysis
        /*
        cv::Mat sobelX, sobelY, sobel;
        cv::Sobel(blurredImage, sobelX, CV_16S, 1, 0, 3);
        cv::Sobel(blurredImage, sobelY, CV_16S, 0, 1, 3);
        cv::convertScaleAbs(sobelX, sobelX);
        cv::convertScaleAbs(sobelY, sobelY);
        cv::addWeighted(sobelX, 0.5, sobelY, 0.5, 0, sobel);
        */
        //Dimensions for the processing
        const int resizedWidth = static_cast<int>(outputWidth * scaleFactor);
        const int resizedHeight = static_cast<int>(outputHeight * scaleFactor);
        // Iterate through contours and store each processed object in the vector
        for (const auto& contour : contours) {
            // Ignore small contours that might be noise
            if (cv::contourArea(contour) > 100) {  // Tune this value based on your needs
                cv::Rect boundingRect = cv::boundingRect(contour);
                // Extract the object from the original image using the bounding rectangle
                cv::Mat objectImage = originalImage(boundingRect);
                // Resize the extracted object
                cv::Mat resizedObject;
                cv::resize(objectImage, resizedObject, cv::Size(resizedWidth, resizedHeight));
                // Create a white background image
                cv::Mat background(outputHeight, outputWidth, CV_8UC3, cv::Scalar(255, 255, 255));
                // Calculate the position to center the resized object
                int startX = (outputWidth - resizedWidth) / 2;
                int startY = (outputHeight - resizedHeight) / 2;
                // Place the resized object onto the center of the white background
                resizedObject.copyTo(background(cv::Rect(startX, startY, resizedWidth, resizedHeight)));
                // Add the processed image to the vector
                objectImages.push_back(objectImage);
            }
        }
    } 
    catch (const cv::Exception& e) 
    {  
        std::cerr << "OpenCV error: " << e.what() << std::endl;  
    } catch (const std::exception& e) {  
        std::cerr << "Standard exception: " << e.what() << std::endl;  
    } catch (...) {  
        std::cerr << "Unknown exception occurred." << std::endl;  
    }  
    return objectImages;
}
void cvLib::read_image_detect_edges(const std::string& imagePath,unsigned int gradientMagnitude_threshold,const std::string& outImgPath,const brushColor& markerColor, const brushColor& bgColor){
    if(imagePath.empty()){
        return;
    }
    std::vector<pubstructs::RGB> pixelToPaint;
    imgSize img_size = this->get_image_size(imagePath);
    std::vector<std::vector<pubstructs::RGB>> image_rgb = this->get_img_matrix(imagePath,img_size.width,img_size.height,pubstructs::inputImgMode::Color);  
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
void cvLib::convertToBlackAndWhite(cv::Mat& image, std::vector<std::vector<pubstructs::RGB>>& datasets) {
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
                // Access the pixel and its pubstructs::RGB values
                cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);
                uint8_t r = pixel[pubstructs::C_2];
                uint8_t g = pixel[pubstructs::C_1];
                uint8_t b = pixel[pubstructs::C_0];
                // Calculate the grayscale value using the luminance method
                uint8_t grayValue = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
                // Determine the binary value with thresholding
                uint8_t bwValue = (grayValue < 128) ? 0 : 255;
                // Assign the calculated binary value to the image
                pixel[pubstructs::C_0] = bwValue;
                pixel[pubstructs::C_1] = bwValue;
                pixel[pubstructs::C_2] = bwValue;
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
        if (knnMatches[i].size() > 1 && knnMatches[i][pubstructs::C_0].distance < ratioThresh * knnMatches[i][pubstructs::C_1].distance) {  
            goodMatches.push_back(knnMatches[i][pubstructs::C_0]);  
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
    std::vector<std::vector<pubstructs::RGB>> dataset_img1(m_img1.rows, std::vector<pubstructs::RGB>(m_img1.cols));  
    std::vector<std::vector<pubstructs::RGB>> dataset_img2(m_img2.rows, std::vector<pubstructs::RGB>(m_img2.cols));  
    subf_j.convertToBlackAndWhite(m_img1, dataset_img1);
    subf_j.convertToBlackAndWhite(m_img2, dataset_img2);
    cv::Mat gray1 = subf_j.convertDatasetToMat(dataset_img1);
    cv::Mat gray2 = subf_j.convertDatasetToMat(dataset_img2);
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
std::vector<std::vector<pubstructs::RGB>> cvLib::objectsInImage(const std::string& imgPath, unsigned int gradientMagnitude_threshold, const pubstructs::inputImgMode& out_mode) {  
    if (imgPath.empty()) {
        std::cerr << "Error: Image path is empty." << std::endl;
        return {};
    }
    imgSize img_size = this->get_image_size(imgPath);  
    std::vector<std::vector<pubstructs::RGB>> image_rgb;
    if(out_mode == pubstructs::inputImgMode::Color){
        image_rgb = this->get_img_matrix(imgPath, img_size.height, img_size.width, pubstructs::inputImgMode::Color);  
    }
    else if(out_mode == pubstructs::inputImgMode::Gray){
        image_rgb = this->get_img_matrix(imgPath, img_size.height, img_size.width, pubstructs::inputImgMode::Gray);
    }
    if (!image_rgb.empty()) {
        auto outliers = this->findOutlierEdges(image_rgb, gradientMagnitude_threshold);
        return subf_j.getPixelsInsideObject(image_rgb, outliers);
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
    while (true) {  
        // Capture a new frame from the webcam  
        cap >> frame; // Alternatively, you can use cap.read(frame);  
        // Check if the frame is empty  
        if (frame.empty()) break;
        /*
            start marking on the input frame
        */
        //cv::Scalar bgColor(0,0,0);
        // subf_j.markVideo(frame,brush_color,bgColor);
        // cv::Mat oframe = subf_j.getObjectsInVideo(frame);
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
cv::Mat cvLib::preprocessImage(const std::string& imgPath, const pubstructs::inputImgMode& img_mode, const unsigned int gradientMagnitude_threshold) {
    if(imgPath.empty()){
        return cv::Mat();
    }
    // // Step 1: Open the image using cv::imread
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "preprocessImage Error: Could not open or find the image." << std::endl;
        return cv::Mat();
    }
    try{
        // Step 2: Resize the image to 120x120 pixels
        //cv::Mat preProImg = this->convert_to_256_8bit(image);
        // Apply Gaussian blur to minimize noise
        cv::Mat resizedImage;
        cv::resize(image, resizedImage, cv::Size(800, 800));
        cv::GaussianBlur(resizedImage, resizedImage, cv::Size(5, 5), 0);
        std::vector<std::vector<pubstructs::RGB>> datasets;
        if(img_mode == pubstructs::inputImgMode::Color){
            datasets = this->cv_mat_to_dataset_color(resizedImage);
        }
        else if(img_mode == pubstructs::inputImgMode::Gray){
            cv::Mat gray_image;
            cv::cvtColor(resizedImage, gray_image, cv::COLOR_BGR2GRAY);
            datasets = this->cv_mat_to_dataset(gray_image);
        }
        auto outliers = this->findOutlierEdges(datasets, gradientMagnitude_threshold);
        std::vector<std::vector<pubstructs::RGB>> trans_img = subf_j.getPixelsInsideObject(datasets, outliers);
        cv::Mat final_image = subf_j.convertDatasetToMat(trans_img);//trans_img
        return final_image;
    }
    catch(const std::exception& ex){
        std::cerr << "cvLib::preprocessImage " << ex.what() << std::endl;
    }
    return cv::Mat();
}
std::vector<std::vector<pubstructs::RGB>> cvLib::get_img_120_gray_for_ML(const std::string& imgPath,const unsigned int gradientMagnitude_threshold) {  
    if(imgPath.empty()){
        return {};
    }
    std::vector<std::vector<pubstructs::RGB>> datasets;  
    cv::Mat image = this->preprocessImage(imgPath,pubstructs::inputImgMode::Gray,gradientMagnitude_threshold); 
    if (image.empty()) {  
        std::cerr << "get_img_120_gray_for_ML Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image,gray_image;
    cv::resize(image, resized_image, cv::Size(800, 800));
    cv::cvtColor(resized_image, gray_image, cv::COLOR_BGR2GRAY);
    // Assume cv_mat_to_dataset_color(resized_image) is implemented correctly
    datasets = this->cv_mat_to_dataset(gray_image);
    return datasets;  
}
std::vector<cv::KeyPoint> cvLib::extractORBFeatures(const cv::Mat& img, cv::Mat& descriptors) const{
    cv::Ptr<cv::ORB> orb = cv::ORB::create();
    std::vector<cv::KeyPoint> keypoints;
    orb->detectAndCompute(img, cv::noArray(), keypoints, descriptors);
    return keypoints;
}
std::vector<cv::KeyPoint> cvLib::extractORBFeatures_multi(const cv::Mat& img, cv::Mat& descriptors, int nFea) const{
    cv::Ptr<cv::ORB> orb = cv::ORB::create(nFea);
    std::vector<cv::KeyPoint> keypoints;
    orb->detectAndCompute(img, cv::noArray(), keypoints, descriptors);
    return keypoints;
}
std::vector<std::vector<cv::KeyPoint>> cvLib::clusterKeypoints(const std::vector<cv::KeyPoint>& keypoints, int k) const{
    // Prepare data for clustering
    cv::Mat data(keypoints.size(), 2, CV_32F);
    for (size_t i = 0; i < keypoints.size(); ++i) {
        data.at<float>(i, 0) = keypoints[i].pt.x;
        data.at<float>(i, 1) = keypoints[i].pt.y;
    }
    // Run k-means clustering
    cv::Mat labels;
    cv::Mat centers;
    cv::kmeans(data, k, labels,
               cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
               3, cv::KMEANS_PP_CENTERS, centers);
    // Prepare vector of clusters
    std::vector<std::vector<cv::KeyPoint>> clusters(k);
    for (size_t i = 0; i < keypoints.size(); ++i) {
        int clusterIndex = labels.at<int>(i);
        clusters[clusterIndex].push_back(keypoints[i]);
    }
    return clusters;
}
void cvLib::save_keymap(const std::unordered_map<std::string, std::vector<pubstructs::RGB>>& dataMap, const std::string& filePath) {
    if (filePath.empty()) {
        return;
    }
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Error: Unable to open file for writing.");
    }
    try {
        // Serialize the map
        size_t mapSize = dataMap.size();
        ofs.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
        for (const auto& [key, vec] : dataMap) {
            size_t keySize = key.size();
            ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
            ofs.write(key.c_str(), keySize);
            size_t vecSize = vec.size();
            ofs.write(reinterpret_cast<const char*>(&vecSize), sizeof(vecSize));
            ofs.write(reinterpret_cast<const char*>(vec.data()), vecSize * sizeof(pubstructs::RGB));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error writing to file: " << e.what() << std::endl;
    }
    ofs.close();
}
void cvLib::load_keymap(const std::string& filePath, std::unordered_map<std::string, std::vector<pubstructs::RGB>>& dataMap) {
    if (filePath.empty()) {
        return;
    }
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open()) {
        throw std::runtime_error("Error: Unable to open file for reading.");
    }
    try {
        // Deserialize the map
        size_t mapSize;
        ifs.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
        for (size_t i = 0; i < mapSize; ++i) {
            size_t keySize;
            ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
            std::string key(keySize, '\0');
            ifs.read(&key[pubstructs::C_0], keySize);
            size_t vecSize;
            ifs.read(reinterpret_cast<char*>(&vecSize), sizeof(vecSize));
            std::vector<pubstructs::RGB> vec(vecSize);
            ifs.read(reinterpret_cast<char*>(vec.data()), vecSize * sizeof(pubstructs::RGB));
            dataMap[key] = vec;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading from file: " << e.what() << std::endl;
    }
    ifs.close();
}
void cvLib::train_img_occurrences(const std::string& images_folder_path, const double learning_rate, const std::string& model_output_path, const std::string& model_output_key_path, const std::string& model_output_map_path,const unsigned int gradientMagnitude_threshold, const pubstructs::inputImgMode& img_mode){
    std::unordered_map<std::string, std::vector<cv::Mat>> dataset; 
    std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>> dataset_keypoint;  
    std::unordered_map<std::string, std::vector<pubstructs::RGB>> dataMap;
    if (images_folder_path.empty()) {  
        return;  
    }  
    try {  
        for (const auto& entryMainFolder : std::filesystem::directory_iterator(images_folder_path)) {  
            if (entryMainFolder.is_directory()) { // Check if the entry is a directory  
                std::string sub_folder_name = entryMainFolder.path().filename().string();  
                std::string sub_folder_path = entryMainFolder.path().string();  
                std::vector<cv::Mat> sub_folder_all_images;  
                std::vector<std::vector<cv::KeyPoint>> sub_folder_keypoints;
                std::vector<pubstructs::RGB> img_keymap;
                // Accumulate pixel count for memory reservation  
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file()) {  
                        std::string imgFilePath = entrySubFolder.path().string(); 
                        cv::Mat get_img;
                        if(img_mode == pubstructs::inputImgMode::Gray){ 
                            get_img = this->preprocessImage(imgFilePath,pubstructs::inputImgMode::Gray,gradientMagnitude_threshold);
                        }
                        else{
                            get_img = this->preprocessImage(imgFilePath,pubstructs::inputImgMode::Color,gradientMagnitude_threshold);
                        }
                        if(!get_img.empty()){
                            cv::Mat descriptors;
                            std::vector<cv::KeyPoint> sub_key = this->extractORBFeatures(get_img,descriptors);
                            sub_folder_all_images.push_back(descriptors);
                            sub_folder_keypoints.push_back(sub_key);
                            /*
                                Output model data
                            */
                            if(!sub_key.empty()){
                                double total_record = static_cast<double>(sub_key.size());
                                unsigned int learning_count = 0;
                                double no_data_to_save = learning_rate * total_record;
                                unsigned int learning_no = static_cast<unsigned int> (no_data_to_save);
                                std::vector<std::pair<std::vector<unsigned int>,double>> count_response;
                                for(unsigned int i = 0; i < sub_key.size(); ++i){
                                    const cv::KeyPoint& kp = sub_key[i];
                                    std::vector<unsigned int> point_x_y{
                                        static_cast<unsigned int>(kp.pt.x),
                                        static_cast<unsigned int>(kp.pt.y)
                                    };
                                    count_response.push_back(std::make_pair(point_x_y,static_cast<double>(kp.response)));
                                }
                                if(!count_response.empty()){
                                    std::sort(count_response.begin(),count_response.end(),[](const auto& a, const auto& b){
                                        return a.second > b.second;
                                    });
                                }
                                else{
                                    std::cerr << sub_folder_name << " error reading the image." << std::endl;
                                    continue;
                                }
                                for(const auto& item : count_response){
                                    learning_count++;
                                    if(learning_count > learning_no){
                                        break;//reach the limit
                                    }
                                    std::pair<std::vector<unsigned int>,double> printItem = item;
                                    //printItem.first[0],printItem.first[1]
                                    unsigned int x = printItem.first[0];
                                    unsigned int y = printItem.first[1];
                                    // Ensure the coordinates are within the image bounds
                                    if (x < static_cast<unsigned int>(get_img.cols) && y < static_cast<unsigned int>(get_img.rows)) {
                                        // Get the pubstructs::RGB pixel value at (x, y)
                                        cv::Vec3b pixel = get_img.at<cv::Vec3b>(cv::Point(x, y));
                                        uint8_t blue = pixel[pubstructs::C_0];
                                        uint8_t green = pixel[pubstructs::C_1];
                                        uint8_t red = pixel[pubstructs::C_2];
                                        // Create an pubstructs::RGB object and add it to the vector
                                        pubstructs::RGB color(red, green, blue);
                                        img_keymap.push_back(color);
                                    } else {
                                        std::cerr << "Coordinate (" << x << ", " << y << ") is out of bounds" << std::endl;
                                    }
                                }
                            }
                        }
                        else{
                            std::cout << imgFilePath << " (get_img is empty)!" << std::endl;
                        } 
                    }  
                }  
                dataset[sub_folder_name] = sub_folder_all_images;
                dataset_keypoint[sub_folder_name] = sub_folder_keypoints;
                dataMap[sub_folder_name] = img_keymap;
                std::cout << sub_folder_name << " is done!" << std::endl;  
            }  
        }  
    } 
    catch (const std::filesystem::filesystem_error& e) {  
        std::cerr << "Filesystem error: " << e.what() << std::endl;  
        return; // Return an empty dataset in case of filesystem error  
    }  
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
    subf_j.saveModel(dataset, model_output_path); 
    subf_j.saveModel_keypoint(dataset_keypoint,model_output_key_path); 
    this->save_keymap(dataMap,model_output_map_path);
    std::cout << "Successfully saved the images into the dataset, all jobs are done!" << std::endl;  
}
//cvl_j.loadImageRecog("/Users/dengfengji/ronnieji/lib/project/main/model_keymap.dat",99,true,3,0.05);
void cvLib::loadImageRecog(const std::string& keymap_path,const unsigned int gradientMagnitude_threshold, 
const bool display_time_spend,const unsigned int dis_bias, double learningrate, double percent_check){
    if(keymap_path.empty()){
        return;
    }
    this->set_display_time(display_time_spend);
    this->set_gradientMagnitude_threshold(gradientMagnitude_threshold);
    this-> set_distance_bias(dis_bias);
    std::unordered_map<std::string, std::vector<pubstructs::RGB>> keymap;
    this->load_keymap(keymap_path,keymap);
    this->set_loaddataMap(keymap);
    this->set_learing_rate(learningrate);
    this->set_percent_to_check(percent_check);
}
cvLib::the_obj_in_an_image cvLib::what_is_this(const std::string& img_path, const cvLib::mark_font_info& maker_info){
    cvLib::the_obj_in_an_image str_result;
    if(img_path.empty()){
        return str_result;
    }
    cv::Mat getImg = this->preprocessImage(img_path,pubstructs::inputImgMode::Gray,_gradientMagnitude_threshold);
    if(!getImg.empty()){
        str_result = subf_j.getObj_in_an_image(*this,getImg,maker_info);
    }
    return str_result;
}
void cvLib::save_trained_model(
    const std::unordered_map<std::string, cv::Mat>& summarizedDataset, 
    const std::unordered_map<std::string, std::vector<cv::KeyPoint>>& summarizedKeypoints,
    const std::string& model_file_path) {
    std::ofstream ofs(model_file_path, std::ios::binary);
    if (!ofs.is_open()) {
        throw std::runtime_error("Error: Unable to open file for writing.");
    }
    // Serialize the summarized dataset
    size_t datasetSize = summarizedDataset.size();
    ofs.write(reinterpret_cast<const char*>(&datasetSize), sizeof(datasetSize));
    for (const auto& [category, mat] : summarizedDataset) {
        size_t keySize = category.size();
        ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
        ofs.write(category.c_str(), keySize);
        int rows = mat.rows;
        int cols = mat.cols;
        int type = mat.type();
        ofs.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        ofs.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
        ofs.write(reinterpret_cast<const char*>(&type), sizeof(type));
        size_t dataSize = mat.elemSize() * rows * cols;
        ofs.write(reinterpret_cast<const char*>(mat.data), dataSize);
    }
    // Serialize the summarized keypoints
    size_t keypointsSize = summarizedKeypoints.size();
    ofs.write(reinterpret_cast<const char*>(&keypointsSize), sizeof(keypointsSize));
    for (const auto& [category, keypoints] : summarizedKeypoints) {
        size_t keySize = category.size();
        ofs.write(reinterpret_cast<const char*>(&keySize), sizeof(keySize));
        ofs.write(category.c_str(), keySize);
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
    ofs.close();
}
void cvLib::machine_learning_result(
    const unsigned int clusterNo,
    const std::unordered_map<std::string, std::vector<cv::Mat>>& dataset,
    const std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>>& dataset_keypoint,
    std::unordered_map<std::string, cv::Mat>& summarizedDataset,
    std::unordered_map<std::string, std::vector<cv::KeyPoint>>& summarizedKeypoints,
    const std::string& model_file_path) {
    //const int K = clusterNo;  // Increased number of clusters for better precision in complex datasets
    for (const auto& [category, descriptors] : dataset) {
        if (descriptors.empty()) continue;
        cv::Mat allDescriptors;
        for (const auto& descriptor : descriptors) {
            cv::Mat floatDescriptor;
            descriptor.convertTo(floatDescriptor, CV_32F);
            allDescriptors.push_back(floatDescriptor);
        }
        if (!allDescriptors.empty()) {
            cv::Mat labels, centers;
            cv::kmeans(allDescriptors, clusterNo, labels, 
                cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 2000, 0.1),//150,0.1
                20, cv::KMEANS_PP_CENTERS, centers);//10
            summarizedDataset[category] = centers;
        }
    }
    for (const auto& [category, keypointSets] : dataset_keypoint) {
        if (keypointSets.empty()) continue;
        std::vector<cv::Point2f> allPoints;
        for (const auto& keypoints : keypointSets) {
            for (const auto& kp : keypoints) {
                allPoints.push_back(kp.pt);
            }
        }
        if (!allPoints.empty()) {
            cv::Mat allPointsMat(static_cast<int>(allPoints.size()), 2, CV_32F);
            for (size_t i = 0; i < allPoints.size(); ++i) {
                allPointsMat.at<float>(static_cast<int>(i), 0) = allPoints[i].x;
                allPointsMat.at<float>(static_cast<int>(i), 1) = allPoints[i].y;
            }
            cv::Mat labels, centers;
            cv::kmeans(allPointsMat, clusterNo, labels,
                cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 2000, 0.1),//150,0.1
                20, cv::KMEANS_PP_CENTERS, centers);//10
            std::vector<cv::KeyPoint> clusteredKeypoints;
            for (int i = 0; i < centers.rows; ++i) {
                cv::KeyPoint kp;
                kp.pt.x = centers.at<float>(i, 0);
                kp.pt.y = centers.at<float>(i, 1);
                clusteredKeypoints.push_back(kp);
            }
            summarizedKeypoints[category] = clusteredKeypoints;
        }
    }
    this->save_trained_model(summarizedDataset, summarizedKeypoints, model_file_path);
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
        ifs.read(&className[pubstructs::C_0], keySize);
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
void cvLib::loadModel_keypoint(std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>>& featureMap, const std::string& filename) {
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
            std::string className(keySize, '\0');  // Ensure string is properly initialized
            ifs.read(&className[pubstructs::C_0], keySize);

            size_t setCount;
            ifs.read(reinterpret_cast<char*>(&setCount), sizeof(setCount));
            std::vector<std::vector<cv::KeyPoint>> keypointSets(setCount);
            for (size_t j = 0; j < setCount; ++j) {
                size_t keypointCount;
                ifs.read(reinterpret_cast<char*>(&keypointCount), sizeof(keypointCount));
                std::vector<cv::KeyPoint> keypoints(keypointCount);
                for (size_t k = 0; k < keypointCount; ++k) {
                    cv::KeyPoint kp;
                    ifs.read(reinterpret_cast<char*>(&kp.pt.x), sizeof(kp.pt.x));
                    ifs.read(reinterpret_cast<char*>(&kp.pt.y), sizeof(kp.pt.y));
                    ifs.read(reinterpret_cast<char*>(&kp.size), sizeof(kp.size));
                    ifs.read(reinterpret_cast<char*>(&kp.angle), sizeof(kp.angle));
                    ifs.read(reinterpret_cast<char*>(&kp.response), sizeof(kp.response));
                    ifs.read(reinterpret_cast<char*>(&kp.octave), sizeof(kp.octave));
                    ifs.read(reinterpret_cast<char*>(&kp.class_id), sizeof(kp.class_id));
                    keypoints[k] = kp;
                }
                keypointSets[j] = keypoints;
            }
            featureMap[className] = keypointSets;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error reading from file: " << e.what() << std::endl;
    }
    ifs.close();
}
void cvLib::load_trained_model(
        const std::string& filename,
        std::unordered_map<std::string, cv::Mat>& summarizedDataset,
        std::unordered_map<std::string, std::vector<cv::KeyPoint>>& summarizedKeypoints){
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("Error: Unable to open file for reading.");
        }
        // Load summarizedDataset
        size_t datasetSize;
        ifs.read(reinterpret_cast<char*>(&datasetSize), sizeof(datasetSize));
        for (size_t i = 0; i < datasetSize; ++i) {
            size_t keySize;
            ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
            std::string category(keySize, '\0');
            ifs.read(&category[pubstructs::C_0], keySize);
            int rows, cols, type;
            ifs.read(reinterpret_cast<char*>(&rows), sizeof(rows));
            ifs.read(reinterpret_cast<char*>(&cols), sizeof(cols));
            ifs.read(reinterpret_cast<char*>(&type), sizeof(type));
            cv::Mat mat(rows, cols, type);
            ifs.read(reinterpret_cast<char*>(mat.data), mat.elemSize() * rows * cols);
            summarizedDataset[category] = mat;
        }
        // Load summarizedKeypoints
        size_t keypointsSize;
        ifs.read(reinterpret_cast<char*>(&keypointsSize), sizeof(keypointsSize));
        for (size_t i = 0; i < keypointsSize; ++i) {
            size_t keySize;
            ifs.read(reinterpret_cast<char*>(&keySize), sizeof(keySize));
            std::string category(keySize, '\0');
            ifs.read(&category[pubstructs::C_0], keySize);
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
            summarizedKeypoints[category] = keypoints;
        }
        ifs.close();
}
//const std::string& img1, const std::string& img2, const img2info& img2_info, const std::string& output_img,const unsigned int in_img_width, const unsigned int in_img_height, const unsigned int out_img_width, const unsigned int out_img_height
cv::Mat cvLib::put_img2_in_img1(
    const std::string& image1, 
    const std::string& image2, 
    const int img2Width,
    const int img2Height,
    const int img2Left,
    const int img2Top,
    const uint8_t img2alpha, 
    const std::string& output_image_path,
    const unsigned int inImg_width, 
    const unsigned int inImg_height, 
    const unsigned int outImg_width, 
    const unsigned int outImg_height, 
    unsigned int inDepth){
    if (image1.empty() || image2.empty() || output_image_path.empty()) {
        return cv::Mat();
    }
    try{
        cv::Mat getMergedImg = subsd_j.put_img2_in_img1(
            *this,
            image1,
            image2,
            img2Width,
            img2Height,
            img2Left,
            img2Top,
            img2alpha,
            output_image_path,
            inImg_width,
            inImg_height,
            outImg_width,
            outImg_height,
            inDepth
        );
        return getMergedImg;
    }
    catch (const cv::Exception& e) {  
        std::cerr << "OpenCV error: " << e.what() << std::endl;  
    } catch (const std::exception& e) {  
        std::cerr << "Standard exception: " << e.what() << std::endl;  
    } catch (...) {  
        std::cerr << "Unknown exception occurred." << std::endl;  
    }  
    return cv::Mat();
}
std::vector<cvLib::the_obj_in_an_image> cvLib::what_are_these(const std::string& img_path, const cvLib::mark_font_info& markerInfo){
    // /*
    //     preprocess image
    // */
    // std::vector<cvLib::the_obj_in_an_image> str_result;
    // if(img_path.empty()){
    //     return str_result;
    // }
    // std::chrono::time_point<std::chrono::high_resolution_clock> t_count_start;
    // std::chrono::time_point<std::chrono::high_resolution_clock> t_count_end;
    // std::unordered_map<std::string, std::vector<pubstructs::RGB>> _loaddataMap;
    // unsigned int _gradientMagnitude_threshold = 33; 
    // bool display_time = false;
    // unsigned int distance_bias = 2;
    // display_time = this->get_display_time();
    // distance_bias = this->get_distance_bias();
    // _gradientMagnitude_threshold = this->get_gradientMagnitude_threshold();
    // _loaddataMap = this->get_loaddataMap();
    // double percent_check_sub = this->get_percent_to_check();
    // if(!img_path.empty()){
    //     /*
    //         start
    //     */
    //     // // Step 1: Open the image using cv::imread
    //     cv::Mat image_ori = cv::imread(img_path);
    //     if(!image_ori.empty()){
    //         // Use Canny to detect edges
    //         cv::Mat edges;
    //         subfunctions subf_j;
    //         subf_j.automaticCanny(image_ori,edges,0.05);//0.33
    //         // Find contours
    //         std::vector<std::vector<cv::Point>> contours;
    //         std::vector<cv::Vec4i> hierarchy;
    //         cv::findContours(edges, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
    //         // // Create an output image to draw contours
    //         // cv::Mat contourImage = cv::Mat::zeros(final_image.size(), CV_8UC3);
    //         // // Draw each contour with a random color
    //         // for (size_t i = 0; i < contours.size(); ++i) {
    //         //     cv::Scalar color = cv::Scalar(rand() % 256, rand() % 256, rand() % 256);
    //         //     cv::drawContours(contourImage, contours, static_cast<int>(i), color, 2, cv::LINE_8, hierarchy, 0);
    //         // }
    //         // Extract contours as keypoints
    //         std::vector<cv::Mat> get_all_contours_in_the_image = subf_j.drawEachContourOnSeparateWhiteBackground(contours);
    //         if(!get_all_contours_in_the_image.empty()){

    //         }
    //         std::unordered_map<std::string,std::vector<cv::KeyPoint>> da;
    //         cv::Mat final_image = this->preprocessImage(img_path,pubstructs::inputImgMode::Gray,this->_gradientMagnitude_threshold);
    //         da = this->extractContoursAsKeyPoints(edges);
    //         if(!da.empty()){
    //             for(const auto& item : da){
    //                 /*
    //                     start recognite
    //                     searching for objs in the image
    //                 */
    //                 if(display_time){
    //                     t_count_start = std::chrono::high_resolution_clock::now(); // Initialize start time 
    //                 }
    //                 std::vector<cv::KeyPoint> testKey = item.second;
    //                 std::vector<std::pair<cvLib::the_obj_in_an_image,double>> test_img_data;
    //                 cvLib::the_obj_in_an_image temp_obj;//one obj
    //                 for (size_t i = 0; i < testKey.size(); ++i) {
    //                     const cv::KeyPoint& kp = testKey[i]; 
    //                     unsigned int x = static_cast<unsigned int>(kp.pt.x);
    //                     unsigned int y = static_cast<unsigned int>(kp.pt.y);
    //                     // Ensure the coordinates are within the image bounds
    //                     //if (x < static_cast<unsigned int>(final_image.cols) && y < static_cast<unsigned int>(final_image.rows)) {
    //                     if (x < 800 && y < 800) {
    //                         // Get the pubstructs::RGB pixel value at (x, y)
    //                         cv::Vec3b pixel = final_image.at<cv::Vec3b>(cv::Point(x, y));
    //                         uint8_t blue = pixel[pubstructs::C_0];
    //                         uint8_t green = pixel[pubstructs::C_1];
    //                         uint8_t red = pixel[pubstructs::C_2];
    //                         // Create an pubstructs::RGB object and add it to the vector
    //                         /*
    //                             start debug code
    //                         */
    //                         //pubstructs::RGB color(red, green, blue);
    //                         /*
    //                             end debug code
    //                         */
    //                         pubstructs::RGB color(0, 0, 0);
    //                         temp_obj.rgb = color;
    //                         temp_obj.fontface = markerInfo.fontface;
    //                         temp_obj.fontScale = markerInfo.fontScale;
    //                         temp_obj.thickness = markerInfo.thickness;
    //                         temp_obj.fontcolor = markerInfo.fontcolor;
    //                         temp_obj.text_position = markerInfo.text_position;
    //                         temp_obj.x = static_cast<int>(kp.pt.x);
    //                         temp_obj.y = static_cast<int>(kp.pt.y);
    //                         temp_obj.rec_topLeft = cv::Point(kp.pt.x - kp.size / 2, kp.pt.y - kp.size / 2);
    //                         temp_obj.rec_bottomRight = cv::Point(kp.pt.x + kp.size / 2, kp.pt.y + kp.size / 2);
    //                         test_img_data.push_back(std::make_pair(temp_obj,static_cast<double>(kp.response)));
    //                     } else {
    //                         std::cerr << "Coordinate (" << x << ", " << y << ") is out of bounds" << std::endl;
    //                     }
    //                 }
    //                 if(!test_img_data.empty()){
    //                     std::sort(test_img_data.begin(),test_img_data.end(),[](const auto& a, const auto& b){
    //                         return a.second > b.second;
    //                     });
    //                     /*
    //                         start debug code
    //                     */
    //                     for(const auto& item : test_img_data){
    //                         cvLib::the_obj_in_an_image obj_i = item.first;
    //                         std::cout << obj_i.x << "  " << obj_i.y << std::endl;
    //                     }
    //                     /*
    //                         end   debug code
    //                     */
    //                     subf_j.visual_recognize_obj(test_img_data,percent_check_sub,_loaddataMap,temp_obj);
    //                     std::cout << "Obj detect: " << temp_obj.objName << std::endl;
    //                 }
    //                 else{
    //                     std::cerr << "cvLib::what_are_these(line: 2537) test_img_data is empty!" << std::endl;
    //                 }
    //                 if(display_time){
    //                     t_count_end = std::chrono::high_resolution_clock::now();   
    //                     std::chrono::duration<double> duration = t_count_end - t_count_start;  
    //                     //std::cout << "Execution time: " << duration.count() << " seconds\n"; 
    //                     temp_obj.timespent = duration;
    //                 }
    //                 /*
    //                     one obj is done
    //                 */
    //                 /*
    //                     add to the collections
    //                 */
    //                 str_result.push_back(temp_obj);//it's not this simple
    //             }
    //         }
    //         else{
    //             std::cerr << "cvLib::what_are_these da is empty!" << std::endl;
    //         }
    //     }
    //     else{
    //         std::cerr << "The final image is empty!" << std::endl;
    //     }
    // }
    // return str_result;
}