#ifndef CVLIB_H
#define CVLIB_H
#include <iostream> 
#include <vector>
#include <unordered_map>
#include <map>
#include <opencv2/opencv.hpp>  
#include <functional>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ImageWidth 120
#define ImageHeight 120

namespace pubstructs{
    const uint8_t C_0 = 0;
    const uint8_t C_1 = 1;
    const uint8_t C_2 = 2;
    const uint8_t C_3 = 3;
    enum class inputImgMode{
        Gray,
        Color
    };
    struct RGB {
            uint8_t r; // Red
            uint8_t g; // Green
            uint8_t b; // Blue
            // Default constructor
            RGB() : r(0), g(0), b(0) {} // Initializes RGB to black (0, 0, 0)
            // Parameterized constructor
            RGB(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
            // Define equality and inequality operators
            bool operator==(const RGB& other) const {
                return (r == other.r) && (g == other.g) && (b == other.b);
            }
            bool operator!=(const RGB& other) const {
                return !(*this == other);
            }
    }; 
}
class cvLib{
    private:
        std::unordered_map<std::string, std::vector<pubstructs::RGB>> _loaddataMap;
        unsigned int _gradientMagnitude_threshold = 33; 
        bool display_time = false;
        unsigned int distance_bias = 2;
        double learning_rate = 0.05;
    public:
        struct mark_font_info{
            int fontface;
            double fontScale;
            unsigned int thickness;
            cv::Scalar fontcolor;
            cv::Point text_position;//default is cv::Point position(5, 30); // Typically, (5, 30) is a good starting point for large fonts to avoid cutting off the text
            mark_font_info() : fontface(cv::FONT_HERSHEY_SIMPLEX),fontScale(1.0),thickness(2),fontcolor(cv::Scalar(0, 255, 0)),text_position(cv::Point(5, 30)) {}
            bool empty() const {
                return fontface == -1 &&      // Assuming -1 is an uninitialized state for fontface
                    fontScale == 0.0 &&    // Assuming 0.0 is an uninitialized state for fontScale
                    thickness == 0 &&      // Assuming 0 is an uninitialized state for thickness
                    fontcolor == cv::Scalar(0, 0, 0, 0) && // Assuming (0, 0, 0, 0) is "empty" for fontcolor
                    text_position == cv::Point(0, 0); // Assuming (0, 0) is an "empty" position
            }
        };
        struct the_obj_in_an_image {
            std::string objName;
            pubstructs::RGB rgb;
            int x;
            int y;
            cv::Point2f rec_topLeft;
            cv::Point2f rec_bottomRight;
            int fontface;
            double fontScale;
            unsigned int thickness;
            cv::Scalar fontcolor;
            cv::Point text_position;
            std::chrono::duration<double> timespent;
            bool empty() const {
                // Check if objName is empty, timespent is zero, and other members are at default values
                return objName.empty() &&
                    timespent.count() == 0.0 &&
                    rgb == pubstructs::RGB(0, 0, 0) &&  // Assuming RGB(0, 0, 0) is considered "empty"
                    x == 0 &&
                    y == 0 &&
                    rec_topLeft.x == 0 &&
                    rec_topLeft.y == 0 &&
                    rec_bottomRight.x == 0 &&
                    rec_bottomRight.y == 0 &&
                    fontface == -1 &&      // Assuming -1 is an uninitialized state for fontface
                    fontScale == 0.0 &&    // Assuming 0.0 is an uninitialized state for fontScale
                    thickness == 0 &&      // Assuming 0 is an uninitialized state for thickness
                    fontcolor == cv::Scalar(0, 0, 0, 0) && // Assuming (0, 0, 0, 0) is "empty" for fontcolor
                    text_position == cv::Point(0, 0); // Assuming (0, 0) is an "empty" position
            }
        };
        struct ObjsInImage{
            unsigned int img_index;
            std::pair<int,int> img_pos;
            std::pair<unsigned int,unsigned int> img_size;
            std::pair<double,double> img_centroid;
            cv::Mat img_seg;
            // Define what "empty" means for this struct
            bool empty() const {
                // Example condition: img_seg should not have any elements
                return img_seg.empty() && 
                    img_size == std::make_pair(0U, 0U);
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
        void set_distance_bias(const unsigned int&);
        unsigned int get_distance_bias() const;
        void set_display_time(const bool&);
        bool get_display_time() const;
        void set_gradientMagnitude_threshold(const unsigned int&);
        unsigned int get_gradientMagnitude_threshold() const;
        void set_loaddataMap(const std::unordered_map<std::string, std::vector<pubstructs::RGB>>&);
        std::unordered_map<std::string, std::vector<pubstructs::RGB>> get_loaddataMap() const;
        void set_learing_rate(const double&);
        double get_learning_rate() const;
        std::vector<std::string> splitString(const std::string&, char);//tokenize a string, sperated by char for example ','
        /*
            Function to open an image and put it into a width x height matrix and return a cv::Mat
            para1: Input image cv::Mat
            para2: Output image width
            para3: Output image height
        */
        cv::Mat placeOnTransparentBackground(const cv::Mat&,unsigned int,unsigned int);
        /*
            Function to open an image and convert it to an 256, 8 bit image
            para1: cv::Mat input image
        */
        cv::Mat convert_to_256_8bit(const cv::Mat&);
        /*
            Function to convert std::vector<uint8_t> to std::vector<std::vector<pubstructs::RGB>> 
        */
        std::vector<std::vector<pubstructs::RGB>> convertToRGB(const std::vector<uint8_t>&, unsigned int,  unsigned int);
        /*
            Function to convert std::vector<std::vector<pubstructs::RGB>> back to std::vector<uint8_t> 
        */
        std::vector<uint8_t> convertToPacked(const std::vector<std::vector<pubstructs::RGB>>&);
        /*
            Function to convert std::vector<std::vector<RGB>> to cv::Mat 
            para1: packed pixel data
            para2: width
            para3: height
        */
        cv::Mat vectorToImage(const std::vector<std::vector<pubstructs::RGB>>&, unsigned int, unsigned int);
        /*
            Function to detect keypoints by clusters
            para1: input image 
            para2: clusters cv::Rect(0, 0, 100, 100), cv::Rect(100, 100, 100, 100)
        */
        std::unordered_map<std::string, std::vector<cv::KeyPoint>> detectKeypointsByClusters(const cv::Mat&, const std::vector<cv::Rect>&);
        /*
            Input an image path, will return an pubstructs::RGB dataset std::vector<std::vector<pubstructs::RGB>> -gray
        */
        std::vector<std::vector<pubstructs::RGB>> cv_mat_to_dataset(const cv::Mat&);
        /*
            Input an image path, will return an pubstructs::RGB dataset - color
        */
        std::vector<std::vector<pubstructs::RGB>> cv_mat_to_dataset_color(const cv::Mat&);
        /*
            Function to convert an input image to WebP format and save it
            para1: Input image Path
            para2: output WebP file path Path (path/to/output.webp)
        */
        void convertAndSaveAsWebP(const std::string&, const std::string&);
        /*
            para1: input an image path, will return the image size 
            struct imgSize{
                unsigned int width;
                unsigned int height;
            };
        */
        imgSize get_image_size(const std::string&);
        /*
            Function to extra contours as keypoint
            para1: input edges
        */
        std::unordered_map<std::string, std::vector<cv::KeyPoint>> extractContoursAsKeyPoints(const cv::Mat&);
        /*
            Function to find contours in an image
            para1: image path
            para2: output std::unordered_map<std::string, std::vector<cv::KeyPoint>> 
        */
        void sub_find_contours_in_an_image(const std::string&,std::unordered_map<std::string, std::vector<cv::KeyPoint>>&);
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
        void saveVectorRGB(const std::vector<std::vector<pubstructs::RGB>>&, unsigned int, unsigned int, const std::string&);
        /*
            1.read an image, 2.resize the image to expected size, 3. remove image colors
            Turn into a std::vector<std::vector<pubstructs::RGB>> dataset (matrix: dimention-rows: dataset.size(), dimention-columns: std::vector<pubstructs::RGB> size())
            para1: image path
            para2: output matrix rows number(height)
            para3: output matrix columns number(width)
            para4: inputImgMode::Gray use gray image, inputImgMode::Color use full color imag
        */
        std::vector<std::vector<pubstructs::RGB>> get_img_matrix(const std::string&, unsigned int,unsigned int,const pubstructs::inputImgMode&);
        /*
            read all images in a folder to a std::vector<std::vector<pubstructs::RGB>> dataset
            para1: folder path
        */
        std::multimap<std::string, std::vector<pubstructs::RGB>> read_images(std::string&);
        /*
             Function to find outlier edges using a simple gradient method
             para1: input the std::vector<std::vector<pubstructs::RGB>> from "get_img_matrix" function
             para2: Define a threshold for detecting edges, 0-100;
        */
        std::vector<std::pair<int, int>> findOutlierEdges(const std::vector<std::vector<pubstructs::RGB>>&, unsigned int);
        /*
            Function to mark outliers in the image data
            para1: std::vector<std::vector<pubstructs::RGB>>& data from "get_img_matrix"
            para2: const std::vector<std::pair<int, int>>& outliers the ourliers from "findOutlierEdges" function
            para3: the color of marker; define: cv::Scalar markColor(0,255,0); //green color 
        */
        void markOutliers(std::vector<std::vector<pubstructs::RGB>>&, const std::vector<std::pair<int, int>>&, const cv::Scalar&);
        /*
            save the matrix std::vector<std::vector<pubstructs::RGB>> to an image file
            para1: matrix std::vector<std::vector<pubstructs::RGB>> data
            para2: output file path *.ppm 
        */
        bool saveImage(const std::vector<std::vector<pubstructs::RGB>>&, const std::string&);
        /*
            Function to create an output image with only outliers
            para1: std::vector<std::vector<pubstructs::RGB>> the original image matrix data
            para2: the outliers from "findOutlierEdges" function
            para3: output image path
            para4: output image background color cv::Scalar bgColor(0,0,0);
        */
        void createOutlierImage(const std::vector<std::vector<pubstructs::RGB>>&, const std::vector<std::pair<int, int>>&, const std::string&, const cv::Scalar&);
        /*
            Function to extract, resize, and center objects onto a white background.
            para1: imagePath
            para2: cannyThreshold Lower (0 - 255)
            para3: cannyThreshold Upper (0 - 255)
            Canny Thresholds:
                cannyThreshold1 (Lower Threshold):
                It is the lower bound threshold in Canny edge detection.
                When determining whether a pixel is an edge, if its intensity gradient is greater than this value, it may become an edge pixel, but not decisively.
                It helps in edge linking; pixels with gradient intensity between this threshold and the upper threshold are considered only if they're connected to a pixel with a gradient above cannyThreshold2.
            cannyThreshold2 (Upper Threshold):
                It is the higher bound threshold for edge pixel determination.
                Any pixel with a gradient intensity above this threshold is considered a strong edge pixel.
                It greatly influences the number of visible/hard edges — higher values typically result in fewer edges being detected.
            para4: gradientMagnitude_threshold gradientMagnitude threshold 0-100, better result with small digits
            para5: output image width
            para6: output image height
            para7: the percentage of the content in the image - scaleFactor(value 0 to 1)
        */
        std::vector<cv::Mat> extractAndProcessObjects(const std::string& imagePath, unsigned int cannyThreshold1 = 100, unsigned int cannyThreshold2 = 200, unsigned int gradientMagnitude = 99, unsigned int outputWidth=800, unsigned int outputHeight=800, double scaleFactor=0.8);
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
            para2:  return a black-and-white std::vector<pubstructs::RGB> dataset
        */
        void convertToBlackAndWhite(cv::Mat&, std::vector<std::vector<pubstructs::RGB>>&);
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
            This function will return all the object as std::vector<std::vector<pubstructs::RGB>> in an image
            para1: image path
            para2 : gradientMagnitude threshold 0-100, better result with small digits
            para3: inputImgMode(::Color, ::Gray)
        */
        std::vector<std::vector<pubstructs::RGB>> objectsInImage(const std::string&, unsigned int, const pubstructs::inputImgMode&);
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
            para1: image path
            para2: inputImgMode (Gray or Color)
            para3: gradientMagnitude_threshold gradientMagnitude threshold 0-100, better result with small digits
            This function will open an image and convert it to type CV_32F
        */
        cv::Mat preprocessImage(const std::string&, const pubstructs::inputImgMode&, const unsigned int);
        /*
            para1: image path
            para2: gradientMagnitude_threshold gradientMagnitude threshold 0-100, better result with small digits
            open an image, resize to 120x120 pixels, remove image colors
        */
        std::vector<std::vector<pubstructs::RGB>> get_img_120_gray_for_ML(const std::string&, const unsigned int);
        /*
            get the key point of an image
            para1: cv::Mat input image
            para2: cv::Mat descriptors
        */
        std::vector<cv::KeyPoint> extractORBFeatures(const cv::Mat&, cv::Mat&) const;
        /*
            get the key point of an image
            para1: cv::Mat input image
            para2: cv::Mat descriptors
            para3: Tune ORB Detector Parameters, (increase the number of recognized points in clusters)
        */
        std::vector<cv::KeyPoint> extractORBFeatures_multi(const cv::Mat&, cv::Mat&, int) const;
        /*
            Function to get different clusters points into a dataset
            para1: keypoints from extractORBFeatures
            para2: Number of Clusters  
        */
        std::vector<std::vector<cv::KeyPoint>> clusterKeypoints(const std::vector<cv::KeyPoint>&, int) const;
        /*
            save model keypoints
            para1: dataMap
            para2: output filePath path/model.dat
        */
        void save_keymap(const std::unordered_map<std::string, std::vector<pubstructs::RGB>>&, const std::string&);
        /*
            load model keypoints
            para1:model file path
            para2: output std::unordered_map<std::string, std::vector<pubstructs::RGB>> dataMap
        */
        void load_keymap(const std::string&, std::unordered_map<std::string, std::vector<pubstructs::RGB>>&);
        /*
            Function to convert 
        */
        std::vector<std::vector<float>> convertKeypointsToVector(const std::vector<cv::KeyPoint>&, const std::vector<std::pair<unsigned int, unsigned int>>&);
        /*
            para1: train images folder
            para2: learning rate (Between 0-1, takes more time if number is bigger)
            para3: output model file path/output.dat 
            para4: output model_keypoints file path/output_key.dat
            para5: output model_keymap file path/output_map.dat
            para6: gradientMagnitude_threshold gradientMagnitude threshold 0-100, better result with small digits
            para7: input image mode, inputImgMode::Color, or inputImgMode::Gray
        */
        void train_img_occurrences(const std::string&, const double, const std::string&,const std::string&,const std::string&,const unsigned int,const pubstructs::inputImgMode&);
        /*
            initialize recognition 
            para1: input the train model file from function train_img_occurrences para5: output model_keymap file path/output_map.dat
            para2: //gradientMagnitude_threshold gradientMagnitude threshold 0-100, better result with small digits, but takes longer (default: 33)
            para3: bool = true (display time spent)
            para4: distance allow bias from the trained data default = 2;
            para5: learning rate (much be the save as train_img_occurrences, and train_for_multi_imgs_recognition)
        */
        void loadImageRecog(const std::string&,const unsigned int, const bool, unsigned int, double);
        /*
            Function to input an image and return the recognition (std::string)
            para1: input an image file path
            para2: marker's font info
        */
        the_obj_in_an_image what_is_this(const std::string&, const mark_font_info&);
        /*
            Save the void machine_learning_result(); result
            para1: input: const std::unordered_map<std::string, cv::Mat>& summarizedDataset, 
            para2: input:  const std::unordered_map<std::string, std::vector<cv::KeyPoint>>& summarizedKeypoints,
            para3: output model file path path/to/model.dat
        */
        void save_trained_model(const std::unordered_map<std::string, cv::Mat>&, 
               const std::unordered_map<std::string, std::vector<cv::KeyPoint>>&,
               const std::string&);
        /*
            para1: cluster number < dataset.size(), 
            para1: std::unordered_map<std::string, std::vector<cv::Mat>> dataset; //descriptors
            para2: std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>> dataset_keypoint; //keypoints
            para3: return value-> std::unordered_map<std::string, cv::Mat>& summarizedDataset
            para4: return value-> std::unordered_map<std::string, std::vector<cv::KeyPoint>>& summarizedKeypoints
            para5: output model file path : path/to/model.dat
        */
        void machine_learning_result(
            const unsigned int,
            const std::unordered_map<std::string, std::vector<cv::Mat>>&, 
            const std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>>&,
            std::unordered_map<std::string, cv::Mat>&, 
            std::unordered_map<std::string, std::vector<cv::KeyPoint>>&,
            const std::string&); 
        /*
            para1: return a pre-defined std::unordered_map<std::string, std::vector<cv::Mat>>
            para2: the model's file path
        */
        void loadModel(std::unordered_map<std::string, std::vector<cv::Mat>>&, const std::string&);
        /*
            load keypoints
            para1: return a pre-defined std::unordered_map<std::string, std::vector<KeyPoint>>
        */
       void loadModel_keypoint(std::unordered_map<std::string, std::vector<std::vector<cv::KeyPoint>>>&, const std::string&);
        /*
            Load the void machine_learning_result(); result
            para1: modle file name path
            para2: output-> std::unordered_map<std::string, cv::Mat>& summarizedDataset
            para3: output-> std::unordered_map<std::string, std::vector<cv::KeyPoint>>& summarizedKeypoints
        */
        void load_trained_model(const std::string&,std::unordered_map<std::string, cv::Mat>&, 
               std::unordered_map<std::string, std::vector<cv::KeyPoint>>&);
        /*
            Function to put image2 to image1, and share the lighting effect and texture of image1
            para1: image 1 (target image) path
            para2: image 2 path
            para3: image 2's width
            para4: image 2's height
            para5: image 2's left on the image1
            para6: image 2's top on the image1
            para7: image 2's alpha modulation 0-255 value to apply transparency.
                   0: Fully transparent; the texture will not be visible at all.
                   255: Fully opaque; the texture will be fully visible without any
            para8: output image path
            para9: target image1 width
            para10: target image1 height
            para11: output image width
            para12: output image height
            para13: 
                int depth:
                Purpose: The depth of the surface in bits per pixel (bpp). The value 32 in this context indicates that you're allocating space for 32 bits per pixel (typically used for a standard pubstructs::RGBA format with 8 bits per color channel).
                Example Values: 32 for pubstructs::RGBA (4 bytes per pixel: 8 bits each for red, green, blue, and alpha).
        */
        cv::Mat put_img2_in_img1(
            const std::string&, 
            const std::string&, 
            const int,
            const int,
            const int,
            const int,
            const uint8_t, 
            const std::string&,
            const unsigned int, 
            const unsigned int, 
            const unsigned int, 
            const unsigned int, 
            unsigned int
            );
        
        /*
            Function to input an image and return all objects in it
            para1: input an image file path
            para2: pass const std::unordered_map<std::string, std::vector<std::pair<int, int>>>& multi-objects recognition corpus
            para3: learning rate (much be the same as function: get_outliers_for_ml)
            return: std::unordered_map<std::string,std::pair<std::vector<unsigned i nt>,std::vector<unsigned int>>>
        */
        std::vector<the_obj_in_an_image> what_are_these(const std::string&);
};     

#ifdef __cplusplus
}
#endif

#endif