#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <random>
#include <Eigen/Dense>
#include "../lib/nemslib.h"

const int WINDOW_SIZE = 5; // Size of the context window
const int EMBEDDING_SIZE = 50; // Size of the word embeddings
const double LEARNING_RATE = 0.01; // Learning rate for gradient descent
const int NUM_EPOCHS = 10; // Number of epochs to train for

// Function to convert Eigen::VectorXd to std::vector<double>
std::vector<double> eigenToStdVector(const Eigen::VectorXd& eigenVector) {
    return std::vector<double>(eigenVector.data(), eigenVector.data() + eigenVector.size());
}
// Helper function to initialize word embeddings randomly
// Function to initialize embeddings using Eigen::VectorXd
std::map<std::string, Eigen::VectorXd> initializeEmbeddings(std::map<std::string, int>& vocab) {
    std::map<std::string, Eigen::VectorXd> embeddings;
    
    // Initialize random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-0.5, 0.5);
    
    for (auto& word : vocab) {
        Eigen::VectorXd embedding = Eigen::VectorXd::Random(EMBEDDING_SIZE);
        embedding = (embedding.array() + 0.5) / RAND_MAX; // Scale to range [-0.5, 0.5]
        
        embeddings[word.first] = embedding;
    }
    
    return embeddings;
}
// Helper function to compute the softmax function
std::vector<double> softmax(std::vector<double>& x) {
    std::vector<double> y(x.size());
    double sum = 0;
    for (double xi : x) {
        sum += std::exp(xi);
    }
    for (int i = 0; i < x.size(); i++) {
        y[i] = std::exp(x[i]) / sum;
    }
    return y;
}
// Helper function to compute the dot product of two vectors
double dotProduct(const Eigen::VectorXd& x, const Eigen::VectorXd& y) {
    // Check if the sizes of the vectors match
    if (x.size() != y.size()) {
        // Handle error or return a default value
        return 0.0;
    }

    // Compute the dot product using Eigen's vector operations
    return x.dot(y);
}
Eigen::VectorXd computeInputGradient(const Eigen::VectorXd& outputGradient, const Eigen::VectorXd& outputWeights) {
    Eigen::VectorXd inputGradient = outputGradient.array() * outputWeights.array();
    return inputGradient;
}
void updateEmbeddings(Eigen::VectorXd& embedding, const Eigen::VectorXd& gradient, double LEARNING_RATE) {
    // Check if the gradient vector size matches the embedding vector size
    if (embedding.size() != gradient.size()) {
        std::cerr << "Gradient size does not match embedding size." << std::endl;
        return;
    }

    // Update the embedding using the gradient and learning rate
    embedding -= LEARNING_RATE * gradient;
}
void process_input(const std::string& input_folder_path){
    nemslib nem_j;
    std::vector<std::string> lines = nem_j.readTextFile(input_folder_path);
    std::vector<std::vector<std::string>> corpus;
    std::map<std::string,int> vocab;
    for(std::string line : lines){
        std::cout << line << std::endl;
        std::vector<std::string> tokens = nem_j.tokenize_en(line);
        corpus.push_back(tokens);
        for(std::string token : tokens){
            if(vocab.count(token) == 0){
                vocab[token] = vocab.size(); 
            }
        }
    }
    std::map<std::string,Eigen::VectorXd> embeddings = initializeEmbeddings(vocab);
    for (const auto& pair : embeddings) {
        const std::string& key = pair.first;
        const Eigen::VectorXd& value = pair.second;
        Eigen::VectorXd eigenValue = Eigen::VectorXd::Map(value.data(), value.size());

        embeddings[key] = eigenValue;
    }
    for(int epoch = 1; epoch <= NUM_EPOCHS; epoch++){
        double epochLoss = 0.0;
        for(std::vector<std::string>& sentence : corpus){
            for(int i = 0; i < sentence.size(); i++){
                std::string targetWord = sentence[i];
                std::vector<std::string> contextWords;
                for(int j= i - WINDOW_SIZE; j < i + WINDOW_SIZE; j++){
                    if(j!= i && j > 0 && j < sentence.size()){
                        contextWords.push_back(sentence[j]);
                    }
                }
                Eigen::VectorXd hiddenActivations = Eigen::VectorXd::Zero(EMBEDDING_SIZE);
                for(std::string contextWord : contextWords){
                    hiddenActivations += embeddings[contextWord];
                }
                Eigen::VectorXd outputWeights = embeddings[targetWord];
                double logits = dotProduct(hiddenActivations,outputWeights);
                double loss = -log(std::exp(logits) / (std::exp(logits) + vocab.size() - 1));
                double outputGradient = std::exp(logits) / (std::exp(logits) + vocab.size() - 1) - 1;
                Eigen::VectorXd hiddenGradient = outputGradient * outputWeights;
                Eigen::VectorXd inputGradient = computeInputGradient(hiddenGradient,outputWeights);
                updateEmbeddings(embeddings[targetWord],inputGradient,LEARNING_RATE);
                i += EMBEDDING_SIZE;
                epochLoss += loss;
            }
            std::cout << "Epoch " << epoch << " loss: " << epochLoss << std::endl;
        }
    }
    std::ofstream outFile("/home/ronnieji/lib/MLCpplib-main/output/embeddings.txt", std::ios::out);
    if(!outFile.is_open()){
        outFile.open("/home/ronnieji/lib/MLCpplib-main/output/embeddings.txt", std::ios::out);
    }
    for(const auto& it : embeddings){
        outFile << it.first << " ";
        for(double xi : it.second){
            outFile << xi << " ";
        }
        outFile << std::endl;
    }
    outFile.close();
}
int main(){
    process_input("/home/ronnieji/lib/MLCpplib-main/citizen.txt");
    return 0;
}