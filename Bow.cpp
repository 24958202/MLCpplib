#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <random>
#include <Eigen/Dense>
#include "../lib/nemslib.h"

const int WINDOW_SIZE = 5; 
const int EMBEDDING_SIZE = 50; 
const double LEARNING_RATE = 0.01; 
const int NUM_EPOCHS = 10; 

// Output embeddings to file
void outputEmbeddingsToFile(const std::string& filePath, const std::map<std::string, Eigen::VectorXd>& embeddings, const std::map<std::string, Eigen::VectorXd>& embeddings_context) {
    std::ofstream outFile(filePath, std::ios::out);
    if (!outFile.is_open()) {
        std::cerr << "Error: Unable to open output file." << std::endl;
        return;
    }
    for (const auto& entry : embeddings) {
        outFile << "Word: " << entry.first << "\n";
        outFile << "Embedding: " << entry.second.transpose() << "\n";
        
        // Note: Outputting the entire context vocab for EVERY word creates massive files.
        // You might want to remove this inner loop in the future.
        for (const auto& contextWord : embeddings_context) {
            outFile << "Context Word: " << contextWord.first << "\n";
            outFile << "Context Embedding: " << contextWord.second.transpose() << "\n";
        }
        outFile << "\n";
    }
    outFile.close();
}

std::map<std::string, Eigen::VectorXd> initializeEmbeddings(const std::map<std::string, int>& vocab) {
    std::map<std::string, Eigen::VectorXd> embeddings;
    std::random_device rd;
    std::mt19937 gen(rd());
    // Scaled down initialization to prevent exploding gradients
    std::uniform_real_distribution<double> dis(-0.1, 0.1); 
    
    for (const auto& word : vocab) {
        Eigen::VectorXd embedding = Eigen::VectorXd::NullaryExpr(EMBEDDING_SIZE, [&](){ return dis(gen); });
        embeddings[word.first] = embedding;
    }
    return embeddings;
}

double dotProduct(const Eigen::VectorXd& x, const Eigen::VectorXd& y) {
    if (x.size() != y.size()) return 0.0;
    return x.dot(y);
}

void updateEmbeddings(Eigen::VectorXd& embedding, const Eigen::VectorXd& gradient, double learning_rate) {
    if (embedding.size() != gradient.size()) {
        std::cerr << "Gradient size does not match embedding size." << std::endl;
        return;
    }
    embedding -= learning_rate * gradient;
}

void process_input(const std::string& input_folder_path) {
    nemslib nem_j;
    std::vector<std::string> lines = nem_j.readTextFile(input_folder_path);
    std::vector<std::vector<std::string>> corpus;
    std::map<std::string, int> vocab;
    
    // Pass strings by const reference to avoid deep copies
    for(const std::string& line : lines){
        std::vector<std::string> tokens = nem_j.tokenize_en(line);
        corpus.push_back(tokens);
        for(const std::string& token : tokens){
            if(vocab.find(token) == vocab.end()){
                vocab[token] = vocab.size(); 
            }
        }
    }
    
    // Initialize target weights and context weights separately ONCE
    std::map<std::string, Eigen::VectorXd> embeddings = initializeEmbeddings(vocab);
    std::map<std::string, Eigen::VectorXd> embeddings_context = initializeEmbeddings(vocab);

    for(int epoch = 1; epoch <= NUM_EPOCHS; epoch++){
        double epochLoss = 0.0;
        
        for(const std::vector<std::string>& sentence : corpus){
            for(int i = 0; i < sentence.size(); i++){
                const std::string& targetWord = sentence[i];
                std::vector<std::string> contextWords;
                
                // Fix: j >= 0 to include the first word of the sentence
                for(int j = i - WINDOW_SIZE; j <= i + WINDOW_SIZE; j++){
                    if(j != i && j >= 0 && j < sentence.size()){
                        contextWords.push_back(sentence[j]);
                    }
                }
                
                if (contextWords.empty()) continue;

                // CBOW style: sum the context vectors
                Eigen::VectorXd hiddenActivations = Eigen::VectorXd::Zero(EMBEDDING_SIZE);
                for(const std::string& contextWord : contextWords){
                    hiddenActivations += embeddings_context[contextWord];
                }
                
                Eigen::VectorXd outputWeights = embeddings[targetWord];
                double logits = dotProduct(hiddenActivations, outputWeights);
                
                // Note: This is an approximation of Softmax assuming other logits are 0
                double exp_logits = std::exp(logits);
                double probability = exp_logits / (exp_logits + vocab.size() - 1);
                
                double loss = -log(probability);
                epochLoss += loss;
                
                double outputGradient = probability - 1.0; 
                
                // Gradient for the target embedding
                Eigen::VectorXd targetGradient = outputGradient * hiddenActivations;
                updateEmbeddings(embeddings[targetWord], targetGradient, LEARNING_RATE);

                // Gradient for the context embeddings (propagated backwards)
                Eigen::VectorXd hiddenGradient = outputGradient * outputWeights;
                
                // Fix: Apply the same hidden gradient to all context words that contributed to the sum
                for (const std::string& contextWord : contextWords) {
                    updateEmbeddings(embeddings_context[contextWord], hiddenGradient, LEARNING_RATE);
                }
            }
        }
        // Fix: Moved console output outside the sentence loop
        std::cout << "Epoch " << epoch << " loss: " << epochLoss << std::endl;
    }
    
    if(!embeddings.empty() && !embeddings_context.empty()){
        outputEmbeddingsToFile("/home/ronnieji/lib/MLCpplib-main/output/embeddings.txt", embeddings, embeddings_context);
    }
}

int main(){
    process_input("/home/ronnieji/lib/MLCpplib-main/citizen.txt");
    return 0;
}