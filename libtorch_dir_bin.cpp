/*

    Download : libtorch
    export LIBTORCH_PATH=/opt/homebrew/Cellar/libtorch
    export DYLD_LIBRARY_PATH=$LIBTORCH_PATH/lib:$DYLD_LIBRARY_PATH
	
    training folder:
    main folder
    |------------
       |       |
    catalog1, catalog2 ....

g++ -std=c++20 /Users/dengfengji/ronnieji/lib/MLCpplib-main/libtorch_dir_bin.cpp -o /Users/dengfengji/ronnieji/lib/MLCpplib-main/libtorch_dir_bin -I/opt/homebrew/Cellar/tesseract/5.5.0/include \
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
#include <chrono>
#include <thread>
#include <cstdint>
// ---------------------
// Data Preparation
// ---------------------
int audoBatchSize(int dataset_size){ //auto caculate batch size
    if(dataset_size < 5000){
        return 32;
    }
    else if(dataset_size < 50000){
        return 64;
    }
    else{
        return 128;
    }
}
int autoEpochs(int dataset_size){
    return 30 + (100000 - dataset_size) / 2000;
}
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
    try{
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
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors" << std::endl;
    }
    return label_mapping;
}
void saveModel(const std::vector<testImageData>& trainedDataSet, const std::string& filename) {  
    if (filename.empty()) {  
        return;  
    }  
    std::ofstream ofs(filename, std::ios::binary);  
    if (!ofs.is_open()) {  
        throw std::runtime_error("Unable to open file for writing.");  
    }  
	try{
		// Save the size of the dataset  
		size_t dataSize = trainedDataSet.size();  
		ofs.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));  
		// Save each testImageData object  
		for (const auto& data : trainedDataSet) {  
			// Save the label  
			ofs.write(reinterpret_cast<const char*>(&data.label), sizeof(data.label));  
			// Save the image tensor  
			auto imageTensor = data.imageTensor.contiguous(); // Ensure the tensor is contiguous in memory  
			auto imageShape = imageTensor.sizes();  
			size_t imageDims = imageShape.size();  
			ofs.write(reinterpret_cast<const char*>(&imageDims), sizeof(imageDims));  
			for (const auto& dim : imageShape) {  
				ofs.write(reinterpret_cast<const char*>(&dim), sizeof(dim));  
			}  
			int imageType = static_cast<int>(imageTensor.scalar_type());  
			ofs.write(reinterpret_cast<const char*>(&imageType), sizeof(imageType));  
			ofs.write(reinterpret_cast<const char*>(imageTensor.data_ptr()), imageTensor.nbytes());  
			// Save the embedding tensor  
			auto embeddingTensor = data.embedding.contiguous();  
			auto embeddingShape = embeddingTensor.sizes();  
			size_t embeddingDims = embeddingShape.size();  
			ofs.write(reinterpret_cast<const char*>(&embeddingDims), sizeof(embeddingDims));  
			for (const auto& dim : embeddingShape) {  
				ofs.write(reinterpret_cast<const char*>(&dim), sizeof(dim));  
			}  
			int embeddingType = static_cast<int>(embeddingTensor.scalar_type());  
			ofs.write(reinterpret_cast<const char*>(&embeddingType), sizeof(embeddingType));  
			ofs.write(reinterpret_cast<const char*>(embeddingTensor.data_ptr()), embeddingTensor.nbytes());  
		}  
		ofs.close(); 
	} 
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors." << std::endl;
	}
}
void loadModel(std::vector<testImageData>& trainedDataSet, const std::string& filename) {  
    if (filename.empty()) {  
        return;  
    }  
    std::ifstream ifs(filename, std::ios::binary);  
    if (!ifs.is_open()) {  
        throw std::runtime_error("Unable to open file for reading.");  
    }  
	try{
		// Read the size of the dataset  
		size_t dataSize;  
		ifs.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));  
		trainedDataSet.resize(dataSize);  
		// Read each testImageData object  
		for (auto& data : trainedDataSet) {  
			// Read the label  
			ifs.read(reinterpret_cast<char*>(&data.label), sizeof(data.label));  
			// Read the image tensor  
			size_t imageDims;  
			ifs.read(reinterpret_cast<char*>(&imageDims), sizeof(imageDims));  
			std::vector<int64_t> imageShape(imageDims);  
			for (size_t i = 0; i < imageDims; ++i) {  
				ifs.read(reinterpret_cast<char*>(&imageShape[i]), sizeof(imageShape[i]));  
			}  
			int imageType;  
			ifs.read(reinterpret_cast<char*>(&imageType), sizeof(imageType));  
			torch::Tensor imageTensor = torch::empty(imageShape, static_cast<torch::ScalarType>(imageType));  
			ifs.read(reinterpret_cast<char*>(imageTensor.data_ptr()), imageTensor.nbytes());  
			// Read the embedding tensor  
			size_t embeddingDims;  
			ifs.read(reinterpret_cast<char*>(&embeddingDims), sizeof(embeddingDims));  
			std::vector<int64_t> embeddingShape(embeddingDims);  
			for (size_t i = 0; i < embeddingDims; ++i) {  
				ifs.read(reinterpret_cast<char*>(&embeddingShape[i]), sizeof(embeddingShape[i]));  
			}  
			int embeddingType;  
			ifs.read(reinterpret_cast<char*>(&embeddingType), sizeof(embeddingType));  
			torch::Tensor embeddingTensor = torch::empty(embeddingShape, static_cast<torch::ScalarType>(embeddingType));  
			ifs.read(reinterpret_cast<char*>(embeddingTensor.data_ptr()), embeddingTensor.nbytes());  
			// Assign the tensors to the data object  
			data.imageTensor = imageTensor;  
			data.embedding = embeddingTensor;  
		}  
		ifs.close();  
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors." << std::endl;
	}
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
std::vector<ImageData> loadDataset(
        const std::string& dataset_dat_Path, 
        int imageSize,
        const std::string& label_map_path
    ) {
    std::vector<ImageData> dataset;
    std::map<int,std::string> label_mapping;
    if(dataset_dat_Path.empty()){
        return dataset;
    }
    if(std::filesystem::exists(dataset_dat_Path)){
        int currentLabel = 0;
        try{
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
        }
        catch(const std::exception& ex){
            std::cerr << ex.what() << std::endl;
        }
        catch(...){
            std::cerr << "Unknown errors" << std::endl;
        }
	}//if
	if(!label_mapping.empty()){
        try{
            std::ofstream ofile(label_map_path, std::ios::out);
            if(!ofile.is_open()){
                ofile.open(label_map_path, std::ios::out);
            }
            for(const auto & item : label_mapping){
                ofile << item.first << "," << item.second << '\n';
            }
            ofile.close();
        }
        catch(const std::exception& ex){
            std::cerr << ex.what() << std::endl;
        }
        catch(...){
            std::cerr << "Unknown errors" << std::endl;
        }
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
std::vector<testImageData> loadDatasetWithEmbeddings(const std::string& dataset_dat_Path, const std::string& labelMap_path, int imageSize, SiameseNetwork& model, torch::Device& device) {
    std::vector<testImageData> d_item;
    if (dataset_dat_Path.empty() || labelMap_path.empty()) {
        std::cerr << "Dataset image.dat or labelMap_path.txt file path is empty!" << std::endl;
        return d_item;
    }
	std::vector<ImageData> image_to_train = loadDataset(dataset_dat_Path,imageSize, labelMap_path);
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
// ---------------------
// Main Function train_model(dataset_dat_Path, modelPath, model_embedded_path,128,10,16,1e-3);
// ---------------------
void train_model(
		const std::string& train_folder, 
		const std::string& model_output_path, //siamese_model.pt
        const std::string& output_label_map_path,
		const std::string& model_embedded_output_path,//trained_model.dat
		const int imageSize = 128,
		const int numEpochs = 10,
		const int batchSize = 16,
		const float learningRate = 1e-3
	){
    // Set device to CPU or CUDA if available
    torch::Device device(torch::kCPU);
    if (torch::cuda::is_available()) {
        std::cout << "CUDA is available! Training on GPU." << std::endl;
        device = torch::Device(torch::kCUDA);
    }
    std::cout << "Start training, please wait..." << std::endl;
    // Load dataset
    auto dataset = loadDataset(train_folder, imageSize, output_label_map_path);
	std::cout << "Successfully loaded the dataset, start creating image pairs..." << std::endl;
    // Create image pairs and labels
    std::vector<std::pair<torch::Tensor, torch::Tensor>> imagePairs;
    std::vector<int> pairLabels;
    createImagePairs(dataset, imagePairs, pairLabels);
    // Initialize model
    SiameseNetwork model;
    model->to(device);
	//model->eval(); // Set model to evaluation mode
    // Create Adam optimizer
    torch::optim::Adam optimizer(model->parameters(), torch::optim::AdamOptions(learningRate));
	std::cout << "Successfully finished image pairing, start training the model..." << std::endl;
    // Train the model
    trainModel(model, imagePairs, pairLabels, optimizer, numEpochs, batchSize);
    // Save the model
    torch::save(model, model_output_path);
    std::cout << "Successfully saved the model to: " << model_output_path << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(20));
	std::cout << "Start creating trained binary file trained_model.dat" << std::endl;
	/*
	 * Generate tensor-with-embedding file 
	*/
	/*
        initialize the recognition engine
    */
    std::vector<testImageData> trainedDataSet;
    std::cout << "Initialize the recognization engine..." << std::endl;
    initialize_embeddings(
        train_folder,
        model_output_path,
        output_label_map_path,
        model,
        device,
        imageSize,//default image size
        trainedDataSet
    );
	if(!trainedDataSet.empty()){
		saveModel(trainedDataSet,model_embedded_output_path);
	}
	std::cout << "trained_model.dat was successfully saved to: " << model_embedded_output_path << std::endl;
}
void testModel(
	const std::string& imagePath, 
	int imageSize, 
	const std::vector<testImageData>& dataset, 
	const std::map<int, std::string>& labelMap, 
	SiameseNetwork& model, 
	torch::Device& device,
	double similarityThreshold = 0.75, // Similarity threshold
	double obj_left_bound = 0.99,//obj.second >
	double obj_right_bound = 1.0 //obj.second < 
	) {
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
                    else if(obj.second > obj_left_bound && obj.second < obj_right_bound){//adjust parameters as needed
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
	/*
		const int imageSize = 443,
		const int numEpochs = 10,
		const int batchSize = 16,
		const float learningRate = 1e-3 
	*/
	const std::string dataset_dat_Path = "/Users/dengfengji/ronnieji/kaggle/Fruits_new/train"; // Path to your main dataset folder
	const std::string output_lable_map_path = "/Users/dengfengji/ronnieji/kaggle/Fruits_new/labelMap.txt";//label map
    const std::string output_modelPath = "/Users/dengfengji/ronnieji/kaggle/Fruits_new/siamese_model.pt";//output siamese_model
	const std::string output_model_embedded_path = "/Users/dengfengji/ronnieji/kaggle/Fruits_new/trained_model.dat";// trained images dataset
	if(std::filesystem::exists(dataset_dat_Path)){
        train_model(dataset_dat_Path, output_modelPath, output_lable_map_path, output_model_embedded_path, 128, 10, 16, 1e-3);
    }
    else{
        std::cerr << dataset_dat_Path << " does not exists." << std::endl;
        return -1;
    }
	/*
        -----------------  Use models to predict new images --------------------------------
    */
	//load labels
    std::map<int,std::string> labelMap;
    if(std::filesystem::exists(output_lable_map_path)){
        labelMap = read_label_map(output_lable_map_path);
    }
	//load trained_model.dat
	std::vector<testImageData> trainedDataSet;  
    if(std::filesystem::exists(output_model_embedded_path)){
        loadModel(trainedDataSet,output_model_embedded_path);
    }
    else{
        std::cerr << output_model_embedded_path << " could not be found." << std::endl;
        return -1;
    }
	if(trainedDataSet.empty()){
		std::cerr << "trained_model is empty, exist." << std::endl;
		return -1;
	}
	/*
	 initialize SiameseNetwork model
	*/
	// Set device to CPU or CUDA if available
    torch::Device device(torch::kCPU);
    if (torch::cuda::is_available()) {
        std::cout << "CUDA is available! Training on GPU." << std::endl;
        device = torch::Device(torch::kCUDA);
    }
	SiameseNetwork model;
    torch::load(model, output_modelPath);//siamese_model.pt
    model->to(device);
    model->eval(); // Set model to evaluation mode
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
    std::cout << "Number of test images: " << testimgs.size() << std::endl;
	/*
		const std::string& imagePath, 
        int imageSize, 
        const std::vector<testImageData>& dataset, 
        const std::map<int, std::string>& labelMap, 
        SiameseNetwork& model, 
        torch::Device& device,
        double similarityThreshold = 0.75, // Similarity threshold
        double obj_left_bound = 0.99,//obj.second >
        double obj_right_bound = 1.0 //obj.second < 
	*/
    if(!testimgs.empty()){
        for(const auto& item : testimgs){
            testModel(
            item,
            128,
            trainedDataSet,
            labelMap,
            model,
            device,
			0.75,
			0.99,
			1.0
            );
        }
    }
    return 0;
}
