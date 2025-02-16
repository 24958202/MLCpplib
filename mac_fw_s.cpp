/*
        Updated Firewall Monitor Code
        Includes fixes for error handling, thread safety, and deprecated GTKmm APIs.
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
    #include <cstdlib>
    #include <sstream>
    #include <array>
    #include <libproc.h>
    #include <unistd.h>
    #include <mach/mach.h>
    #include <stdexcept>
    #include <atomic>
    #include <mutex>

    // Global variables
    static std::atomic<bool> ufw_checking(false);
    static std::atomic<bool> processes_checking(false);
    static std::atomic<bool> enable_new_process(false);
    static std::vector<std::string> friend_users{"root", "dengfengji"};
    static std::map<std::string, int> pure_mac_processes{
        {"/usr/sbin/coreaudiod", 389},
        {"/usr/sbin/filecoordinationd", 598},
        {"/usr/sbin/spindump", 2747},
        {"/usr/sbin/systemsoundserverd", 756},
        {"/usr/sbin/systemstats", 303},
        {"/usr/sbin/universalaccessd", 568},
        {"/usr/sbin/usernoted", 590}};

    class SysLogLib
    {
    private:
        struct CurrentDateTime
        {
            std::string current_date;
            std::string current_time;
        };

        CurrentDateTime getCurrentDateTime();

    public:
        void sys_timedelay(size_t &); // 3000 = 3 seconds
        void writeLog(const std::string &, const std::string &);
    };

    SysLogLib::CurrentDateTime SysLogLib::getCurrentDateTime()
    {
        std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
        std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
        std::tm *current_time_tm = std::localtime(&current_time_t);
        SysLogLib::CurrentDateTime currentDateTime;
        currentDateTime.current_date = std::to_string(current_time_tm->tm_year + 1900) + "-" + std::to_string(current_time_tm->tm_mon + 1) + "-" + std::to_string(current_time_tm->tm_mday);
        currentDateTime.current_time = std::to_string(current_time_tm->tm_hour) + ":" + std::to_string(current_time_tm->tm_min) + ":" + std::to_string(current_time_tm->tm_sec);
        return currentDateTime;
    }

    void SysLogLib::sys_timedelay(size_t &mini_sec)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(mini_sec));
    }

    void SysLogLib::writeLog(const std::string &logpath, const std::string &log_message)
    {
        if (logpath.empty() || log_message.empty())
        {
            std::cerr << "SysLogLib::writeLog input empty!" << '\n';
            return;
        }
        std::string strLog = logpath;
        if (strLog.back() != '/')
        {
            strLog.append("/");
        }
        if (!std::filesystem::exists(strLog))
        {
            try
            {
                std::filesystem::create_directory(strLog);
            }
            catch (const std::exception &e)
            {
                std::cout << "Error creating log folder: " << e.what() << std::endl;
                return;
            }
        }
        SysLogLib::CurrentDateTime currentDateTime = SysLogLib::getCurrentDateTime();
        strLog.append(currentDateTime.current_date);
        strLog.append(".txt");
        std::ofstream file(strLog, std::ios::app);
        if (!file.is_open())
        {
            std::cerr << "Error opening log file: " << strLog << std::endl;
            return;
        }
        file << currentDateTime.current_time + " : " + log_message << std::endl;
        file.close();
    }

    void readSystemLogs(const std::string &command)
    {
        std::array<char, 128> buffer;
        std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe)
        {
            std::cerr << "popen() failed!" << std::endl;
            return;
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            std::string msgLog = buffer.data();
            if (msgLog.find("/Users/dengfengji/ronnieji/lib") != std::string::npos)
            {
                SysLogLib syslog_j;
                syslog_j.writeLog("/Users/dengfengji/ronnieji/watchdog/mac_sys_logs", msgLog);
            }
        }
    }

    void write_log_related_to_my_folder()
    {
        std::string command = "log show --last 1m --info";
        std::cout << "Reading system logs..." << std::endl;
        readSystemLogs(command);
    }

    void removeLogs()
    {
        std::vector<std::string> logfolders = {
            "/private/var/log",
            "/private/var/logs"};
        try
        {
            for (const auto &lf : logfolders)
            {
                if (std::filesystem::exists(lf) && std::filesystem::is_directory(lf))
                {
                    std::filesystem::remove_all(lf);
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error removing log folders: " << e.what() << std::endl;
        }
    }

    void terminateProcess(pid_t pid)
    {
        if (pid <= 0)
        {
            std::cerr << "Invalid PID: " << pid << std::endl;
            return;
        }
        if (kill(pid, SIGTERM) == -1)
        {
            perror("SIGTERM failed");
            if (kill(pid, SIGKILL) == -1)
            {
                perror("SIGKILL also failed");
            }
            else
            {
                std::cout << "Process " << pid << " forcefully terminated with SIGKILL." << std::endl;
            }
        }
        else
        {
            std::cout << "Process " << pid << " terminated successfully with SIGTERM." << std::endl;
        }
    }

    std::vector<std::string> get_users()
    {
        std::vector<std::string> users;
        FILE *pipe = popen("dscl . list /Users", "r");
        if (!pipe)
        {
            perror("popen failed");
            return users;
        }
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string user(buffer);
            user.erase(user.find_last_of(" \n\r\t") + 1);
            users.push_back(user);
        }
        pclose(pipe);
        return users;
    }

    void delete_unknown_users()
    {
        while (true)
        {
            std::vector<std::string> list_users = get_users();
            if (!list_users.empty())
            {
                for (const auto &item : list_users)
                {
                    bool is_a_friend_user = false;
                    for (const auto &flist : friend_users)
                    {
                        if (item == flist)
                        {
                            is_a_friend_user = true;
                            break;
                        }
                    }
                    if (!is_a_friend_user)
                    {
                        std::string str_comm = "sudo dscl . -delete /Users/" + item;
                        std::cout << "Executing command: " << str_comm << std::endl;
                        int result = std::system(str_comm.c_str());
                        if (result == -1)
                        {
                            perror("Error executing system command");
                        }
                        else if (WEXITSTATUS(result) != 0)
                        {
                            std::cerr << "Command failed with exit code: " << WEXITSTATUS(result) << std::endl;
                        }
                        else
                        {
                            std::cout << "User " << item << " removed successfully." << std::endl;
                        }
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    int main(int argc, char **argv)
    {
        auto app = Gtk::Application::create(argc, argv, "org.24958202.pf_monitor");
        Gtk::Window window;
        window.set_title("<<<24958202@qq.com Ronnie Ji>>>");
        window.set_default_size(400, 200);
        Gtk::Box vbox(Gtk::Orientation::VERTICAL);
        window.add(vbox);
        Gtk::Label label("Monitoring PF rules.24958202@qq.com");
        vbox.append(label);
        window.show_all();
        std::thread deleteUsersThread(delete_unknown_users);
        deleteUsersThread.detach();
        return app->run(window);
    }
