// main.cpp

#include <torch/torch.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <random>
#include <map>
#include <fstream>

bool isValidImageFile(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".jpg" || ext == ".jpeg" || ext == ".png");
}
std::vector<std::string> JsplitString(const std::string& input, char delimiter){
    std::vector<std::string> result;
    if(input.empty() || delimiter == '\0'){
        std::cerr << "Jsonlib::splitString input empty" << '\n';
    	return result;
    }
    std::stringstream ss(input);
    std::string token;
    while(std::getline(ss,token,delimiter)){
        result.push_back(token);
    }
    return result;
}
std::map<int,std::string> read_label_map(const std::string& lable_map_path){
    std::map<int,std::string> label_mapping;
    if(lable_map_path.empty()){
        return label_mapping;
    }
    std::ifstream ifile(lable_map_path);
    if(ifile.is_open()){
        std::string line;
        while(std::getline(ifile, line)){
            if(!line.empty()){
                std::vector<std::string> get_line = JsplitString(line,',');
                if(!get_line.empty()){
                    int label = std::stoi(get_line[0]);
                    std::string className = get_line[1];
                    label_mapping[label] = className;
                }
            }
        }
    }
    ifile.close();
    return label_mapping;
}
// ---------------------
// Siamese Network Model
// ---------------------

struct SubNetImpl : torch::nn::Module {
    torch::nn::Sequential features;

    SubNetImpl() {
        features = torch::nn::Sequential(
            torch::nn::Conv2d(torch::nn::Conv2dOptions(3, 32, 3).padding(1)),
            torch::nn::ReLU(),
            torch::nn::MaxPool2d(2),

            torch::nn::Conv2d(torch::nn::Conv2dOptions(32, 64, 3).padding(1)),
            torch::nn::ReLU(),
            torch::nn::MaxPool2d(2),

            torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 128, 3).padding(1)),
            torch::nn::ReLU(),
            torch::nn::AdaptiveAvgPool2d(1)
        );
        register_module("features", features);
    }

    torch::Tensor forward(torch::Tensor x) {
        x = features->forward(x);
        x = x.view({x.size(0), -1}); // Flatten
        return x;
    }
};
TORCH_MODULE(SubNet);

struct SiameseNetworkImpl : torch::nn::Module {
    SubNet subnetwork;

    SiameseNetworkImpl() {
        subnetwork = SubNet();
        register_module("subnetwork", subnetwork);
    }

    torch::Tensor forward(torch::Tensor input1, torch::Tensor input2) {
        auto output1 = subnetwork->forward(input1);
        auto output2 = subnetwork->forward(input2);

        // Compute Euclidean distance between embeddings
        auto distance = torch::pairwise_distance(output1, output2, 2); // p=2 for Euclidean distance
        return distance;
    }
};
TORCH_MODULE(SiameseNetwork);

// ---------------------
// Contrastive Loss Function
// ---------------------

torch::Tensor contrastiveLoss(torch::Tensor distance, torch::Tensor label, float margin = 1.0f) {
    // label: 1 for similar pairs, 0 for dissimilar pairs
    auto loss = label * torch::pow(distance, 2) +
                (1 - label) * torch::pow(torch::clamp(margin - distance, /*min=*/0.0f), 2);
    return loss.mean();
}

// ---------------------
// Data Preparation
// ---------------------

struct ImageData {
    torch::Tensor imageTensor;
    int label;
};
struct testImageData {
    torch::Tensor imageTensor;
    torch::Tensor embedding;
    int label;
};
/*
    load
*/
torch::Tensor load_and_preprocess_image(const std::string& imagePath, int imageSize) {
    if(imagePath.empty()){
        return torch::Tensor();
    }
    cv::Mat img = cv::imread(imagePath);
    if (img.empty()) {
        std::cerr << "Could not read image: " << imagePath << std::endl;
        return torch::Tensor();
    }
    // Resize and convert to RGB
    cv::resize(img, img, cv::Size(imageSize, imageSize));
    cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
    // Convert to float tensor and normalize
    img.convertTo(img, CV_32F, 1.0 / 255.0);
    auto tensor = torch::from_blob(img.data, {img.rows, img.cols, 3}, torch::kFloat);
    tensor = tensor.permute({2, 0, 1}); // Change to C x H x W
    // Normalize using mean and std of ImageNet dataset
    tensor = tensor.clone(); // Ensure that tensor owns its memory
    tensor[0] = tensor[0].sub_(0.485).div_(0.229);
    tensor[1] = tensor[1].sub_(0.456).div_(0.224);
    tensor[2] = tensor[2].sub_(0.406).div_(0.225);
    return tensor;
}
std::vector<ImageData> loadDataset(const std::string& datasetPath, int imageSize) {
    std::vector<ImageData> dataset;
    std::map<int,std::string> label_mapping;
    int currentLabel = 0;
    for (const auto& entry : std::filesystem::directory_iterator(datasetPath)) {
        if (std::filesystem::is_directory(entry.status())) {
            std::string sub_folder_name = entry.path().filename().string();  
            std::string sub_folder_path = entry.path().string();  
            int label = currentLabel;
            for (const auto& imgEntry : std::filesystem::directory_iterator(sub_folder_path)) {
                if (std::filesystem::is_regular_file(imgEntry.status())) {
                    std::string imagePath = imgEntry.path().string();
                    if(isValidImageFile(imagePath)){
                        // Read image using OpenCV
                        cv::Mat img = cv::imread(imagePath);
                        if (img.empty()) {
                            std::cerr << "Could not read image: " << imagePath << std::endl;
                            continue;
                        }
                        // Resize image
                        cv::resize(img, img, cv::Size(imageSize, imageSize));
                        // Convert to tensor
                        cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
                        img.convertTo(img, CV_32F, 1.0 / 255.0);
                        auto imgTensor = torch::from_blob(img.data, {img.rows, img.cols, 3}, torch::kFloat32);
                        imgTensor = imgTensor.permute({2, 0, 1}); // [H,W,C] -> [C,H,W]
                        dataset.push_back({imgTensor.clone(), label});
                        label_mapping[label]=sub_folder_name;
                    }
                }
            }
            currentLabel++;
        }
    }
    if(!label_mapping.empty()){
        std::ofstream ofile("/Users/dengfengji/ronnieji/lib/project/main/labelMap.txt",std::ios::out);
        if(!ofile.is_open()){
            ofile.open("/Users/dengfengji/ronnieji/lib/project/main/labelMap.txt",std::ios::out);
        }
        for(const auto& item : label_mapping){
            ofile << item.first << "," << item.second << '\n';
        }
        ofile.close();
    }
    return dataset;
}
std::vector<testImageData> loadDatasetWithEmbeddings(const std::string& datasetPath, int imageSize, SiameseNetwork& model, torch::Device device) {
    std::vector<testImageData> dataset;
    std::map<int,std::string> label_mapping;
    if(datasetPath.empty()){
        return dataset;
    }
    /*
        load label mapping file
    */
    std::map<int,std::string> lable_m = read_label_map("/Users/dengfengji/ronnieji/lib/project/main/labelMap.txt");
    if(!lable_m.empty()){
        for (const auto& dirEntry : std::filesystem::directory_iterator(datasetPath)) {
            if (std::filesystem::is_directory(dirEntry)) {
                int label = 0;
                std::string sub_folder_name = dirEntry.path().filename().string();  
                std::string sub_folder_path = dirEntry.path().string();
                for(const auto& lbm : lable_m){
                    if(lbm.second == sub_folder_name){
                        label = lbm.first;
                        break;
                    }
                }
                for (const auto& imgEntry : std::filesystem::directory_iterator(sub_folder_path)) {
                    if (std::filesystem::is_regular_file(imgEntry)) {
                        std::string imagePath = imgEntry.path().string();
                        if(isValidImageFile(imagePath)){
                            torch::Tensor imageTensor = load_and_preprocess_image(imagePath, imageSize);
                            if (imageTensor.numel() > 0) {
                                // Move tensor to device
                                imageTensor = imageTensor.to(device);
                                // Compute embedding
                                torch::Tensor embedding = model->subnetwork->forward(imageTensor);
                                embedding = embedding.flatten(); // Flatten the embedding
                                // Store imageTensor and embedding
                                dataset.push_back({imageTensor, embedding, label});
                            }
                        }
                    }
                }
            }
        }
    }
    else{
        std::cerr << "label map file is empty!" << std::endl;
    }
    return dataset;
}
void createImagePairs(const std::vector<ImageData>& dataset,
                      std::vector<std::pair<torch::Tensor, torch::Tensor>>& imagePairs,
                      std::vector<int>& pairLabels) {
    // Create similar and dissimilar pairs
    std::vector<ImageData> shuffledDataset = dataset;
    std::shuffle(shuffledDataset.begin(), shuffledDataset.end(), std::mt19937{std::random_device{}()});
    for (size_t i = 0; i < shuffledDataset.size(); ++i) {
        // Positive pair
        for (size_t j = i + 1; j < shuffledDataset.size(); ++j) {
            if (shuffledDataset[i].label == shuffledDataset[j].label) {
                imagePairs.push_back({shuffledDataset[i].imageTensor, shuffledDataset[j].imageTensor});
                pairLabels.push_back(1);
                break; // Limit positive pairs
            }
        }
        // Negative pair
        for (size_t j = i + 1; j < shuffledDataset.size(); ++j) {
            if (shuffledDataset[i].label != shuffledDataset[j].label) {
                imagePairs.push_back({shuffledDataset[i].imageTensor, shuffledDataset[j].imageTensor});
                pairLabels.push_back(0);
                break; // Limit negative pairs
            }
        }
    }
    // Shuffle pairs
    std::shuffle(imagePairs.begin(), imagePairs.end(), std::mt19937{std::random_device{}()});
}

// ---------------------
// Training Function
// ---------------------

void trainModel(SiameseNetwork& model,
                std::vector<std::pair<torch::Tensor, torch::Tensor>>& imagePairs,
                std::vector<int>& pairLabels,
                torch::optim::Adam& optimizer,
                int epochs = 10,
                int batch_size = 16) {
    model->train();
    size_t dataset_size = imagePairs.size();
    for (int epoch = 1; epoch <= epochs; ++epoch) {
        double epoch_loss = 0.0;
        int batches = 0;
        for (size_t i = 0; i < dataset_size; i += batch_size) {
            size_t end = std::min(i + batch_size, dataset_size);
            std::vector<torch::Tensor> batch_input1, batch_input2, batch_labels;
            for (size_t j = i; j < end; ++j) {
                batch_input1.push_back(imagePairs[j].first);
                batch_input2.push_back(imagePairs[j].second);
                batch_labels.push_back(torch::tensor(static_cast<float>(pairLabels[j])));
            }
            auto input1 = torch::stack(batch_input1).to(torch::kFloat32);
            auto input2 = torch::stack(batch_input2).to(torch::kFloat32);
            auto labels = torch::stack(batch_labels);
            optimizer.zero_grad();
            auto distances = model->forward(input1, input2);
            auto loss = contrastiveLoss(distances, labels);
            loss.backward();
            optimizer.step();
            epoch_loss += loss.item<double>();
            ++batches;
        }
        std::cout << "Epoch [" << epoch << "/" << epochs << "] Loss: " << epoch_loss / batches << std::endl;
    }
}

// ---------------------
// Evaluation Function
// ---------------------

float computeSimilarity(SiameseNetwork& model, torch::Tensor image1, torch::Tensor image2) {
    model->eval(); // Set the model to evaluation mode
    torch::NoGradGuard no_grad; // Disable gradient computation
    image1 = image1.unsqueeze(0); // Add batch dimension
    image2 = image2.unsqueeze(0);
    auto distance = model->forward(image1, image2);
    return distance.item<float>(); // Convert to scalar
}
double computeCosineSimilarity(const torch::Tensor& tensor1, const torch::Tensor& tensor2) {
    // Use torch::Tensor as input and specify the dimension using the options struct
    auto similarity = torch::nn::functional::cosine_similarity(tensor1, tensor2, torch::nn::CosineSimilarityOptions().dim(0));
    return similarity.item<double>();
}
int findMostSimilarImageLabel(const torch::Tensor& newImageEmbedding, const std::vector<testImageData>& dataset) {
    double highestSimilarity = -1.0;
    int mostSimilarLabel = -1;
    for (const auto& data : dataset) {
        double similarity = computeCosineSimilarity(newImageEmbedding, data.embedding);
        if (similarity > highestSimilarity) {
            highestSimilarity = similarity;
            mostSimilarLabel = data.label;
        }
    }
    return mostSimilarLabel;
}
// ---------------------
// Main Function
// ---------------------
void train_model(){
    // Set device to CPU or CUDA if available
    torch::Device device(torch::kCPU);
    if (torch::cuda::is_available()) {
        std::cout << "CUDA is available! Training on GPU." << std::endl;
        device = torch::Device(torch::kCUDA);
    }
    // Parameters
    const std::string datasetPath = "/Users/dengfengji/ronnieji/Kaggle/archive-2/train"; // Path to your main dataset folder
    const int imageSize = 100; // Resize images to 100x100
    const int numEpochs = 10;
    const int batchSize = 16;
    const float learningRate = 1e-3;
    const std::string modelPath = "/Users/dengfengji/ronnieji/lib/project/main/siamese_model.pt";
    // Load dataset
    auto dataset = loadDataset(datasetPath, imageSize);
    // Create image pairs and labels
    std::vector<std::pair<torch::Tensor, torch::Tensor>> imagePairs;
    std::vector<int> pairLabels;
    createImagePairs(dataset, imagePairs, pairLabels);
    // Initialize model
    SiameseNetwork model;
    model->to(device);
    // Create Adam optimizer
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(learningRate));
    // Train the model
    trainModel(model, imagePairs, pairLabels, optimizer, numEpochs, batchSize);
    // Save the model
    torch::save(model, modelPath);
    std::cout << "Successfully saved the model to: " << modelPath << std::endl;
}
void testModel(const std::string& imagePath){
    if(imagePath.empty()){
        return;
    }
    /*
        load label mapping file
    */
    std::map<int,std::string> lable_m = read_label_map("/Users/dengfengji/ronnieji/lib/project/main/labelMap.txt");
    // Set device to CPU or CUDA if available
    torch::Device device(torch::kCPU);
    if (torch::cuda::is_available()) {
        std::cout << "CUDA is available! Training on GPU." << std::endl;
        device = torch::Device(torch::kCUDA);
    }
    // Parameters
    const std::string datasetPath = "/Users/dengfengji/ronnieji/Kaggle/archive-2/train";
    const int imageSize = 100;
    const int numEpochs = 10;
    const int batchSize = 16;
    const float learningRate = 1e-3;
    const std::string modelPath = "/Users/dengfengji/ronnieji/lib/project/main/siamese_model.pt";
    // Load the model
    SiameseNetwork model;
    torch::load(model, modelPath);
    model->to(device);
    model->eval(); // Set model to evaluation mode
    // Load dataset and compute embeddings
    auto dataset = loadDatasetWithEmbeddings(datasetPath, imageSize, model, device);
    // Load and preprocess new image
    //imagePath = "/Users/dengfengji/ronnieji/Kaggle/test/Image_5.jpg";
    torch::Tensor newImageTensor = load_and_preprocess_image(imagePath, imageSize);
    if (newImageTensor.numel() > 0) {
        // Compute embedding for the new image
        newImageTensor = newImageTensor.to(device);
        torch::Tensor newImageEmbedding = model->subnetwork->forward(newImageTensor);
        newImageEmbedding = newImageEmbedding.flatten(); // Flatten the embedding
        // Find the most similar image label
        int mostSimilarLabel = findMostSimilarImageLabel(newImageEmbedding, dataset);
        // Output the result
        if (mostSimilarLabel != -1) {
            std::cout << "The most similar image label is: " << mostSimilarLabel << " object name: " << lable_m[mostSimilarLabel] << std::endl;
        } else {
            std::cout << "No similar image found." << std::endl;
        }
    } else {
        std::cerr << "Failed to load new image." << std::endl;
    }
   
}
int main() {
    //train_model();
    // ---------------------
    // Recognize New Images
    // ---------------------
    testModel("/Users/dengfengji/ronnieji/Kaggle/test/multiObjs2.jpg");
    return 0;
}
