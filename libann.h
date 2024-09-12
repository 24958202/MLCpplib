#ifndef LIBANN_H  
#define LIBANN_H  

#include <iostream>  
#include <vector>  
#include <functional>
#include <string>
/*  
    ANN Library (Artificial Neural Networks) are a type of machine learning  
    algorithm that are designed to simulate the structure, function, and learning  
    process of the human brain.  
*/  
class libann {  
public:  
    // Define a struct to represent a neuron  
    struct Neuron {  
        double value;                     // The output value of the neuron  
        std::vector<double> weights;      // Weights for the inputs to this neuron  
        double delta;                     // The error term for backpropagation  
        Neuron() : value(0.0), delta(0.0) {} // Constructor initializing value and delta  
    };  
    // Define a struct to represent a layer of neurons  
    struct Layer {  
        std::vector<Neuron> neurons; // A layer consists of multiple neurons  
    };  
    // Member variables  
    std::vector<Layer> layers; // Vector to hold layers of the neural network  
    // Constructor  
    libann(const std::vector<unsigned int>& layerSizes);  
    // Member functions  
    std::vector<double> feedForward(const std::vector<double>& inputs);  
    /*
        para1: trainingData
        para2: labels
        para3: numEpochs
        para4: learning rate
        para5: model file
        para6: callback function
        Example:
        std::vector<std::vector<double>> trainingData;  
        std::vector<unsigned int> labels;  
        LoadMNISTData("/Users/dengfengji/ronnieji/libs/mnist/train-images-idx3-ubyte",  
                    "/Users/dengfengji/ronnieji/libs/mnist/train-labels-idx1-ubyte",   
                    trainingData, labels);
        //pass the trainingData and labels into train function
        std::vector<unsigned int> layerSizes = {784, 100, 10};  
        libann nn(layerSizes);  
        nn.train(trainingData, labels, 10, 0.07, "/Users/dengfengji/ronnieji/libs/mnist/model.dat",   
              [&nn, logFilePath](const std::string&logMessage,const std::string&) {  
                  nn.callbackfun(logMessage, logFilePath); // Pass logMessage and logFilePath to the callback function  
              });    
    */
    void train(
        const std::vector<std::vector<double>>& trainingData, 
        const std::vector<unsigned int>& labels, 
        unsigned int numEpochs, 
        double learningRate,
        const std::string& file_path, 
        const std::string& logFile_path,
        std::function<void(const std::string&, const std::string&)> callback);  
    /*
        load the training model
        para1: model file path
    */
    void loadModel(const std::string& filename);
};
#endif // LIBANN_H