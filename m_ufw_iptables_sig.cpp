#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <set>
#include <fstream>
#include <sstream>
#include <map>
#include <sys/types.h>
#include <signal.h>
#include <csignal>
#include <stdlib.h>
#include <atomic>
static std::atomic<bool> enable_wifi = false;
static std::atomic<bool> enable_new_process = false;
static std::atomic<bool> running = true;
// Atomic flag to keep the main loop running
std::atomic<bool> signal_running{true};
static std::vector<std::string> apps_excluded{
	"m_ufw_iptables",
	"faceDetectVideo",
	"webCamMain"
};
static std::map<std::string,int> pure_linux_processes{
	{"(sd-pam)",640},
	{"-bash",655},
	{"./get_current",788},
	{"/bin/login",612},
	{"/lib/systemd/systemd",639},
	{"/lib/systemd/systemd-journald",164},
	{"/lib/systemd/systemd-logind",468},
	{"/lib/systemd/systemd-udevd",186},
	{"/sbin/agetty",613},
	{"/sbin/init",1},
	{"/sbin/wpa_supplicant",552},
	{"/usr/bin/dbus-daemon",461},
	{"/usr/lib/polkit-1/polkitd",464},
	{"/usr/libexec/bluetooth/bluetoothd",459},
	{"/usr/sbin/ModemManager",569},
	{"/usr/sbin/NetworkManager",593},
	{"/usr/sbin/cron",460},
	{"/usr/sbin/thd",470},
	{"avahi-daemon: chroot helper",484},
	{"avahi-daemon: running [ronnie24958202.local]",458},
	{"brcmf_wdog/mmc1:0001:1",304},
	{"card1-crtc0",403},
	{"card1-crtc1",404},
	{"card1-crtc2",405},
	{"card1-crtc3",406},
	{"cec-vc4-hdmi-0",388},
	{"cec-vc4-hdmi-1",396},
	{"cpuhp/0",22},
	{"cpuhp/1",23},
	{"cpuhp/2",28},
	{"cpuhp/3",33},
	{"hwrng",74},
	{"irq/161-mmc0",97},
	{"irq/162-mmc1",102},
	{"irq/170-1000800000.codec",299},
	{"irq/174-vc4 hdmi hpd connected",386},
	{"irq/175-vc4 hdmi hpd disconnected",387},
	{"irq/176-vc4 hdmi cec rx",389},
	{"irq/177-vc4 hdmi cec tx",390},
	{"irq/178-vc4 hdmi hpd connected",394},
	{"irq/179-vc4 hdmi hpd disconnected",395},
	{"irq/180-vc4 hdmi cec rx",397},
	{"irq/181-vc4 hdmi cec tx",398},
	{"irq/38-aerdrv",89},
	{"jbd2/mmcblk0p2-8",107},
	{"kauditd",44},
	{"kcompactd0",49},
	{"kcompactd1",50},
	{"kcompactd2",51},
	{"kcompactd3",52},
	{"kdevtmpfs",42},
	{"khungtaskd",45},
	{"ksoftirqd/0",17},
	{"ksoftirqd/1",25},
	{"ksoftirqd/2",30},
	{"ksoftirqd/3",35},
	{"kswapd0",65},
	{"kswapd1",66},
	{"kswapd2",67},
	{"kswapd3",68},
	{"kthreadd",2},
	{"kworker/0:0",780},
	{"kworker/0:1-events_power_efficient",741},
	{"kworker/0:1H-mmc_complete",60},
	{"kworker/0:2-events",92},
	{"kworker/0:2H",132},
	{"kworker/0:3-mm_percpu_wq",230},
	{"kworker/1:0-events",736},
	{"kworker/1:1-mm_percpu_wq",772},
	{"kworker/1:1H-kblockd",81},
	{"kworker/1:2-events",72},
	{"kworker/1:2H",167},
	{"kworker/2:0-mm_percpu_wq",748},
	{"kworker/2:1H-kblockd",134},
	{"kworker/2:2-events",93},
	{"kworker/2:2H",213},
	{"kworker/3:0-events",773},
	{"kworker/3:1-events",59},
	{"kworker/3:1H-kblockd",109},
	{"kworker/3:2-events",735},
	{"kworker/3:2H",131},
	{"kworker/R-DWC Notification WorkQ",79},
	{"kworker/R-blkcg_punt_bio",55},
	{"kworker/R-brcmf_wq/mmc1:0001:1",303},
	{"kworker/R-cfg80211",298},
	{"kworker/R-ext4-rsv-conversion",108},
	{"kworker/R-inet_frag_wq",43},
	{"kworker/R-ipv6_addrconf",111},
	{"kworker/R-iscsi_conn_cleanup",75},
	{"kworker/R-kblockd",54},
	{"kworker/R-kintegrityd",53},
	{"kworker/R-kthrotld",70},
	{"kworker/R-kvfree_rcu_reclaim",4},
	{"kworker/R-mld",110},
	{"kworker/R-mm_percpu_wq",13},
	{"kworker/R-mmc_complete",106},
	{"kworker/R-netns",8},
	{"kworker/R-nfsiod",69},
	{"kworker/R-nvme-delete-wq",78},
	{"kworker/R-nvme-reset-wq",77},
	{"kworker/R-nvme-wq",76},
	{"kworker/R-rcu_gp",5},
	{"kworker/R-rpciod",61},
	{"kworker/R-scsi_tmf_0",759},
	{"kworker/R-sdhci",101},
	{"kworker/R-slub_flushwq",7},
	{"kworker/R-sync_wq",6},
	{"kworker/R-uas",80},
	{"kworker/R-v3d_bin",277},
	{"kworker/R-v3d_cache_clean",285},
	{"kworker/R-v3d_cpu",286},
	{"kworker/R-v3d_csd",284},
	{"kworker/R-v3d_render",281},
	{"kworker/R-v3d_tfu",283},
	{"kworker/R-writeback",48},
	{"kworker/R-xprtiod",62},
	{"kworker/u16:0-ipv6_addrconf",12},
	{"kworker/u16:1-ipv6_addrconf",112},
	{"kworker/u17:0-kvfree_rcu_reclaim",38},
	{"kworker/u17:2-events_unbound",56},
	{"kworker/u17:3-kvfree_rcu_reclaim",57},
	{"kworker/u18:0-events_unbound",734},
	{"kworker/u18:1-events_unbound",64},
	{"kworker/u18:2-writeback",87},
	{"kworker/u19:0-events_unbound",40},
	{"kworker/u19:1",90},
	{"kworker/u19:2-events_unbound",632},
	{"kworker/u20:0-flush-179:0",756},
	{"kworker/u20:1-flush-8:0",100},
	{"kworker/u21:0-hci0",82},
	{"kworker/u21:1-brcmf_wq/mmc1:0001:1",367},
	{"kworker/u22:0",83},
	{"kworker/u23:0",84},
	{"kworker/u24:0",85},
	{"kworker/u25:0",86},
	{"migration/0",21},
	{"migration/1",24},
	{"migration/2",29},
	{"migration/3",34},
	{"oom_reaper",46},
	{"pool_workqueue_release",3},
	{"rcu_exp_gp_kthread_worker",20},
	{"rcu_exp_par_gp_kthread_worker/0",19},
	{"rcu_preempt",18},
	{"rcu_tasks_kthread",14},
	{"rcu_tasks_rude_kthread",15},
	{"rcu_tasks_trace_kthread",16},
	{"scsi_eh_0",758},
	{"spi10",231},
	{"sudo",787},
	{"usb-storage",760},
	{"watchdogd",58}
};
void disableWiFi() {
    enable_wifi = false;
    system("sudo DEBIAN_FRONTEND=noninteractive apt install -y iptables-persistent");
    system("sudo iptables -F");
    system("sudo iptables -X");
    system("sudo iptables -P INPUT DROP");
    system("sudo iptables -P FORWARD DROP");
    system("sudo iptables -P OUTPUT DROP");
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
std::map<std::string, int> getCurrentProcesses() {
    std::map<std::string, int> processList;
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
void terminateProcessByName(const std::string& processName, int pid) {
    if (kill(pid, SIGKILL) == 0) {
        std::cout << "Terminated process: " << processName << " (PID: " << pid << ")" << std::endl;
    } else {
        std::cerr << "Failed to terminate process: " << processName << " (PID: " << pid << ")" << std::endl;
    }
}
void monitorProcesses() {
    while (running) {
        if (enable_new_process) {
            std::map<std::string, int> currentProcesses = getCurrentProcesses();
            for (const auto& [processName, pid] : currentProcesses) {
				bool processInExcludedList = false;
				for(const auto& item : apps_excluded){
					if(processName.find(item) != std::string::npos){
						processInExcludedList = true;
						break;
					}
				}
				if(processInExcludedList){
					continue;
				}
                if (pure_linux_processes.find(processName) == pure_linux_processes.end()) {
                    std::cout << "New process detected: " << processName << " (PID: " << pid << ")" << std::endl;
                    terminateProcessByName(processName, pid);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}
void monitorUfwRules() {
    while (running) {
        if (!enable_wifi) {
            disableWiFi();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}
void handle_signal(int signum) {
    switch(signum) {
        case SIGINT:
            std::cout << "Caught SIGINT (Interrupt: Ctrl+C)\n";
            signal_running = false;
            break;
        case SIGTERM:
            std::cout << "Caught SIGTERM (Terminate)\n";
            signal_running = false;
            break;
        case SIGQUIT:
            std::cout << "Caught SIGQUIT (Quit)\n";
            signal_running = false;
            break;
        case SIGTSTP:
            std::cout << "Caught SIGTSTP (Terminal Stop: Ctrl+Z)\n";
            // Do not stop, just print message
            break;
        default:
            std::cout << "Caught signal: " << signum << "\n";
    }
}
void checkSignalHandle(){
   // Register catchable signals
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGQUIT, handle_signal);
    std::signal(SIGTSTP, handle_signal); // Note: Default action still stops process unless overridden
    std::cout << "Listener started (PID: " << getpid() << ").\n";
    std::cout << "Send SIGINT (Ctrl+C), SIGTERM (kill), SIGQUIT, or SIGTSTP (Ctrl+Z).\n";
    std::cout << "SIGKILL and SIGSTOP cannot be caught!\n";
    // Main work loop
    while(signal_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Exiting listener.\n";
    system("sudo reboot");
}
int main(int argc, char *argv[]) {
    //signal(SIGINT, signalHandler); // Handle Ctrl+C
    std::thread monitorUfwThread(monitorUfwRules);
    monitorUfwThread.detach();
    std::thread monitorProcessThread(monitorProcesses);
    monitorProcessThread.detach();
    std::thread checkSignalThread(checkSignalHandle);
    enable_new_process = true;
    disableWiFi();
    // Keep main alive until running is false
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    if (checkSignalThread.joinable()) {
        checkSignalThread.join();
    }
    return 0;
}
