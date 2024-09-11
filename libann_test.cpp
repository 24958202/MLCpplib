#include <iostream>
#include <string>
#include <fstream>
#include "libann.h"
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
    // Step 1: Load MNIST training data  
    std::vector<std::vector<double>> trainingData;  
    std::vector<unsigned int> labels;  
    LoadMNISTData("/Users/dengfengji/ronnieji/libs/mnist/train-images-idx3-ubyte",  
                   "/Users/dengfengji/ronnieji/libs/mnist/train-labels-idx1-ubyte",   
                   trainingData, labels);  
    
    // Step 2: Set up and train the neural network  
    std::vector<unsigned int> layerSizes = {784, 100, 10};  
    libann nn(layerSizes);  
    nn.train(trainingData, labels, 10, 0.01,"/Users/dengfengji/ronnieji/libs/mnist/model.dat");  
    
    // Model is saved within the train method  
    // Step 3: Load the trained model into a new instance  
    libann nnLoaded(layerSizes);  
    nnLoaded.loadModel("/Users/dengfengji/ronnieji/libs/mnist/model.dat");  

    // Step 4: Evaluate the model (use the same training data or new data)  
    std::cout << "Evaluating loaded model on training data:" << std::endl;  
    for (unsigned int i = 0; i < 100; ++i) {  
        std::vector<double> output = nnLoaded.feedForward(trainingData[i]);  
        
        // Find the predicted class (index of max output)  
        unsigned int predictedLabel = std::distance(output.begin(), std::max_element(output.begin(), output.end()));  
        std::cout << "Input: " << i << " Predicted: " << predictedLabel << " True: " << labels[i] << std::endl;  
    }  
    return 0;
}