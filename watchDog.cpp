#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/inotify.h>
#include <cstdlib> // For std::system
#include <limits.h> // For PATH_MAX
#include <unistd.h>
#include <libgen.h>
#include <ctime>
#include <filesystem>
#include <thread>
#include <chrono>
#include "../lib/lib/nemslib.h"

// Function to extract the file name from a path
struct CurrentDateTime{
    std::string current_date;
    std::string current_time;
};
CurrentDateTime getCurrentDateTime() {
    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::tm* current_time_tm = std::localtime(&current_time_t);
    CurrentDateTime currentDateTime;
    currentDateTime.current_date = std::to_string(current_time_tm->tm_year + 1900) + "-" + std::to_string(current_time_tm->tm_mon + 1) + "-" + std::to_string(current_time_tm->tm_mday);
    currentDateTime.current_time = std::to_string(current_time_tm->tm_hour) + ":" + std::to_string(current_time_tm->tm_min) + ":" + std::to_string(current_time_tm->tm_sec);
    return currentDateTime;
}
void logEvent(const std::string& event, const std::string& folder) {
    SysLogLib syslog_j;
    nlp_lib nl_j;
    std::stringstream ss;
    std::string strLog = "/home/ronnieji/watchdog/log";
    time_t now = time(0);
    tm *ltm = localtime(&now);
    ss << "[" << ltm->tm_hour << ":" << ltm->tm_min << ":" << ltm->tm_sec << "] " << event << " in folder: " << folder << '\n';
    std::string strMsg = ss.str();
    syslog_j.writeLog(strLog,strMsg);
    if(strLog.back() != '/'){
        strLog.append("/");
    }
    if (!std::filesystem::exists(strLog)) {
        try {
            std::filesystem::create_directory(strLog);
        } catch (const std::exception& e) {
            std::cout << "Error creating log folder: " << e.what() << '\n';
            return;
        }
    }
    CurrentDateTime currentDateTime = getCurrentDateTime();
    strLog.append(currentDateTime.current_date);
    strLog.append(".bin");
    nl_j.AppendBinaryOne(strLog,strMsg);
}

void playSound() {
    std::string filePath = "/home/ronnieji/watchdog/alam.mp3";
    // Construct the command by appending the file path
    std::string command = "mpg123 " + filePath;
    // Execute the command
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Failed to play the file with mpg123." << std::endl;
        // Handle the error accordingly
    }
}

int main() {
    std::vector<std::string> log_events_binary;
    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "Error initializing inotify" << std::endl;
        return 1;
    }
    nlp_lib nl_j;
    std::vector<std::string> watch_folders = nl_j.ReadBinaryOne("/home/ronnieji/watchdog/watchfolder.bin");
    std::map<unsigned int, std::string> wdToFolderPath;
    // Setting up watches and storing wd and folder path in the map
    if(!watch_folders.empty()){
        std::cout << "Watchdog is running..." << '\n';
        for (const auto& folderPath : watch_folders) {
            int wd = inotify_add_watch(fd, folderPath.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
            if (wd >= 0) {
                wdToFolderPath[wd] = folderPath;
            }
        }
        char buf[4096];
        ssize_t len;
        struct inotify_event *event;
        while (true) {
            len = read(fd, buf, sizeof(buf));
            event = reinterpret_cast<struct inotify_event *>(buf);
            std::string folderPath;
            if(wdToFolderPath.find(event->wd) != wdToFolderPath.end()){
                folderPath = wdToFolderPath[event->wd];
            }
            else{
                folderPath = "Unknown_folder";
            }
            std::string eventName = (event->len > 0) ? std::string(event->name) : "Unknown";
            std::string fullPath = folderPath + "/" + eventName;

            if (event->mask & IN_MODIFY) {
                logEvent("File modified", fullPath + " >> "  + std::string(event->name));
                playSound();
            }
            if (event->mask & IN_CREATE) {
                logEvent("File created", fullPath + " >> "  + std::string(event->name));
                playSound();
            }
            if (event->mask & IN_DELETE) {
                logEvent("File deleted", fullPath + " >> "  + std::string(event->name));
                playSound();
            }
        }
        close(fd);
    }
    return 0;
}
