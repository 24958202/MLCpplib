/*
    c++20 lib for using opencv
*/
#include <opencv2/opencv.hpp>  
#include <opencv2/features2d.hpp> 
#include <tesseract/baseapi.h>  
#include <tesseract/publictypes.h>  
#include <Eigen/Dense> 
#include "authorinfo/author_info.h" 
#include <vector>  
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
#include <ranges> //std::views
#include <cstdint> 
#include <functional>
#include <cstdlib>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "cvLib.h"
class subfunctions{
    public:
        void convertToBlackAndWhite(cv::Mat&, std::vector<std::vector<RGB>>&);
        // Function to convert a dataset to cv::Mat  
        cv::Mat convertDatasetToMat(const std::vector<std::vector<RGB>>&);
        void markVideo(cv::Mat&, const cv::Scalar&);
        
};
void subfunctions::convertToBlackAndWhite(cv::Mat& image, std::vector<std::vector<RGB>>& datasets) {   
    for (int i = 0; i < image.rows; ++i) {  
        for (int j = 0; j < image.cols; ++j) {  
            // Access the pixel and its RGB values  
            cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);  
            int r = pixel[2];  
            int g = pixel[1];  
            int b = pixel[0];  
            // Calculate the grayscale value using the luminance method  
            int grayValue = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);  
            // Determine the binary value with thresholding  
            int bwValue = (grayValue < 128) ? 0 : 255;  
            // Assign the calculated binary value to the image  
            pixel[0] = bwValue;  
            pixel[1] = bwValue;  
            pixel[2] = bwValue;  
            // Update the corresponding dataset  
            datasets[i][j] = {bwValue, bwValue, bwValue};  
        }  
    }  
}  
cv::Mat subfunctions::convertDatasetToMat(const std::vector<std::vector<RGB>>& dataset) { 
    if (dataset.empty() || dataset[0].empty()) {  
        throw std::runtime_error("Dataset is empty or has no columns.");  
    }   
    int rows = dataset.size();  
    int cols = dataset[0].size();  
    cv::Mat image(rows, cols, CV_8UC3); // Create a Mat with 3 channels (BGR)  
    for (int i = 0; i < rows; ++i) {  
        for (int j = 0; j < cols; ++j) {  
            // Access the RGB values from the dataset  
            const RGB& rgb = dataset[i][j];  
            // Set the pixel in the cv::Mat  
            image.at<cv::Vec3b>(i, j) = cv::Vec3b(rgb.b, rgb.g, rgb.r); // OpenCV uses BGR format  
        }  
    }  
    return image;  
}  
void subfunctions::markVideo(cv::Mat& frame,const cv::Scalar& brush_color){
    if(frame.empty()){
        return;
    }
    cvLib cvl_j;
    std::vector<std::vector<RGB>> frame_to_mark = cvl_j.cv_mat_to_dataset(frame);
    if(!frame_to_mark.empty()){
        std::vector<std::pair<int, int>> outliers_found = cvl_j.cvLib::findOutlierEdges(frame_to_mark, 70);
        if(!outliers_found.empty()){
            cvl_j.markOutliers(frame_to_mark,outliers_found,brush_color);
            frame = this->convertDatasetToMat(frame_to_mark);
        }
    }
}
/*
    This function to convert an cv::Mat into a std::vector<std::vector<RGB>> dataset
*/
std::vector<std::vector<RGB>> cvLib::cv_mat_to_dataset(const cv::Mat& genImg){
    std::vector<std::vector<RGB>> datasets(genImg.rows,std::vector<RGB>(genImg.cols));
    for (int i = 0; i < genImg.rows; ++i) {  //rows
        for (int j = 0; j < genImg.cols; ++j) {  //cols
            // Get the intensity value  
            uchar intensity = genImg.at<uchar>(i, j);  
            // Populate the RGB struct for grayscale  
            datasets[i][j] = {static_cast<int>(intensity), static_cast<int>(intensity), static_cast<int>(intensity)};  
        }  
    }  
    return datasets;
}
imgSize cvLib::get_image_size(const std::string& imgPath){
    imgSize im_s;
    if(imgPath.empty()){
        return im_s;
    }
    cv::Mat img = cv::imread(imgPath);
    im_s.width = img.cols;
    im_s.height = img.rows;
    return im_s; 
}
/*
    cv::Scalar markerColor(0,255,0);
    cv::Scalar txtColor(255, 255, 255);
    rec_thickness = 2;
*/
// Function to draw a green rectangle on an image  
void cvLib::drawRectangleWithText(cv::Mat& image, int x, int y, int width, int height, const std::string& text, int rec_thickness, const cv::Scalar& markerColor, const cv::Scalar& txtColor) {  
    // Define rectangle vertices  
    cv::Point top_left(x, y);  
    cv::Point bottom_right(x + width, y + height);  
    // Draw the rectangle  
    cv::rectangle(image, top_left, bottom_right, markerColor, rec_thickness); // Rectangle with specified thickness and color  
    // Define text position at the upper-left corner of the rectangle  
    cv::Point text_position(x + 5, y - 15); // Adjust as necessary, '5' from left and '20' from top for padding  
    // Add text  
    cv::putText(image, text, text_position, cv::FONT_HERSHEY_SIMPLEX, 0.6, txtColor, 1); // Text positioning based on updated text_position  
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
void cvLib::saveVectorRGB(const std::vector<std::vector<RGB>>& img, int width, int height, const std::string& filename){
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
std::vector<std::vector<RGB>> cvLib::get_img_matrix(const std::string& imgPath, int img_rows, int img_cols){
    std::vector<std::vector<RGB>> datasets(img_rows, std::vector<RGB>(img_cols)); // Create a vector of vectors for RGB values  
    // Read the image using imread function  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if(image.empty()){  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(img_rows, img_cols));  
    cv::Mat gray_image;  
    cv::cvtColor(resized_image,gray_image,cv::COLOR_BGR2GRAY); 
    /*---------------------------------------------------------*/
    // std::string output_folder = "/home/ronnieji/lib/images/output/grayscale_image.jpg";
    // cv::imwrite(output_folder,gray_image);
    /*---------------------------------------------------------*/ 
    datasets = cv_mat_to_dataset(gray_image);
    return datasets;  
}
std::multimap<std::string, std::vector<RGB>> cvLib::read_images(std::string& folderPath){  
    std::multimap<std::string, std::vector<RGB>> datasets;  
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
            std::vector<std::vector<RGB>> image_rgb = this->get_img_matrix(nuts_key,img_size.width,img_size.height);  
            std::vector<RGB> one_d_image_rgb;
            for(const auto& mg : image_rgb){
                for(const auto& m : mg){
                    RGB data_add = m;
                    one_d_image_rgb.push_back(data_add);
                }
            }
            datasets.insert({nuts_key,one_d_image_rgb});
        }  
    }  
    return datasets;  
}
// Function to find outlier edges using a simple gradient method  
std::vector<std::pair<int, int>> cvLib::findOutlierEdges(const std::vector<std::vector<RGB>>& data, int gradientMagnitude_threshold) {  
    std::vector<std::pair<int, int>> outliers;  
    int height = data.size();  
    int width = data[0].size();  
    for (int i = 1; i < height - 1; ++i) {  
        for (int j = 1; j < width - 1; ++j) {  
            // Get pixel values  
            const RGB& pixel = data[i][j];  
            // Calculate the gradient  
            int gx = -data[i-1][j-1].r + data[i+1][j+1].r +  
                     -2 * data[i][j-1].r + 2 * data[i][j+1].r +  
                     -data[i-1][j+1].r + data[i+1][j-1].r;  
            int gy = data[i-1][j-1].r + 2 * data[i-1][j].r + data[i-1][j+1].r -  
                     (data[i+1][j-1].r + 2 * data[i+1][j].r + data[i+1][j+1].r);  
            // Calculate the gradient magnitude  
            double gradientMagnitude = std::sqrt(gx * gx + gy * gy);  
            // Define a threshold for detecting edges  
            if (gradientMagnitude > gradientMagnitude_threshold) { // Adjust threshold as needed  
                outliers.emplace_back(i, j);  
            }  
        }  
    }  
    return outliers;  
}
bool cvLib::saveImage(const std::vector<std::vector<RGB>>& data, const std::string& filename){ 
     if (data.empty() || data[0].empty()){   
        std::cerr << "Error: Image data is empty." << std::endl;  
        return false;  
    }
    std::ofstream file(filename);  
    if (!file.is_open()){   
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;  
        return false; // Return false if the file couldn't be opened  
    }
    int width = data[0].size(); // Corrected width and height calculation  
    int height = data.size();   
    file << "P3\n" << width << " " << height << "\n255\n"; // PPM header  
    for (int i = 0; i < height; ++i){   
        for (int j = 0; j < width; ++j){   
            const RGB& rgb = data[i][j];  
            file << rgb.r << " " << rgb.g << " " << rgb.b << "\n";  
        }
    }
    file.close();  
    std::cout << "Image saved as " << filename << std::endl;  
    return true; // Return true if saving was successful
}
void cvLib::markOutliers(std::vector<std::vector<RGB>>& data, const std::vector<std::pair<int, int>>& outliers, const cv::Scalar& markerColor) {  
    if (data.empty() || data[0].empty()) {  
        return;  
    }  
    // Find the bounding rectangle for drawing  
    if (outliers.empty()) return;  
    bool isEmpty = (markerColor[0]==0 && markerColor[1]==0 && markerColor[2]==0);
    if (isEmpty) return;
    int minX = std::numeric_limits<int>::max();  
    int minY = std::numeric_limits<int>::max();  
    int maxX = std::numeric_limits<int>::min();  
    int maxY = std::numeric_limits<int>::min();  
    for (const auto& outlier : outliers) {  
        /*
            //Mark the green line around the edge of the object
            int x = outlier.first;  
            int y = outlier.second;  
            data[x][y] = {0, 255, 0}; // Mark in green  
        */
        /*
            Check if the outlier is within the selections rectangle
            draw a rectangle around the object
        */
        int x = outlier.first;  
        int y = outlier.second;  
        minX = std::min(minX, x);  
        minY = std::min(minY, y);  
        maxX = std::max(maxX, x);  
        maxY = std::max(maxY, y);  
        // Mark the outlier in green  
        data[x][y] = {static_cast<int>(markerColor[0]), static_cast<int>(markerColor[1]), static_cast<int>(markerColor[2])}; // Mark in green   
    }  
}
void cvLib::createOutlierImage(const std::vector<std::vector<RGB>>& originalData, const std::vector<std::pair<int, int>>& outliers, const std::string& outImgPath, const cv::Scalar& bgColor){
    // Calculate the minimum and maximum coordinates of the outliers  
    if (outliers.empty()) {  
        std::cerr << "No outliers provided." << std::endl;  
        return;  
    }  
    int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;  
    for (const auto& outlier : outliers) {  
        minX = std::min(minX, outlier.first);  
        minY = std::min(minY, outlier.second);  
        maxX = std::max(maxX, outlier.first);  
        maxY = std::max(maxY, outlier.second);  
    }  
    // Calculate width and height based on outliers  
    int outputWidth = maxY - minY + 1;  // Width of the rectangle  
    int outputHeight = maxX - minX + 1; // Height of the rectangle  
    // Create a blank image with a black background  
    cv::Mat outputImage(outputHeight, outputWidth, CV_8UC3, bgColor); // Black background  
    // Draw outliers in the output image  
    for (const auto& outlier : outliers) {  
        int outlierX = outlier.first; // original image x-coordinate  
        int outlierY = outlier.second; // original image y-coordinate  
        // Ensure the outlier is within the bounds of the original image  
        if (outlierX < originalData.size() && outlierY < originalData[0].size()) {  
            // Calculate position in the output image  
            int newX = outlierY - minY; // Adjusted X for output image  
            int newY = outlierX - minX; // Adjusted Y for output image  
            // Get the RGB values from the original data  
            RGB pixel = originalData[outlierX][outlierY];  
            outputImage.at<cv::Vec3b>(newY, newX) = cv::Vec3b(pixel.b, pixel.g, pixel.r); // BGR format for OpenCV  
        }  
    }  
    // Save the output image in various formats  
    this->savePPM(outputImage, outImgPath); 
    this->saveImage(originalData,outImgPath + "_orig.ppm");
    cv::imwrite(outImgPath, outputImage);  
    // cv::imwrite("/home/ronnieji/lib/images/output/outlier_uncompressed_image.png", outputImage, {cv::IMWRITE_PNG_COMPRESSION, 0});  
    // cv::imwrite("/home/ronnieji/lib/images/output/outlier_compressed9_image.png", outputImage, {cv::IMWRITE_PNG_COMPRESSION, 9});  
    // cv::imwrite("/home/ronnieji/lib/images/output/outlier_image.bmp", outputImage);  
    std::cout << "Outlier image saved successfully." << std::endl;  
}
void cvLib::read_image_detect_edges(const std::string& imagePath,int gradientMagnitude_threshold,const std::string& outImgPath,const brushColor& markerColor, const brushColor& bgColor){
    if(imagePath.empty()){
        return;
    }
    std::vector<RGB> pixelToPaint;
    imgSize img_size = get_image_size(imagePath);
    std::vector<std::vector<RGB>> image_rgb = this->get_img_matrix(imagePath,img_size.width,img_size.height);  
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
void cvLib::convertToBlackAndWhite(const std::string& filename, std::vector<std::vector<RGB>>& datasets, int width, int height) {   
    if (filename.empty()) {  
        return;  
    }   
    cv::Mat image = cv::imread(filename, cv::IMREAD_COLOR);  
    if (image.empty()) {  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return;   
    }  
    for (int i = 0; i < image.rows; ++i) {  
        for (int j = 0; j < image.cols; ++j) {  
            // Access the pixel and its RGB values  
            cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);  
            int r = pixel[2];  
            int g = pixel[1];  
            int b = pixel[0];  
            // Calculate the grayscale value using the luminance method  
            int grayValue = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);       
            // Determine the binary value with thresholding  
            int bwValue = (grayValue < 128) ? 0 : 255;   
            // Assign the calculated binary value to the image  
            pixel[0] = bwValue;  
            pixel[1] = bwValue;  
            pixel[2] = bwValue;  
            // Update the corresponding dataset  
            datasets[i][j] = {bwValue, bwValue, bwValue};  
        }  
    }  
}  
bool cvLib::read_image_detect_objs(const std::string& img1, const std::string& img2, int featureCount, float ratioThresh, int de_threshold) {  
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
    if (m_img1.channels() == 3) {  
        cv::cvtColor(m_img1, gray1, cv::COLOR_BGR2GRAY);  
    } else {  
        gray1 = m_img1;  
    }  
    if (m_img2.channels() == 3) {  
        cv::cvtColor(m_img2, gray2, cv::COLOR_BGR2GRAY);  
    } else {  
        gray2 = m_img2;  
    }  
    // Use ORB for keypoint detection and description  
    cv::Ptr<cv::ORB> detector = cv::ORB::create(featureCount); // Adjust number of features as needed  
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
    const float ratio_thresh = ratioThresh;  
    std::vector<cv::DMatch> goodMatches;  
    for (size_t i = 0; i < knnMatches.size(); i++) {  
        if (knnMatches[i][0].distance < ratio_thresh * knnMatches[i][1].distance) {  
            goodMatches.push_back(knnMatches[i][0]);  
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
bool cvLib::isObjectInImage(const std::string& img1, const std::string& img2, int featureCount, float ratioThresh, int deThreshold) {  
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
    std::vector<std::vector<RGB>> dataset_img1(m_img1.rows, std::vector<RGB>(m_img1.cols));  
    std::vector<std::vector<RGB>> dataset_img2(m_img2.rows, std::vector<RGB>(m_img2.cols));  
    subfunctions sub_j;
    sub_j.convertToBlackAndWhite(m_img1, dataset_img1);
    sub_j.convertToBlackAndWhite(m_img2, dataset_img2);
    cv::Mat gray1 = sub_j.convertDatasetToMat(dataset_img1);
    cv::Mat gray2 = sub_j.convertDatasetToMat(dataset_img2);
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
void cvLib::StartWebCam(int webcame_index,const std::string& winTitle,const std::vector<std::string>& imageListToFind,const cv::Scalar& brush_color,std::function<void(cv::Mat&)> callback){
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
    cv::Mat frame; // To hold each frame captured from the webcam  
    // Capture frames in a loop 
    subfunctions sub_j; 
    while (true) {  
        // Capture a new frame from the webcam  
        cap >> frame; // Alternatively, you can use cap.read(frame);  
        // Check if the frame is empty  
        if (frame.empty()) {  
            std::cerr << "Error: Could not grab a frame." << std::endl;  
            break;  
        }  
        /*
            start marking on the input frame
        */
        //cv::Scalar markColor(0,255,0);
        sub_j.markVideo(frame,brush_color);
        /*
            end marking video
        */
        // Invoke the callback with the captured frame  
        callback(frame);  
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
