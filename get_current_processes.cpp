
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm> // For std::all_of

#ifdef __linux__
#include <dirent.h>
#include <unistd.h>
#elif __APPLE__
#include <sys/sysctl.h>
#include <libproc.h>
#endif

std::map<std::string, int> fetchProcesses() {
    std::map<std::string, int> processes;
#ifdef __linux__
    // On Linux, use the /proc filesystem
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        if (entry.is_directory()) {
            const std::string pidStr = entry.path().filename().string();
            if (std::all_of(pidStr.begin(), pidStr.end(), ::isdigit)) {
                const int pid = std::stoi(pidStr);
                std::string cmdlinePath = entry.path().string() + "/cmdline";
                std::ifstream cmdlineFile(cmdlinePath);
                std::string processName;
                if (cmdlineFile.is_open()) {
                    std::getline(cmdlineFile, processName, '\0');
                    if (processName.empty()) {
                        // If cmdline is empty, fallback to comm
                        std::string commPath = entry.path().string() + "/comm";
                        std::ifstream commFile(commPath);
                        if (commFile.is_open()) {
                            std::getline(commFile, processName);
                        }
                    }
                }
                if (!processName.empty()) {
                    processes[processName] = pid;
                }
            }
        }
    }
#elif __APPLE__
    // On macOS, use sysctl to fetch process information
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t bufferSize;
    // Get the size of the process list
    if (sysctl(mib, 4, nullptr, &bufferSize, nullptr, 0) == -1) {
        perror("sysctl");
        return processes;
    }
    std::vector<char> buffer(bufferSize);
    if (sysctl(mib, 4, buffer.data(), &bufferSize, nullptr, 0) == -1) {
        perror("sysctl");
        return processes;
    }
    struct kinfo_proc* procList = reinterpret_cast<struct kinfo_proc*>(buffer.data());
    int procCount = static_cast<int>(bufferSize / sizeof(struct kinfo_proc));
    for (int i = 0; i < procCount; ++i) {
        const pid_t pid = procList[i].kp_proc.p_pid;
        char processName[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(pid, processName, sizeof(processName)) > 0) {
            processes[processName] = pid;
        }
    }
#endif
    return processes;
}
int main() {
    try {
        auto processes = fetchProcesses();
		std::ofstream ofile("/home/ronnieji/watchdog/current_processes.txt",std::ios::out);
		if(!ofile.is_open()){
			ofile.open("/home/ronnieji/watchdog/current_processes.txt",std::ios::out);
		}
        ofile << "static std::map<std::string,int> pure_linux_processes{" << '\n';
        for (const auto& [name, pid] : processes) {
            ofile << "\t{\"" << name << "\"," << pid << "},\n";
        }
        ofile << "};\n";
		ofile.close();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }
    return 0;
}

