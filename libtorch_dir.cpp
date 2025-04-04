/*

    Download : libtorch
    export LIBTORCH_PATH=/opt/homebrew/Cellar/libtorch
    export DYLD_LIBRARY_PATH=$LIBTORCH_PATH/lib:$DYLD_LIBRARY_PATH
	
    training folder:
    main folder
    |------------
       |       |
    catalog1, catalog2 ....

g++ -std=c++20 /Users/dengfengji/ronnieji/lib/MLCpplib-main/libtorch_dir.cpp -o /Users/dengfengji/ronnieji/lib/MLCpplib-main/libtorch_dir -I/opt/homebrew/Cellar/tesseract/5.5.0/include \
-I/opt/homebrew/Cellar/tesseract/5.5.0/include \
-L/opt/homebrew/Cellar/tesseract/5.5.0/lib -ltesseract \
-I/opt/homebrew/Cellar/eigen/3.4.0_1/include/eigen3 \
-I/opt/homebrew/Cellar/boost/1.87.0/include \
-I/opt/homebrew/Cellar/icu4c@76/76.1_1/include \
-L/opt/homebrew/Cellar/icu4c@76/76.1_1/lib -licuuc -licudata \
/opt/homebrew/Cellar/boost/1.87.0/lib/libboost_system.a \
/opt/homebrew/Cellar/boost/1.87.0/lib/libboost_filesystem.a \
-I/opt/homebrew/Cellar/libtorch/include \
-I/opt/homebrew/Cellar/libtorch/include/torch/csrc/api/include \
-L/opt/homebrew/Cellar/libtorch/lib \
-Wl,-rpath,/opt/homebrew/Cellar/libtorch/lib \
-ltorch -ltorch_cpu -lc10 \
`pkg-config --cflags --libs opencv4`
 * 
*/

#include <torch/torch.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <random>
#include <map>
#include <fstream>
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
std::map<std::string, std::vector<cv::Mat>> loadImages(int imageSize,const std::string& inputPath) {
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "Error opening input file for reading: " << inputPath << std::endl;
        return {};
    }
    std::map<std::string, std::vector<cv::Mat>> dataset;
    while (true) {
        // Read catalog name length
        size_t catalogLength;
        if (!inFile.read(reinterpret_cast<char*>(&catalogLength), sizeof(size_t))) break;
        // Read catalog name
        std::string catalog(catalogLength, '\0');
        inFile.read(&catalog[0], catalogLength);
        // Read image size
        size_t serializedImageSize;
        inFile.read(reinterpret_cast<char*>(&serializedImageSize), sizeof(size_t));
        // Read image data
        std::vector<uchar> buffer(serializedImageSize);
        inFile.read(reinterpret_cast<char*>(buffer.data()), serializedImageSize);
        // Create cv::Mat for the image directly from buffer
        cv::Mat image(imageSize, imageSize, CV_32FC3, buffer.data());  // Assuming images are resized to (imageSize, imageSize)
        if (!image.empty()) {
            // No need to convert back to CV_32F, as it's already in the tensor format
            dataset[catalog].push_back(image.clone());  // Clone to store in the map
        } else {
            std::cerr << "Failed to create image for catalog: " << catalog << std::endl;
        }
    }
    inFile.close();
    return dataset;
}
/*
    end Serialize the image
*/
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
std::vector<ImageData> loadDataset(const std::string& dataset_dat_Path, int imageSize) {
    std::vector<ImageData> dataset;
    std::map<int,std::string> label_mapping;
    if(dataset_dat_Path.empty()){
        return dataset;
    }
    if(std::filesystem::exists(dataset_dat_Path)){
        int currentLabel = 0;
		for (const auto& dirEntry : std::filesystem::directory_iterator(dataset_dat_Path)) {
			if (std::filesystem::is_directory(dirEntry)) {
				std::string catalog = dirEntry.path().filename().string();
				// Loop through images in the catalog
				for (const auto& imageEntry : std::filesystem::directory_iterator(dirEntry.path())) {
					if (std::filesystem::is_regular_file(imageEntry)) {
						std::string imagePath = imageEntry.path().string();
						if (!isValidImageFile(imagePath)) {
							std::cout << "Not a valid image file: " << imagePath << std::endl;
							continue;
						}
						cv::Mat image = cv::imread(imagePath);
						if (image.empty()) {
							std::cout << "Failed to load image: " << imagePath << std::endl;
							continue;
						}
						// Resize image
						cv::resize(image, image, cv::Size(imageSize, imageSize));
						// Convert to tensor format
						cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
						image.convertTo(image, CV_32F, 1.0 / 255.0);
						auto imgTensor = torch::from_blob(image.data, {image.rows, image.cols, 3}, torch::kFloat32);
						imgTensor = imgTensor.permute({2, 0, 1}); // [H,W,C] -> [C,H,W]
						imgTensor = imgTensor.clone(); // Ensure that tensor owns its memory
						imgTensor[0] = imgTensor[0].sub_(0.485).div_(0.229);
						imgTensor[1] = imgTensor[1].sub_(0.456).div_(0.224);
						imgTensor[2] = imgTensor[2].sub_(0.406).div_(0.225);
						dataset.push_back({imgTensor, currentLabel});
						label_mapping[currentLabel]= catalog;
					}//if
				}//for
				currentLabel++;
			}//if
		}//for
	}//if
	if(!label_mapping.empty()){
		std::ofstream ofile("/Users/dengfengji/ronnieji/lib/MLCpplib-main/labelMap.txt", std::ios::out);
		if(!ofile.is_open()){
			ofile.open("/Users/dengfengji/ronnieji/lib/MLCpplib-main/labelMap.txt", std::ios::out);
		}
		for(const auto & item : label_mapping){
			ofile << item.first << "," << item.second << '\n';
		}
		ofile.close();
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
double computeCosineSimilarity(const torch::Tensor& tensor1, const torch::Tensor& tensor2) {
    // Use torch::Tensor as input and specify the dimension using the options struct
    auto similarity = torch::nn::functional::cosine_similarity(tensor1, tensor2, torch::nn::CosineSimilarityOptions().dim(0));
    return similarity.item<double>();
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
    std::cout << "Start training, please wait..." << std::endl;
    // Parameters
    const std::string dataset_dat_Path = "/Users/dengfengji/ronnieji/kaggle/train_sample"; // Path to your main dataset folder
    const int imageSize = 128; // Resize images to 100x100
    const int numEpochs = 10;
    const int batchSize = 16;
    const float learningRate = 1e-3;
    const std::string modelPath = "/Users/dengfengji/ronnieji/lib/MLCpplib-main/siamese_model.pt";
    // Load dataset
    auto dataset = loadDataset(dataset_dat_Path, imageSize);
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
std::vector<testImageData> loadDatasetWithEmbeddings(const std::string& dataset_dat_Path, const std::string& labelMap_path, int imageSize, SiameseNetwork& model, torch::Device& device) {
    std::vector<testImageData> d_item;
    if (dataset_dat_Path.empty()) {
        std::cerr << "Dataset image.dat file path is empty!" << std::endl;
        return d_item;
    }
	std::vector<ImageData> image_to_train = loadDataset(dataset_dat_Path,imageSize);
    // Load label mapping file
    std::map<int, std::string> label_m = read_label_map(labelMap_path);
    if (label_m.empty()) {
        std::cerr << "Label map file is empty or not found!" << std::endl;
        return d_item;
    }
    if(!image_to_train.empty()){
        for(const auto& item : image_to_train){
            int currentLabel = -1;
            for (const auto& lbm : label_m) {
                if (lbm.first == item.label) {
                    currentLabel = lbm.first;
                    break;
                }
            }
            if (currentLabel == -1) {
                std::cerr << "No matching label found for directory: " << item.label << std::endl;
                continue;
            }
			torch::Tensor imgTensor = item.imageTensor.to(device);
			torch::Tensor embedding = model->subnetwork->forward(imgTensor);
			embedding = embedding.flatten();
			d_item.push_back({imgTensor, embedding, currentLabel});
        }
    }
    return d_item;
}
/*
   // Parameters
    //const std::string datasetPath = "/Users/dengfengji/ronnieji/Kaggle/archive-2/train";
    //const std::string embeddingsPath = "/Users/dengfengji/ronnieji/lib/project/main/embeddings"; // Directory to save embeddings
    //const int imageSize = 100;
    //const std::string modelPath = "/Users/dengfengji/ronnieji/lib/project/main/siamese_model.pt";
*/
void initialize_embeddings(
	const std::string& train_folder_path,
    const std::string& modelPath, 
    const std::string& labelMapPath, 
    SiameseNetwork& model, 
    torch::Device& device, 
    int imageSize, 
    std::vector<testImageData>& oDataSet
) {
    if (train_folder_path.empty() || modelPath.empty() || labelMapPath.empty()) {
        std::cerr << "Error: One or more paths are empty." << std::endl;
        return;
    }
    // Load label mapping file
    std::map<int, std::string> labelMap = read_label_map(labelMapPath);
    if (labelMap.empty()) {
        std::cerr << "Error: Label map could not be loaded." << std::endl;
        return;
    }
    // Load dataset and embeddings
    std::cout << "Loading dataset with embeddings..." << std::endl;
    oDataSet = loadDatasetWithEmbeddings(train_folder_path,labelMapPath, imageSize, model, device);
	
    if (oDataSet.empty()) {
        std::cerr << "Error: Failed to load dataset with embeddings." << std::endl;
    } else {
        std::cout << "Dataset with embeddings successfully loaded." << std::endl;
    }
}
void testModel(const std::string& imagePath, int imageSize, const std::vector<testImageData>& dataset, const std::map<int, std::string>& labelMap, SiameseNetwork& model, torch::Device& device) {
    if (imagePath.empty() || dataset.empty() || labelMap.empty()) {
        return;
    }
    std::vector<std::string> objs_found;
    std::unordered_map<std::string,double> obj_scores;
    // Load and preprocess new image
    torch::Tensor newImageTensor = load_and_preprocess_image(imagePath, imageSize);
    if (newImageTensor.numel() > 0) {
        newImageTensor = newImageTensor.to(device);
        torch::Tensor newImageEmbedding = model->subnetwork->forward(newImageTensor);
        newImageEmbedding = newImageEmbedding.flatten(); // Flatten the embedding
        double similarityThreshold = 0.75; // Similarity threshold
        std::map<int, double> detectedObjects; // Store object labels with their max similarity
        for (const auto& data : dataset) {
            torch::Tensor datasetEmbedding = data.embedding.to(device);
			double similarity = computeCosineSimilarity(newImageEmbedding, datasetEmbedding);
			// Check if similarity is above the threshold
			if (similarity >= similarityThreshold) {
				auto it = detectedObjects.find(data.label);
				if (it == detectedObjects.end() || similarity > it->second) {
					detectedObjects[data.label] = similarity;
				}
			}
        }
        // Output all detected object names
        if (!detectedObjects.empty()) {
            std::cout << imagePath << " contains the following objects:" << std::endl;
            for (const auto& obj : detectedObjects) {
                auto it = labelMap.find(obj.first);
                if (it != labelMap.end()) {
                    if(obj.second == 1.0){
                        objs_found.push_back(it->second);
                    }
                    else if(obj.second > 0.99 && obj.second < 1.0){//adjust parameters as needed
                        obj_scores[it->second] = obj.second;
                        //std::cout << "- " << it->second << " (similarity: " << obj.second << ")" << std::endl;
                    }
                }
            }
        } else {
            std::cout << "No significant objects found in " << imagePath << " that match the threshold." << std::endl;
        }
        if(!objs_found.empty()){
            for(const auto& item : objs_found){
                std::cout << imagePath << " has: " << item << std::endl;
            }
        }
        else{
           if(!obj_scores.empty()){
                std::vector<std::pair<std::string, double>> sorted_score_counting(obj_scores.begin(), obj_scores.end());
                // Sort the vector of pairs
                std::sort(sorted_score_counting.begin(), sorted_score_counting.end(), [](const auto& a, const auto& b) {
                    return a.second > b.second;
                });
                auto it = sorted_score_counting.begin();
                std::cout << imagePath << " is a(an): " << it->first << std::endl;
           }
        }
    } else {
        std::cerr << "Failed to load new image: " << imagePath << std::endl;
    }
}
int main() {
    train_model();
    // ---------------------
    // Recognize New Images
    // ---------------------
    /*
        initialize the recognition engine
    */
    // Set device to CPU or CUDA if available
    torch::Device device(torch::kCPU);
    if (torch::cuda::is_available()) {
        std::cout << "CUDA is available! Training on GPU." << std::endl;
        device = torch::Device(torch::kCUDA);
    }
    // Load the model
    std::string modelPath = "/Users/dengfengji/ronnieji/lib/MLCpplib-main/siamese_model.pt";
    SiameseNetwork model;
    torch::load(model, modelPath);
    model->to(device);
    model->eval(); // Set model to evaluation mode
    std::vector<testImageData> trainedDataSet;
    std::cout << "Initialize the recognization engine..." << std::endl;
    initialize_embeddings(
		"/Users/dengfengji/ronnieji/kaggle/train_sample",
        "/Users/dengfengji/ronnieji/lib/MLCpplib-main/siamese_model.pt",
        "/Users/dengfengji/ronnieji/lib/MLCpplib-main/labelMap.txt",
        model,
        device,
        128,//default image size
        trainedDataSet
    );
    /*
        load label mapping file
    */
    std::map<int,std::string> labelMap = read_label_map("/Users/dengfengji/ronnieji/lib/MLCpplib-main/labelMap.txt");
    std::cout << "Successfully load the engine, start recognizating..." << std::endl;
    /*
        test input image
        test all image in a folder
    */
    std::vector<std::string> testimgs;
    std::string sub_folder_path = "/Users/dengfengji/ronnieji/kaggle/test"; //"/Users/dengfengji/ronnieji/Kaggle/test";
    for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {
        if (!entrySubFolder.is_regular_file()) {
            continue; // Skip non-file entries
        }
        std::string ext = entrySubFolder.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
            std::string imgFilePath = entrySubFolder.path().string();
            testimgs.push_back(imgFilePath);
        }
    }
    std::cout << testimgs.size() << std::endl;
    if(trainedDataSet.empty()){
        std::cout << "trainedDataSet is empty!" << std::endl;
        return 0;
    }
    if(!testimgs.empty()){
        for(const auto& item : testimgs){
            testModel(
            item,
            128,
            trainedDataSet,
            labelMap,
            model,
            device
            );
        }
    }
    return 0;
}
