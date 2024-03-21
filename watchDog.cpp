#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/inotify.h>
#include <unistd.h>
#include <ctime>
#include <filesystem>
#include <thread>
#include <chrono>
#include "../lib/lib/nemslib.h"
//#include <SDL2/SDL.h>
//#include <SDL2/SDL_mixer.h>
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
    syslog_j.writeLog(strLog,ss.str());
    if(strLog.back() != '/'){
        strLog.append("/");
    }
    if (!std::filesystem::exists(strLog)) {
        try {
            std::filesystem::create_directory(strLog);
        } catch (const std::exception& e) {
            std::cout << "Error creating log folder: " << e.what() << std::endl;
            return;
        }
    }
    CurrentDateTime currentDateTime = getCurrentDateTime();
    strLog.append(currentDateTime.current_date);
    strLog.append(".bin");
    nl_j.AppendBinaryOne(strLog,ss.str());
}

// void playSound() {
//     Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
//     Mix_Music *sound = Mix_LoadMUS("sound.mp3");
//     Mix_PlayMusic(sound, 1);
// }

int main() {
    //SDL_Init(SDL_INIT_AUDIO);
    std::vector<std::string> log_events_binary;
    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "Error initializing inotify" << std::endl;
        return 1;
    }
    nlp_lib nl_j;
    std::vector<std::string> watch_folders = nl_j.ReadBinaryOne("/home/ronnieji/watchdog/watchfolder.bin");
    if(!watch_folders.empty()){
        for(const auto& wf : watch_folders){
            int wd = inotify_add_watch(fd, wf.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
            if (wd == -1) {
                std::cerr << "Error adding watch for folder: " << wf << std::endl;
            }
        }
    }
    else{
        std::cerr << "Watch_folders is empty!" << '\n';
    }

    char buf[4096];
    ssize_t len;
    struct inotify_event *event;

    while (true) {
        len = read(fd, buf, sizeof(buf));
        event = reinterpret_cast<struct inotify_event *>(buf);

        if (event->mask & IN_MODIFY) {
            logEvent("File modified", event->name);
            //playSound();
        }
        if (event->mask & IN_CREATE) {
            logEvent("File created", event->name);
            //playSound();
        }
        if (event->mask & IN_DELETE) {
            logEvent("File deleted", event->name);
            //playSound();
        }
    }

    close(fd);
    //Mix_Quit();
    //SDL_Quit();
    return 0;
}
