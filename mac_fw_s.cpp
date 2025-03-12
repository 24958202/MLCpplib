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
#include <cstdint> 
#include <csignal>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <signal.h>
#include <cstdlib> // For exit()
#include <sstream>
#include <array>
#include <net/if.h>  
#include <net/if_dl.h>  
#include <ifaddrs.h>  
#include <libproc.h> // For proc_pidpath and other process-related functions
#include <unistd.h>  // For getpid()
#include <mach/mach.h> //use mac's system api, much add '-framework System' at the compile command
#include <map>

// Global variables for thread control  
static std::atomic<bool> ufw_checking = false;  
static std::atomic<bool> processes_checking = false;  
static std::atomic<bool> monitor_network = true; // Used to stop the network monitoring thread  
static std::vector<std::string> users_to_keep{  
    "root",  
    "dengfengji"  
};  
// Function to terminate a process by PID  
void terminateProcess(pid_t pid) {  
    std::cout << "Attempting to terminate process with PID: " << pid << std::endl;  
    try {  
        // Check if the process exists  
        if (kill(pid, 0) == -1) {  
            perror("Error: Process does not exist or cannot be accessed");  
            return;  
        }  
        // Send SIGTERM (graceful termination) to the specified process  
        if (kill(pid, SIGTERM) == -1) {  
            perror("SIGTERM failed");  
            // If the process doesn't terminate, send SIGKILL  
            if (kill(pid, SIGKILL) == -1) {  
                perror("SIGKILL also failed");  
            } else {  
                std::cout << "Process " << pid << " forcefully terminated with SIGKILL." << std::endl;  
            }  
        } else {  
            std::cout << "Process " << pid << " terminated successfully with SIGTERM." << std::endl;  
        }  
    } catch (const std::exception &ex) {  
        std::cerr << ex.what() << std::endl;  
    } catch (...) {  
        std::cerr << "Unknown error." << std::endl;  
    }  
}  
// Function to get the process name by PID  
std::string getProcessName(pid_t pid) {  
    char pathBuffer[PROC_PIDPATHINFO_MAXSIZE];  
    try {  
        if (proc_pidpath(pid, pathBuffer, sizeof(pathBuffer)) > 0) {  
            return std::string(pathBuffer);  
        }  
    } catch (const std::exception &ex) {  
        std::cerr << ex.what() << std::endl;  
    } catch (...) {  
        std::cerr << "Unknown error." << std::endl;  
    }  
    return "Unknown";  
}  
// Function to get network statistics for all interfaces  
std::map<std::string, std::pair<uint64_t, uint64_t>> getNetworkStats() {  
    std::map<std::string, std::pair<uint64_t, uint64_t>> stats; // Map of interface name to (bytes sent, bytes received)  
    struct ifaddrs *ifap, *ifa;  
    if (getifaddrs(&ifap) == 0) {  
        for (ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {  
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK) {  
                struct if_data *ifd = reinterpret_cast<struct if_data *>(ifa->ifa_data);  
                if (ifd) {  
                    std::string interfaceName = ifa->ifa_name;  
                    uint64_t bytesSent = ifd->ifi_obytes;   // Outgoing bytes  
                    uint64_t bytesReceived = ifd->ifi_ibytes; // Incoming bytes  
                    stats[interfaceName] = {bytesSent, bytesReceived};  
                }  
            }  
        }  
        freeifaddrs(ifap);  
    } else {  
        std::cerr << "Failed to get network interfaces." << std::endl;  
    }  
    return stats;  
}  
// Function to monitor network usage and terminate processes with activity  
void monitorNetworkUsage() {  
    pid_t selfPid = getpid(); // Get the PID of the current process  
    std::map<std::string, std::pair<uint64_t, uint64_t>> previousStats = getNetworkStats();  
    while (monitor_network) { // Check the exit condition
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Monitor every second  
		if(ufw_checking){
			continue;
		}
        auto currentStats = getNetworkStats();  
        std::cout << "Network Usage (Bytes/sec):" << std::endl;  
        for (const auto &[interfaceName, currentData] : currentStats) {  
            auto previousData = previousStats[interfaceName];  
            uint64_t sentPerSec = currentData.first - previousData.first;  
            uint64_t receivedPerSec = currentData.second - previousData.second;  
            std::cout << "Interface: " << interfaceName  
                      << " | Sent: " << sentPerSec << " B/s"  
                      << " | Received: " << receivedPerSec << " B/s" << std::endl;  
            // If there is network activity, find and terminate associated processes  
            if (sentPerSec > 0 || receivedPerSec > 0) {  
                int numProcesses = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);  
                if (numProcesses <= 0) {  
                    std::cerr << "Failed to list processes." << std::endl;  
                    continue;  
                }  
                std::vector<pid_t> pids(numProcesses);  
                numProcesses = proc_listpids(PROC_ALL_PIDS, 0, pids.data(), sizeof(pid_t) * pids.size());  
                if (numProcesses <= 0) {  
                    std::cerr << "Failed to retrieve process list." << std::endl;  
                    continue;  
                }  
                for (int i = 0; i < numProcesses; ++i) {  
                    pid_t pid = pids[i];  
                    if (pid == 0 || pid == selfPid) continue; // Skip invalid PIDs and the current process  
                    std::string processName = getProcessName(pid);  
                    if (processName == "mac_fw_s") continue; // Skip specific processes  
                    // Log and terminate the process  
                    std::cout << "Terminating process: " << processName << " (PID: " << pid << ")" << std::endl;  
                    terminateProcess(pid);  
                }  
            }  
        }  
        std::cout << "----------------------------------------" << std::endl;  
        previousStats = currentStats; // Update previous stats for the next iteration  
    }  
    std::cout << "Exiting network monitor thread..." << std::endl;  
}  
// Function to enable network rules  
void enable_443() {  
    if (!ufw_checking) {  
        ufw_checking = true;  
    }  
    std::vector<std::string> write_en_rules{  
        "pass out proto tcp from any to any port 80",  
        "pass out proto udp from any to any port 53",  
        "pass out proto tcp from any to any port 443"  
    };  
    std::ofstream file("/Users/ronnieji/ufw/pf.conf", std::ios::out);  
    if (file.is_open()) {  
        for (const auto &item : write_en_rules) {  
            file << item << '\n';  
        }  
        file.close();  
        std::system("sudo pfctl -f /Users/ronnieji/ufw/pf.conf && sudo pfctl -e");  
    } else {  
        std::cerr << "Error opening pf.conf for writing.\n";  
    }  
}  
// Function to disable network rules  
void disable_443() {  
    if (ufw_checking) {  
        ufw_checking = false;  
    }  
    std::ofstream file("/Users/ronnieji/ufw/pfdis.conf", std::ios::out);  
    if (file.is_open()) {  
        file << "block in all\n";  
        file << "block out all\n";  
        file.close();  
        std::system("sudo pfctl -f /Users/ronnieji/ufw/pfdis.conf && sudo pfctl -e");  
    } else {  
        std::cerr << "Error opening pfdis.conf for writing.\n";  
    }  
}  
// Menu callbacks  
void on_menu_open() {  
    std::cout << "Enable WiFi...\n";  
    enable_443();  
}  
void on_menu_close() {  
    std::cout << "Disable WiFi...\n";  
    disable_443();  
}  
void clearHistory(const std::string& historyFile) {  
    try {  
        // Check if the history file exists  
        if (std::filesystem::exists(historyFile)) {  
            // Overwrite the file with an empty file  
            std::ofstream ofs(historyFile, std::ios::trunc);  
            if (ofs.is_open()) {  
                ofs.close();  
                std::cout << "Cleared history file: " << historyFile << std::endl;  
            } else {  
                std::cerr << "Failed to open history file: " << historyFile << std::endl;  
            }  
        } else {  
            std::cout << "History file does not exist: " << historyFile << std::endl;  
        }  
    }
	catch (const std::exception& e) {  
        std::cerr << "Error clearing history file: " << e.what() << std::endl;  
    }
	catch(...){
		std::cerr << "Unknown errors" << std::endl;
	}
}  
void reloadShellHistory() {  
    // Reload the shell history (works for bash and zsh)  
	try{
		std::system("history -c"); // Clear the in-memory history  
		std::system("history -w"); // Write the cleared history to the file  
		std::cout << "Reloaded shell history." << std::endl;  
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors" << std::endl;
	}
}  
void on_clean_terminal(){
	// Paths to common shell history files  
    const std::string bashHistory = std::getenv("HOME") + std::string("/.bash_history");  
    const std::string zshHistory = std::getenv("HOME") + std::string("/.zsh_history"); 
	// Clear history for bash and zsh  
    clearHistory(bashHistory);  
    clearHistory(zshHistory);  
    // Reload the shell history  
    reloadShellHistory();  
}
std::vector<std::string> get_list_users(){
	std::vector<std::string> c_users;
	try{
		FILE* pipe = popen("dscl . list /Users", "r");
		if(!pipe){
			std::cerr << "Failed to run dscl . list /Users" << std::endl;
			return c_users;
		}
		char buffer[128];
		while(fgets(buffer, sizeof(buffer), pipe) != nullptr){
			std::string user(buffer);
			user.erase(user.find_last_of(" \n\r\t") + 1);
			c_users.push_back(user);
		}
		pclose(pipe);
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors" << std::endl;
	}
	return c_users;
}
void removeOtherUsers(){
	try{
		std::vector<std::string> getUsers = get_list_users();
		if(!getUsers.empty()){
			for(const auto& item : getUsers){
				bool is_in_friend_list = false;
				for(const auto& friendItem : users_to_keep){
					if(item.find(friendItem) != std::string::npos){
						is_in_friend_list = true;
						break;
					}
				}
				if(!is_in_friend_list){
					std::string str_comm = "sudo dscl . -delete /Users/" + item;
					std::system(str_comm.c_str());
				}
			}
		}
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors" << std::endl;
	}
	std::cout << "Successfully removed other users." << std::endl;
}
// Function to create the menu  
void create_menu(Gtk::Box &vbox) {  
    auto file_item = Gtk::manage(new Gtk::MenuItem("WiFi", true));  
    auto file_menu = Gtk::manage(new Gtk::Menu());  
    file_item->set_submenu(*file_menu);  
    auto open_item = Gtk::manage(new Gtk::MenuItem("Enable", true));  
    open_item->signal_activate().connect(sigc::ptr_fun(&on_menu_open));  
    file_menu->append(*open_item);  
    auto close_item = Gtk::manage(new Gtk::MenuItem("Disable", true));  
    close_item->signal_activate().connect(sigc::ptr_fun(&on_menu_close));  
    file_menu->append(*close_item); 
	// Add a separator  
    auto separator = Gtk::manage(new Gtk::SeparatorMenuItem());  
    file_menu->append(*separator);  
	auto remove_users = Gtk::manage(new Gtk::MenuItem("Remove other users", true));  
	file_menu->append(*remove_users);
    remove_users->signal_activate().connect(sigc::ptr_fun(&removeOtherUsers));  
	auto separator2 = Gtk::manage(new Gtk::SeparatorMenuItem());  
    file_menu->append(*separator2);  
	//add clean history 
	auto mnuCleanHistory = Gtk::manage(new Gtk::MenuItem("Clean terminal history", true));  
    mnuCleanHistory->signal_activate().connect(sigc::ptr_fun(&on_clean_terminal));  
    file_menu->append(*mnuCleanHistory);  
	auto menu_bar = Gtk::manage(new Gtk::MenuBar());  
	menu_bar->append(*file_item);  
    // Add the menu bar to the vbox  
    vbox.pack_start(*menu_bar, Gtk::PACK_SHRINK);  
}  
// Main function  
int main(int argc, char **argv) {  
    auto app = Gtk::Application::create(argc, argv, "org.24958202.pf_monitor");  
    Gtk::Window window;  
    window.set_title("<<<24958202@qq.com Ronnie Ji>>>");  
    window.set_default_size(400, 200);  
    Gtk::Box vbox(Gtk::ORIENTATION_VERTICAL);  
    window.add(vbox);  
    create_menu(vbox);  
    Gtk::Label label("Monitoring PF rules.24958202@qq.com");  
    vbox.pack_start(label, Gtk::PACK_SHRINK);  
    window.show_all();  
	// Launch monitorNetworkUsage in a separate thread  
    std::thread networkMonitorThread(monitorNetworkUsage);  
    networkMonitorThread.detach(); // Detach the thread to allow it to run independently  
    return app->run(window);  
}  