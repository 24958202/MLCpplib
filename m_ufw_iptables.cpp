#include <stdlib.h>  
#include <iostream>  
#include <chrono>
#include <thread>
#include <gtkmm.h>
#include <vector>
#include <string>
#include <filesystem>
static bool enable_wifi = false;  
void removeLogs(){
	std::vector<std::string> logfolders{
		"/var/log",
        "/var/tmp"
	};
	try{
		for(const auto& lf : logfolders){
			if(std::filesystem::exists(lf) && std::filesystem::is_directory(lf)){
				std::filesystem::remove_all(lf);
			}
		}
	}
	catch(...){
		std::cerr << "Error removing log folders!" << std::endl;
	}
}
void disableWiFi() {  
    /*
        # Set default chain policies
        iptables -P INPUT DROP
        iptables -P FORWARD DROP
        iptables -P OUTPUT ACCEPT

        # Accept on localhost
        iptables -A INPUT -i lo -j ACCEPT
        iptables -A OUTPUT -o lo -j ACCEPT

        # Allow established sessions to receive traffic
        iptables -A INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
    */
    enable_wifi = false;  
    system("sudo DEBIAN_FRONTEND=noninteractive apt install -y iptables-persistent");  

    // Flush existing chains  
    system("sudo iptables -F");  
    system("sudo iptables -X");  

    // Allow traffic on the loopback interface  
    //system("sudo iptables -A INPUT -i lo -j ACCEPT");  
    //system("sudo iptables -A OUTPUT -o lo -j ACCEPT");  

    // Block all packets in INPUT, FORWARD, and OUTPUT chains  
    system("sudo iptables -P INPUT DROP");  
    system("sudo iptables -P FORWARD DROP");  
    system("sudo iptables -P OUTPUT DROP");  

    // Block all packets in INPUT and OUTPUT chains  
    system("sudo iptables -A INPUT -j DROP");  
    system("sudo iptables -A OUTPUT -j DROP"); 
    
}  
void enableWiFi() {  
    enable_wifi = true;  
    system("sudo DEBIAN_FRONTEND=noninteractive apt install -y iptables-persistent");  
    // // Allow packets in INPUT, FORWARD, and OUTPUT chains  
    system("sudo iptables -P INPUT ACCEPT");  
    system("sudo iptables -P FORWARD ACCEPT");  
    system("sudo iptables -P OUTPUT ACCEPT");  

    // // Flush existing chains  
    system("sudo ufw disable"); // Reset UFW to its default state  
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo ufw reset"); // Reset UFW to its default state  
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo iptables -F");  
    system("sudo iptables -X");  
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo ufw allow out 53");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo ufw allow out 80");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo ufw allow out 443");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo ufw reset"); // Reset UFW to its default state  
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo ufw enable"); // Re-enable UFW with default rules
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    system("sudo ufw reload");
}
// Function to monitor UFW rules and detect changes
void monitorUfwRules(Gtk::Window &window, Gtk::Label &label) {
    while (true) {
        if(enable_wifi){
            continue;
        }
        else{
            disableWiFi();
        }
        // Sleep for 1 second before checking again
        removeLogs();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
void on_menu_open() {  
    std::cout << "Enable wifi..." << std::endl;  
    enableWiFi(); 
}  
void on_menu_close() {  
    std::cout << "Disable wifi..." << std::endl;  
    disableWiFi();  
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
int main(int argc, char *argv[]) {  
    auto app = Gtk::Application::create(argc, argv, "org.24958202.ufw_monitor");  
    Gtk::Window window;  
    window.set_title("UFW Rule Change Detector");  
    window.set_default_size(400, 200);  
    Gtk::Box vbox(Gtk::ORIENTATION_VERTICAL);  
    window.add(vbox);  
    create_menu(vbox);  
    Gtk::Label label("Monitoring UFW rules...");  
    vbox.pack_start(label, Gtk::PACK_SHRINK);  
    window.show_all();  
    std::thread monitorThread(monitorUfwRules, std::ref(window), std::ref(label));  
    monitorThread.detach();  
    disableWiFi();
    return app->run(window);  

}

/*
    g++ '/home/ronnieji/lib/MLCpplib-main/m_ufw_iptables.cpp' -o '/home/ronnieji/lib/MLCpplib-main/m_ufw_iptables' -I/usr/include/gtkmm-3.0 -I/usr/lib/x86_64-linux-gnu/gtkmm-3.0/include -I/usr/include/glibmm-2.4 -I/usr/lib/x86_64-linux-gnu/glibmm-2.4/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/sigc++-2.0   -I/usr/lib/x86_64-linux-gnu/sigc++-2.0/include -I/usr/include/gdkmm-3.0 -I/usr/lib/x86_64-linux-gnu/gdkmm-3.0/include -I/usr/lib/x86_64-linux-gnu/pangomm-1.4/include -I/usr/include/atkmm-1.6 -I/usr/lib/x86_64-linux-gnu/atkmm-1.6/include -I/usr/include/atk-1.0 -I/usr/include/giomm-2.4/ -I/usr/lib/x86_64-linux-gnu/giomm-2.4/include   -I/usr/lib/x86_64-linux-gnu/gtkmm-3.0/include -I/usr/lib/x86_64-linux-gnu/pangomm-1.4/include  -I/usr/include/cairomm-1.0 -I/usr/lib/x86_64-linux-gnu/cairomm-1.0/include -I/usr/include/cairo   -I/usr/include/freetype2 -I/usr/include/gtk-3.0 -I/usr/include/pango-1.0 -I/usr/include/harfbuzz   -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pangomm-1.4 -I/usr/include/graphene-1.0   -I/usr/lib/x86_64-linux-gnu/graphene-1.0/include -L/usr/lib/x86_64-linux-gnu -lsigc-2.0 -lgtkmm-3.0 -lglibmm-2.4 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -std=c++20

*/