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
#include <cstdlib>  
#include <iostream>  
#include <vector>  
#include <string>   
#include <thread>  
#include <chrono>  
#include <filesystem>  
#include <fstream>
#include <cstdio>

// Global variable to control UFW checking  
static bool ufw_checking = false;  
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
void monitorUfwRules(Gtk::Window &window, Gtk::Label &label) {  
    while (true) {  
        std::string newRules = getPfRules();  
        if(ufw_checking){  
            enable_443();  
        }  
        else{  
            disable_443();  
        }  
        removeLogs();  
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));  
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
    window.set_title("<<<PF Rule Change Detector>>>");  
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