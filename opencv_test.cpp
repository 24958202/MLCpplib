/*
g++ -std=c++20 /Users/dengfengji/ronnieji/lib/project/main/opencv_test.cpp -o /Users/dengfengji/ronnieji/lib/project/main/opencv_test -I/Users/dengfengji/ronnieji/lib/project/include -I/Users/dengfengji/ronnieji/lib/project/src /Users/dengfengji/ronnieji/lib/project/src/*.cpp -I/opt/homebrew/Cellar/tesseract/5.4.1/include -L/opt/homebrew/Cellar/tesseract/5.4.1/lib -ltesseract -I/opt/homebrew/Cellar/opencv/4.10.0_6/include/opencv4 -L/opt/homebrew/Cellar/opencv/4.10.0_6/lib -Wl,-rpath,/opt/homebrew/Cellar/opencv/4.10.0_6/lib -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_photo -lopencv_features2d -lopencv_imgproc -lopencv_imgcodecs -lopencv_calib3d -lopencv_video -I/opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3 -I/opt/homebrew/Cellar/boost/1.86.0/include -I/opt/homebrew/Cellar/icu4c/74.2/include -L/opt/homebrew/Cellar/icu4c/74.2/lib -licuuc -licudata /opt/homebrew/Cellar/boost/1.86.0/lib/libboost_system.a /opt/homebrew/Cellar/boost/1.86.0/lib/libboost_filesystem.a
*/
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp> 
#include <map>
#include <unordered_map>
#include <stdint.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include "cvLib.h"

void trainImage(){
    cvLib cvl_j;
    std::cout << "Training images..." << std::endl;
    cvl_j.train_img_occurrences("/Users/dengfengji/ronnieji/Kaggle/archive-2/train",
    0.05,//0.05(learning rate)
    "/Users/dengfengji/ronnieji/lib/project/main/data.dat",
    "/Users/dengfengji/ronnieji/lib/project/main/data_key.dat",
    "/Users/dengfengji/ronnieji/lib/project/main/model_keymap.dat",
    99,
    inputImgMode::Gray
    );
    std::cout << "Successfully loaded test images, start recognizing..." << std::endl;
}
void test_image_recognition(){
    std::vector<std::string> testimgs;
    std::string sub_folder_path = "/Users/dengfengji/ronnieji/Kaggle/test";
    for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
        if (entrySubFolder.is_regular_file()) {  
            std::string imgFilePath = entrySubFolder.path().string();  
            testimgs.push_back(imgFilePath);
        }
    }
    /*
        para1: input the train model file from function train_img_occurrences para5: output model_keymap file path/output_map.dat
        para2: //gradientMagnitude_threshold gradientMagnitude threshold 0-100, better result with small digits, but takes longer (default: 33)
        para3: bool = true (display time spent)
        para4: distance allow bias from the trained data default = 2;(better result with small digits)
    */
    cvLib cvl_j;
    cvl_j.loadImageRecog("/Users/dengfengji/ronnieji/lib/project/main/model_keymap.dat",99,true,3,0.05);
    for(const auto& item : testimgs){
        the_obj_in_an_image strReturn = cvl_j.what_is_this(item);
        if(!strReturn.empty()){
            bool display_time = cvl_j.get_display_time();
            std::cout << item << " is a(an) " << strReturn.objName << std::endl;
            if(display_time){
                std::cout << "Execution time: " << strReturn.timespent << " seconds\n";
            }
        }
    }
}

void multi_objs_readImgs(){
    cvLib cvl_j;
    std::vector<std::string> testimgs;
    std::string sub_folder_path = "/Users/dengfengji/ronnieji/Kaggle/test";
    for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
        if (entrySubFolder.is_regular_file()) {  
            std::string imgFilePath = entrySubFolder.path().string();  
            testimgs.push_back(imgFilePath);
        }
    }
    if(!testimgs.empty()){
        std::cout << "test images number: " << testimgs.size() << std::endl;
        for(const auto& item : testimgs){
            if(std::filesystem::is_regular_file(item)){
                std::vector<cv::Mat> getObjs =  cvl_j.extractAndProcessObjects(item);
                if(!getObjs.empty()){
                    for(int i = 0; i < getObjs.size(); ++i){
                        std::string outputF = "/Users/dengfengji/ronnieji/lib/images/segments/img" + std::to_string(i) + ".jpg";
                        cv::imwrite(outputF.c_str(),getObjs[i]);
                    }
                }
            }
            
        }
       
    }
}
int main(){
    trainImage();
    test_image_recognition();
    //multi_objs_readImgs();
    return 0;
}