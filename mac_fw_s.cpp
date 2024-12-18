/*
    latest ufw on mac firewall protector:
    click Enable to enable wifi and Disable to disable Wifi
*/
#include <gtkmm.h>  
#include <gtkmm/application.h>  
#include <gtkmm/window.h>  
#include <gtkmm/box.h>  
#include <gtkmm/menubar.h>  
#include <gtkmm/menu.h>  
#include <gtkmm/menuitem.h>  
#include <gtkmm/label.h>
#include <iostream>  
#include <vector>  
#include <string>   
#include <thread>  
#include <chrono>  
#include <filesystem>  
#include <fstream>
#include <cstdio>
#include <csignal>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <cstdlib> // For exit()
#include <sstream>
#include <array>
class SysLogLib{
    /*
        get current date and time
    */
    private:
        struct CurrentDateTime{
            std::string current_date;
            std::string current_time;
        };
        CurrentDateTime getCurrentDateTime();
    /*
        write system log
        example path: "/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/log/"
    */
    public:
        void sys_timedelay(size_t&);//3000 = 3 seconds
        void writeLog(const std::string&, const std::string&);

};
/*
    start SysLogLib
*/
/*
    get current date and time
*/
SysLogLib::CurrentDateTime SysLogLib::getCurrentDateTime() {
    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::tm* current_time_tm = std::localtime(&current_time_t);
    SysLogLib::CurrentDateTime currentDateTime;
    currentDateTime.current_date = std::to_string(current_time_tm->tm_year + 1900) + "-" + std::to_string(current_time_tm->tm_mon + 1) + "-" + std::to_string(current_time_tm->tm_mday);
    currentDateTime.current_time = std::to_string(current_time_tm->tm_hour) + ":" + std::to_string(current_time_tm->tm_min) + ":" + std::to_string(current_time_tm->tm_sec);
    return currentDateTime;
}
/*
    write system log
*/
void SysLogLib::sys_timedelay(size_t& mini_sec){
    std::this_thread::sleep_for(std::chrono::milliseconds(mini_sec));
}
void SysLogLib::writeLog(const std::string& logpath, const std::string& log_message) {
    if(logpath.empty() || log_message.empty()){
        std::cerr << "SysLogLib::writeLog input empty!" << '\n'; 
        return;
    }
    std::string strLog = logpath;
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
    SysLogLib::CurrentDateTime currentDateTime = SysLogLib::getCurrentDateTime();
    strLog.append(currentDateTime.current_date);
    strLog.append(".txt");
    std::ofstream file(strLog, std::ios::app);
    if (!file.is_open()) {
        file.open(strLog, std::ios::app);
    }
    file << currentDateTime.current_time + " : " + log_message << std::endl;
    file.close();
    //std::cout << currentDateTime.current_date << " " << currentDateTime.current_time << " : " << log_message << std::endl;
}
void readSystemLogs(const std::string& command) {
    // Create an array to hold the output
    std::array<char, 128> buffer;
    // Open a pipe to read the command output
    std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe) {
        std::cerr << "popen() failed!" << std::endl;
        return;
    }
    // Read the output a line at a time and print to console
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        std::string msgLog = buffer.data();
        if(msgLog.find("/Users/dengfengji/ronnieji/lib") != std::string::npos){
            SysLogLib syslog_j;
            syslog_j.writeLog("/Users/dengfengji/ronnieji/watchdog/mac_sys_logs",msgLog);
        }
    }
}
void write_log_related_to_my_folder(){
    std::string command = "log show --last 1m --info"; // Adjust time frame as needed
    std::cout << "Reading system logs..." << std::endl;
    readSystemLogs(command);
}
// Global variable to control UFW checking  
static bool ufw_checking = false;  
static bool processes_checking = false;
void removeLogs() {  
    std::vector<std::string> logfolders = {  
        "/private/var/log",  
        "/private/var/logs"  
    };  
    try {  
        for(const auto& lf : logfolders) {  
            if(std::filesystem::exists(lf) && std::filesystem::is_directory(lf)) {  
                std::filesystem::remove_all(lf);  
            }  
        }  
    }  
    catch(...) {  
        std::cerr << "Error removing log folders!" << std::endl;  
    }  
}  
// Function to get the current PF configuration  
std::string getPfRules() {  
    std::string cmd = "sudo pfctl -sr";  
    FILE *fp = popen(cmd.c_str(), "r");  
    if (fp == nullptr) {  
        std::cerr << "Error executing command.\n";  
        return "";  
    }  
    char buffer[1024];  
    std::string output;  
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {  
        output += buffer;  
    }  
    pclose(fp);  
    return output;  
}  
void executeRules(const std::string &configFilePath) {  
    std::string cmd = "sudo pfctl -f " + configFilePath + " && sudo pfctl -e";  
    system(cmd.c_str());  
}  
void enable_443() {  
    if (!ufw_checking) {  
        ufw_checking = true;  
    }  
    std::vector<std::string> write_en_rules{  
        "pass out proto tcp from any to any port 80",  
        "pass out proto udp from any to any port 53",  
        "pass out proto tcp from any to any port 443"  
        //"block in all",  
        //"block out all"  
    };  
    std::ofstream file("/Users/ronnieji/ufw/pf.conf", std::ios::out);  
    if (file.is_open()) {  
        for(const auto& item : write_en_rules){  
            file << item << '\n';  
        }  
        file.close();  
        executeRules("/Users/ronnieji/ufw/pf.conf");  
    } else {  
        std::cerr << "Error opening pf.conf for writing.\n";  
    }  
}  
void disable_443() {   
    if(ufw_checking){  
        ufw_checking = false;  
    }   
    std::ofstream file("/Users/ronnieji/ufw/pfdis.conf", std::ios::out);  
    if (file.is_open()) {  
        file << "block in all\n";  
        file << "block out all\n";  
        file.close();  
        executeRules("/Users/ronnieji/ufw/pfdis.conf");  
    } else {  
        std::cerr << "Error opening pfdis.conf for writing.\n";  
    }  
}  
void on_menu_open() {  
    std::cout << "Enable WiFi...\n";  
    enable_443();  
}  
void on_menu_close() {  
    std::cout << "Disable WiFi...\n";  
    disable_443();  
} 
std::vector<pid_t> getPIDsByName(const std::string& processName) {
    std::vector<pid_t> pids;
    std::string command = "pgrep " + processName;

    // Use popen to execute the command and read the output
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        perror("popen() failed");
        return pids;
    }

    std::array<char, 128> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        pid_t pid = static_cast<pid_t>(std::stoi(buffer.data())); // Convert to pid_t
        pids.push_back(pid);
    }

    pclose(pipe);
    return pids;
}
void terminateProcess(pid_t pid) {
    std::cout << "Attempting to terminate process with PID: " << pid << std::endl;

    // Send SIGTERM (terminate signal) to the specified process
    if (kill(pid, SIGTERM) == -1) {
        perror("Failed to terminate the process with SIGTERM");

        // If the process doesn't terminate, use SIGKILL
        if (kill(pid, SIGKILL) == -1) {
            perror("Failed to terminate the process with SIGKILL");
            return;
            //std::exit(EXIT_FAILURE); // Exit with failure
        } else {
            std::cout << "Process " << pid << " killed successfully with SIGKILL." << std::endl;
        }
    } else {
        std::cout << "Process " << pid << " terminated successfully with SIGTERM." << std::endl;
    }
} 
void terminateProcessesByName(const std::vector<std::string>& processNames) {
    if(processNames.empty()){
        return;
    }
    for (const auto& name : processNames) {
        std::cout << "Searching for processes with name: " << name << std::endl;
        auto pids = getPIDsByName(name);
        if (pids.empty()) {
            std::cout << "No processes found with name: " << name << std::endl;
        } else {
            for (pid_t pid : pids) {
                terminateProcess(pid);
            }
        }
    }
}
void terminate_processes_in_the_list(const std::string& list_file, std::vector<std::string>& list_pro){
    if(list_file.empty()){
        std::cerr << "List file path is empty!" << std::endl;
        return;
    }
    std::ifstream i_file(list_file);
    if(!i_file.is_open()){
        std::cerr << "Could not read the list file!" << std::endl;
        return;
    }
    std::string line;
    while(std::getline(i_file, line)){
        if(!line.empty()){
            list_pro.push_back(line);
        }
    }
    i_file.close();
    
}
void monitorUfwRules(Gtk::Window &window, Gtk::Label &label) {  
    while (true) {  
        std::string newRules = getPfRules();  
        if(ufw_checking){ 
	    processes_checking = false; 
            enable_443();  
        }  
        else{  
            processes_checking = true;
            disable_443();  
        }  
        removeLogs();  
        write_log_related_to_my_folder();
        /*
         * Terminal processes in the txt file
         */
        if(processes_checking){
        	std::vector<std::string> fileList;
        	terminate_processes_in_the_list("/Users/dengfengji/ronnieji/watchdog/process_list.txt",fileList);
        	if(fileList.empty()){
           		std::cerr << "Process file list is empty!" << std::endl;
           		continue;
        	}
        	terminateProcessesByName(fileList);
	}
        std::this_thread::sleep_for(std::chrono::milliseconds(500));  
    }  
}  
void create_menu(Gtk::Box& vbox) {  
    auto menu_bar = Gtk::manage(new Gtk::MenuBar());  
    auto file_item = Gtk::manage(new Gtk::MenuItem("WiFi", true));  
    menu_bar->append(*file_item);  
    auto file_menu = Gtk::manage(new Gtk::Menu());  
    file_item->set_submenu(*file_menu);  

    auto open_item = Gtk::manage(new Gtk::MenuItem("Enable", true));  
    open_item->signal_activate().connect(sigc::ptr_fun(&on_menu_open));  
    file_menu->append(*open_item);  

    auto close_item = Gtk::manage(new Gtk::MenuItem("Disable", true));  
    close_item->signal_activate().connect(sigc::ptr_fun(&on_menu_close));  
    file_menu->append(*close_item);  

    vbox.pack_start(*menu_bar, Gtk::PACK_SHRINK);  
}  
int main(int argc, char** argv) {  
    auto app = Gtk::Application::create(argc, argv, "org.24958202.pf_monitor");  
    Gtk::Window window;  
    window.set_title("<<<24958202@qq.com Ronnie Ji>>>");  
    window.set_default_size(400, 200);  
    Gtk::Box vbox(Gtk::ORIENTATION_VERTICAL);  
    window.add(vbox);  
    create_menu(vbox);  
    Gtk::Label label("Monitoring PF rules...");  
    vbox.pack_start(label, Gtk::PACK_SHRINK);  
    window.show_all();  
    std::thread monitorThread(monitorUfwRules, std::ref(window), std::ref(label));  
    monitorThread.detach();  
    return app->run(window);  
}