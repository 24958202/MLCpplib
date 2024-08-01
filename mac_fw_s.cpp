/*
    latest ufw on mac firewall protector:
    click Enable to enable wifi and Disable to disable Wifi
*/
#include <gtkmm.h>  
#include <cstdlib>  
#include <iostream>  
#include <vector>  
#include <string>   
#include <thread>  
#include <chrono>  
#include <filesystem>  
#include <fstream>

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
// Function to get the current UFW rules  
std::string getUfwRules() {  
    std::string cmd = "sudo ufw status verbose";  
    std::string output = "";  
    FILE *fp = popen(cmd.c_str(), "r");  
    if (fp == nullptr) {  
        std::cerr << "Error executing command.\n";  
        return "";  
    }  
    char buffer[1024];  
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {  
        output += buffer;  
    }  
    pclose(fp);  
    return output;  
} 
void executeRules_enable(){
    system("sudo pfctl -f /Users/dengfengji/ronnieji/ufw/pf.conf");
    system("sudo pfctl -E");
} 
void executeRules_disable(){
    system("sudo pfctl -f /Users/dengfengji/ronnieji/ufw/pfdis.conf");
    system("sudo pfctl -E");
}
void enable_443() {  
    if (!ufw_checking) {  
        ufw_checking = true;  
    }  
    std::vector<std::string> write_en_rules{
        "pass out proto tcp from any to any port 80",
        "pass out proto udp from any to any port 53",
        "pass out proto tcp from any to any port 443"
    };
    std::ofstream file("/Users/dengfengji/ronnieji/ufw/pf.conf", std::ios::out);
    if(!file.is_open()){
        file.open("/Users/dengfengji/ronnieji/ufw/pf.conf", std::ios::out);
    }
    for(const auto& item : write_en_rules){
        file << item << '\n';
    }
    file.close();
    executeRules_enable();
    // At the end turn on ufw checking  
}  
void disable_443() { 
    if(ufw_checking){
        ufw_checking = false;
    } 
    std::vector<std::string> write_dis_rules{
        "block in all",
        "block out all"
    };
    std::ofstream file("/Users/dengfengji/ronnieji/ufw/pfdis.conf",std::ios::out);
    if(!file.is_open()){
        file.open("/Users/dengfengji/ronnieji/ufw/pfdis.conf",std::ios::out);
    }
    for(const auto& item : write_dis_rules){
        file << item << '\n';
    }
    file.close();
    // At the end turn on ufw checking  
    executeRules_disable();
}  

void executeRules_com(){
    std::vector<std::string> exerules{
        "sudo pfctl -E",
        "sudo pfctl -sr",
        "sudo launchctl stop com.apple.pf",
        "sudo launchctl start com.apple.pf"
    };
    for(const auto& er : exerules){
        std::string strExe = er;
        system(strExe.c_str());
    }
}
void deleteUfwRule(const std::string& rule) {  
    std::string cmd = "sudo ufw delete " + rule;  
    system(cmd.c_str());  
}  
void onRuleChangeDetected(Gtk::Window &window, Gtk::Label &label, const std::string &newRules) {  
    label.set_text("A new UFW rule was detected. Deleting the rule...");  
    system("sudo ufw delete 1");  
    window.show_all();  
}  
// Function to monitor UFW rules and detect changes  
void monitorUfwRules(Gtk::Window &window, Gtk::Label &label) {  
    std::string prevRules = getUfwRules();  
    std::cout << prevRules << std::endl;  
    while (true) {  
        std::string newRules = getUfwRules();  
        if(ufw_checking){
            enable_443();
        }
        else{
            disable_443();
        }
        executeRules_com();
        removeLogs();  
        // Sleep for 1 second before checking again  
        std::this_thread::sleep_for(std::chrono::seconds(1));  
    }  
}  
void on_menu_open() {  
    std::cout << "Enable wifi..." << std::endl;  
    enable_443();  
}  
void on_menu_close() {  
    std::cout << "Disable wifi..." << std::endl;  
    disable_443();  
}  
void create_menu(Gtk::Box& vbox) {  
    // Create the menu bar  
    auto menu_bar = Gtk::make_managed<Gtk::MenuBar>();  

    // Create the File menu  
    auto file_item = Gtk::make_managed<Gtk::MenuItem>("Wifi", true);  
    menu_bar->append(*file_item);  
    auto file_menu = Gtk::make_managed<Gtk::Menu>();  
    file_item->set_submenu(*file_menu);  

    auto open_item = Gtk::make_managed<Gtk::MenuItem>("Enable", true);  
    open_item->signal_activate().connect(sigc::ptr_fun(&on_menu_open));  
    file_menu->append(*open_item);  

    auto close_item = Gtk::make_managed<Gtk::MenuItem>("Disable", true);  
    close_item->signal_activate().connect(sigc::ptr_fun(&on_menu_close));  
    file_menu->append(*close_item);  

    // Add the menu bar to the box  
    vbox.pack_start(*menu_bar, Gtk::PACK_SHRINK);  
}  
int main(int argc, char** argv) {  
    auto app = Gtk::Application::create(argc, argv, "org.24958202.ufw_monitor");  
    Gtk::Window window;  
    window.set_title("<<<UFW Rule Change Detector>>>");  
    window.set_default_size(400, 200);  
    Gtk::Box vbox(Gtk::ORIENTATION_VERTICAL);  
    window.add(vbox);  
    create_menu(vbox);  
    Gtk::Label label("Monitoring UFW rules...");  
    vbox.pack_start(label, Gtk::PACK_SHRINK);  
    window.show_all();  
    std::thread monitorThread(monitorUfwRules, std::ref(window), std::ref(label));  
    monitorThread.detach();  
    return app->run(window);  
}