#ifndef LIBANN_H  
#define LIBANN_H  

#include <iostream>  
#include <vector>  

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
        the last parameter is the output model file path
    */
    void train(const std::vector<std::vector<double>>& trainingData, const std::vector<unsigned int>& labels, unsigned int numEpochs, double learningRate,const std::string& file_path);  
    /*
        load the training model
        para1: model file path
    */
    void loadModel(const std::string& filename);
};
#endif // LIBANN_H