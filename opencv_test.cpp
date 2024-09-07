#include <iostream>
#include <string>
#include <opencv2/opencv.hpp> 
#include "cvLib.h"
void find_img1_in_img2(){
    cvLib cv_j;
    std::string img1_path = "/Users/dengfengji/ronnieji/lib/images/sample3.jpg";
    std::string img2_path = "/Users/dengfengji/ronnieji/lib/images/sample1.jpg";
    // bool img1Found = cv_j.isObjectInImage("/Users/dengfengji/ronnieji/lib/images/sample15.jpg",img2_path,500,0.7,10);
    // if(img1Found){
    //     std::cout << "img1 is in the image." << std::endl;
    // }
    bool img1Found = cv_j.read_image_detect_objs(img1_path,img2_path);
    if(img1Found){
        std::cout << "img1 color is in the image." << std::endl;
    }
    img1Found = cv_j.isObjectInImage("/Users/dengfengji/ronnieji/lib/images/sample15.jpg",img2_path,500,0.7,10);
    if(img1Found){
        std::cout << "img1 is in the image." << std::endl;
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
    std::vector<std::vector<RGB>> frame_to_mark = cvl_j.get_img_matrix(imgPath,886,1560);
    if(!frame_to_mark.empty()){
        std::vector<std::pair<int, int>> outliers_found = cvl_j.cvLib::findOutlierEdges(frame_to_mark, 70);
        if(!outliers_found.empty()){
            cv::Scalar brush_color(0,255,0);
            cvl_j.markOutliers(frame_to_mark,outliers_found,brush_color);
            cvl_j.saveImage(frame_to_mark,"/Users/dengfengji/ronnieji/lib/images/sample1.ppm");
            std::cout << "Successfully saved the marked image." << std::endl;
        }
    }
    cvl_j.read_image_detect_edges("/Users/dengfengji/ronnieji/lib/images/sample1.jpg",10,"/Users/dengfengji/ronnieji/lib/images/sample1_marked.ppm",brushColor::Green,brushColor::Black);
}
int main(){
    //find_img1_in_img2();
    startMacWebCam();
    //mark_edge_image("/Users/dengfengji/ronnieji/lib/images/sample1.jpg");

    return 0;
}