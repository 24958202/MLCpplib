#ifndef CVLIB_H
#define CVLIB_H
#include <iostream> 
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>  
#include <Eigen/Dense> 
struct RGB {  
    int r;  
    int g;  
    int b;  
};  
struct imgSize{
    int width;
    int height;
};
enum class brushColor{
    Green,
    Red,
    White,
    Black
};
class cvLib{
    public:
        std::vector<std::vector<RGB>> cv_mat_to_dataset(const cv::Mat&);
        /*
            para1: input an image path, will return the image size 
            struct imgSize{
                int width;
                int height;
            };
        */
        imgSize get_image_size(const std::string&);
        /*
            cv::Scalar markerColor(0,255,0);
            cv::Scalar txtColor(255, 255, 255);
            rec_thickness = 2;
        */
        // Function to draw a green rectangle on an image  
        void drawRectangleWithText(cv::Mat&, int, int, int, int, const std::string&, int, const cv::Scalar&, const cv::Scalar&);
        /*
            save cv::Mat to a file, para1: input a cv::Mat file, para2: output file path *.ppm
        */
        void savePPM(const cv::Mat&, const std::string&);
        /*
            1.read an image, 2.resize the image to expected size, 3. remove image colors
            Turn into a std::vector<std::vector<RGB>> dataset (matrix: dimention-rows: dataset.size(), dimention-columns: std::vector<RGB> size())
            para1: image path
            para2: output matrix rows number
            para3: output matrix columns number
        */
        std::vector<std::vector<RGB>> get_img_matrix(const std::string&, int,int);
        /*
            read all images in a folder to a std::vector<std::vector<RGB>> dataset
            para1: folder path
        */
        std::multimap<std::string, std::vector<RGB>> read_images(std::string&);
        /*
             Function to find outlier edges using a simple gradient method
             para1: input the std::vector<std::vector<RGB>> from "get_img_matrix" function
             para2: Define a threshold for detecting edges, 0-100;
        */
        std::vector<std::pair<int, int>> findOutlierEdges(const std::vector<std::vector<RGB>>&, int);
        /*
            Function to mark outliers in the image data
            para1: std::vector<std::vector<RGB>>& data from "get_img_matrix"
            para2: const std::vector<std::pair<int, int>>& outliers the ourliers from "findOutlierEdges" function
            para3: the color of marker; define: cv::Scalar markColor(0,255,0); //green color 
        */
        void markOutliers(std::vector<std::vector<RGB>>&, const std::vector<std::pair<int, int>>&, const cv::Scalar&);
        /*
            save the matrix std::vector<std::vector<RGB>> to an image file
            para1: matrix std::vector<std::vector<RGB>> data
            para2: output file path *.ppm 
        */
        bool saveImage(const std::vector<std::vector<RGB>>&, const std::string&);
        /*
            Function to create an output image with only outliers
            para1: std::vector<std::vector<RGB>> the original image matrix data
            para2: the outliers from "findOutlierEdges" function
            para3: output image path
            para4: output image background color cv::Scalar bgColor(0,0,0);
        */
        void createOutlierImage(const std::vector<std::vector<RGB>>&, const std::vector<std::pair<int, int>>&, const std::string&, const cv::Scalar&);
        /*
            This function can read an image, and mark all the edges of objects in the image
            para1: the image path
            para2: gradientMagnitude threshold 0-100, better result with small digits
            para3: the output image path (*.ppm file)
            para4: marker color
            para5: background color
        */
        void read_image_detect_edges(const std::string&,int,const std::string&, const brushColor&, const brushColor&);
         /*
            This function can read two images and return true if imgage1 is in image2
            para1: the image1 path
            para2: the image2 path
            para3: threshold for detecting the matching score. if score >threshold, matched, else did not. default value = 10;
        */
        bool read_image_detect_objs(const std::string&,const std::string&, int de_threshold = 10);
};
#endif