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
class subMainFunctions{
    public:
        std::vector<std::string> splitString(const std::string&, char);
};
std::vector<std::string> subMainFunctions::splitString(const std::string& input, char delimiter){
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
void find_img1_in_img2() {  
    cvLib cv_j;  
    std::string img1_path = "/Users/dengfengji/ronnieji/lib/images/sample4.jpg";  
    std::string img2_path = "/Users/dengfengji/ronnieji/lib/images/sample1.jpg";  
    // Check if the images exist  
    if (!std::filesystem::exists(img1_path) || !std::filesystem::exists(img2_path)) {  
        std::cerr << "One or both image paths do not exist." << std::endl;  
        return;  
    }  
    bool img1Found = cv_j.read_image_detect_objs(img1_path, img2_path);  
    if (img1Found) {  
        std::cout << "img1 color is detected in the image." << std::endl;  
    } else {  
        std::cout << "img1 not detected in the image." << std::endl;  
    }  
    img1Found = cv_j.isObjectInImage("/Users/dengfengji/ronnieji/lib/images/sample5.jpg", "/Users/dengfengji/ronnieji/lib/images/sample2.jpg", 500, 0.7, 10);  
    if (img1Found) {  
        std::cout << "img1 is in the image." << std::endl;  
    } else {  
        std::cout << "img1 is not found in the image." << std::endl;  
    }  
}
void trainImage(){
    cvLib cvl_j;
    std::cout << "Training images..." << std::endl;
    cvl_j.train_img_occurrences("/Users/dengfengji/ronnieji/Kaggle/archive-2/train",
    0.05,
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
        para4: distance allow bias from the trained data default = 2;
    */
    cvLib cvl_j;
    cvl_j.loadImageRecog("/Users/dengfengji/ronnieji/lib/project/main/model_keymap.dat",99,true,3);
    for(const auto& item : testimgs){
        std::string strReturn = cvl_j.what_is_this(item);
        std::cout << item << " is a(an) " << strReturn << std::endl;
    }
}
int main(){
    trainImage();
    test_image_recognition();
    return 0;
}