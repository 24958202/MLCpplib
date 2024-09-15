#ifndef LIBANN_H  
#define LIBANN_H  

#include <iostream>  
#include <vector>  
#include <functional>
#include <string>
#include <random>
#include <cmath>
#include <functional> 
/*  
    ANN Library (Artificial Neural Networks) are a type of machine learning  
    algorithm that are designed to simulate the structure, function, and learning  
    process of the human brain.  
*/  
class libann_relu {  
public:  
    struct Neuron {  
        double value;  
        std::vector<double> weights;  
        double delta;  
        Neuron() : value(0.0), delta(0.0) {}  
    };  
    struct Layer {  
        std::vector<Neuron> neurons;  
    };  
    std::vector<Layer> layers;  
    libann_relu(const std::vector<unsigned int>& layerSizes);  
    std::vector<double> feedForward(const std::vector<double>& inputs);  
    void train(  
        const std::vector<std::vector<double>>& trainingData,  
        const std::vector<unsigned int>& labels,  
        unsigned int numEpochs,  
        double learningRate,  
        const std::string& file_path,  
        std::function<void(const std::string&)> callback  
    );  
    void loadModel(const std::string& filename);  
private:  
    double relu(double x) {  //image LLM:sigmon
        return std::max(0.0, x);  
    }  
    double reluDerivative(double x) {  
        return x > 0 ? 1.0 : 0.0;  
    }  
    double randomWeight() {  
        static std::random_device rd;  
        static std::mt19937 gen(rd());  
        static std::uniform_real_distribution<> dis(-1.0, 1.0);  
        return dis(gen);  
    }  
}; 
#endif // LIBANN_H