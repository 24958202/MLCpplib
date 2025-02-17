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
// Global variable to control UFW checking  
static bool ufw_checking = false;  
static bool processes_checking = false;
static bool enable_new_process = false;
static std::vector<std::string> users_to_keep{
	"root",
	"dengfengji"
};
static std::map<std::string,int> pure_mac_processes{
    {"/Applications/Xcode.app/Contents/MacOS/Xcode",7973},
    {"/Applications/Xcode.app/Contents/PlugIns/GPUDebugger.ideplugin/Contents/Frameworks/GPUToolsTransportAgents.framework/Versions/A/XPCServices/GPUToolsAgentService.xpc/Contents/MacOS/GPUToolsAgentService",8038},
    {"/Applications/Xcode.app/Contents/PlugIns/GPUDebugger.ideplugin/Contents/Frameworks/GPUToolsTransportAgents.framework/Versions/A/XPCServices/GPUToolsCompatService.xpc/Contents/MacOS/GPUToolsCompatService",8035},
    {"/Applications/Xcode.app/Contents/SharedFrameworks/DVTFoundation.framework/Versions/A/XPCServices/com.apple.dt.Xcode.DeveloperSystemPolicyService.xpc/Contents/MacOS/com.apple.dt.Xcode.DeveloperSystemPolicyService",7991},
    {"/Applications/Xcode.app/Contents/SharedFrameworks/DVTSourceControl.framework/Versions/A/XPCServices/com.apple.dt.Xcode.sourcecontrol.SSHHelper.xpc/Contents/MacOS/com.apple.dt.Xcode.sourcecontrol.SSHHelper",8016},
    {"/Applications/Xcode.app/Contents/SharedFrameworks/DVTSourceControl.framework/Versions/A/XPCServices/com.apple.dt.Xcode.sourcecontrol.WorkingCopyScanner.xpc/Contents/MacOS/com.apple.dt.Xcode.sourcecontrol.WorkingCopyScanner",8037},
    {"/Applications/Xcode.app/Contents/SharedFrameworks/SourceKit.framework/Versions/A/XPCServices/com.apple.dt.SKAgent.xpc/Contents/MacOS/com.apple.dt.SKAgent",8025},
    {"/Applications/codelite.app/Contents/MacOS/codelite",36948},
    {"/Library/Apple/System/Library/CoreServices/XProtect.app/Contents/MacOS/XProtect",26081},
    {"/Library/Apple/System/Library/CoreServices/XProtect.app/Contents/XPCServices/XProtectPluginService.xpc/Contents/MacOS/XProtectPluginService",540},
    {"/Library/Apple/System/Library/PrivateFrameworks/MobileDevice.framework/Versions/A/Resources/usbmuxd",327},
    {"/Library/Apple/System/Library/PrivateFrameworks/RemotePairing.framework/Versions/A/Resources/bin/RemotePairingDataVaultHelper",8006},
    {"/Library/Apple/System/Library/PrivateFrameworks/RemotePairing.framework/Versions/A/XPCServices/remotepairingd.xpc/Contents/MacOS/remotepairingd",8004},
    {"/Library/Developer/PrivateFrameworks/CoreDevice.framework/Versions/A/XPCServices/CoreDeviceService.xpc/Contents/MacOS/CoreDeviceService",7997},
    {"/Library/Developer/PrivateFrameworks/CoreSimulator.framework/Versions/A/Resources/bin/simdiskimaged",8005},
    {"/Library/Developer/PrivateFrameworks/CoreSimulator.framework/Versions/A/XPCServices/SimulatorTrampoline.xpc/Contents/MacOS/SimulatorTrampoline",8007},
    {"/Library/Developer/PrivateFrameworks/CoreSimulator.framework/Versions/A/XPCServices/com.apple.CoreSimulator.CoreSimulatorService.xpc/Contents/MacOS/com.apple.CoreSimulator.CoreSimulatorService",7995},
    {"/System/Applications/Calendar.app/Contents/Extensions/CalendarFocusConfigurationExtension.appex/Contents/MacOS/CalendarFocusConfigurationExtension",821},
    {"/System/Applications/Calendar.app/Contents/PlugIns/CalendarWidgetExtension.appex/Contents/MacOS/CalendarWidgetExtension",713},
    {"/System/Applications/Clock.app/Contents/PlugIns/WorldClockWidget.appex/Contents/MacOS/WorldClockWidget",723},
    {"/System/Applications/Mail.app/Contents/Extensions/MailShortcutsExtension.appex/Contents/MacOS/MailShortcutsExtension",822},
    {"/System/Applications/Messages.app/Contents/Extensions/MessagesActionExtension.appex/Contents/MacOS/MessagesActionExtension",824},
    {"/System/Applications/Notes.app/Contents/MacOS/Notes",37107},
    {"/System/Applications/Stickies.app/Contents/MacOS/Stickies",37199},
    {"/System/Applications/Stocks.app/Contents/PlugIns/StocksWidget.appex/Contents/MacOS/StocksWidget",707},
    {"/System/Applications/System Settings.app/Contents/MacOS/System Settings",33361},
    {"/System/Applications/System Settings.app/Contents/PlugIns/GeneralSettings.appex/Contents/MacOS/GeneralSettings",33499},
    {"/System/Applications/Utilities/Activity Monitor.app/Contents/MacOS/Activity Monitor",2688},
    {"/System/Applications/Utilities/Activity Monitor.app/Contents/XPCServices/com.apple.activitymonitor.helper.xpc/Contents/MacOS/com.apple.activitymonitor.helper",3771},
    {"/System/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal",804},
    {"/System/Library/CoreServices/APFSUserAgent",588},
    {"/System/Library/CoreServices/AirPlayUIAgent.app/Contents/MacOS/AirPlayUIAgent",674},
    {"/System/Library/CoreServices/Applications/Feedback Assistant.app/Contents/Library/LaunchServices/fbahelperd",4978},
    {"/System/Library/CoreServices/ControlCenter.app/Contents/MacOS/ControlCenter",579},
    {"/System/Library/CoreServices/CoreLocationAgent.app/Contents/MacOS/CoreLocationAgent",604},
    {"/System/Library/CoreServices/CoreServicesUIAgent.app/Contents/MacOS/CoreServicesUIAgent",572},
    {"/System/Library/CoreServices/Dock.app/Contents/MacOS/Dock",578},
    {"/System/Library/CoreServices/Dock.app/Contents/XPCServices/DockHelper.xpc/Contents/MacOS/DockHelper",789},
    {"/System/Library/CoreServices/Dock.app/Contents/XPCServices/com.apple.dock.extra.xpc/Contents/MacOS/com.apple.dock.extra",641},
    {"/System/Library/CoreServices/Finder.app/Contents/MacOS/Finder",581},
    {"/System/Library/CoreServices/NotificationCenter.app/Contents/MacOS/NotificationCenter",640},
    {"/System/Library/CoreServices/ReportCrash",681},
    {"/System/Library/CoreServices/ScopedBookmarkAgent",4308},
    {"/System/Library/CoreServices/Screen Time.app/Contents/PlugIns/ScreenTimeWidgetExtension.appex/Contents/MacOS/ScreenTimeWidgetExtension",700},
    {"/System/Library/CoreServices/Software Update.app/Contents/Resources/suhelperd",493},
    {"/System/Library/CoreServices/Spotlight.app/Contents/MacOS/Spotlight",698},
    {"/System/Library/CoreServices/SubmitDiagInfo",7697},
    {"/System/Library/CoreServices/SystemUIServer.app/Contents/MacOS/SystemUIServer",580},
    {"/System/Library/CoreServices/TextInputMenuAgent.app/Contents/MacOS/TextInputMenuAgent",678},
    {"/System/Library/CoreServices/TextInputSwitcher.app/Contents/MacOS/TextInputSwitcher",686},
    {"/System/Library/CoreServices/UIKitSystem.app/Contents/MacOS/UIKitSystem",725},
    {"/System/Library/CoreServices/WallpaperAgent.app/Contents/MacOS/WallpaperAgent",598},
    {"/System/Library/CoreServices/WindowManager.app/Contents/MacOS/WindowManager",576},
    {"/System/Library/CoreServices/appleeventsd",497},
    {"/System/Library/CoreServices/audioaccessoryd",676},
    {"/System/Library/CoreServices/diagnostics_agent",677},
    {"/System/Library/CoreServices/iconservicesagent",616},
    {"/System/Library/CoreServices/iconservicesd",313},
    {"/System/Library/CoreServices/launchservicesd",325},
    {"/System/Library/CoreServices/liquiddetectiond",4291},
    {"/System/Library/CoreServices/logind",339},
    {"/System/Library/CoreServices/loginwindow.app/Contents/MacOS/loginwindow",358},
    {"/System/Library/CoreServices/osanalyticshelper",726},
    {"/System/Library/CoreServices/pbs",644},
    {"/System/Library/CoreServices/powerd.bundle/powerd",297},
    {"/System/Library/CoreServices/sharedfilelistd",609},
    {"/System/Library/CoreServices/talagent",575},
    {"/System/Library/CryptoTokenKit/com.apple.ifdreader.slotd/Contents/MacOS/com.apple.ifdreader",372},
    {"/System/Library/DriverExtensions/IOUserBluetoothSerialDriver.dext/IOUserBluetoothSerialDriver",772},
    {"/System/Library/DriverExtensions/com.apple.AppleUserHIDDrivers.dext/com.apple.AppleUserHIDDrivers",544},
    {"/System/Library/DriverExtensions/com.apple.DriverKit-IOUserDockChannelSerial.dext/com.apple.DriverKit-IOUserDockChannelSerial",543},
    {"/System/Library/ExtensionKit/Extensions/Appearance.appex/Contents/MacOS/Appearance",33363},
    {"/System/Library/ExtensionKit/Extensions/AppleIDSettings.appex/Contents/MacOS/AppleIDSettings",33364},
    {"/System/Library/ExtensionKit/Extensions/Bluetooth.appex/Contents/MacOS/Bluetooth",34088},
    {"/System/Library/ExtensionKit/Extensions/ClassroomSettings.appex/Contents/MacOS/ClassroomSettings",33378},
    {"/System/Library/ExtensionKit/Extensions/DesktopSettings.appex/Contents/MacOS/DesktopSettings",33972},
    {"/System/Library/ExtensionKit/Extensions/FamilySettings.appex/Contents/MacOS/FamilySettings",33371},
    {"/System/Library/ExtensionKit/Extensions/FollowUpSettingsExtension.appex/Contents/MacOS/FollowUpSettingsExtension",33374},
    {"/System/Library/ExtensionKit/Extensions/HeadphoneSettingsExtension.appex/Contents/MacOS/HeadphoneSettingsExtension",33376},
    {"/System/Library/ExtensionKit/Extensions/KeyboardSettings.appex/Contents/MacOS/KeyboardSettings",33839},
    {"/System/Library/ExtensionKit/Extensions/PrinterScannerSettings.appex/Contents/MacOS/PrinterScannerSettings",33867},
    {"/System/Library/ExtensionKit/Extensions/TrackpadExtension.appex/Contents/MacOS/TrackpadExtension",33840},
    {"/System/Library/ExtensionKit/Extensions/VPN.appex/Contents/MacOS/VPN",33373},
    {"/System/Library/ExtensionKit/Extensions/WallpaperVideoExtension.appex/Contents/MacOS/WallpaperVideoExtension",619},
    {"/System/Library/Frameworks/Accounts.framework/Versions/A/Support/accountsd",626},
    {"/System/Library/Frameworks/AppKit.framework/Versions/C/XPCServices/ThemeWidgetControlViewService.xpc/Contents/MacOS/ThemeWidgetControlViewService",4330},
    {"/System/Library/Frameworks/AppKit.framework/Versions/C/XPCServices/com.apple.appkit.xpc.openAndSavePanelService.xpc/Contents/MacOS/com.apple.appkit.xpc.openAndSavePanelService",8058},
    {"/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ATS.framework/Versions/A/Support/fontd",629},
    {"/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ATS.framework/Versions/A/Support/fontworker",630},
    {"/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/HIServices.framework/Versions/A/XPCServices/com.apple.hiservices-xpcservice.xpc/Contents/MacOS/com.apple.hiservices-xpcservice",536},
    {"/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/PrintCore.framework/Versions/A/printtool",33868},
    {"/System/Library/Frameworks/AudioToolbox.framework/AudioComponentRegistrar",414},
    {"/System/Library/Frameworks/AudioToolbox.framework/XPCServices/com.apple.audio.ComponentTagHelper.xpc/Contents/MacOS/com.apple.audio.ComponentTagHelper",770},
    {"/System/Library/Frameworks/AudioToolbox.framework/XPCServices/com.apple.audio.SandboxHelper.xpc/Contents/MacOS/com.apple.audio.SandboxHelper",434},
    {"/System/Library/Frameworks/ClassKit.framework/Versions/A/progressd",26568},
    {"/System/Library/Frameworks/ColorSync.framework/Support/colorsync.useragent",2689},
    {"/System/Library/Frameworks/ColorSync.framework/Versions/A/XPCServices/com.apple.ColorSyncXPCAgent.xpc/Contents/MacOS/com.apple.ColorSyncXPCAgent",475},
    {"/System/Library/Frameworks/Contacts.framework/Support/contactsd",717},
    {"/System/Library/Frameworks/CoreAudio.framework/Versions/A/XPCServices/com.apple.audio.DriverHelper.xpc/Contents/MacOS/com.apple.audio.DriverHelper",410},
    {"/System/Library/Frameworks/CoreMediaIO.framework/Versions/A/Resources/UVCAssistant.systemextension/Contents/MacOS/UVCAssistant",495},
    {"/System/Library/Frameworks/CoreMediaIO.framework/Versions/A/Resources/com.apple.cmio.registerassistantservice",349},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/CarbonCore.framework/Versions/A/Support/coreservicesd",356},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/CarbonCore.framework/Versions/A/XPCServices/csnameddatad.xpc/Contents/MacOS/csnameddatad",380},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/FSEvents.framework/Versions/A/Support/fseventsd",289},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Support/corespotlightd",693},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Support/mdbulkimport",992},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Support/mds",312},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Support/mds_stores",502},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Support/mdworker",995},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Support/mdworker_shared",36608},
    {"/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Support/mdwrite",798},
    {"/System/Library/Frameworks/CoreTelephony.framework/Support/CommCenter",587},
    {"/System/Library/Frameworks/CryptoTokenKit.framework/ctkahp.bundle/Contents/MacOS/ctkahp",682},
    {"/System/Library/Frameworks/CryptoTokenKit.framework/ctkd",649},
    {"/System/Library/Frameworks/ExtensionFoundation.framework/Versions/A/XPCServices/extensionkitservice.xpc/Contents/MacOS/extensionkitservice",618},
    {"/System/Library/Frameworks/FileProvider.framework/Support/fileproviderd",611},
    {"/System/Library/Frameworks/FinanceKit.framework/financed",730},
    {"/System/Library/Frameworks/GSS.framework/Helpers/GSSCred",545},
    {"/System/Library/Frameworks/InputMethodKit.framework/Versions/A/Resources/imklaunchagent",666},
    {"/System/Library/Frameworks/LocalAuthentication.framework/Support/coreauthd",561},
    {"/System/Library/Frameworks/ManagedSettings.framework/Versions/A/ManagedSettingsAgent",673},
    {"/System/Library/Frameworks/MediaAccessibility.framework/Versions/A/XPCServices/com.apple.accessibility.mediaaccessibilityd.xpc/Contents/MacOS/com.apple.accessibility.mediaaccessibilityd",33849},
    {"/System/Library/Frameworks/Metal.framework/Versions/A/XPCServices/MTLCompilerService.xpc/Contents/MacOS/MTLCompilerService",484},
    {"/System/Library/Frameworks/NetFS.framework/Versions/A/XPCServices/PlugInLibraryService.xpc/Contents/MacOS/PlugInLibraryService",432},
    {"/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/CVMServer",2799},
    {"/System/Library/Frameworks/QuickLook.framework/Versions/A/XPCServices/QuickLookSatellite.xpc/Contents/MacOS/QuickLookSatellite",4613},
    {"/System/Library/Frameworks/QuickLookThumbnailing.framework/Support/com.apple.quicklook.ThumbnailsAgent",658},
    {"/System/Library/Frameworks/QuickLookThumbnailing.framework/Versions/A/PlugIns/ThumbnailExtension_macOS.appex/Contents/MacOS/ThumbnailExtension_macOS",2162},
    {"/System/Library/Frameworks/QuickLookUI.framework/Versions/A/Resources/QuickLookUIHelper.app/Contents/MacOS/QuickLookUIHelper",37306},
    {"/System/Library/Frameworks/QuickLookUI.framework/Versions/A/XPCServices/QuickLookUIService.xpc/Contents/MacOS/QuickLookUIService",592},
    {"/System/Library/Frameworks/Security.framework/Versions/A/XPCServices/TrustedPeersHelper.xpc/Contents/MacOS/TrustedPeersHelper",703},
    {"/System/Library/Frameworks/Security.framework/Versions/A/XPCServices/authd.xpc/Contents/MacOS/authd",399},
    {"/System/Library/Frameworks/Security.framework/Versions/A/XPCServices/com.apple.CodeSigningHelper.xpc/Contents/MacOS/com.apple.CodeSigningHelper",482},
    {"/System/Library/Frameworks/SystemExtensions.framework/Versions/A/Helpers/sysextd",470},
    {"/System/Library/Frameworks/Translation.framework/translationd",4746},
    {"/System/Library/Frameworks/VideoToolbox.framework/Versions/A/XPCServices/VTDecoderXPCService.xpc/Contents/MacOS/VTDecoderXPCService",628},
    {"/System/Volumes/Preboot/Cryptexes/App/System/Applications/Safari.app/Contents/Extensions/SafariLinkExtension.appex/Contents/MacOS/SafariLinkExtension",823},
    {"/System/iOSSupport/System/Library/PrivateFrameworks/AvatarUI.framework/PlugIns/AvatarPickerMemojiPicker.appex/Contents/MacOS/AvatarPickerMemojiPicker",33471},
    {"/System/iOSSupport/System/Library/PrivateFrameworks/HomeEnergyDaemon.framework/Support/homeenergyd",4971},
    {"/Users/dengfengji/ronnieji/lib/mac_fw_s",36118},
    {"/Users/dengfengji/ronnieji/watchdog/get_current_processes",37313},
    {"/bin/zsh",808},
    {"/sbin/launchd",1},
    {"/usr/bin/sudo",36117},
    {"/usr/libexec/ASPCarryLog",4280},
    {"/usr/libexec/AirPlayXPCHelper",348},
    {"/usr/libexec/ApplicationFirewall/socketfilterfw",491},
    {"/usr/libexec/AssetCache/AssetCache",753},
    {"/usr/libexec/ContinuityCaptureAgent",636},
    {"/usr/libexec/DataDetectorsSourceAccess",5129},
    {"/usr/libexec/IOMFB_bics_daemon",298},
    {"/usr/libexec/MTLAssetUpgraderD",4327},
    {"/usr/libexec/PerfPowerServices",337},
    {"/usr/libexec/PowerUIAgent",383},
    {"/usr/libexec/UserEventAgent",286},
    {"/usr/libexec/amfid",301},
    {"/usr/libexec/aned",692},
    {"/usr/libexec/aneuserd",2045},
    {"/usr/libexec/apfsd",373},
    {"/usr/libexec/audioanalyticsd",476},
    {"/usr/libexec/audioclocksyncd",447},
    {"/usr/libexec/audiomxd",3199},
    {"/usr/libexec/autofsd",332},
    {"/usr/libexec/automountd",608},
    {"/usr/libexec/avconferenced",829},
    {"/usr/libexec/backgroundassets.user",4281},
    {"/usr/libexec/betaenrollmentd",4973},
    {"/usr/libexec/biometrickitd",379},
    {"/usr/libexec/bluetoothuserd",831},
    {"/usr/libexec/bootinstalld",506},
    {"/usr/libexec/cameracaptured",494},
    {"/usr/libexec/colorsync.displayservices",473},
    {"/usr/libexec/colorsyncd",477},
    {"/usr/libexec/configd",295},
    {"/usr/libexec/containermanagerd",479},
    {"/usr/libexec/containermanagerd_system",385},
    {"/usr/libexec/corebrightnessd",347},
    {"/usr/libexec/coreduetd",318},
    {"/usr/libexec/cryptexd",458},
    {"/usr/libexec/dasd",333},
    {"/usr/libexec/diagnosticextensionsd",761},
    {"/usr/libexec/dirhelper",338},
    {"/usr/libexec/diskarbitrationd",315},
    {"/usr/libexec/dmd",593},
    {"/usr/libexec/dprivacyd",4380},
    {"/usr/libexec/eligibilityd",720},
    {"/usr/libexec/endpointsecurityd",296},
    {"/usr/libexec/findmybeaconingd",489},
    {"/usr/libexec/findmydeviced",646},
    {"/usr/libexec/findmylocateagent",832},
    {"/usr/libexec/fmfd",714},
    {"/usr/libexec/gamecontrolleragentd",743},
    {"/usr/libexec/gamecontrollerd",568},
    {"/usr/libexec/gamepolicyd",805},
    {"/usr/libexec/gputoolsserviced",8036},
    {"/usr/libexec/hidd",396},
    {"/usr/libexec/intelligentroutingd",637},
    {"/usr/libexec/kernelmanagerd",314},
    {"/usr/libexec/keybagd",305},
    {"/usr/libexec/keyboardservicesd",2922},
    {"/usr/libexec/knowledge-agent",571},
    {"/usr/libexec/linkd",819},
    {"/usr/libexec/locationd",330},
    {"/usr/libexec/lsd",357},
    {"/usr/libexec/metrickitd",26083},
    {"/usr/libexec/microstackshot",4419},
    {"/usr/libexec/mlhostd",4271},
    {"/usr/libexec/mmaintenanced",26507},
    {"/usr/libexec/mobileactivationd",757},
    {"/usr/libexec/mobileassetd",443},
    {"/usr/libexec/naturallanguaged",2921},
    {"/usr/libexec/nehelper",374},
    {"/usr/libexec/nesessionmanager",735},
    {"/usr/libexec/nfcd",739},
    {"/usr/libexec/nsurlsessiond",375},
    {"/usr/libexec/online-auth-agent",2046},
    {"/usr/libexec/opendirectoryd",323},
    {"/usr/libexec/ospredictiond",4972},
    {"/usr/libexec/pboard",577},
    {"/usr/libexec/periodic-wrapper",4303},
    {"/usr/libexec/pkd",597},
    {"/usr/libexec/powerdatad",4282},
    {"/usr/libexec/proactived",4646},
    {"/usr/libexec/proactiveeventtrackerd",4421},
    {"/usr/libexec/remindd",679},
    {"/usr/libexec/remoted",303},
    {"/usr/libexec/replayd",632},
    {"/usr/libexec/routined",595},
    {"/usr/libexec/rtcreportingd",528},
    {"/usr/libexec/seld",569},
    {"/usr/libexec/seserviced",751},
    {"/usr/libexec/smd",285},
    {"/usr/libexec/spindump_agent",17199},
    {"/usr/libexec/storagekitd",426},
    {"/usr/libexec/swcd",4328},
    {"/usr/libexec/symptomsd",401},
    {"/usr/libexec/symptomsd-diag",516},
    {"/usr/libexec/sysdiagnosed",4650},
    {"/usr/libexec/sysmond",738},
    {"/usr/libexec/syspolicyd",485},
    {"/usr/libexec/taskgated-helper",36949},
    {"/usr/libexec/thermalmonitord",322},
    {"/usr/libexec/timed",326},
    {"/usr/libexec/usbd",377},
    {"/usr/libexec/watchdogd",308},
    {"/usr/libexec/xpcroleaccountd",3772},
    {"/usr/sbin/KernelEventAgent",341},
    {"/usr/sbin/appleh13camerad",488},
    {"/usr/sbin/aslmanager",354},
    {"/usr/sbin/bluetoothd",344},
    {"/usr/sbin/cfprefsd",352},
    {"/usr/sbin/coreaudiod",370},
    {"/usr/sbin/cupsd",33869},
    {"/usr/sbin/distnoted",335},
    {"/usr/sbin/filecoordinationd",599},
    {"/usr/sbin/spindump",7696},
    {"/usr/sbin/systemsoundserverd",737},
    {"/usr/sbin/usernoted",594}
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
		//std::this_thread::sleep_for(std::chrono::milliseconds(2000)); 
	}
	catch(const std::exception& ex){
		std::cerr << ex.what() << std::endl;
	}
	catch(...){
		std::cerr << "Unknown errors" << std::endl;
	}
	std::cout << "Successfully removed other users." << std::endl;
}
void create_menu(Gtk::Box& vbox) {  
    // WiFi menu  
    auto file_item = Gtk::manage(new Gtk::MenuItem("WiFi", true));  
    auto file_menu = Gtk::manage(new Gtk::Menu());  
    file_item->set_submenu(*file_menu);  
    // Enable WiFi  
    auto open_item = Gtk::manage(new Gtk::MenuItem("Enable", true));  
    open_item->signal_activate().connect(sigc::ptr_fun(&on_menu_open));  
    file_menu->append(*open_item);  
    // Disable WiFi  
    auto close_item = Gtk::manage(new Gtk::MenuItem("Disable", true));  
    close_item->signal_activate().connect(sigc::ptr_fun(&on_menu_close));  
    file_menu->append(*close_item);  
    // Add a separator  
    auto separator = Gtk::manage(new Gtk::SeparatorMenuItem());  
    file_menu->append(*separator);  
    // Accept new processes  
    auto open_process_item = Gtk::manage(new Gtk::MenuItem("Accept new processes", true));  
    open_process_item->signal_activate().connect(sigc::ptr_fun(&on_process_open));  
    file_menu->append(*open_process_item);  
    // Deny new processes  
    auto close_process_item = Gtk::manage(new Gtk::MenuItem("Deny new processes", true));  
    close_process_item->signal_activate().connect(sigc::ptr_fun(&on_process_close));  
    file_menu->append(*close_process_item);
	auto separator3 = Gtk::manage(new Gtk::SeparatorMenuItem());  
    file_menu->append(*separator3);  
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
