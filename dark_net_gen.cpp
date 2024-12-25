/*
 *Program to generate all configuration files darknet needs 
 */
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>
#include <regex>

namespace fs = std::filesystem;
// Function to write obj.names
void writeObjNames(const std::vector<std::string>& classNames, const std::string& outputDir) {
    std::ofstream namesFile(outputDir + "/obj.names");
    if (!namesFile) {
        std::cerr << "Error: Could not open obj.names for writing!" << std::endl;
        return;
    }
    for (const auto& className : classNames) {
        namesFile << className << std::endl;
    }
    namesFile.close();
    std::cout << "Generated obj.names" << std::endl;
}
// Function to write obj.data
void writeObjData(const std::vector<std::string>& classNames, const std::string& outputDir) {
    std::ofstream dataFile(outputDir + "/obj.data");
    if (!dataFile) {
        std::cerr << "Error: Could not open obj.data for writing!" << std::endl;
        return;
    }
    dataFile << "classes = " << classNames.size() << std::endl;
    dataFile << "train = " << outputDir << "/train.txt" << std::endl;
    dataFile << "valid = " << outputDir << "/test.txt" << std::endl;
    dataFile << "names = " << outputDir << "/obj.names" << std::endl;
    dataFile << "backup = backup/" << std::endl;
    dataFile.close();
    std::cout << "Generated obj.data" << std::endl;
}
// Function to write yolov3.cfg
void writeYoloConfig(const std::vector<std::string>& classNames, const std::string& outputDir) {
    const int numClasses = classNames.size();
    const int filters = (numClasses + 5) * 3;
    std::ofstream cfgFile(outputDir + "/yolov3.cfg");
    if (!cfgFile) {
        std::cerr << "Error: Could not open yolov3.cfg for writing!" << std::endl;
        return;
    }
    // YOLOv3 Configuration Template (simplified)
    cfgFile << "[net]";
    cfgFile << "batch=64";
    cfgFile << "subdivisions=16";
    cfgFile << "width=416";
    cfgFile << "height=416";
    cfgFile << "channels=3";
    cfgFile << "momentum=0.9";
    cfgFile << "decay=0.0005";
    cfgFile << "angle=0";
    cfgFile << "saturation=1.5";
    cfgFile << "exposure=1.5";
    cfgFile << "hue=.1";
    cfgFile << "learning_rate=0.001";
    cfgFile << "burn_in=1000";
    cfgFile << "max_batches = " << numClasses * 2000 << ""; // Recommended: 2000 * classes
    cfgFile << "policy=steps";
    cfgFile << "steps=" << int(numClasses * 2000 * 0.8) << "," << int(numClasses * 2000 * 0.9) << ""; // 80%, 90% of max_batches
    cfgFile << "scales=.1,.1";
    // Layer Definitions
    for (int i = 0; i < 3; i++) { // YOLOv3 has 3 Yolo layers
        cfgFile << "[convolutional]";
        cfgFile << "filters=" << filters << "";
        cfgFile << "size=1";
        cfgFile << "stride=1";
        cfgFile << "pad=1";
        cfgFile << "activation=linear";
        cfgFile << "[yolo]";
        cfgFile << "mask = " << i * 3 << "," << i * 3 + 1 << "," << i * 3 + 2 << "";
        cfgFile << "anchors = 10,13,  16,30,  33,23,  30,61,  62,45,  59,119,  116,90,  156,198,  373,326";
        cfgFile << "classes=" << numClasses << "";
        cfgFile << "num=9";
        cfgFile << "jitter=.3";
        cfgFile << "ignore_thresh = .7";
        cfgFile << "truth_thresh = 1";
        cfgFile << "random=1";
    }
    cfgFile.close();
    std::cout << "Generated yolov3.cfg" << std::endl;
}
// Function to generate YOLO labels and write metadata files
void generateYOLOLabels(const std::string& trainDir, const std::string& outputDir) {
    if (!fs::exists(trainDir) || !fs::is_directory(trainDir)) {
        std::cerr << "Error: The train folder doesn't exist or is not a directory." << std::endl;
        return;
    }
    std::map<std::string, int> classMap; // Maps class names to indices
    std::vector<std::string> classNames; // Stores class names
    std::vector<std::string> trainList; // List of images to write into train.txt
    for (const auto& entry : fs::directory_iterator(trainDir)) {
        if (fs::is_directory(entry)) {
            std::string className = entry.path().filename().string();
            if (classMap.find(className) == classMap.end()) {
                classMap[className] = classNames.size();
                classNames.push_back(className);
            }
            int classID = classMap[className];
            for (const auto& file : fs::directory_iterator(entry)) {
                if (file.path().extension() == ".jpg" || file.path().extension() == ".png") {
                    std::string imagePath = file.path().string();
                    // Fix for replace_extension issue
                    std::filesystem::path tempPath = file.path(); // Copy path to non-const
                    tempPath.replace_extension(".txt");          // Modify extension
                    std::string labelPath = tempPath.string();   // Convert to string
                    // Create the YOLO label file
                    std::ofstream labelFile(labelPath);
                    if (!labelFile) {
                        std::cerr << "Error: Could not create label file for " << imagePath << std::endl;
                        continue;
                    }
                    // Default YOLO annotation (centered, whole image)
                    labelFile << classID << " 0.5 0.5 1 1" << std::endl;
                    labelFile.close();
                    trainList.push_back(imagePath); // Add to train.txt list
                }
            }
        }
    }
    // Write obj.names
    writeObjNames(classNames, outputDir);
    // Write obj.data
    writeObjData(classNames, outputDir);
    // Write yolov3.cfg
    writeYoloConfig(classNames, outputDir);
    // Write train.txt
    std::ofstream trainFile(outputDir + "/train.txt");
    if (!trainFile) {
        std::cerr << "Error: Could not write train.txt!" << std::endl;
        return;
    }
    for (const auto& trainPath : trainList) {
        trainFile << trainPath << std::endl;
    }
    trainFile.close();
    std::cout << "Generated train.txt with all image paths" << std::endl;
}
int main() {
    std::string trainDir = "/Users/dengfengji/ronnieji/Kaggle/animals"; // Train directory path
    std::string outputDir = "/Users/dengfengji/ronnieji/Kaggle/data";  // Output directory for config files
    // Ensure the output directory exists
    if (!fs::exists(outputDir)) {
        fs::create_directory(outputDir);
    }
    // Generate YOLO annotations and metadata files
    generateYOLOLabels(trainDir, outputDir);
    return 0;
}
