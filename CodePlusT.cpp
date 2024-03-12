#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <termios.h>
#include <unistd.h>
#include <iterator>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "../lib/nemslib.h"

std::string encrypt(const std::string& str, int key) {
    std::string encrypted_str = str;
    std::transform(encrypted_str.begin(), encrypted_str.end(), encrypted_str.begin(), [key](unsigned char c) {
        return c + key;
    });
    return encrypted_str;
}
std::string decrypt(const std::string& str, int key) {
    return encrypt(str, -key);
}
void start_en_folder(const std::string& str_folder_path,const std::string& strKey){
    std::string strMsg;
    if(str_folder_path.empty() || strKey.empty()){
        strMsg = "start_en_folder input empty!";
        std::cout << strMsg << '\n';
        return;
    }
    //std::string folderPath = folder_path; //"/Volumes/WorkDisk/MacBk/pytest/NLP_test/corpus/english_ebooks"; // Open the english book folder path
    for (const auto& entry : std::filesystem::directory_iterator(str_folder_path)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".h" || entry.path().extension() == ".cpp")) {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::string line;
                std::vector<std::string> strCode;
                while (std::getline(file, line)) {
                    std::cout << line << std::endl; // Process the content as needed
                    line = encrypt(line,std::stoi(strKey));
                    strCode.push_back(line);
                }
                std::string strFile_output = str_folder_path;
                if(strFile_output.back()!='/'){
                    strFile_output.append("/");
                }
                strFile_output.append(entry.path().filename());
                strFile_output.append("_.bin");
                std::ofstream ofile(strFile_output,std::ios::out);
                if(!ofile.is_open()){
                    ofile.open(strFile_output,std::ios::out);
                }
                for(const auto& sc : strCode){
                    ofile << sc << '\n';
                }
                ofile.close();
            }
            /*
                delete the original file
            */
            std::filesystem::remove(entry.path());
        }
    }
    std::cout << "Successfully encrypted the folder! at " << str_folder_path << '\n';
}
void start_de_folder(const std::string& str_folder_path,const std::string& strKey){
    std::string strMsg;
    if(str_folder_path.empty() || strKey.empty()){
        strMsg = "start_de_folder input empty!";
        std::cout << strMsg << '\n';
        return;
    }
    //std::string folderPath = folder_path; //"/Volumes/WorkDisk/MacBk/pytest/NLP_test/corpus/english_ebooks"; // Open the english book folder path
    for (const auto& entry : std::filesystem::directory_iterator(str_folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".bin") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::string line;
                std::vector<std::string> strCode;
                while (std::getline(file, line)) {
                    std::cout << line << std::endl; // Process the content as needed
                    line = decrypt(line,std::stoi(strKey));
                    strCode.push_back(line);
                }
                std::string strFile_output = str_folder_path;
                if(strFile_output.back()!='/'){
                    strFile_output.append("/");
                }
                strFile_output.append(entry.path().filename());
                boost::algorithm::replace_all(strFile_output,"_.bin","");
                std::ofstream ofile(strFile_output,std::ios::out);
                if(!ofile.is_open()){
                    ofile.open(strFile_output,std::ios::out);
                }
                for(const auto& sc : strCode){
                    ofile << sc << '\n';
                }
                ofile.close();
            }
        }
    }
    std::cout << "Successfully decrypted the folder! at " << str_folder_path << '\n';
}
std::string readPassword() {
    std::string password;
    struct termios oldSettings, newSettings;

    // Turn off terminal echoing
    tcgetattr(STDIN_FILENO, &oldSettings);
    newSettings = oldSettings;
    newSettings.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);

    // Read password character by character
    char ch;
    while (std::cin.get(ch) && ch != '\n') {
        password.push_back(ch);
    }

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);

    return password;
}
int main(int argc, char **argv) {
    if(argc != 2){
		return 1;
	}
	int choice;
    std::string folder_name = argv[1];
	std::cout << "You've choosed the folder: " << folder_name << '\n';
    std::cout << "Select your option:" << '\n';
    std::cout << "1-Entcryption" << '\n';
    std::cout << "0-Decryption" << '\n';
    std::cin >> choice;
    std::string password;
    std::cout << "Enter password: " << '\n';
    do{
        password = readPassword();
    }while(password.empty());
    std::cout << folder_name << '\n';
    std::cout << password << '\n';
    switch (choice){
        case 1/* constant-expression */:
            /* code */
            std::cout << "You've choosen: " << "1-Entcryption" << '\n';
            std::cout << "Start processing..." << '\n';
            start_en_folder(folder_name,password);
            break;
        case 0:
            std::cout << "You've choosen: " << "0-Entcryption" << '\n';
            std::cout << "Start processing..." << '\n';
            start_de_folder(folder_name,password);
            break;
        default:
            std::cout << "You've choosen: " << "1-Entcryption by default." << '\n';
            std::cout << "Start processing..." << '\n';
            start_en_folder(folder_name,password);
            break;
    }
   
}