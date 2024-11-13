/*
    Program to pick up trained image from the image net according to the marked image location and size
    It also provide a compress function to compress the image after the image is saved.
*/
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <set>
#include <ranges>
#include <string_view>
#include <stdexcept>
#include <utility>
#include <opencv2/opencv.hpp>
#include <jpeglib.h>
#include <stdlib.h>

std::vector<std::string> splitString(const std::string& input, char delimiter){
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
std::vector<std::string> JsplitString_bystring(const std::string& input, const std::string& delimiter){
    std::vector<std::string> tokens;
    if(input.empty() || delimiter.empty()){
        std::cerr << "Jsonlib::splitString_bystring input empty" << '\n';
    	return tokens;
    }
    size_t start = 0;
    size_t end = input.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(input.substr(start, end - start));
        start = end + delimiter.length();
        end = input.find(delimiter, start);
    }
    tokens.push_back(input.substr(start, end));
    return tokens;
}
std::string Jtrim(const std::string& str_input) {
    // Check if the input string is empty
    if (str_input.empty()) {
        return {};
    }
    // Lambda to check for non-space characters
    auto is_not_space = [](unsigned char ch) { return !std::isspace(ch); };
    // Find the start of the non-space characters
    auto start = std::ranges::find_if(str_input, is_not_space);
    // Find the end of the non-space characters by reverse iteration
    auto end = std::ranges::find_if(str_input | std::views::reverse, is_not_space).base();
    return (start < end) ? std::string(start, end) : std::string{};
}
std::vector<std::string> tokenize_en(const std::string& str_line) {
    std::vector<std::string> result;
    if(str_line.empty()){
        std::cerr << "nemslib::tokenize_en input empty!" << '\n';
        return result;
    }
    std::stringstream ss(str_line);
    for(const auto& token : std::ranges::istream_view<std::string>(ss)){
        result.push_back(token);
    }
    return result;
}
/*
    compress an image
*/
void compressJPEG(const std::string& inputFilename, const std::string& outputFilename, int quality) {
    if(inputFilename.empty() || outputFilename.empty()){
        return;
    }
    // Create a JPEG compression struct and error handler
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    // Open the input file
    FILE* infile = fopen(inputFilename.c_str(), "rb");
    if (!infile) {
        throw std::runtime_error("Unable to open input file: " + inputFilename);
    }
    // Initialize JPEG decompression
    jpeg_decompress_struct dinfo;
    dinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&dinfo);
    jpeg_stdio_src(&dinfo, infile);
    jpeg_read_header(&dinfo, TRUE);
    jpeg_start_decompress(&dinfo);
    // Get image properties
    int width = dinfo.output_width;
    int height = dinfo.output_height;
    int numChannels = dinfo.num_components;
    // Allocate memory for the image data
    unsigned char* buffer = new unsigned char[width * height * numChannels];
    while (dinfo.output_scanline < height) {
        unsigned char* row_pointer = buffer + dinfo.output_scanline * width * numChannels;
        jpeg_read_scanlines(&dinfo, &row_pointer, 1);
    }
    // Finish decompression and close the input file
    jpeg_finish_decompress(&dinfo);
    fclose(infile);
    jpeg_destroy_decompress(&dinfo);
    // Set up compression
    FILE* outfile = fopen(outputFilename.c_str(), "wb");
    if (!outfile) {
        delete[] buffer;
        throw std::runtime_error("Unable to open output file: " + outputFilename);
    }
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = numChannels;
    cinfo.in_color_space = (numChannels == 3) ? JCS_RGB : JCS_GRAYSCALE;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_stdio_dest(&cinfo, outfile);
    // Start compression
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
        unsigned char* row_pointer = buffer + cinfo.next_scanline * width * numChannels;
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }
    // Finish compression and clean up
    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
    delete[] buffer;
    std::cout << "JPEG image compressed and saved to " << outputFilename << std::endl;
}
std::unordered_map<std::string,std::string> get_synset_mapping(const std::string& synset_file_path){
    std::unordered_map<std::string,std::string> result;
    if(synset_file_path.empty()){
        return result;
    }
    if(std::filesystem::exists(synset_file_path)){
        std::ifstream ifile(synset_file_path);
        if(!ifile.is_open()){
            std::cerr << "Could not open the syset mapping file: " << synset_file_path << std::endl;
            return result;
        }
        std::string line;
        while (std::getline(ifile, line)) {
            if(!line.empty()){
                line = Jtrim(line);
                std::string n_string = line.substr(0,9);
                std::string name_string = line.substr(10);
                name_string = Jtrim(name_string);
                if(!n_string.empty() && !name_string.empty()){
                    result[n_string] = name_string;
                }
            }
        }
        ifile.close();
    }
    return result;
}
std::vector<std::pair<std::string,std::vector<int>>> get_pos_info(const std::string& train_pos_file_path){
    std::vector<std::pair<std::string,std::vector<int>>> result;
    if(train_pos_file_path.empty()){
        return result;
    }
    if(std::filesystem::exists(train_pos_file_path)){
        std::ifstream ifile(train_pos_file_path);
        if(!ifile.is_open()){
            std::cerr << "Could not open the train pos file: " << train_pos_file_path << std::endl;
            return result;
        }
        std::string line;
        while (std::getline(ifile, line)) {
            if(!line.empty()){
                line = Jtrim(line);
                std::vector<std::string> get_str_line = tokenize_en(line);
                std::vector<std::string> get_file_name;
                if(!get_str_line.empty()){
                    get_file_name = splitString(get_str_line[0],',');
                }
                if(get_file_name.empty()){
                    std::cerr << "Failed to get file name from std::vector<std::pair<std::string,std::vector<int>>> get_pos_info " << std::endl;
                    return result;
                }
                /*
                    look for the first 9 chars to get the n_string
                */
                std::string n_string;
                if(!get_str_line.empty()){
                    n_string = get_str_line[0].substr(0,9);
                }
                bool multiple_parts_image_pos_size = false;
                for(auto it = get_str_line.begin(); it != get_str_line.end(); ){
                    if (*it == n_string){
                        multiple_parts_image_pos_size = true;
                        break;
                    } else {
                        ++it;
                    }
                }
                if(multiple_parts_image_pos_size ==true){
                    if(!n_string.empty()){
                        n_string = Jtrim(n_string);
                        std::vector<std::string> split_line_by_n_string = JsplitString_bystring(line,n_string);
                        if(!split_line_by_n_string.empty()){
                            for(unsigned int i = 0; i < split_line_by_n_string.size(); ++i){
                                if(split_line_by_n_string[i].empty()){
                                    continue;
                                }
                                std::vector<std::string> split_line = tokenize_en(split_line_by_n_string[i]);
                                if(i == 0){
                                    if(!split_line.empty() 
                                        && !split_line[1].empty() &&
                                        !split_line[2].empty() && 
                                        !split_line[3].empty() && 
                                        !split_line[4].empty()){
                                        std::vector<int> img_pos_size{
                                            std::stoi(split_line[1]),
                                            std::stoi(split_line[2]),
                                            std::stoi(split_line[3]),
                                            std::stoi(split_line[4])
                                        };
                                        result.push_back(std::make_pair(get_file_name[0],img_pos_size));
                                    }
                                }
                                else{
                                    if(!split_line.empty() 
                                        && !split_line[0].empty() &&
                                        !split_line[1].empty() && 
                                        !split_line[2].empty() && 
                                        !split_line[3].empty()){
                                        std::vector<int> img_pos_size{
                                                std::stoi(split_line[0]),
                                                std::stoi(split_line[1]),
                                                std::stoi(split_line[2]),
                                                std::stoi(split_line[3])
                                        };
                                        result.push_back(std::make_pair(get_file_name[0],img_pos_size));
                                    }
                                }
                            }
                        }
                    }
                    else{
                        std::cerr << "n_string is empty!" << std::endl;
                    }
                }
                else{
                    if(!get_str_line.empty() && 
                        !get_str_line[1].empty() && 
                        !get_str_line[2].empty() &&
                        !get_str_line[3].empty() &&
                        !get_str_line[4].empty()){
                        std::vector<int> img_pos_size{
                            std::stoi(get_str_line[1]),
                            std::stoi(get_str_line[2]),
                            std::stoi(get_str_line[3]),
                            std::stoi(get_str_line[4])
                        };
                        std::vector<std::string> get_n_string = splitString(get_str_line[0],',');
                        if(get_n_string.size() == 2){
                            result.push_back(std::make_pair(get_n_string[0],img_pos_size));
                        }
                        else{
                            std::cerr << "get_n_string is empty!" << std::endl;
                        }
                    }
                }
            }
        }
        ifile.close();
    }
    return result;
}
void renamefolder_get_image_by_pos(const std::string& folder_path, 
                                     const std::unordered_map<std::string, std::string>& fileNameMapping, 
                                     const std::vector<std::pair<std::string, std::vector<int>>>& imgPosSize,const std::string& output_folder_path) {
    if (folder_path.empty() || fileNameMapping.empty() || imgPosSize.empty()) {
        return;
    }
    if (!std::filesystem::exists(folder_path)) {
        std::cerr << "The folder does not exist" << std::endl;
        return;
    }
    try {
        for (const auto& entryMainFolder : std::filesystem::directory_iterator(folder_path)) {  
            if (entryMainFolder.is_directory()) {  
                std::string sub_folder_path = entryMainFolder.path().string();
                std::cout << "Start working on folder: " << sub_folder_path << std::endl;
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file() && 
                        (entrySubFolder.path().extension() == ".JPEG" || entrySubFolder.path().extension() == ".jpeg")) {   
                        std::string imgFilePath = entrySubFolder.path().string(); 
                        std::string imgFileName = entrySubFolder.path().filename().string();
                        imgFileName = imgFileName.substr(0, imgFileName.find_last_of('.'));
                        for (const auto& pair : imgPosSize) {
                            if (pair.first == imgFileName) {
                                std::vector<int> pos_size_of_the_image = pair.second;
                                if (!pos_size_of_the_image.empty()) {
                                    cv::Mat image = cv::imread(imgFilePath);
                                    if (image.empty()) {
                                        std::cerr << "Error: Unable to load image: " << imgFilePath << std::endl;
                                        continue; // Skip to the next image
                                    }
                                    // Debug print the image size and requested ROI
                                    std::cout << "Start working on image: " << imgFilePath << std::endl;
                                    std::cout << "Image size: " << image.cols << "x" << image.rows << std::endl;
                                    std::cout << "Requested ROI: [" << pos_size_of_the_image[0] << ", "
                                              << pos_size_of_the_image[1] << ", "
                                              << pos_size_of_the_image[2] << ", "
                                              << pos_size_of_the_image[3] << "]" << std::endl;
                                    // Validate and adjust ROI dimensions
                                    int x = pos_size_of_the_image[0];
                                    int y = pos_size_of_the_image[1];
                                    int width = pos_size_of_the_image[2];
                                    int height = pos_size_of_the_image[3];
                                    // Adjust width and height if they exceed the image boundaries
                                    if (x < 0) x = 0;
                                    if (y < 0) y = 0;
                                    if (x + width > image.cols) {
                                        width = image.cols - x; // Adjust width if it exceeds bounds
                                    }
                                    if (y + height > image.rows) {
                                        height = image.rows - y; // Adjust height if it exceeds bounds
                                    }
                                    // If adjusted dimensions still invalidate the ROI, report it
                                    if (width <= 0 || height <= 0) {
                                        std::cerr << "Error: The adjusted ROI is invalid." << std::endl;
                                        continue; // Skip to the next image
                                    }
                                    // Create a rectangle for the ROI
                                    cv::Rect roi(x, y, width, height);
                                    cv::Mat imageROI = image(roi); // Extract the ROI from the image
                                    std::string n_folder_name = imgFileName.substr(0, imgFileName.find_last_of('_'));
                                    // Access filename mapping safely
                                    auto it = fileNameMapping.find(n_folder_name);
                                    if (it != fileNameMapping.end()) {
                                        std::string outputFileName = it->second; // Get the corresponding output filename
                                        // Construct destination folder name
                                        std::string des_folder_name = output_folder_path + '/' + outputFileName;
                                        if (!std::filesystem::exists(des_folder_name)) {
                                            try {
                                                if (std::filesystem::create_directories(des_folder_name)) {
                                                    std::cout << "Folder created: " << des_folder_name << std::endl;
                                                }
                                            } catch (const std::filesystem::filesystem_error& e) {
                                                std::cerr << "Filesystem error: " << e.what() << std::endl;
                                            } catch (const std::exception& e) {
                                                std::cerr << "Error: " << e.what() << std::endl;
                                            }
                                        }
                                        // Save the extracted ROI
                                        std::string outputImagePath = des_folder_name + "/" + imgFileName + ".JPEG";
                                        cv::imwrite(outputImagePath, imageROI);
                                        std::cout << "Saved ROI to: " << outputImagePath << std::endl;
                                    } else {
                                        std::cerr << "Error: Key '" << imgFileName << "' not found in fileNameMapping." << std::endl;
                                        continue; // Skip to the next image
                                    }
                                } else {
                                    std::cerr << "pos_size_of_the_image is empty!" << std::endl;
                                }
                                break; // Exit the loop after processing this image
                            }
                        }
                    }
                }
            }
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}
void img_compress(const std::string& input_folder){
    if(input_folder.empty()){
        return;
    }
     if (!std::filesystem::exists(input_folder)) {
        std::cerr << "The folder does not exist" << std::endl;
        return;
    }
    try {
        for (const auto& entryMainFolder : std::filesystem::directory_iterator(input_folder)) {  
            if (entryMainFolder.is_directory()) {  
                std::string sub_folder_path = entryMainFolder.path().string();
                std::cout << "Start working on folder: " << sub_folder_path << std::endl;
                for (const auto& entrySubFolder : std::filesystem::directory_iterator(sub_folder_path)) {  
                    if (entrySubFolder.is_regular_file() && 
                        (entrySubFolder.path().extension() == ".JPEG" || entrySubFolder.path().extension() == ".jpeg")) {   
                        std::string imgFilePath = entrySubFolder.path().string(); 
                        compressJPEG(imgFilePath,imgFilePath,18);
                        std::cout << "Successfully compressed the image: " << imgFilePath << std::endl;
                    }
                }
            }
        }
    }
    catch(std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }

}
int main(){
    std::unordered_map<std::string,std::string> getmapping = get_synset_mapping("/Users/dengfengji/ronnieji/Kaggle/imagenet/ILSVRC/LOC_synset_mapping.txt");
    std::vector<std::pair<std::string,std::vector<int>>> get_img_pos = get_pos_info("/Users/dengfengji/ronnieji/Kaggle/imagenet/ILSVRC/LOC_train_pos.csv");
    if(!getmapping.empty()){
        renamefolder_get_image_by_pos(
            "/Users/dengfengji/ronnieji/Kaggle/imagenet/ILSVRC/Data/CLS-LOC/train",
            getmapping,
            get_img_pos,
            "/Users/dengfengji/ronnieji/Kaggle/imagenet/ILSVRC/Done"
        );
    }
    else{
        std::cout << "getmapping is empty!" << std::endl;
    }
    std::cout << "All jobs are done! Start compressing images..." << std::endl;
    img_compress("/Users/dengfengji/ronnieji/Kaggle/imagenet/ILSVRC/Done");
    std::cout << "All jobs are done!" << std::endl;
    return 0;
}