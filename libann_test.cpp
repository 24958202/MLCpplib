#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <opencv2/opencv.hpp> 
#include <unordered_map>
#include <ctime>  
#include <iomanip> // For std::put_time  
#include "libann.h"
struct CurrentDateTime {  
    std::string current_date;  
    std::string current_time;  
};  
void callbackfun(const std::string& strLog, const std::string& log_path) {  
    auto current_time = std::chrono::system_clock::now();  
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);  
    std::tm* current_time_tm = std::localtime(&current_time_t);  
    CurrentDateTime currentDateTime;  
    std::ostringstream dateStream, timeStream;  
    dateStream << std::put_time(current_time_tm, "%Y-%m-%d");  
    timeStream << std::put_time(current_time_tm, "%H:%M:%S");  
    currentDateTime.current_date = dateStream.str();  
    currentDateTime.current_time = timeStream.str();  
    std::ostringstream ss;  
    ss << currentDateTime.current_date << " " << currentDateTime.current_time << " --> " << strLog;  
    std::string strMsg = ss.str();  
    std::ofstream ofile(log_path, std::ios::app);  
    if (!ofile) {  
        std::cerr << "Failed to open log file: " << log_path << std::endl;  
        return;  
    }  
    ofile << strMsg << '\n';  
    ofile.close();  
    std::cout << strMsg << std::endl;  
}  
void LoadMNISTData(const std::string& imagesFile, const std::string& labelsFile,   
                   std::vector<std::vector<double>>& trainingData, std::vector<unsigned int>& labels) {  
    std::ifstream images(imagesFile, std::ios::binary);  
    std::ifstream labelsStream(labelsFile, std::ios::binary);  
    if (!images.is_open() || !labelsStream.is_open()) {  
        std::cerr << "Failed to open MNIST data files." << std::endl;  
        return;  
    }  
    unsigned int magicNumber, numImages, numRows, numCols;  
    images.read(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));  
    images.read(reinterpret_cast<char*>(&numImages), sizeof(numImages));  
    images.read(reinterpret_cast<char*>(&numRows), sizeof(numRows));  
    images.read(reinterpret_cast<char*>(&numCols), sizeof(numCols));  
    labelsStream.read(reinterpret_cast<char*>(&magicNumber), sizeof(magicNumber));  
    labelsStream.read(reinterpret_cast<char*>(&numImages), sizeof(numImages));  
    for (unsigned int i = 0; i < numImages; i++) {  
        std::vector<double> imageData(numRows * numCols);  
        for (unsigned int j = 0; j < numRows * numCols; j++) {  
            unsigned char pixel;  
            images.read(reinterpret_cast<char*>(&pixel), sizeof(pixel));  
            imageData[j] = static_cast<double>(pixel) / 255.0; // Normalize pixel values  
        }  
        unsigned char label;   
        labelsStream.read(reinterpret_cast<char*>(&label), sizeof(label)); // Read the label after reading the image  
        unsigned int digit = static_cast<unsigned int>(label); // Cast label  
        trainingData.push_back(imageData);  
        labels.push_back(digit);  
    }  
    images.close(); // Close files after the loop  
    labelsStream.close();  
}
int main(){
    // // Step 1: Load MNIST training data  
    std::vector<std::vector<double>> trainingData;  
    std::vector<unsigned int> labels;  
    LoadMNISTData("/Users/dengfengji/ronnieji/libs/mnist/train-images-idx3-ubyte",  
                   "/Users/dengfengji/ronnieji/libs/mnist/train-labels-idx1-ubyte",   
                   trainingData, labels);  
    std::string logFilePath = "/Users/dengfengji/ronnieji/libs/mnist/logFile.txt";  
    // Step 2: Set up and train the neural network  
    std::vector<unsigned int> layerSizes = {784, 100, 10};  
    libann nn(layerSizes);  
    // Assuming `trainingData` and `labels` are defined and initialized  
    nn.train(trainingData, labels, 10, 0.07,   
          "/Users/dengfengji/ronnieji/libs/mnist/model.dat", // file_path  
          logFilePath, // logFile_path  
          [&nn, logFilePath](const std::string&logMessage,const std::string&) {  
              callbackfun(logMessage, logFilePath); // Pass logMessage and logFilePath to the callback function  
          });  
    // Model is saved within the train method  
    // Step 3: Load the trained model into a new instance  
    // Load the image  
    // Image processing and evaluation  
    std::vector<std::string> images{  
        "/Users/dengfengji/ronnieji/lib/images/2.jpg",  
        "/Users/dengfengji/ronnieji/lib/images/5.jpg",  
        "/Users/dengfengji/ronnieji/lib/images/6.jpg",  
        "/Users/dengfengji/ronnieji/lib/images/8.jpg"  
    };  
    std::unordered_map<std::string, std::vector<double>> img_inputs;  
    for (const auto& im : images) {  
        auto read_img = cv::imread(im, cv::IMREAD_GRAYSCALE);  
        if (read_img.empty()) {  
            std::cerr << "Error loading image: " << im << std::endl;  
            continue; // Skip this image and move to the next  
        }  
        // Resize the image if necessary (e.g., 28x28 for MNIST)  
        cv::resize(read_img, read_img, cv::Size(28, 28));  
        // Convert the image to a vector of doubles  
        std::vector<double> inputs;  
        for (int i = 0; i < read_img.rows; ++i) {  
            for (int j = 0; j < read_img.cols; ++j) {  
                // Normalize pixel values to [0, 1]  
                inputs.push_back(read_img.at<uchar>(i, j) / 255.0);  
            }  
        }  
        img_inputs[im] = inputs;  
    }  
    libann nnLoaded(layerSizes);  
    nnLoaded.loadModel("/Users/dengfengji/ronnieji/libs/mnist/model.dat");  
    // Step 4: Evaluate the model (use the same training data or new data)  
    std::cout << "Evaluating loaded model on training data:" << std::endl;  
    if (!img_inputs.empty()) {  
        for (const auto& item : img_inputs) {  
            std::vector<double> output = nnLoaded.feedForward(item.second);  
            if (!output.empty()) {  
                unsigned int predictedLabel = std::distance(output.begin(), std::max_element(output.begin(), output.end()));  
                std::cout << "Input: " << item.first << " Predicted: " << predictedLabel << std::endl;  
            } else {  
                std::cerr << "Error: No output from feedForward for input: " << item.first << std::endl;  
            }  
        }  
    }  
    return 0;
}