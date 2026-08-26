#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <array>
#include <map>
#include <vector>
#include <format>            // For std::format in Logger
#include "nlohmann/json.hpp" // For JSON logging and parsing

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <unistd.h>
#else
#include <unistd.h>
#endif

// ==========================================
// Thread-Safe nlohmann JSON Logger
// ==========================================
class Logger {
private:
    static inline std::mutex log_mutex;

public:
    template<typename... Args>
    static void info(const std::format_string<Args...> fmt, Args&&... args) {
        log("INFO", fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(const std::format_string<Args...> fmt, Args&&... args) {
        log("WARN", fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(const std::format_string<Args...> fmt, Args&&... args) {
        log("ERROR", fmt, std::forward<Args>(args)...);
    }

private:
    template<typename... Args>
    static void log(const char* level, const std::format_string<Args...> fmt, Args&&... args) {
        // 1. Get current time
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        char time_buf[25];
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time));

        // 2. Format the message
        std::string message = std::format(fmt, std::forward<Args>(args)...);

        // 3. Construct a nlohmann JSON Object
        nlohmann::json log_entry;
        log_entry["timestamp"] = time_buf;
        log_entry["level"] = level;
        log_entry["message"] = message;

        // 4. Serialize to string
        std::string json_str = log_entry.dump();

        // 5. Write to console and file in a thread-safe manner
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << json_str << "\n";
        
        // Appends each JSON object on a new line (JSON Lines format)
        std::ofstream out("log.json", std::ios::app);
        if (out) {
            out << json_str << "\n";
        }
    }
};

// ==========================================
// Main Server Class
// ==========================================
class Server {
private:
    std::mutex queue_mutex;
    std::queue<std::string> pending_tasks;
    std::jthread monitor_thread;
    
    const size_t MIN_RAM_MB = 500;
    const size_t MIN_DISK_MB = 1024;

    size_t get_free_ram_mb() {
#ifdef _WIN32
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        return status.ullAvailPhys / (1024 * 1024);
#elif defined(__APPLE__)
        mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
        vm_statistics64_data_t vm_stat;
        if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &count) == KERN_SUCCESS) {
            long long free_memory = (int64_t)(vm_stat.free_count + vm_stat.inactive_count) * sysconf(_SC_PAGESIZE);
            return free_memory / (1024 * 1024);
        }
        return 0;
#else
        long pages = sysconf(_SC_AVPHYS_PAGES);
        long page_size = sysconf(_SC_PAGE_SIZE);
        return (pages * page_size) / (1024 * 1024);
#endif
    }

    size_t get_free_disk_mb() {
        std::filesystem::space_info si = std::filesystem::space(".");
        return si.available / (1024 * 1024);
    }

    bool check_resources(bool log_failure = true) {
        size_t free_ram = get_free_ram_mb();
        size_t free_disk = get_free_disk_mb();
        if (free_ram < MIN_RAM_MB || free_disk < MIN_DISK_MB) {
            if (log_failure) {
                // Using the new logger with format arguments
                Logger::warn("RESOURCE LIMIT REACHED - RAM: {}MB, Disk: {}MB", free_ram, free_disk);
            }
            return false;
        }
        return true;
    }

    std::string execute_slave(const std::string& params) {
        std::string cmd;
#ifdef _WIN32
        cmd = "slave.exe \"" + params + "\"";
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
        cmd = "/Users/jidengfeng/Downloads/MLCpplib/slave \"" + params + "\""; 
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif
        if (!pipe) {
            Logger::error("Failed to start slave process.");
            return "";
        }
        
        std::array<char, 256> buffer;
        std::string result;
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    void resource_monitor(std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::lock_guard<std::mutex> lock(queue_mutex);
            if (!pending_tasks.empty() && check_resources(false)) {
                std::string task = pending_tasks.front();
                pending_tasks.pop();
                
                Logger::info("Resources recovered. Dequeued task: {}", task);
                std::thread(&Server::run_task_async, this, task, [](const std::string&){}).detach();
            }
        }
    }

    void run_task_async(const std::string& params, std::function<void(const std::string&)> callback) {
        Logger::info("Starting slave process for task: {}", params);
        std::string result = execute_slave(params);
        
        // Log the return from the slave (it will automatically escape the JSON quotes)
        Logger::info("Slave successfully returned data.");
        
        if (callback) callback(result);
    }

public:
    Server() {
        monitor_thread = std::jthread([this](std::stop_token stoken) { this->resource_monitor(stoken); });
        Logger::info("Server initialized. Monitoring resources.");
    }

    void submit_task(const std::string& params, std::function<void(const std::string&)> callback) {
        if (check_resources(true)) {
            std::thread(&Server::run_task_async, this, params, callback).detach();
        } else {
            Logger::warn("Task queued due to low resources: {}", params);
            std::lock_guard<std::mutex> lock(queue_mutex);
            pending_tasks.push(params);
        }
    }
};

int main() {
    Server server;

    auto on_slave_complete = [](const std::string& raw_output) {
        try {
            nlohmann::json parsed = nlohmann::json::parse(raw_output);
            std::map<std::string, std::vector<std::string>> parsed_map;
            
            if (parsed.is_object()) {
                for (auto const& [key, val] : parsed.items()) {
                    if (val.is_array()) {
                        std::vector<std::string> vec;
                        for (auto const& item : val) {
                            vec.push_back(item.get<std::string>());
                        }
                        parsed_map[key] = vec;
                    }
                }
            }

            std::cout << "\n[Callback] Native C++ Map Reconstructed:\n";
            for (const auto& [key, vec] : parsed_map) {
                std::cout << "  - " << key << ": [ ";
                for (const auto& item : vec) std::cout << item << " ";
                std::cout << "]\n";
            }

        } catch (const std::exception& e) {
            Logger::error("Failed to parse slave JSON: {}", e.what());
        }
    };

    server.submit_task("Param_A=10;Param_B=20", on_slave_complete);

    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    return 0;
}