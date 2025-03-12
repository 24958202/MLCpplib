#include <stdlib.h>  
#include <iostream>  
#include <chrono>  
#include <thread>  
#include <vector>  
#include <string>  
#include <filesystem>  
#include <set>  
#include <fstream>  
#include <sstream>  
#include <sys/types.h>  
#include <signal.h>  
#include <map>
#include <exception>
// Function to get network statistics for a specific process  
std::pair<uint64_t, uint64_t> getProcessNetworkStats(pid_t pid) {  
    std::string path = "/proc/" + std::to_string(pid) + "/net/dev";  
    std::ifstream file(path);  
    if (!file.is_open()) {  
        return {0, 0}; // Return 0 if the file cannot be opened  
    }  
    std::string line;  
    uint64_t bytesReceived = 0, bytesSent = 0;  
    // Skip the first two lines (headers)  
    std::getline(file, line);  
    std::getline(file, line);  
    // Parse the remaining lines for network statistics  
    while (std::getline(file, line)) {  
        std::istringstream iss(line);  
        std::string interfaceName;  
        uint64_t receiveBytes, transmitBytes;  
        // Parse the interface name and statistics  
        iss >> interfaceName >> receiveBytes;  
        for (int i = 0; i < 7; ++i) iss >> transmitBytes; // Skip other fields  
        bytesReceived += receiveBytes;  
        bytesSent += transmitBytes;  
    }  
    return {bytesReceived, bytesSent};  
}  
// Function to get the list of all processes  
std::vector<pid_t> getAllProcesses() {  
    std::vector<pid_t> pids;  
    for (const auto &entry : std::filesystem::directory_iterator("/proc")) {  
        if (entry.is_directory()) {  
            std::string dirName = entry.path().filename().string();  
            if (std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {  
                pids.push_back(std::stoi(dirName));  
            }  
        }  
    } 
    return pids;  
}  
// Function to terminate a process by its name  
void terminateProcessByName(const std::string& processName, int pid) {  
    if (kill(pid, SIGKILL) == 0) {  
        std::cout << "Terminated process: " << processName << " (PID: " << pid << ")" << std::endl;  
    } else {  
        std::cerr << "Failed to terminate process: " << processName << " (PID: " << pid << ")" << std::endl;  
    }  
} 
void disableWiFi() {  
    system("sudo DEBIAN_FRONTEND=noninteractive apt install -y iptables-persistent");  
    // Flush existing chains  
    system("sudo iptables -F");  
    system("sudo iptables -X");  
    // Block all packets in INPUT, FORWARD, and OUTPUT chains  
    system("sudo iptables -P INPUT DROP");  
    system("sudo iptables -P FORWARD DROP");  
    system("sudo iptables -P OUTPUT DROP");  
    // Block all packets in INPUT and OUTPUT chains  
    system("sudo iptables -A INPUT -j DROP");  
    system("sudo iptables -A OUTPUT -j DROP");   
}
// Function to monitor processes with data sent/received  
void monitorProcessesWithNetworkActivity() {  
    std::map<pid_t, std::pair<uint64_t, uint64_t>> previousStats; 
	try{
		while (true) {  
			std::this_thread::sleep_for(std::chrono::seconds(1)); // Monitor every second  
			std::vector<pid_t> pids = getAllProcesses();  
			std::map<pid_t, std::pair<uint64_t, uint64_t>> currentStats;  
			for (pid_t pid : pids) {  
				auto stats = getProcessNetworkStats(pid);  
				currentStats[pid] = stats;  
				// Compare with previous stats to calculate activity  
				if (previousStats.find(pid) != previousStats.end()) {  
					uint64_t prevReceived = previousStats[pid].first;  
					uint64_t prevSent = previousStats[pid].second;  
					uint64_t receivedDiff = stats.first - prevReceived;  
					uint64_t sentDiff = stats.second - prevSent;  
					if (receivedDiff > 0 || sentDiff > 0) {  
						// Get the process name  
						std::string processName = "Unknown";  
						std::ifstream cmdlineFile("/proc/" + std::to_string(pid) + "/cmdline");  
						if (cmdlineFile.is_open()) {  
							std::getline(cmdlineFile, processName, '\0');  
						}  
						// Print the process with network activity  
						std::cout << "Process: " << processName  
								  << " (PID: " << pid << ")"  
								  << " | Sent: " << sentDiff << " B"  
								  << " | Received: " << receivedDiff << " B" << std::endl;  
						if(processName.find("raspi_fw_lite") != std::string::npos || processName.find("faceDetectVideo") != std::string::npos){//
							continue;
						}
						terminateProcessByName(processName, pid);
					}  
				}  
			}  
			// Update previous stats for the next iteration  
			previousStats = currentStats;  
		}  
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors." << std::endl;
	}
}  
int main() {  
    std::cout << "Monitoring processes with network activity..." << std::endl;  
    monitorProcessesWithNetworkActivity(); 
	disableWiFi(); 
    return 0;  
}  
