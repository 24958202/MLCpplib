#ifndef CVLIB_H
#define CVLIB_H
#include <iostream> 
#include <vector>
#include <unordered_map>
#include <map>
#include <opencv2/opencv.hpp>  
#include <functional>
#include <stdint.h>
#define ImageWidth 120
#define ImageHeight 120
const uint32_t C_0 = 0;
const uint32_t C_1 = 1;
const uint32_t C_2 = 2;
const uint32_t C_3 = 3;
struct RGB {  
    uint32_t r; // Red  
    uint32_t g; // Green  
    uint32_t b; // Blue  
    // Default constructor  
    RGB() : r(0), g(0), b(0) {} // Initializes RGB to black (0, 0, 0)  
    // Parameterized constructor  
    RGB(uint32_t red, uint32_t green, uint32_t blue) : r(red), g(green), b(blue) {}  
};   
struct VectorHash {  
    template <typename T>  
    std::size_t operator()(const std::vector<T>& vec) const {  
        std::size_t seed = vec.size(); // Start with the size of vector  
        for (const auto& i : vec) {  
            seed ^= std::hash<T>()(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2); // Combining hashes  
        }  
        return seed;  
    }  
}; 
struct imgSize{
    unsigned int width;
    unsigned int height;
};
enum class brushColor{
    Green,
    Red,
    White,
    Black
};
enum class outputImgMode{
    Gray,
    Color
};
class cvLib{
    public:
        /*
            para1: image path
            para2: outputImgMode (Gray or Color)
            This function will open an image and convert it to type CV_32F
        */
        cv::Mat preprocessImage(const std::string&, const outputImgMode&);
        /*
            para1: image path
            open an image, resize to 120x120 pixels, remove image colors
        */
        std::vector<std::vector<RGB>> get_img_120_gray_for_ML(const std::string&);
        /*
            para1: main dataset 
            para2: sub dataset
        */
        unsigned int count_occurrences(const std::vector<uint32_t>&, const std::vector<uint32_t>&);
        /*
            Function to convert std::vector<uint32_t> to std::vector<std::vector<RGB>> 
        */
        std::vector<std::vector<RGB>> convertToRGB(const std::vector<uint32_t>&, unsigned int,  unsigned int);
        /*
            Function to convert std::vector<std::vector<RGB>> back to std::vector<uint32_t> 
        */
        std::vector<uint32_t> convertToPacked(const std::vector<std::vector<RGB>>&);
        /*
            Function to convert std::vector<uint32_t> to cv::Mat 
            para1: packed pixel data
            para2: width
            para3: height
        */
        cv::Mat vectorToImage(const std::vector<uint32_t>&, unsigned int, unsigned int); 
        /*
            Input an image path, will return an RGB dataset std::vector<std::vector<RGB>> -gray
        */
        std::vector<std::vector<RGB>> cv_mat_to_dataset(const cv::Mat&);
        /*
            Input an image path, will return an RGB dataset - color
        */
        std::vector<std::vector<RGB>> cv_mat_to_dataset_color(const cv::Mat&);
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
        void drawRectangleWithText(cv::Mat&, int, int, unsigned int, unsigned int, const std::string&, unsigned int, const cv::Scalar&, const cv::Scalar&);
        /*
            save cv::Mat to a file, para1: input a cv::Mat file, para2: output file path *.ppm
        */
        void savePPM(const cv::Mat&, const std::string&);
        /*
            save std::vector<RGB> pixels to a file, 
            para1: input a std::vector<RGB> dataset, 
            para2: image width
            para3: image height
            para4: output file path *.ppm
        */
        void saveVectorRGB(const std::vector<std::vector<RGB>>&, unsigned int, unsigned int, const std::string&);
        /*
            1.read an image, 2.resize the image to expected size, 3. remove image colors
            Turn into a std::vector<std::vector<RGB>> dataset (matrix: dimention-rows: dataset.size(), dimention-columns: std::vector<RGB> size())
            para1: image path
            para2: output matrix rows number(height)
            para3: output matrix columns number(width)
        */
        std::vector<std::vector<RGB>> get_img_matrix(const std::string&, unsigned int,unsigned int);
         /*
            1.read an image, 2.resize the image to expected size, (keep image colors)
            Turn into a std::vector<std::vector<RGB>> dataset (matrix: dimention-rows: dataset.size(), dimention-columns: std::vector<RGB> size())
            para1: image path
            para2: output matrix rows number(height)
            para3: output matrix columns number(width)
        */
        std::vector<std::vector<RGB>> get_img_matrix_color(const std::string&, unsigned int,unsigned int);
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
        std::vector<std::pair<int, int>> findOutlierEdges(const std::vector<std::vector<RGB>>&, unsigned int);
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
        void read_image_detect_edges(const std::string&,unsigned int,const std::string&, const brushColor&, const brushColor&);
        /*
            this function to normalize function to preprocess images  
            to turn images to black and white
            prar1: input image path
            para2:  return a black-and-white std::vector<RGB> dataset
        */
        void convertToBlackAndWhite(cv::Mat&, std::vector<std::vector<RGB>>&);
         /*
            This function can read two images and return true if imgage1 is in image2
            para1: the image1 path
            para2: the image2 path
            para3: threshold for detecting the matching score. if score >threshold, matched, else did not. default value = 10;
            Typical Range:
            Low: 100-300 keypoints for quick processing or when images have few distinctive features.
            Medium: 500-1000 keypoints for balanced performance and accuracy.
            High: 1500-3000+ keypoints for detailed images with many features, at the cost of increased processing time.
            Ratio Threshold (ratioThresh)
            Purpose: Used in the ratio test to filter out poor matches. It compares the distance of the best match to the second-best match.
            Effect: A lower ratio threshold makes the matching criteria stricter, reducing false positives but potentially missing some true matches. A higher threshold allows more matches but may include more false positives.
            
            Typical Range:
            Strict: 0.5-0.6 for high precision, reducing false matches.
            Balanced: 0.7 (commonly used default) for a good balance between precision and recall.
            Relaxed: 0.8-0.9 for higher recall, allowing more matches but with a risk of false positives.
            
            deThreshold Overview:
            Definition: deThreshold typically acts as a cut-off or threshold for the number of good matches required to consider img1 as present in img2.
            Range Context: The actual value you should set for deThreshold heavily depends on:
            Image complexity and textures.
            Noise and quality levels in the images.
            Whether images have significant occlusions or scale differences.
            Typical Usage: Start by experimenting with values around 10 to 30. If you need higher confidence, increase deThreshold.
        */
        bool read_image_detect_objs(const std::string&,const std::string&, unsigned int featureCount = 500, float ratioThresh = 0.7f, unsigned int de_threshold = 10);
        /*
            This function turn both images to gray before comparing the image.
            para1: the image1 path
            para2: the image2 path
            para3: threshold for detecting the matching score. if score >threshold, matched, else did not. default value = 10;
            Feature Count (featureCount)
            Purpose: Determines the maximum number of keypoints to detect in each image.
            Effect: Increasing the feature count can potentially increase the number of matches, as more keypoints are available for matching. However, it also increases computational cost.
            
            Typical Range:
            Low: 100-300 keypoints for quick processing or when images have few distinctive features.
            Medium: 500-1000 keypoints for balanced performance and accuracy.
            High: 1500-3000+ keypoints for detailed images with many features, at the cost of increased processing time.
            Ratio Threshold (ratioThresh)
            Purpose: Used in the ratio test to filter out poor matches. It compares the distance of the best match to the second-best match.
            Effect: A lower ratio threshold makes the matching criteria stricter, reducing false positives but potentially missing some true matches. A higher threshold allows more matches but may include more false positives.
            
            Typical Range:
            Strict: 0.5-0.6 for high precision, reducing false matches.
            Balanced: 0.7 (commonly used default) for a good balance between precision and recall.
            Relaxed: 0.8-0.9 for higher recall, allowing more matches but with a risk of false positives.
            
            deThreshold Overview:
            Definition: deThreshold typically acts as a cut-off or threshold for the number of good matches required to consider img1 as present in img2.
            Range Context: The actual value you should set for deThreshold heavily depends on:
            Image complexity and textures.
            Noise and quality levels in the images.
            Whether images have significant occlusions or scale differences.
            Typical Usage: Start by experimenting with values around 10 to 30. If you need higher confidence, increase deThreshold.

        */
        bool isObjectInImage(const std::string&, const std::string&, unsigned int featureCount = 500, float ratioThresh = 0.7f, unsigned int deThreshold = 10);
        /*
            This function will return all the object as std::vector<std::vector<RGB>> in an image
            para1: image path
            para2 : gradientMagnitude threshold 0-100, better result with small digits
        */
        std::vector<std::vector<RGB>> objectsInImage(const std::string&, unsigned int, const outputImgMode&);
        /*
            This function can recognize text in an image
            para1: the image path
            return: text1, text2... in the image
            You need to install library: 
            brew install tesseract
            brew install tesseract-lang (multi-language supported)
            // Call the function and handle the result  
            char* recognizedText = read_image_detect_text("/Users/dengfengji/ronnieji/imageRecong/samples/board2.jpg");  
            if (recognizedText) {  
                std::cout << "Recognized text: " << recognizedText << std::endl;  
                delete[] recognizedText; //Free the allocated memory for text  
            }  
        */
        char* read_image_detect_text(const std::string&);
        /*
            This function can open the default webcam and pass the 
            video to a callback function
            para1: video camera index : default 0
            para2: video window title
            para3: list of image list (files path) that might be detected in the video std::vector<std::string> imageList;
            para4: marker's color cv::Scalar markColor(0,255,0);
            para5: callback function
            // Exit the loop if the user presses the 'q' key 
            Usage:
            // Example callback function  
            void ProcessFrame(cv::Mat& frame) {  
                // Process the frame (e.g., convert to grayscale)  
                cv::Mat grayFrame;  
                cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);  
                // Display the processed frame (optional)  
                cv::imshow("Processed Frame", grayFrame);  
            }  
            int main() {  
                // Start the webcam and pass the callback function  
                StartWebCam(ProcessFrame);  
                return 0;  
            }  
        */
        void StartWebCam(unsigned int,const std::string&,const std::vector<std::string>&, const cv::Scalar&, std::function<void(cv::Mat&)> callback);
        /*
            read an image and return std::vector<uint32_t>
            para1: image path
        */
        std::vector<uint32_t>get_one_image(const std::string&);
        /*
            get the key point of an image
            para1: cv::Mat input image
            para2: cv::Mat descriptors
        */
        std::vector<cv::KeyPoint> extractORBFeatures(const cv::Mat&, cv::Mat&);
        /*
            para1: train images folder
            para2: output model file path/output.dat
            para3: output model_keypoints file path/output_key.dat
        */
        void train_img_occurrences(const std::string&, const std::string&,const std::string&);
        /*
            para1: return a pre-defined std::unordered_map<std::string, std::vector<cv::Mat>>
            para2: the model's file path
        */
        void loadModel(std::unordered_map<std::string, std::vector<cv::Mat>>&, const std::string&);
        /*
            load keypoints
            para1: return a pre-defined std::unordered_map<std::string, std::vector<KeyPoint>>
        */
       void loadModel_keypoint(std::unordered_map<std::string, std::vector<cv::KeyPoint>>&, const std::string&);

};      
#endif