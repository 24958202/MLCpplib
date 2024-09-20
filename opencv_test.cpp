/*
g++ -std=c++20 \
/Users/dengfengji/ronnieji/lib/project/main/opencv_test.cpp \
-o /Users/dengfengji/ronnieji/lib/project/main/opencv_test \
-I/Users/dengfengji/ronnieji/lib/project/include \
-I/Users/dengfengji/ronnieji/lib/project/src \
/Users/dengfengji/ronnieji/lib/project/src/*.cpp \
-I/opt/homebrew/Cellar/tesseract/5.4.1/include \
-L/opt/homebrew/Cellar/tesseract/5.4.1/lib -ltesseract \
-I/opt/homebrew/Cellar/opencv/4.10.0_6/include/opencv4 \
-L/opt/homebrew/Cellar/opencv/4.10.0_6/lib \
-lopencv_core -lopencv_highgui -lopencv_videoio -lopencv_photo \
-lopencv_features2d -lopencv_imgproc -lopencv_imgcodecs -lopencv_calib3d -lopencv_video \
-I/opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3 \
-I/opt/homebrew/Cellar/boost/1.86.0/include \
-I/opt/homebrew/Cellar/icu4c/74.2/include \
-L/opt/homebrew/Cellar/icu4c/74.2/lib -licuuc -licudata \
/opt/homebrew/Cellar/boost/1.86.0/lib/libboost_system.a \
/opt/homebrew/Cellar/boost/1.86.0/lib/libboost_filesystem.a
*/
#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp> 
#include <map>
#include <unordered_map>
#include <stdint.h>
#include <filesystem>
#include "cvLib_uint32.h"
void find_img1_in_img2() {  
    cvLib cv_j;  
    std::string img1_path = "/Users/dengfengji/ronnieji/lib/images/sample3.jpg";  
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
    img1Found = cv_j.isObjectInImage("/Users/dengfengji/ronnieji/lib/images/sample4.jpg", img2_path, 500, 0.7, 10);  
    if (img1Found) {  
        std::cout << "img1 is in the image." << std::endl;  
    } else {  
        std::cout << "img1 is not found in the image." << std::endl;  
    }  
}
void ProcessFrame(cv::Mat& frame) {  
    //cv::Mat grayFrame;  
    //cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);  
    // Display the processed frame (optional)  
    //cv::imshow("OpencvTest", grayFrame);  
    cv::resizeWindow("OpencvTest", 1024, 1024); 
    cv::imshow("OpencvTest", frame);
}  
void startMacWebCam(){
    std::vector<std::string> imglist{
        "/Users/dengfengji/ronnieji/lib/images/sample4.jpg",
        "/Users/dengfengji/ronnieji/lib/images/sample5.jpg"
    };
    cv::Scalar markColor(0,255,0);
    cvLib cv_j;
    cv_j.StartWebCam(1,"Opencv Win Title",imglist,markColor,ProcessFrame);
}
void mark_edge_image(const std::string& imgPath){
    cvLib cvl_j;
    std::vector<uint32_t> frame_to_mark = cvl_j.get_img_matrix(imgPath,886,1560);
    if(!frame_to_mark.empty()){
        std::vector<std::pair<int, int>> outliers_found = cvl_j.findOutlierEdges(frame_to_mark,886,1560,70);
        if(!outliers_found.empty()){
            cv::Scalar brush_color(0,255,0);
            cvl_j.markOutliers(frame_to_mark,outliers_found,brush_color,886);
            cvl_j.saveImage(frame_to_mark,"/Users/dengfengji/ronnieji/lib/images/sample3_output.ppm",886,1560);
            std::cout << "Successfully saved the marked image." << std::endl;
        }
    }
    cvl_j.read_image_detect_edges("/Users/dengfengji/ronnieji/lib/images/sample1.jpg",10,"/Users/dengfengji/ronnieji/lib/images/sample1_marked.ppm",brushColor::Green,brushColor::Black);
}
void getObjectInImgs(){
    cvLib cvl_j;
    std::vector<uint32_t> obj_output =  cvl_j.objectsInImage("/Users/dengfengji/ronnieji/lib/images/sample15.jpg",20);
    //cvl_j.createOutlierImage();
    if(!obj_output.empty()){
        if(cvl_j.saveImage(obj_output,"/Users/dengfengji/ronnieji/lib/images/objects_in_sample2.ppm",600,726)){
            std::cout << "Successfully output the objects in image." << std::endl;
        }
    }
    else{
        std::cout << "obj_output is empty!" << std::endl;
    }
}
void getImageDataset(){
     std::vector<std::string> testimgs{
        "/Users/dengfengji/ronnieji/Kaggle/test/Image_12.jpg",
        "/Users/dengfengji/ronnieji/Kaggle/test/Image_20.jpg",
        "/Users/dengfengji/ronnieji/Kaggle/test/Image_32.jpg",
        "/Users/dengfengji/ronnieji/Kaggle/test/Image_34.jpg",
        "/Users/dengfengji/ronnieji/Kaggle/test/Image_44.jpg"
     };
     std::unordered_map<std::string,std::vector<uint32_t>> test_imgs;
     cvLib cvl_j;
     std::unordered_map<std::string,std::vector<uint32_t>> dataset = cvl_j.get_img_in_folder("/Users/dengfengji/ronnieji/Kaggle/archive-2/train");
     if(!dataset.empty()){
        std::unordered_map<std::string,std::vector<uint32_t>> training_data = cvl_j.train_img_in_folder(dataset,"/Users/dengfengji/ronnieji/lib/project/main/data.dat");
        for(const auto& item : testimgs){
            std::vector<uint32_t> test_img = cvl_j.get_one_image(item);
            if(!test_img.empty()){
                test_imgs[item] = test_img;
            }
        }
        /*
            start recognizing 
        */
     }
}
int main(){
    find_img1_in_img2();
    //startMacWebCam();
    //mark_edge_image("/Users/dengfengji/ronnieji/lib/images/sample3.jpg");
    //getObjectInImgs();
    //getImageDataset();
    return 0;
}