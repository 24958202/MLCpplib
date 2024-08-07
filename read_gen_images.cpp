/*
    This program can generate text in 120x120 pixels
*/
#include <opencv2/opencv.hpp>  
#include <opencv2/imgcodecs.hpp>  
#include <opencv2/imgproc.hpp>  
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <string>  
#include <vector>
#include<filesystem>
#include <fstream>
#include "../lib/nemslib.h"
// Custom predicate to decide which characters to erase  
// Struct Definition  
struct imgTxtBin {  
    std::string imgName;  
    cv::Mat img_0;  
    cv::Mat img_left;  
    cv::Mat img_right;  
};  
bool keepOrErase(char c) {  
    return std::ispunct(static_cast<unsigned char>(c)) && c != '\''; // Keep the apostrophe  
}  
cv::Mat write_text_in_an_image(int imgWidth, int imgHeight, const std::string& strText, double txtAngle) {  
    // Calculate the diagonal length to handle rotation  
    int diagonalLength = static_cast<int>(std::sqrt(imgWidth * imgWidth + imgHeight * imgHeight));  
    // Create a larger image to accommodate rotation  
    cv::Mat largeTextImage = cv::Mat::zeros(diagonalLength, diagonalLength, CV_8UC3);  
    // Define text properties  
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;  
    double fontScale = 1.0;    
    int thickness = 2;   
    int baseline = 0;  
    // Calculate the initial text size  
    cv::Size textSize = cv::getTextSize(strText, fontFace, fontScale, thickness, &baseline);  
    baseline += thickness;  
    // Calculate the scale to fit the text to 80% of the image width  
    double scaleFactor = 0.8 * imgWidth / textSize.width;  
    fontScale = scaleFactor;  
    // Recalculate the text size with the new scale  
    textSize = cv::getTextSize(strText, fontFace, fontScale, thickness, &baseline);  
    baseline += thickness;  
    // Calculate the text position to center it on the larger image  
    cv::Point textOrg((largeTextImage.cols - textSize.width) / 2, (largeTextImage.rows + textSize.height) / 2);  
    // Put text onto the blank (larger) image  
    cv::putText(largeTextImage, strText, textOrg, fontFace, fontScale, cv::Scalar(255, 255, 255), thickness);  // White text  
    // Get the center of the larger image  
    cv::Point2f center(largeTextImage.cols / 2.0, largeTextImage.rows / 2.0);  
    // Create the rotation matrix  
    cv::Mat rotMat = cv::getRotationMatrix2D(center, txtAngle, 1.0);  
    // Rotate the large text image  
    cv::Mat rotatedLargeImage;  
    cv::warpAffine(largeTextImage, rotatedLargeImage, rotMat, largeTextImage.size());  
    // Crop the central part of the rotated image to get back to the original size  
    int xCrop = (rotatedLargeImage.cols - imgWidth) / 2;  
    int yCrop = (rotatedLargeImage.rows - imgHeight) / 2;  
    cv::Mat croppedImage = rotatedLargeImage(cv::Rect(xCrop, yCrop, imgWidth, imgHeight));  
    return croppedImage;  
}  
void serializeMat(const imgTxtBin& mat, std::ostream& out) {  
    // Serialize the imgName  
    size_t nameLength = mat.imgName.size();  
    out.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));  
    out.write(mat.imgName.c_str(), nameLength);  
    // Helper lambda to serialize cv::Mat  
    auto serializeCvMat = [&out](const cv::Mat&image) {  
        int rows = image.rows;  
        int cols = image.cols;  
        int type = image.type();  
        bool continuous = image.isContinuous();  
        out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));  
        out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));  
        out.write(reinterpret_cast<const char*>(&type), sizeof(type));  
        out.write(reinterpret_cast<const char*>(&continuous), sizeof(continuous));  
        if (continuous) {  
            int dataSize = rows * cols * image.elemSize();  
            out.write(reinterpret_cast<const char*>(image.ptr()), dataSize);  
        } else {  
            int rowSize = cols * image.elemSize();  
            for (int i = 0; i < rows; ++i) {  
                out.write(reinterpret_cast<const char*>(image.ptr(i)), rowSize);  
            }  
        }  
    };  
    // Serialize the cv::Mat members  
    serializeCvMat(mat.img_0);  
    serializeCvMat(mat.img_left);  
    serializeCvMat(mat.img_right);  
}  
imgTxtBin deserializeMat(std::istream& in) {  
    imgTxtBin matStruct;  
    // Deserialize the imgName  
    size_t nameLength;  
    in.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));  
    matStruct.imgName.resize(nameLength);  
    in.read(&matStruct.imgName[0], nameLength);  
    // Helper lambda to deserialize cv::Mat  
    auto deserializeCvMat = [&in]() {  
        int rows, cols, type;  
        bool continuous;  
        in.read(reinterpret_cast<char*>(&rows), sizeof(rows));  
        in.read(reinterpret_cast<char*>(&cols), sizeof(cols));  
        in.read(reinterpret_cast<char*>(&type), sizeof(type));  
        in.read(reinterpret_cast<char*>(&continuous), sizeof(continuous));  
        cv::Mat image(rows, cols, type);  
        if (continuous) {  
            int dataSize = rows * cols * image.elemSize();  
            in.read(reinterpret_cast<char*>(image.ptr()), dataSize);  
        } else {  
            int rowSize = cols * image.elemSize();  
            for (int i = 0; i < rows; ++i) {  
                in.read(reinterpret_cast<char*>(image.ptr(i)), rowSize);  
            }  
        }  
        return image;  
    };  
    // Deserialize the cv::Mat members  
    matStruct.img_0 = deserializeCvMat();  
    matStruct.img_left = deserializeCvMat();  
    matStruct.img_right = deserializeCvMat();  
    return matStruct;  
}  
void serializeImgTxt(const imgTxtBin& imgStruct, const std::string& filename) {  
    std::ofstream out(filename, std::ios::binary | std::ios::app);  
    if (!out) {  
        throw std::runtime_error("Failed to open file for writing.");  
    }  
    serializeMat(imgStruct, out);  
    out.close();  
}  
imgTxtBin deserializeImgTxt(const std::string& filename, std::streampos& position) {  
    std::ifstream in(filename, std::ios::binary);  
    if (!in) {  
        throw std::runtime_error("Failed to open file for reading.");  
    }  
    in.seekg(position);  
    imgTxtBin imgStruct = deserializeMat(in);  
    position = in.tellg();  
    in.close();  
    return imgStruct;  
}  
void write_img(const std::string& imgword){
    /*
        generate an 120x120 img
        struct imgTxtBin {  
        std::string imgName;  
        cv::Mat img_0;  
        cv::Mat img_left;  
        cv::Mat img_right;  
        };  
    */
    imgTxtBin saveImg;
    saveImg.imgName = imgword;
    saveImg.img_0 = write_text_in_an_image(120,120,imgword, 0);
    saveImg.img_left = write_text_in_an_image(120,120,imgword, -12);
    saveImg.img_right = write_text_in_an_image(120,120,imgword, 12);
    // Serialize and append the imgTxt object to a binary file  
    serializeImgTxt(saveImg, "/Users/dengfengji/ronnieji/imageRecong/output/imgvoc.bin");  
}
void saveWords(const std::vector<std::string>& words_set){
    nemslib nem_j;
    if(!words_set.empty()){
        for(const auto& ws : words_set){
            if(std::filesystem::exists("/Users/dengfengji/ronnieji/imageRecong/samples/words.txt")){
                std::vector<std::string> words_saved = nem_j.readTextFile("/Users/dengfengji/ronnieji/imageRecong/samples/words.txt");
                if (std::find(words_saved.begin(), words_saved.end(), ws) != words_saved.end()) {  
                    continue; // Skip already saved words  
                }  
                else{
                    std::ofstream file("/Users/dengfengji/ronnieji/imageRecong/samples/words.txt", std::ios::app);
                    if(!file.is_open()){
                        file.open("/Users/dengfengji/ronnieji/imageRecong/samples/words.txt", std::ios::app);
                    }
                    file << ws << '\n';
                    file.close();
                    write_img(ws);
                }
            }
            else{
                std::ofstream file("/Users/dengfengji/ronnieji/imageRecong/samples/words.txt", std::ios::app);
                if(!file.is_open()){
                    file.open("/Users/dengfengji/ronnieji/imageRecong/samples/words.txt", std::ios::app);
                }
                file << ws << '\n';
                file.close();
                write_img(ws);
            }
        }
    }
}
void readFolder(const std::string& folder_name){
    if(folder_name.empty()){
        return;
    }
    if(!std::filesystem::exists(folder_name)){
        return;
    }
    for(const auto& entry: std::filesystem::directory_iterator(folder_name)){
        if(entry.is_regular_file() && entry.path().extension() == ".txt"){
            std::cout << "Start reading: " << entry.path() << std::endl;
            std::ifstream file(entry.path());
            if(file.is_open()){
                std::string line;
                std::string strBook;
                while (std::getline(file,line)){
                    boost::algorithm::trim(line);
                    if(!line.empty()){
                        // Use std::remove_if followed by erase to clean up the string  
                        line.erase(std::remove_if(line.begin(), line.end(), keepOrErase), line.end());  
                        boost::algorithm::to_lower(line);
                        strBook += line + " ";
                    }
                }
                strBook.pop_back();
                std::vector<std::string> result;
                boost::algorithm::split(result, strBook, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
                saveWords(result);
                std::cout << "Successfully saved the book..." << std::endl;
            }
            file.close();
        }
    }
}
int main(){
    nemslib nem_j;
    std::vector<std::string> strBooks;
    readFolder("/Users/dengfengji/ronnieji/english_ebooks");
    return 0;
}
