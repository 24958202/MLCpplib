/*
    The following code can quickly picking up the edge of objects in a image, and output the selection to an image
*/
#include <opencv2/opencv.hpp>  
#include <Eigen/Dense>  
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
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>

struct RGB {  
    size_t r;  
    size_t g;  
    size_t b;  
};  
/*
    This function to convert an cv::Mat into a std::vector<std::vector<RGB>> dataset
*/
std::vector<std::vector<RGB>> cv_mat_to_dataset(const cv::Mat& genImg){
    std::vector<std::vector<RGB>> datasets(genImg.rows,std::vector<RGB>(genImg.cols));
    for (int i = 0; i < genImg.rows; ++i) {  
        for (int j = 0; j < genImg.cols; ++j) {  
            // Get the intensity value  
            uchar intensity = genImg.at<uchar>(i, j);  
            // Populate the RGB struct for grayscale  
            datasets[i][j] = {static_cast<size_t>(intensity), static_cast<size_t>(intensity), static_cast<size_t>(intensity)};  
        }  
    }  
    return datasets;
}
/*
    cv::Scalar markerColor(0,255,0);
    cv::Scalar txtColor(255, 255, 255);
    rec_thickness = 2;
*/
// Function to draw a green rectangle on an image  
void drawRectangleWithText(cv::Mat& image, int x, int y, int width, int height, const std::string& text, int rec_thickness, const cv::Scalar& markerColor, const cv::Scalar& txtColor) {  
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
void savePPM(const cv::Mat& image, const std::string& filename) {  
    std::ofstream ofs(filename, std::ios::binary);  
    if (!ofs) {  
        std::cerr << "Error opening file for writing: " << filename << std::endl;  
        return;  
    }  
    ofs << "P6\n" << image.cols << ' ' << image.rows << "\n255\n";  
    ofs.write(reinterpret_cast<const char*>(image.data), image.total() * image.elemSize());  
}  
std::vector<std::vector<RGB>> get_img_matrix(const std::string& imgPath){
    std::vector<std::vector<RGB>> datasets(1000, std::vector<RGB>(1000)); // Create a vector of vectors for RGB values  
    // Read the image using imread function  
    cv::Mat image = cv::imread(imgPath, cv::IMREAD_COLOR);  
    if(image.empty()){  
        std::cerr << "Error: Could not open or find the image." << std::endl;  
        return datasets;   
    }  
    cv::Mat resized_image;  
    cv::resize(image, resized_image, cv::Size(1000, 1000));  
    cv::Mat gray_image;  
    cv::cvtColor(resized_image,gray_image,cv::COLOR_BGR2GRAY); 
    /*---------------------------------------------------------*/
    // std::string output_folder = "/home/ronnieji/lib/images/output/grayscale_image.jpg";
    // cv::imwrite(output_folder,gray_image);
    /*---------------------------------------------------------*/ 
    datasets = cv_mat_to_dataset(gray_image);
    return datasets;  
}
/*
    Read all images from a folder
*/
std::multimap<std::string, std::vector<RGB>> read_images(const std::string& folderPath){  
    std::multimap<std::string, std::vector<RGB>> datasets;  
    if(folderPath.empty()){  
        return datasets;  
    }  
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {  
        if (entry.is_regular_file()) {  
            std::string imgPath = entry.path().filename().string();
            std::string nuts_key = folderPath + "/" + imgPath;
            std::vector<std::vector<RGB>> image_rgb = get_img_matrix(nuts_key);  
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
std::vector<std::pair<int, int>> findOutlierEdges(const std::vector<std::vector<RGB>>& data) {  
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
            if (gradientMagnitude > 50) { // Adjust threshold as needed  
                outliers.emplace_back(i, j);  
            }  
        }  
    }  
    return outliers;  
}  
// Function to mark outliers in the image data  
void markOutliers(std::vector<std::vector<RGB>>& data, const std::vector<std::pair<int, int>>& outliers) {  
    // Find the bounding rectangle for drawing  
    if (outliers.empty()) return;  
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
        data[x][y] = {0, 255, 0}; // Mark in green  
    }  
}  
// Function to save the image to a PPM file  
bool saveImage(const std::vector<std::vector<RGB>>& data, const std::string& filename) {  
   std::ofstream file(filename);  
    if (!file.is_open()) {  
        //file.open(filename);
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;  
        return false; // Return false if the file couldn't be opened  
    }  
    int width = data.size();  
    int height = data[0].size();   
    // PPM file header  
    file << "P3\n" << width << " " << height << "\n255\n"; // PPM header  
    for (int i = 0; i < width; ++i) {  
        for (int j = 0; j < height; ++j) {  
            const RGB& rgb = data[i][j];  
            file << rgb.r << " " << rgb.g << " " << rgb.b << "\n";  
        }  
    }   
    file.close();  
    std::cout << "Image saved as " << filename << std::endl;  
    return true; // Return true if saving was successful  
}  
// Function to create an output image with only outliers  
void createOutlierImage(const std::vector<std::vector<RGB>>& originalData, const std::vector<std::pair<int, int>>& outliers) {  
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
    cv::Mat outputImage(outputHeight, outputWidth, CV_8UC3, cv::Scalar(0, 0, 0)); // Black background  
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

     //draw rectangles on each objects
    cv::Scalar markColor(0,255,0);
    cv::Scalar txtColor(255,255,255);
    drawRectangleWithText(outputImage, 50, 250, 200, 300, "This is a nuts", 2, markColor, txtColor);
    // Save the output image in various formats  
    savePPM(outputImage, "/home/ronnieji/lib/images/output/outlier_image.ppm");  
    cv::imwrite("/home/ronnieji/lib/images/output/outlier_image.png", outputImage);  
    cv::imwrite("/home/ronnieji/lib/images/output/outlier_uncompressed_image.png", outputImage, {cv::IMWRITE_PNG_COMPRESSION, 0});  
    cv::imwrite("/home/ronnieji/lib/images/output/outlier_compressed9_image.png", outputImage, {cv::IMWRITE_PNG_COMPRESSION, 9});  
    cv::imwrite("/home/ronnieji/lib/images/output/outlier_image.bmp", outputImage);  
   
    std::cout << "Outlier image saved successfully." << std::endl;  
}
void read_image_detect_edges(const std::string& imagePath){
    if(imagePath.empty()){
        return;
    }
    std::vector<RGB> pixelToPaint;
    std::vector<std::vector<RGB>> image_rgb = get_img_matrix(imagePath);  
    // Find outliers (edges)  
    auto outliers = findOutlierEdges(image_rgb);  
    // Mark outliers in green  
    markOutliers(image_rgb, outliers);  
    // //Save the modified image to a file  
    if (!saveImage(image_rgb, "/home/ronnieji/lib/images/output/sample2.ppm")) {  
        std::cout << "Failed to save outliers_image." << std::endl; // Exit if failed to save the image  
    }  
    //save selection images
    createOutlierImage(image_rgb, outliers);
}
int main(){
    read_image_detect_edges("/home/ronnieji/lib/images/sample2.jpg");
    return 0;
}
