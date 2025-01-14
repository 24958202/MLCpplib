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
#include <cstdlib> // For exit()
#include <sstream>
#include <array>
#include <libproc.h> // For proc_pidpath and other process-related functions
#include <unistd.h>  // For getpid()
#include <mach/mach.h> //use mac's system api, much add '-framework System' at the compile command
#include <stdexcept> 
// Global variable to control UFW checking  
static bool ufw_checking = false;  
static bool processes_checking = false;
static bool enable_new_process = false;
static std::map<std::string,int> pure_mac_processes{
  {"AMPDeviceDiscoveryAgent",668},
  {"ANECompilerService",47282},
  {"ANEStorageMaintainer",5876},
  {"APFSUserAgent",601},
  {"ASConfigurationSubscriber",1756},
  {"ASPCarryLog",3614},
  {"AccessibilityVisualsAgent",12283},
  {"AccountSubscriber",1744},
  {"Activity Monitor",644},
  {"AirPlayUIAgent",805},
  {"AirPlayXPCHelper",355},
  {"AppSSOAgent",662},
  {"AppSSODaemon",674},
  {"AppleCredentialManagerDaemon",343},
  {"AppleSpell",60943},
  {"ArchiveService",89513},
  {"AssetCache",768},
  {"AssetCacheLocatorService",766},
  {"AssetCacheManagerService",791},
  {"AssetCacheTetheratorService",819},
  {"AudioComponentRegistrar",462},
  {"AuthenticationServicesAgent",3720},
  {"BTLEServerAgent",821},
  {"BiomeAgent",753},
  {"CGPDFService",74809},
  {"CMFSyncAgent",931},
  {"CSExattrCryptoService",13412},
  {"CVMServer",704},
  {"CalendarFocusConfigurationExtension",916},
  {"CallHistoryPluginHelper",711},
  {"CategoriesService",930},
  {"CommCenter",598},
  {"ContainerMetadataExtractor",709},
  {"ContextService",700},
  {"ContextStoreAgent",607},
  {"ContinuityCaptureAgent",675},
  {"ControlCenter",54477},
  {"CoreLocationAgent",617},
  {"CoreServicesUIAgent",587},
  {"CrashReporterSupportHelper",895},
  {"CursorUIViewService",986},
  {"DataDetectorsSourceAccess",35411},
  {"DiskArbitrationAgent",25325},
  {"DiskUnmountWatcher",25326},
  {"DistributionHelper",73761},
  {"Dock",647},
  {"DockHelper",9458},
  {"EAUpdaterService",26517},
  {"Elmedia Video Player",55856},
  {"EscrowSecurityAlert",4070},
  {"FPCKService",5645},
  {"Family",74436},
  {"Finder",651},
  {"GSSCred",551},
  {"GameControllerConfigService",61711},
  {"GroupSessionService",70621},
  {"IMAutomaticHistoryDeletionAgent",4735},
  {"IMDPersistenceAgent",721},
  {"IOMFB_bics_daemon",305},
  {"IOUserBluetoothSerialDriver",51387},
  {"ImageIOXPCService",56208},
  {"IntelligencePlatformComputeService",907},
  {"InteractiveLegacyProfilesSubscriber",1714},
  {"KernelEventAgent",348},
  {"Keychain Circle Notification",763},
  {"KonaSynthesizer",925},
  {"LegacyProfilesSubscriber",1742},
  {"LinkedNotesUIService",11266},
  {"MTLAssetUpgraderD",6485},
  {"MTLCompilerService",962},
  {"MacinTalkAUSP",914},
  {"MailShortcutsExtension",919},
  {"ManagedConfigurationFilesSubscriber",1755},
  {"ManagedSettingsAgent",743},
  {"ManagementTestSubscriber",1754},
  {"MauiAUSP",910},
  {"MessagesBlastDoorService",957},
  {"Notes",70407},
  {"NotificationCenter",618},
  {"OSDUIHelper",88572},
  {"OmniRecorder",6443},
  {"PasscodeSettingsSubscriber",1743},
  {"PasswordBreachAgent",89769},
  {"PerfPowerServices",91825},
  {"PerfPowerTelemetryClientRegistrationService",493},
  {"PlugInLibraryService",472},
  {"PodcastContentService",6880},
  {"PowerChime",75835},
  {"PowerUIAgent",382},
  {"ProtectedCloudKeySyncing",762},
  {"QuickLookSatellite",20951},
  {"QuickLookUIService",660},
  {"RemoteManagementAgent",5304},
  {"ReportCrash",715},
  {"Safari",56203},
  {"SafariBookmarksSyncAgent",780},
  {"SafariLaunchAgent",784},
  {"SafariLinkExtension",917},
  {"SandboxedServiceRunner",17466},
  {"ScopedBookmarkAgent",749},
  {"ScreenSharingSubscriber",1738},
  {"ScreenTimeAgent",725},
  {"ScreenTimeWidgetExtension",78805},
  {"SecuritySubscriber",1712},
  {"SetStoreUpdateService",826},
  {"SidecarRelay",820},
  {"Sider",61546},
  {"Siri",801},
  {"SiriAUSP",908},
  {"SiriNCService",1055},
  {"SoftwareUpdateNotificationManager",892},
  {"SoftwareUpdateSubscriber",1746},
  {"SpeechSynthesisServerXPC",60065},
  {"Spotlight",724},
  {"StandaloneHIDAudService",816},
  {"StatusKitAgent",713},
  {"StocksWidget",72910},
  {"SubmitDiagInfo",781},
  {"SystemUIServer",650},
  {"Terminal",637},
  {"TextInputMenuAgent",807},
  {"TextInputSwitcher",813},
  {"ThemeWidgetControlViewService",39255},
  {"ThumbnailExtension_macOS",10573},
  {"ThunderboltAccessoryUpdaterService",585},
  {"TrustedPeersHelper",626},
  {"UARPUpdaterServiceAFU",576},
  {"UARPUpdaterServiceDisplay",574},
  {"UARPUpdaterServiceHID",577},
  {"UARPUpdaterServiceLegacyAudio",575},
  {"UARPUpdaterServiceUSBPD",578},
  {"UIKitSystem",719},
  {"USBAgent",24682},
  {"UVCAssistant",489},
  {"UniversalControl",15589},
  {"UsageTrackingAgent",901},
  {"UserEventAgent",292},
  {"VTDecoderXPCService",55886},
  {"VTEncoderXPCService",6759},
  {"ViewBridgeAuxiliary",504},
  {"WallpaperAgent",29697},
  {"WallpaperVideoExtension",29787},
  {"WardaSynthesizer_arm64",921},
  {"WeatherWidget",71338},
  {"WindowManager",592},
  {"WindowServer",358},
  {"WirelessRadioManagerd",463},
  {"XProtect",4733},
  {"XProtectBridgeService",823},
  {"XProtectPluginService",550},
  {"XprotectService",10770},
  {"ZhuGeService",480},
  {"accessoryd",384},
  {"accessoryupdaterd",301},
  {"accountsd",596},
  {"adid",739},
  {"adprivacyd",944},
  {"airportd",376},
  {"akd",874},
  {"amfid",308},
  {"amsaccountsd",69697},
  {"amsengagementd",710},
  {"analyticsd",364},
  {"aned",697},
  {"aneuserd",847},
  {"apfsd",383},
  {"appleaccountd",26440},
  {"appleeventsd",496},
  {"appleh13camerad",487},
  {"appstoreagent",848},
  {"askpermissiond",803},
  {"aslmanager",361},
  {"assistant_cdmd",767},
  {"assistant_service",945},
  {"assistantd",80669},
  {"audioaccessoryd",774},
  {"audioanalyticsd",479},
  {"audioclocksyncd",465},
  {"audiomxd",963},
  {"authd",460},
  {"autofsd",339},
  {"automountd",666},
  {"avatarsd",5644},
  {"avconferenced",760},
  {"awdd",628},
  {"axassetsd",796},
  {"backgroundassets.user",3612},
  {"backgroundtaskmanagementd",415},
  {"backupd",481},
  {"backupd-helper",381},
  {"betaenrollmentd",5504},
  {"biomed",306},
  {"biomesyncd",80715},
  {"biometrickitd",388},
  {"bluetoothd",51336},
  {"bluetoothuserd",698},
  {"bootinstalld",506},
  {"businessservicesd",12122},
  {"calaccessd",663},
  {"callservicesd",627},
  {"cameracaptured",488},
  {"captiveagent",927},
  {"cdpd",625},
  {"cfprefsd",359},
  {"chronod",71334},
  {"ciphermld",5287},
  {"codelite",74311},
  {"colorsync.displayservices",482},
  {"colorsync.useragent",789},
  {"colorsyncd",484},
  {"com.apple.AccountPolicyHelper",514},
  {"com.apple.AppStoreDaemon.StorePrivilegedODRService",866},
  {"com.apple.AppleUserHIDDrivers",494},
  {"com.apple.BKAgentService",6881},
  {"com.apple.CloudDocs.iCloudDriveFileProvider",712},
  {"com.apple.CloudPhotosConfiguration",7273},
  {"com.apple.CodeSigningHelper",486},
  {"com.apple.ColorSyncXPCAgent",483},
  {"com.apple.DriverKit-IOUserDockChannelSerial",492},
  {"com.apple.FaceTime.FTConversationService",695},
  {"com.apple.MobileSoftwareUpdate.CleanupPreparePathService",500},
  {"com.apple.MobileSoftwareUpdate.UpdateBrainService",571},
  {"com.apple.Safari.History",772},
  {"com.apple.Safari.SafeBrowsing.Service",26278},
  {"com.apple.Safari.SandboxBroker",56210},
  {"com.apple.Safari.SearchHelper",56209},
  {"com.apple.SiriTTSService.TrialProxy",918},
  {"com.apple.WebKit.GPU",56211},
  {"com.apple.WebKit.Networking",56207},
  {"com.apple.WebKit.WebContent",56311},
  {"com.apple.accessibility.mediaaccessibilityd",706},
  {"com.apple.appkit.xpc.openAndSavePanelService",56216},
  {"com.apple.audio.ComponentTagHelper",894},
  {"com.apple.audio.DriverHelper",459},
  {"com.apple.audio.SandboxHelper",464},
  {"com.apple.cmio.registerassistantservice",356},
  {"com.apple.dock.extra",703},
  {"com.apple.hiservices-xpcservice",522},
  {"com.apple.ifdreader",825},
  {"com.apple.quicklook.ThumbnailsAgent",849},
  {"com.apple.sbd",589},
  {"com.apple.tonelibraryd",959},
  {"contactsdonationagent",5615},
  {"containermanagerd",508},
  {"containermanagerd_system",390},
  {"contentlinkingd",750},
  {"contextstored",365},
  {"coreaudiod",380},
  {"coreautha",65089},
  {"coreauthd",567},
  {"corebrightnessd",354},
  {"coreduetd",325},
  {"corekdld",580},
  {"coreservicesd",366},
  {"corespeechd",795},
  {"corespotlightd",736},
  {"coresymbolicationd",755},
  {"cryptexd",405},
  {"csnameddatad",435},
  {"ctkahp",810},
  {"ctkd",632},
  {"dasd",340},
  {"devicecheckd",11783},
  {"diagnosticextensionsd",900},
  {"diagnostics_agent",806},
  {"dirhelper",345},
  {"diskarbitrationd",322},
  {"dmd",640},
  {"donotdisturbd",613},
  {"dprivacyd",5298},
  {"eligibilityd",905},
  {"endpointsecurityd",303},
  {"extensionkitservice",696},
  {"fairplayd",858},
  {"fairplaydeviceidentityd",26275},
  {"familycircled",661},
  {"fbahelperd",5507},
  {"filecoordinationd",658},
  {"fileproviderd",667},
  {"financed",794},
  {"findmybeaconingd",466},
  {"findmydevice-user-agent",605},
  {"findmydeviced",593},
  {"findmylocateagent",752},
  {"fmfd",754},
  {"followupd",741},
  {"fontd",673},
  {"fontworker",684},
  {"fseventsd",295},
  {"fudHelperAgent",997},
  {"gamecontrolleragentd",864},
  {"gamecontrollerd",582},
  {"gamepolicyd",656},
  {"geodMachServiceBridge",854},
  {"hidd",449},
  {"homed",6238},
  {"homeenergyd",18869},
  {"iCloudNotificationAgent",782},
  {"icdd",802},
  {"iconservicesagent",676},
  {"iconservicesd",320},
  {"idleassetsd",670},
  {"imagent",716},
  {"imklaunchagent",799},
  {"installcoordinationd",865},
  {"installd",853},
  {"installerauthagent",73791},
  {"intelligenceplatformd",3599},
  {"intelligentroutingd",688},
  {"ioupsd",51373},
  {"kernelmanagerd",321},
  {"keybagd",312},
  {"keyboardservicesd",11782},
  {"knowledge-agent",2721},
  {"knowledgeconstructiond",70447},
  {"launchd",1},
  {"launchservicesd",332},
  {"linkd",846},
  {"liquiddetectiond",3611},
  {"localizationswitcherd",687},
  {"locationd",337},
  {"lockdownmoded",899},
  {"log",86487},
  {"login",9460},
  {"logind",346},
  {"loginwindow",362},
  {"lsd",378},
  {"mac_fw_s",56078},
  {"maild",793},
  {"mapssyncd",966},
  {"mbuseragent",61710},
  {"mdbulkimport",991},
  {"mdmclient",76823},
  {"mds",319},
  {"mds_stores",507},
  {"mdsync",66836},
  {"mdworker",993},
  {"mdworker_shared",86064},
  {"mdwrite",7596},
  {"mediaremoteagent",759},
  {"mediaremoted",296},
  {"metrickitd",4491},
  {"microstackshot",5505},
  {"mlhostd",3609},
  {"mlruntimed",25526},
  {"mmaintenanced",5721},
  {"mobileactivationd",597},
  {"mobileassetd",406},
  {"mobiletimerd",691},
  {"naturallanguaged",11900},
  {"nbagent",68808},
  {"ndoagent",898},
  {"neagent",620},
  {"nearbyd",681},
  {"nehelper",386},
  {"nesessionmanager",822},
  {"networkserviceproxy",746},
  {"nfcd",788},
  {"notifyd",352},
  {"nsattributedstringagent",73784},
  {"online-auth-agent",3598},
  {"opendirectoryd",330},
  {"osanalyticshelper",771},
  {"ospredictiond",4490},
  {"parentalcontrolsd",814},
  {"parsec-fbf",80717},
  {"parsecd",738},
  {"pboard",590},
  {"pbs",694},
  {"periodic-wrapper",5111},
  {"photoanalysisd",758},
  {"pkd",636},
  {"powerd",304},
  {"powerdatad",3613},
  {"printtool",8610},
  {"proactived",6292},
  {"proactiveeventtrackerd",5299},
  {"progressd",933},
  {"promotedcontentd",946},
  {"rapportd",71494},
  {"recentsd",89824},
  {"remindd",61773},
  {"remoted",310},
  {"remotemanagementd",1710},
  {"replayd",80931},
  {"reversetemplated",89855},
  {"revisiond",347},
  {"routined",603},
  {"rtcreportingd",552},
  {"runningboardd",377},
  {"sandboxd",353},
  {"searchpartyd",527},
  {"searchpartyuseragent",941},
  {"secd",610},
  {"secinitd",456},
  {"seld",583},
  {"seserviced",765},
  {"sharedfilelistd",649},
  {"smd",291},
  {"sociallayerd",685},
  {"softwareupdated",313},
  {"spindump",776},
  {"spindump_agent",779},
  {"storagekitd",468},
  {"storeaccountd",11857},
  {"storedownloadd",852},
  {"storekitagent",689},
  {"storeuid",56541},
  {"studentd",74426},
  {"sudo",64231},
  {"suggestd",683},
  {"suhelperd",537},
  {"swcd",936},
  {"symptomsd",392},
  {"symptomsd-diag",518},
  {"syncdefaultsd",629},
  {"sysdiagnosed",6295},
  {"sysextd",478},
  {"sysmond",558},
  {"syspolicyd",470},
  {"system_installd",851},
  {"systemsoundserverd",677},
  {"systemstats",299},
  {"systemstatusd",432},
  {"tccd",360},
  {"test",64426},
  {"thermalmonitord",329},
  {"timed",333},
  {"tipsd",56338},
  {"translationd",26206},
  {"transparencyd",73013},
  {"triald",747},
  {"triald_system",893},
  {"trustdFileHelper",396},
  {"tzd",26363},
  {"uninstalld",294},
  {"usbd",387},
  {"usbmuxd",334},
  {"useractivityd",701},
  {"usermanagerd",349},
  {"usernoted",602},
  {"usernotificationsd",595},
  {"watchdogd",315},
  {"webprivacyd",89767},
  {"xpcroleaccountd",572},
  {"zsh",9465}
};
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
// Function to terminate a process by its name
// Function to terminate a process by its name and PID
void terminateProcessByName(const std::string& processName, int pid) {
    try{
//        //Attempt to terminate the process
//        if (kill(pid, SIGKILL) == 0) {
//            std::cout << "Terminated process: " << processName << " (PID: " << pid << ")" << std::endl;
//        } else {
//            perror(("Failed to terminate process: " + processName + " (PID: " + std::to_string(pid) + ")").c_str());
//        }
          task_t task;
          if(task_for_pid(mach_task_self(),pid,&task) == KERN_SUCCESS){
              if(task_terminate(task) == KERN_SUCCESS){
                  std::cout << "Terminated process: " << processName << " (PID: " << pid << ")" << std::endl;
              }
              else{
                  std::cout << "Failed to terminate process: " << processName << " (PID: " << std::to_string(pid) << ")" << std::endl;
              }
          }
    }
    catch(...){
        std::cerr << "Error removing the process, but continue." << std::endl;
    }
}
// Function to get the list of current processes and their names
std::map<std::string, int> getCurrentProcesses() {
    std::map<std::string, int> processList; // Map of process name to PID
    // Get the maximum number of processes
    int maxProcesses = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0) / sizeof(pid_t);
    std::vector<pid_t> pids(maxProcesses);
    // Get the list of PIDs
    int numProcesses = proc_listpids(PROC_ALL_PIDS, 0, pids.data(), sizeof(pid_t) * maxProcesses);
    numProcesses /= sizeof(pid_t); // Convert from bytes to number of PIDs
    // Iterate over each PID
    for (int i = 0; i < numProcesses; ++i) {
        pid_t pid = pids[i];
        if (pid == 0) {
            continue; // Skip invalid PIDs
        }
        // Get the process name (path) for the given PID
        char processPath[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(pid, processPath, sizeof(processPath)) > 0) {
            // Extract the process name from the full path
            std::string processName = std::string(processPath);
            size_t lastSlash = processName.find_last_of('/');
            if (lastSlash != std::string::npos) {
                processName = processName.substr(lastSlash + 1);
            }
            // Add the process name and PID to the map
            processList[processName] = pid;
        }
    }
    return processList;
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
	}
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  
}  
void on_process_open(){
	std::cout << "Accept new processes..." << std::endl;
    enable_new_process = false;
}
void on_process_close(){
	std::cout << "Deny new processes..." << std::endl;  
    enable_new_process = true;
}
void on_history_clean(){
	try{
		std::string cmd = "sudo rm ~/.zsh_history";  
		system(cmd.c_str());  
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors." << std::endl;
	}
}
void monitorProcesses(){
    while (true) {  
        // Lock the mutex and wait for enable_new_process to become true
        if(enable_new_process){
            std::map<std::string, int> currentProcesses = getCurrentProcesses();  
            // Find new processes  
            for (const auto& [processName, pid] : currentProcesses) {  
                if (pure_mac_processes.find(processName) == pure_mac_processes.end()) {  
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
	// Add a separator  
	auto separator2 = Gtk::make_managed<Gtk::SeparatorMenuItem>(); 
	file_menu->append(*separator2);  
	auto clear_history_item = Gtk::make_managed<Gtk::MenuItem>("Clean terminal history", true);  
    clear_history_item->signal_activate().connect(sigc::ptr_fun(&on_history_clean));
	file_menu->append(*clear_history_item);  
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
    Gtk::Label label("Monitoring PF rules.24958202@qq.com");  
    vbox.pack_start(label, Gtk::PACK_SHRINK);  
    window.show_all();  
    std::thread monitorThread(monitorUfwRules, std::ref(window), std::ref(label));  
    monitorThread.detach(); 
    std::thread monitorProcessThread(monitorProcesses);  
    monitorProcessThread.detach();   
    return app->run(window);  
}