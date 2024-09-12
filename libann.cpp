#include "libann.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include "authorinfo/author_info.h"
libann::libann(const std::vector<unsigned int>& layerSizes) {  
    if (layerSizes.empty()) {  
        return;  
    }  
    unsigned int numLayers = layerSizes.size();  
    for (unsigned int i = 0; i < numLayers; i++) {  
        unsigned int numNeurons = layerSizes[i];  
        Layer layer;  
        for (int j = 0; j < numNeurons; j++) {  
            Neuron neuron;  
            neuron.value = 0.0;  
            if (i > 0) {  
                unsigned int numInputs = layerSizes[i - 1];  
                for (unsigned int k = 0; k < numInputs; k++) {  
                    neuron.weights.push_back(0.5); // Initialize weights with 0.5  
                }  
            }  
            layer.neurons.push_back(neuron);  
        }  
        layers.push_back(layer);  
    }  
}  
std::vector<double> libann::feedForward(const std::vector<double>& inputs) {  
    std::vector<double> outputs;  
    if (inputs.empty()) {  
        return outputs;  
    }  
    // Input layer  
    for (unsigned int i = 0; i < inputs.size(); i++) {  
        layers[0].neurons[i].value = inputs[i];  
    }  
    if (layers.size() > 1) {  
        // Hidden layers and output layer  
        for (unsigned int i = 1; i < layers.size(); i++) {  
            for (unsigned int j = 0; j < layers[i].neurons.size(); j++) {  
                double weightedSum = 0.0;  
                // Calculate weighted sum of inputs  
                for (unsigned int k = 0; k < layers[i - 1].neurons.size(); k++) {  
                    weightedSum += layers[i - 1].neurons[k].value * layers[i].neurons[j].weights[k];  
                }  
                // Apply activation function (sigmoid)  
                layers[i].neurons[j].value = 1.0 / (1.0 + exp(-weightedSum));  
            }  
        }  
        // Extract outputs from the last layer  
        unsigned int lastLayerIndex = layers.size() - 1;  
        for (const auto& neuron : layers[lastLayerIndex].neurons) {  
            outputs.push_back(neuron.value);  
        }  
    }  
    return outputs;  
}
void libann::train(const std::vector<std::vector<double>>& trainingData, const std::vector<unsigned int>& labels, unsigned int numEpochs, double learningRate, const std::string& file_path, const std::string& logFile_path, std::function<void(const std::string&, const std::string&)> call_back) {  
    if (trainingData.empty() || labels.empty()) {  
        return;  
    }  
    std::ostringstream ss;
    unsigned int numTrainingSamples = trainingData.size();  
    ss << "Start training... Total sizes: " << std::to_string(numTrainingSamples);  
    if(call_back){
        call_back(ss.str(),logFile_path);
    }
    for (unsigned int epoch = 0; epoch < numEpochs; epoch++) {  
        ss.str("");
        ss << "epoch: " << epoch;
        if(call_back){
            call_back(ss.str(),logFile_path);
        }
        double totalLoss = 0.0;  
        for (unsigned int i = 0; i < numTrainingSamples; i++) {  
            // Forward pass  
            std::vector<double> outputs = this->feedForward(trainingData[i]);  
            // Calculate loss  
            unsigned int correctLabel = labels[i];  
            double loss = 0.0;  
            for (unsigned int j = 0; j < outputs.size(); j++) {  
                double target = (j == correctLabel) ? 1.0 : 0.0;  
                loss += (outputs[j] - target) * (outputs[j] - target);  
            }  
            totalLoss += loss;  
            ss.str("");
            ss << "feedForward: == " << std::to_string(labels[i]) << "Calculate loss: " << std::to_string(totalLoss) << '\n';
            ss << "Start backward pass...";
            if(call_back){
                call_back(ss.str(),logFile_path);
            }
            // Backward pass  
            for (unsigned j = layers.size() - 1; j > 0; j--) {  
                for (unsigned int k = 0; k < layers[j].neurons.size(); k++) {  
                    double delta = 0.0;  
                    if (j == layers.size() - 1) {  
                        double output = layers[j].neurons[k].value;  
                        double target = (k == correctLabel) ? 1.0 : 0.0;  
                        delta = (output - target) * output * (1.0 - output);  
                    } else {  
                        // Calculate delta for hidden layers  
                        for (unsigned int l = 0; l < layers[j + 1].neurons.size(); l++) {  
                            delta += layers[j + 1].neurons[l].weights[k] * layers[j + 1].neurons[l].delta;  
                        }  
                        delta *= layers[j].neurons[k].value * (1.0 - layers[j].neurons[k].value);  
                    }  
                    // Update weights  
                    for (unsigned int l = 0; l < layers[j - 1].neurons.size(); l++) {  
                        double output = layers[j - 1].neurons[l].value;  
                        layers[j].neurons[k].weights[l] -= learningRate * output * delta;  
                    }  
                    layers[j].neurons[k].delta = delta; // Store delta for next layer calculation  
                }  
            }  
            ss.str("");
            ss << "Lable: " << std::to_string(labels[i]) << " done backward pass.";
            if(call_back){
                call_back(ss.str(),logFile_path);
            }
        }  
        ss.str("");
        ss << "Epoch: " << epoch + 1 << " Loss: " << totalLoss / numTrainingSamples; 
        if(call_back){
            call_back(ss.str(),logFile_path);
        }
    }  
    // Save the model  
    std::ofstream outFile(file_path, std::ios::binary);  
    if (!outFile) {  
        std::cerr << "Error: Could not open file for saving the model." << std::endl;  
        return;  // Exit if the file could not be opened  
    }  
    // Save the number of layers  
    unsigned int numLayers = layers.size();  
    outFile.write(reinterpret_cast<const char*>(&numLayers), sizeof(numLayers));  
    // Save each layer's neurons and their weights  
    for (const auto& layer : layers) {  
        unsigned int numNeurons = layer.neurons.size();  
        outFile.write(reinterpret_cast<const char*>(&numNeurons), sizeof(numNeurons));  
        for (const auto& neuron : layer.neurons) {  
            // Save the value of the neuron  
            outFile.write(reinterpret_cast<const char*>(&neuron.value), sizeof(neuron.value));  
            // Save the weights  
            unsigned int numWeights = neuron.weights.size();  
            outFile.write(reinterpret_cast<const char*>(&numWeights), sizeof(numWeights));  
            outFile.write(reinterpret_cast<const char*>(neuron.weights.data()), numWeights * sizeof(double));  
        }  
    }  
    outFile.close();  
}
void libann::loadModel(const std::string& filename) {  
    std::ifstream inFile(filename, std::ios::binary);  
    if (!inFile) {  
        std::cerr << "Error opening file for reading: " << filename << std::endl;  
        return;  
    }  
    // Clear existing layers  
    layers.clear();  
    // Load the number of layers  
    unsigned int numLayers;  
    if (!inFile.read(reinterpret_cast<char*>(&numLayers), sizeof(numLayers))) {  
        std::cerr << "Error reading number of layers from file: " << filename << std::endl;  
        return;  
    }  
    // Load each layer's neurons and their weights  
    for (unsigned int i = 0; i < numLayers; i++) {  
        Layer layer;  
        unsigned int numNeurons;  
        if (!inFile.read(reinterpret_cast<char*>(&numNeurons), sizeof(numNeurons))) {  
            std::cerr << "Error reading number of neurons for layer " << i << std::endl;  
            return;  
        }  
        for (unsigned int j = 0; j < numNeurons; j++) {  
            Neuron neuron;  
            // Load the value of the neuron  
            if (!inFile.read(reinterpret_cast<char*>(&neuron.value), sizeof(neuron.value))) {  
                std::cerr << "Error reading value for neuron " << j << " in layer " << i << std::endl;  
                return;  
            }         
            // Load the weights  
            unsigned int numWeights;  
            if (!inFile.read(reinterpret_cast<char*>(&numWeights), sizeof(numWeights))) {  
                std::cerr << "Error reading number of weights for neuron " << j << " in layer " << i << std::endl;  
                return;  
            }  
            neuron.weights.resize(numWeights);  
            if (!inFile.read(reinterpret_cast<char*>(neuron.weights.data()), numWeights * sizeof(double))) {  
                std::cerr << "Error reading weights for neuron " << j << " in layer " << i << std::endl;  
                return;  
            }  
            layer.neurons.push_back(neuron);  
        }  
        layers.push_back(layer);  
    }  
    inFile.close();  
}  