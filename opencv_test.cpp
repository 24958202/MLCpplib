/*
g++ -std=c++20 /Users/dengfengji/ronnieji/lib/project/main/opencv_test.cpp -o /Users/dengfengji/ronnieji/lib/project/main/opencv_test -I/Users/dengfengji/ronnieji/lib/project/include -I/Users/dengfengji/ronnieji/lib/project/src /Users/dengfengji/ronnieji/lib/project/src/*.cpp -I/opt/homebrew/Cellar/tesseract/5.4.1/include -L/opt/homebrew/Cellar/tesseract/5.4.1/lib -ltesseract -I/opt/homebrew/Cellar/opencv/4.10.0_10/include/opencv4 -L/opt/homebrew/Cellar/opencv/4.10.0_10/lib -Wl,-rpath,/opt/homebrew/Cellar/opencv/4.10.0_10/lib -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_photo -lopencv_features2d -lopencv_imgproc -lopencv_imgcodecs -lopencv_calib3d -lopencv_video -I/opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3 -I/opt/homebrew/Cellar/boost/1.86.0/include -I/opt/homebrew/Cellar/icu4c/74.2/include -L/opt/homebrew/Cellar/icu4c/74.2/lib -licuuc -licudata /opt/homebrew/Cellar/boost/1.86.0/lib/libboost_system.a /opt/homebrew/Cellar/boost/1.86.0/lib/libboost_filesystem.a
g++ /Users/dengfengji/ronnieji/lib/project/main/opencv_test.cpp -o /Users/dengfengji/ronnieji/lib/project/main/opencv_test -I/Users/dengfengji/ronnieji/lib/project/include -I/Users/dengfengji/ronnieji/lib/project/src /Users/dengfengji/ronnieji/lib/project/src/*.cpp -I/opt/homebrew/Cellar/tesseract/5.4.1/include -L/opt/homebrew/Cellar/tesseract/5.4.1/lib -ltesseract -I/opt/homebrew/Cellar/opencv/4.10.0_10/include/opencv4 -L/opt/homebrew/Cellar/opencv/4.10.0_10/lib -Wl,-rpath,/opt/homebrew/Cellar/opencv/4.10.0_10/lib -lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_photo -lopencv_features2d -lopencv_imgproc -lopencv_imgcodecs -lopencv_calib3d -lopencv_video -I/opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3 -I/opt/homebrew/Cellar/boost/1.86.0/include -I/opt/homebrew/Cellar/icu4c/74.2/include -L/opt/homebrew/Cellar/icu4c/74.2/lib -licuuc -licudata /opt/homebrew/Cellar/boost/1.86.0/lib/libboost_system.a /opt/homebrew/Cellar/boost/1.86.0/lib/libboost_filesystem.a \
 $(pkg-config --cflags --libs sdl2) \
  $(pkg-config --cflags --libs sdl2_image) \
  -DOPENCV_VERSION=4.10.0_10 -std=gnu++20
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
#include <cmath>
#include <stdexcept>
#include "cvLib.h"

void trainImage(){
    cvLib cvl_j;
    std::cout << "Training images..." << std::endl;
    cvl_j.train_img_occurrences("/Users/dengfengji/ronnieji/Kaggle/house_plant_species",
    0.05,//0.05(learning rate)
    "/Users/dengfengji/ronnieji/lib/project/main/data.dat",
    "/Users/dengfengji/ronnieji/lib/project/main/data_key.dat",
    "/Users/dengfengji/ronnieji/lib/project/main/model_keymap.dat",
    99,
    pubstructs::inputImgMode::Gray
    );
    std::cout << "Successfully loaded test images, start recognizing..." << std::endl;
}
void test_image_recognition(){
    std::vector<std::string> testimgs;
    std::string sub_folder_path = "/Users/dengfengji/ronnieji/Kaggle/test"; //"/Users/dengfengji/ronnieji/Kaggle/test";
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
    cvLib::mark_font_info pen_marker;
    pen_marker.fontface = cv::FONT_HERSHEY_SIMPLEX;
    pen_marker.fontScale = 15.0;
    pen_marker.thickness = 2;
    pen_marker.fontcolor = cv::Scalar(0, 255, 0);
    pen_marker.text_position = cv::Point(5, 30);
    for(const auto& item : testimgs){
        cvLib::the_obj_in_an_image strReturn = cvl_j.what_is_this(item,pen_marker);
        if(!strReturn.empty()){
            bool display_time = cvl_j.get_display_time();
            std::cout << item << " is a(an) " << strReturn.objName << std::endl;
            if(display_time){
                std::cout << "Execution time: " << strReturn.timespent << " seconds\n";
            }
            /*
                mark the image and output to a new file
            */
            cv::Mat get_ori = cv::imread(item);
            if (get_ori.empty()) {
                std::cerr << "Error: Unable to load image." << std::endl;
                continue;
            }
            unsigned int rec_width = static_cast<unsigned int>(std::abs(strReturn.rec_bottomRight.x - strReturn.rec_topLeft.x));
            unsigned int rec_height = static_cast<unsigned int>(std::abs(strReturn.rec_bottomRight.y - strReturn.rec_topLeft.y));
            cvl_j.drawRectangleWithText(get_ori,strReturn.x,strReturn.y,rec_width,rec_height,strReturn.objName,pen_marker.thickness,pen_marker.fontcolor,pen_marker.fontcolor);
            std::string file_output = item + "_marked.jpg";
            cv::imwrite(file_output,get_ori);
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
        bool display_time = cvl_j.get_display_time();
        cvl_j.loadImageRecog("/Users/dengfengji/ronnieji/lib/project/main/model_keymap.dat",99,true,3,0.05);
        for(const auto& item : testimgs){
            if(std::filesystem::is_regular_file(item)){
               std::vector<cvLib::the_obj_in_an_image> multiObjsReturn = cvl_j.what_are_these(item);
               if(!multiObjsReturn.empty()){
                    std::cout << item << " image has: " << '\n';
                    for(const auto& multi_item : multiObjsReturn){
                        std::cout << "      " << multi_item.objName << std::endl;
                        if(display_time){
                            std::cout << "Execution time: " << multi_item.timespent << " seconds\n";
                        }
                    }
               }
               else{
                    std::cout << "multiObjsReturn value is empty!" << std::endl;
               }
            }
        }
    }
}
void put_img2_in_img1(){
    cvLib cvl_j;
    cv::Mat get_merged_img = cvl_j.put_img2_in_img1(
        "/Users/dengfengji/ronnieji/Kaggle/test/bg.JPEG",
        "/Users/dengfengji/ronnieji/Kaggle/test/processed_image.png",
        50,
        50,
        400,
        500,
        255,
        "/Users/dengfengji/ronnieji/Kaggle/test/output_image.jpg",
        800,
        600,
        800,
        600,
        32
    );
   cv::imwrite("/Users/dengfengji/ronnieji/Kaggle/test/output_image.jpg",get_merged_img);
}
/*
    resize image,and convert the image to 256, 8bit 
*/
void preprocess_images(const std::string& train_file_folder){
     try {  
        if(train_file_folder.empty()){
            return;
        }
        cvLib cvl_j;
        for (const auto& entryMainFolder : std::filesystem::directory_iterator(train_file_folder)) {  
            if (entryMainFolder.is_directory()) { // Check if the entry is a directory  
                std::string sub_folder_path = entryMainFolder.path().string();  
                // Accumulate pixel count for memory reservation  
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file()) {  
                        std::string imgFilePath = entrySubFolder.path().string(); 
                        if(!imgFilePath.empty()){
                            cv::Mat input_image = cv::imread(imgFilePath);
                            if(!input_image.empty()){
                                input_image = cvl_j.convert_to_256_8bit(input_image);
                                if(!input_image.empty()){
                                    cv::imwrite(imgFilePath,input_image);
                                    std::cout << "Successfully saved the image at: " << imgFilePath << std::endl;
                                }
                            }
                            else{
                                std::cout << "cv::Mat input_image is empty!" << std::endl; 
                            }
                        }
                        else{
                            std::cout << "The file path is invalid!" << std::endl;
                        }
                    }
                }
            }
            else{
                std::cout << "It's not a folder!" << std::endl;
            }
        }
     }
     catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
     }
}
void find_contours(){
    cvLib cvl_j;
    std::unordered_map<std::string,std::vector<cv::KeyPoint>> da;
    cvl_j.sub_find_contours_in_an_image("/Users/dengfengji/ronnieji/lib/images/sample1.jpg",da);
}
int main(){
    //preprocess_images("/Users/dengfengji/ronnieji/Kaggle/Vegetable Images/Memory");
    //trainImage();
    //test_image_recognition();
    //multi_objs_readImgs();
    //put_img2_in_img1();
    find_contours();
    return 0;
}