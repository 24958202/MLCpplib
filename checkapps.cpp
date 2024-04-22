#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdio>
bool checkProcessExists(const std::string& processName) {
    std::string command = "pgrep " + processName;
    return system(command.c_str()) == 0;
}
void startProcess(const std::string& processName) {
    std::string command = "sudo -S " + processName;
    FILE* pipe = popen(command.c_str(), "w");
    if (pipe) {
        std::string password = "your_sudo_password_here\n";
        fwrite(password.c_str(), sizeof(char), password.size(), pipe);
        pclose(pipe);
    }
}
int main() {
    std::ifstream processFile("/home/ronnieji/lib/db_tools/apps_running.txt");
    if (!processFile) {
        std::cerr << "Error opening file\n";
        return 1;
    }

    std::string processName;
    while (processFile >> processName) {
        while (true) {
            if (!checkProcessExists(processName)) {
                std::cout << "Process " << processName << " not found. Starting...\n";
                startProcess(processName);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    processFile.close();
    return 0;
}
