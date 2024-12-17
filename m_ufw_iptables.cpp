#include <stdlib.h>  
#include <iostream>  
#include <chrono>  
#include <thread>  
#include <gtkmm.h>  
#include <vector>  
#include <string>  
#include <filesystem>  
#include <set>  
#include <fstream>  
#include <sstream>  
#include <sys/types.h>  
#include <signal.h>  
#include <map>  

static bool enable_wifi = false;
static bool enable_new_process = false;
static std::map<std::string,int> pure_linux_processes{
	{"(sd-pam)",1526},
	{"/home/ronnieji/watch/m_ufw_iptables",120980},
	{"/opt/microsoft/msedge/msedge",118957},
	{"/opt/microsoft/msedge/msedge --type=gpu-process --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --change-stack-guard-on-fork=enable --gpu-preferences=UAAAAAAAAAAgAAAEAAAAAAAAAAAAAAAAAABgAAEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAABAAAAAAAAAAEAAAAAAAAAAIAAAAAAAAAAgAAAAAAAAA --shared-files --field-trial-handle=3,i,1076598815473689620,7939364082779926872,262144 --variations-seed-version",119006},
	{"/opt/microsoft/msedge/msedge --type=renderer --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --change-stack-guard-on-fork=enable --lang=en-US --js-flags=--ms-user-locale= --num-raster-threads=4 --enable-main-frame-before-activation --renderer-client-id=14 --time-ticks-at-unix-epoch=-1734396587195019 --launch-time-ticks=3801141235 --shared-files=v8_context_snapshot_data:100 --field-trial-handle=3,i,1076598815473689620,7939364082779926872,262144 --variations-seed-version",119149},
	{"/opt/microsoft/msedge/msedge --type=renderer --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --extension-process --renderer-sub-type=extension --change-stack-guard-on-fork=enable --lang=en-US --js-flags=--ms-user-locale= --num-raster-threads=4 --enable-main-frame-before-activation --renderer-client-id=7 --time-ticks-at-unix-epoch=-1734396587195019 --launch-time-ticks=3795956842 --shared-files=v8_context_snapshot_data:100 --field-trial-handle=3,i,1076598815473689620,7939364082779926872,262144 --variations-seed-version",119074},
	{"/opt/microsoft/msedge/msedge --type=renderer --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --instant-process --change-stack-guard-on-fork=enable --lang=en-US --js-flags=--ms-user-locale= --num-raster-threads=4 --enable-main-frame-before-activation --renderer-client-id=9 --time-ticks-at-unix-epoch=-1734396587195019 --launch-time-ticks=3796086831 --shared-files=v8_context_snapshot_data:100 --field-trial-handle=3,i,1076598815473689620,7939364082779926872,262144 --variations-seed-version",119093},
	{"/opt/microsoft/msedge/msedge --type=utility --utility-sub-type=network.mojom.NetworkService --lang=en-US --service-sandbox-type=none --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --change-stack-guard-on-fork=enable --shared-files=v8_context_snapshot_data:100 --field-trial-handle=3,i,1076598815473689620,7939364082779926872,262144 --variations-seed-version",119010},
	{"/opt/microsoft/msedge/msedge --type=utility --utility-sub-type=storage.mojom.StorageService --lang=en-US --service-sandbox-type=utility --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --change-stack-guard-on-fork=enable --shared-files=v8_context_snapshot_data:100 --field-trial-handle=3,i,1076598815473689620,7939364082779926872,262144 --variations-seed-version",119020},
	{"/opt/microsoft/msedge/msedge --type=zygote --no-zygote-sandbox --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --change-stack-guard-on-fork=enable",118974},
	{"/opt/microsoft/msedge/msedge --type=zygote --string-annotations --crashpad-handler-pid=118966 --enable-crash-reporter=, --change-stack-guard-on-fork=enable",118977},
	{"/opt/microsoft/msedge/msedge_crashpad_handler",118968},
	{"/sbin/agetty",1286},
	{"/sbin/init",1},
	{"/usr/bin/codelite",2415},
	{"/usr/bin/csd-a11y-settings",1780},
	{"/usr/bin/csd-automount",1776},
	{"/usr/bin/csd-background",1778},
	{"/usr/bin/csd-clipboard",1792},
	{"/usr/bin/csd-color",1811},
	{"/usr/bin/csd-housekeeping",1779},
	{"/usr/bin/csd-keyboard",1775},
	{"/usr/bin/csd-media-keys",1809},
	{"/usr/bin/csd-power",1794},
	{"/usr/bin/csd-print-notifications",1777},
	{"/usr/bin/csd-screensaver-proxy",1796},
	{"/usr/bin/csd-settings-remap",1781},
	{"/usr/bin/csd-wacom",1784},
	{"/usr/bin/csd-xsettings",1801},
	{"/usr/bin/dbus-daemon",2555},
	{"/usr/bin/gnome-keyring-daemon",1543},
	{"/usr/bin/nemo",59697},
	{"/usr/bin/nemo-desktop",2027},
	{"/usr/bin/nm-applet",2030},
	{"/usr/bin/pipewire",1539},
	{"/usr/bin/pipewire-pulse",1542},
	{"/usr/bin/python3",2409},
	{"/usr/bin/touchegg",899},
	{"/usr/bin/wireplumber",1540},
	{"/usr/lib/policykit-1-gnome/polkit-gnome-authentication-agent-1",2033},
	{"/usr/lib/polkit-1/polkitd",874},
	{"/usr/lib/systemd/systemd",1525},
	{"/usr/lib/systemd/systemd-journald",439},
	{"/usr/lib/systemd/systemd-logind",896},
	{"/usr/lib/systemd/systemd-resolved",826},
	{"/usr/lib/systemd/systemd-timesyncd",830},
	{"/usr/lib/systemd/systemd-udevd",470},
	{"/usr/lib/x86_64-linux-gnu/cinnamon-session-binary",1561},
	{"/usr/lib/x86_64-linux-gnu/xapps/xapp-sn-watcher",1995},
	{"/usr/lib/xorg/Xorg",1283},
	{"/usr/libexec/accounts-daemon",844},
	{"/usr/libexec/at-spi-bus-launcher",1799},
	{"/usr/libexec/at-spi2-registryd",1870},
	{"/usr/libexec/bluetooth/obexd",2176},
	{"/usr/libexec/boltd",1029},
	{"/usr/libexec/colord",1917},
	{"/usr/libexec/csd-printer",1886},
	{"/usr/libexec/dconf-service",1842},
	{"/usr/libexec/evolution-addressbook-factory",2154},
	{"/usr/libexec/evolution-calendar-factory",2117},
	{"/usr/libexec/evolution-data-server/evolution-alarm-notify",2029},
	{"/usr/libexec/evolution-source-registry",2080},
	{"/usr/libexec/geoclue-2.0/demos/agent",2016},
	{"/usr/libexec/gnome-terminal-server",117883},
	{"/usr/libexec/goa-daemon",1914},
	{"/usr/libexec/goa-identity-service",1932},
	{"/usr/libexec/gvfs-afc-volume-monitor",1903},
	{"/usr/libexec/gvfs-goa-volume-monitor",1909},
	{"/usr/libexec/gvfs-gphoto2-volume-monitor",1942},
	{"/usr/libexec/gvfs-mtp-volume-monitor",1892},
	{"/usr/libexec/gvfs-udisks2-volume-monitor",1877},
	{"/usr/libexec/gvfsd",2585},
	{"/usr/libexec/gvfsd-dnssd",12119},
	{"/usr/libexec/gvfsd-fuse",2591},
	{"/usr/libexec/gvfsd-metadata",2224},
	{"/usr/libexec/gvfsd-network",12101},
	{"/usr/libexec/gvfsd-trash",2140},
	{"/usr/libexec/rtkit-daemon",1352},
	{"/usr/libexec/switcheroo-control",892},
	{"/usr/libexec/udisks2/udisksd",904},
	{"/usr/libexec/upowerd",1434},
	{"/usr/libexec/xdg-desktop-portal",2558},
	{"/usr/libexec/xdg-desktop-portal-gtk",2603},
	{"/usr/libexec/xdg-desktop-portal-xapp",2580},
	{"/usr/libexec/xdg-document-portal",2564},
	{"/usr/libexec/xdg-permission-store",2569},
	{"/usr/sbin/ModemManager",1028},
	{"/usr/sbin/NetworkManager",991},
	{"/usr/sbin/cron",848},
	{"/usr/sbin/cups-browsed",1486},
	{"/usr/sbin/cupsd",1250},
	{"/usr/sbin/irqbalance",856},
	{"/usr/sbin/kerneloops",1492},
	{"/usr/sbin/lightdm",1272},
	{"/usr/sbin/rsyslogd",973},
	{"/usr/sbin/thermald",898},
	{"/usr/sbin/wpa_supplicant",992},
	{"@dbus-daemon",849},
	{"avahi-daemon: chroot helper",950},
	{"avahi-daemon: running [dengfeng-XM23RM5S.local]",846},
	{"bash",117891},
	{"cat",118964},
	{"cinnamon",1963},
	{"cinnamon-killer-daemon",2031},
	{"cinnamon-launcher",1922},
	{"dbus-launch",2554},
	{"fusermount3",2576},
	{"lightdm",1515},
	{"mintUpdate",2349},
	{"mintreport-tray",2446},
	{"sudo",120979},
	{"zed",906}
};
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
    enable_wifi = false;  
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
void enableWiFi() {  
    enable_wifi = true;  
    system("sudo DEBIAN_FRONTEND=noninteractive apt install -y iptables-persistent");  
    system("sudo iptables -P INPUT ACCEPT");  
    system("sudo iptables -P FORWARD ACCEPT");  
    system("sudo iptables -P OUTPUT ACCEPT");  
    system("sudo ufw disable");  
    std::this_thread::sleep_for(std::chrono::milliseconds(20));  
    system("sudo ufw reset");  
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
    system("sudo ufw reset");  
    std::this_thread::sleep_for(std::chrono::milliseconds(20));  
    system("sudo ufw enable");  
    std::this_thread::sleep_for(std::chrono::milliseconds(20));  
    system("sudo ufw reload");  
}  
// Function to get the list of current processes and their names  
std::map<std::string, int> getCurrentProcesses() {  
    std::map<std::string, int> processList; // Map of process name to PID  
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {  
        if (entry.is_directory()) {  
            std::string dirName = entry.path().filename().string();  
            if (std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {  
                int pid = std::stoi(dirName);  
                std::ifstream cmdlineFile("/proc/" + dirName + "/cmdline");  
                if (cmdlineFile) {  
                    std::string cmdline;  
                    std::getline(cmdlineFile, cmdline, '\0');  
                    if (!cmdline.empty()) {  
                        std::string processName = cmdline.substr(0, cmdline.find('\0'));  
                        processList[processName] = pid;  
                    }  
                }  
            }  
        }  
    }  
    return processList;  
}  
// Function to terminate a process by its name  
void terminateProcessByName(const std::string& processName, int pid) {  
    if (kill(pid, SIGKILL) == 0) {  
        std::cout << "Terminated process: " << processName << " (PID: " << pid << ")" << std::endl;  
    } else {  
        std::cerr << "Failed to terminate process: " << processName << " (PID: " << pid << ")" << std::endl;  
    }  
}  
// Function to monitor processes and terminate new ones by name  
void monitorProcesses() {  
    while (true) {  
		if(enable_new_process){
			std::map<std::string, int> currentProcesses = getCurrentProcesses();  
			// Find new processes  
			for (const auto& [processName, pid] : currentProcesses) {  
				if (pure_linux_processes.find(processName) == pure_linux_processes.end()) {  
					// New process detected  
					std::cout << "New process detected: " << processName << " (PID: " << pid << ")" << std::endl;  
					terminateProcessByName(processName, pid); // Terminate the new process  
				}  
			} 
			// Sleep for a short interval before checking again  
			std::this_thread::sleep_for(std::chrono::milliseconds(500));  
		}
	}
}  
// Function to monitor UFW rules and detect changes  
void monitorUfwRules(Gtk::Window &window, Gtk::Label &label) {  
    while (true) {  
        if (!enable_wifi) {  
            disableWiFi();  
        }  
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
void on_process_open(){
	std::cout << "Accept new processes..." << std::endl;  
    enable_new_process = false;
}
void on_process_close(){
	std::cout << "Deny new processes..." << std::endl;  
    enable_new_process = true;
}
void create_menu(Gtk::Box& vbox) {  
    auto menu_bar = Gtk::make_managed<Gtk::MenuBar>();  
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
	 // Add a separator  
    auto separator = Gtk::make_managed<Gtk::SeparatorMenuItem>();  
    file_menu->append(*separator);  
	// Add "Enable" menu item  
    auto open_process_item = Gtk::make_managed<Gtk::MenuItem>("Accept new processes", true);  
    open_process_item->signal_activate().connect(sigc::ptr_fun(&on_process_open)); // Connect to on_process_open  
    file_menu->append(*open_process_item);  
	// Add "Disable" menu item  
    auto close_process_item = Gtk::make_managed<Gtk::MenuItem>("Deny new processes", true);  
    close_process_item->signal_activate().connect(sigc::ptr_fun(&on_process_close)); // Connect to on_process_close  
    file_menu->append(*close_process_item);  
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
    Gtk::Label label("Monitoring UFW rules.org.24958202@qq.com");  
    vbox.pack_start(label, Gtk::PACK_SHRINK);  
    window.show_all();  
    std::thread monitorUfwThread(monitorUfwRules, std::ref(window), std::ref(label));  
    monitorUfwThread.detach();  
    std::thread monitorProcessThread(monitorProcesses);  
    monitorProcessThread.detach();  
    disableWiFi();  
    return app->run(window);  
}