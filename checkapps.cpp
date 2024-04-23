#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <termios.h>
#include <unistd.h>
#include <iterator>
#include <cstdio>
#include <memory>
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
std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != nullptr) {
            result += buffer;
        }
    }
    return result;
}
void exeCMD(const std::string& strCMD, const std::string& strPass){
    // Construct the full command with echo to provide the password
    //std::string fullCmd = "echo '" + strCMD + "' | " + strPass;
    std::string fullCmd = "echo '" + strPass + "' | sudo -S " + strCMD;
    // Execute the command with sudo and password
    std::string output = exec(fullCmd.c_str());
}
std::string get_pass(){
    std::ifstream file("/home/ronnieji/lib/db_tools/pas.cpp_.bin");
    std::string line;
    if(file){
        std::getline(file,line);
        line = decrypt(line,1234);
    }
    return line;
    file.close();
}
int main() {
    std::string processListFile = "/home/ronnieji/lib/db_tools/apps_running.txt"; // replace with your file name
    std::string sudoPwd = get_pass(); // replace with your sudo password
    std::vector<std::string> processList;
    {
        std::ifstream file(processListFile);
        if (!file) {
            std::cerr << "Failed to open process list file." << std::endl;
            return 1;
        }

        std::string line;
        while (std::getline(file, line)) {
            processList.push_back(line);
        }
        file.close();
    }
    while (true) {
        for (const auto& process : processList) {
            std::string cmd = "pidof " + process;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                std::cout << "Process " << process << " is not running. Starting it..." << std::endl;
                if(process == "webcrawler_english_binary_wordCollections"){
                     /*
                        delete the current link list
                     */
                    std::string filePath = "/home/ronnieji/lib/db_tools/webUrls/str_stored_urls.bin";
                    if (std::filesystem::exists(filePath)) {
                        // Delete the file
                        std::filesystem::remove(filePath);
                    } else {
                        std::cout << "str_stored_urls does not exist or cannot be deleted." << std::endl;
                    }
                     cmd = "sudo /home/ronnieji/lib/db_tools/" + process + " /home/ronnieji/corpus/english_ebooks"; // assume the process needs sudo privileges
                }
                else{
                    cmd = "sudo /home/ronnieji/lib/db_tools/" + process; // assume the process needs sudo privileges
                }
                
            }
            exeCMD(cmd,sudoPwd);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5)); // wait 5 seconds
    }
    return 0;
}
/*
    run_process.sh
*/
/*
    #!/bin/bash

    process="$1"
    sudoPwd="$2"

    pid=$(pidof "$process")
    if [ -z "$pid" ]; then
        echo "Process $process is not running. Starting it..."
        echo "$sudoPwd" | sudo -S "$process"
    fi
*/
