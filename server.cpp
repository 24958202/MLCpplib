#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>
#include <random>
#include <system_error>
#include <optional>  // FIXED: Required for std::optional
#include <cstdlib>   // FIXED: Required for exit()

#if defined(_WIN32)
    #define NOMINMAX
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#elif defined(__linux__)
    #include <unistd.h>
#else
    #error "Executable-path lookup is not implemented for this platform."
#endif

namespace fs = std::filesystem;

// =========================================================
// Cross-Platform Process Management Abstraction
// =========================================================
#ifdef _WIN32
    const std::string SLAVE_EXE_NAME = "slave.exe";
    struct ProcessHandle {
        HANDLE hProcess;
        HANDLE hThread;
        DWORD pid;
    };
#else
    #include <sys/wait.h>
    #include <signal.h>
    #include <unistd.h> // FIXED: Required for fork, execl, kill across all POSIX (macOS included)
    const std::string SLAVE_EXE_NAME = "slave";
    struct ProcessHandle {
        pid_t pid;
    };
#endif


namespace nemslib {

std::string get_executable_dir()
{
    namespace fs = std::filesystem;
    fs::path executablePath;
#if defined(_WIN32)
    std::vector<wchar_t> buffer(260);
    while (true) {
        const DWORD length = GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            executablePath = fs::path(std::wstring(buffer.data(), length));
            break;
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    executablePath = fs::path(buffer.data());
#elif defined(__linux__)
    std::vector<char> buffer(256);
    while (true) {
        const ssize_t length = readlink(
            "/proc/self/exe",
            buffer.data(),
            buffer.size()
        );
        if (length < 0) {
            return {};
        }
        if (static_cast<std::size_t>(length) < buffer.size()) {
            executablePath = fs::path(
                std::string(buffer.data(), static_cast<std::size_t>(length))
            );
            break;
        }
        buffer.resize(buffer.size() * 2);
    }
#endif
    std::error_code ec;
    executablePath = fs::weakly_canonical(executablePath, ec);
    if (ec) {
        return {};
    }
    return executablePath.parent_path().string();
}
} // namespace nemslib

class ProcessManager {
public:
    static std::optional<ProcessHandle> spawn(const std::string& task_id, const std::string& params) {
        fs::path exe_path = fs::current_path() / SLAVE_EXE_NAME;
        
        if (!fs::exists(exe_path)) {
            std::cerr << "\n[ERROR] Cannot find executable: " << exe_path << "\n";
            return std::nullopt;
        }

#ifdef _WIN32
        // FIXED: Wrap exe_path in double quotes to prevent CreateProcess bugs if the path contains spaces
        std::string cmd = "\"" + exe_path.string() + "\" " + task_id + " " + params;
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            return ProcessHandle{pi.hProcess, pi.hThread, pi.dwProcessId};
        }
        return std::nullopt;
#else
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            execl(exe_path.c_str(), exe_path.c_str(), task_id.c_str(), params.c_str(), (char*)NULL);
            exit(1); // Exit if execl fails
        } else if (pid > 0) {
            // Parent process
            return ProcessHandle{pid};
        }
        return std::nullopt; // Fork failed
#endif
    }

    static bool is_finished(const ProcessHandle& ph) {
#ifdef _WIN32
        return WaitForSingleObject(ph.hProcess, 0) == WAIT_OBJECT_0;
#else
        int status;
        return waitpid(ph.pid, &status, WNOHANG) == ph.pid;
#endif
    }

    static void terminate(const ProcessHandle& ph) {
#ifdef _WIN32
        TerminateProcess(ph.hProcess, 1);
#else
        kill(ph.pid, SIGTERM);
#endif
    }

    static void cleanup(const ProcessHandle& ph) {
#ifdef _WIN32
        CloseHandle(ph.hProcess);
        CloseHandle(ph.hThread);
#endif
        // On POSIX, waitpid during is_finished() already cleans up the zombie
    }
};

// =========================================================
// Data Structures & Monitors
// =========================================================
struct Task {
    int task_id;
    std::string parameters;
};

struct ActiveSlave {
    Task task;
    ProcessHandle handle;
    bool unloading; 
};

class SystemMonitor {
public:
    static bool is_disk_critical() {
        try {
            auto space = fs::space(".");
            return space.available < (1024ULL * 1024 * 1024); // 1GB
        } catch (...) { return false; }
    }

    static bool is_ram_critical() {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng) <= 5; // Simulate 5% chance of RAM spike
    }
};

// =========================================================
// Server Class
// =========================================================
class Server {
private:
    std::deque<Task> pending_list;
    std::vector<Task> error_list;
    std::vector<int> completed_list; // Just storing successful IDs for display
    
    // Using a map keyed by task_id to manage active slaves
    std::map<int, ActiveSlave> active_slaves;
    
    const size_t MAX_CONCURRENT_SLAVES = 4;
    bool running = true;

public:
    void add_task(const Task& task) {
        pending_list.push_back(task);
    }

    void run() {
        while (running) {
            check_running_slaves();
            manage_resources_and_slaves();
            update_real_time_display();

            if (pending_list.empty() && active_slaves.empty()) {
                running = false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::cout << "\n[Server] All tasks finished. Shutting down...\n";
    }

private:
    void check_running_slaves() {
        for (auto it = active_slaves.begin(); it != active_slaves.end(); ) {
            ActiveSlave& slave = it->second;

            if (ProcessManager::is_finished(slave.handle)) {
                if (slave.unloading) {
                    // Task was killed for resources, put it back in queue
                    pending_list.push_front(slave.task);
                } else {
                    // Task finished naturally, read results
                    process_slave_output(slave.task);
                }
                
                ProcessManager::cleanup(slave.handle);
                it = active_slaves.erase(it); // Remove from active map
            } else {
                ++it;
            }
        }
    }

    void process_slave_output(const Task& task) {
        std::string filename = "result_" + std::to_string(task.task_id) + ".txt";
        
        if (fs::exists(filename)) {
            std::ifstream file(filename);
            std::string content;
            std::getline(file, content);
            file.close();
            fs::remove(filename); 

            if (content.starts_with("SUCCESS")) {
                completed_list.push_back(task.task_id);
            } else {
                error_list.push_back(task);
            }
        } else {
            // No output file found, slave crashed
            error_list.push_back(task);
        }
    }

    void manage_resources_and_slaves() {
        bool resources_critical = SystemMonitor::is_disk_critical() || SystemMonitor::is_ram_critical();

        if (resources_critical) {
            // Unload a slave
            for (auto& [id, slave] : active_slaves) {
                if (!slave.unloading) {
                    slave.unloading = true;
                    ProcessManager::terminate(slave.handle);
                    break; // Unload one per tick
                }
            }
        } else {
            // Load new slaves
            while (active_slaves.size() < MAX_CONCURRENT_SLAVES && !pending_list.empty()) {
                Task task = pending_list.front();
                pending_list.pop_front();

                auto handle_opt = ProcessManager::spawn(std::to_string(task.task_id), task.parameters);
                
                if (handle_opt.has_value()) {
                    active_slaves[task.task_id] = {task, handle_opt.value(), false};
                } else {
                    // Spawn failed (e.g., file not found). Put it in errors.
                    error_list.push_back(task);
                }
            }
        }
    }

    void update_real_time_display() {
        // Clear screen (ANSI escape code, works on MacOS/Linux and modern Windows 10/11)
        std::cout << "\033[2J\033[1;1H"; 
        
        std::cout << "========================================\n";
        std::cout << "  CROSS-PLATFORM MULTI-PROCESS SERVER   \n";
        std::cout << "========================================\n";
        
        std::cout << "Active Slaves (Task IDs): ";
        for (const auto& [id, slave] : active_slaves) {
            std::cout << "[" << id << (slave.unloading ? " - Unloading!" : "") << "] ";
        }
        std::cout << "\n\n";

        std::cout << "Pending List : " << pending_list.size() << " tasks\n";
        std::cout << "Success List : " << completed_list.size() << " tasks\n";
        std::cout << "Error List   : " << error_list.size() << " tasks\n";
        
        if (!error_list.empty()) {
            std::cout << " -> Errors on Tasks: ";
            for (const auto& t : error_list) {
                std::cout << t.task_id << " ";
            }
            std::cout << "\n";
        }
        std::cout << "========================================\n";
    }
};

// =========================================================
// Main
// =========================================================
int main() {
    const std::string executableDir = nemslib::get_executable_dir();
    if (executableDir.empty()) {
        std::cerr << "Could not determine the executable directory.\n";
        return 1;
    }
    
    // FIXED: Set the application's working directory to the executable's directory.
    // This perfectly synchronizes ProcessManager::spawn, process_slave_output, and
    // the slave instances themselves without needing complex path injections everywhere.
    std::error_code ec;
    fs::current_path(executableDir, ec);
    if (ec) {
        std::cerr << "Warning: Could not set working directory to " << executableDir << "\n";
    }

    // Clean up leftover result files from previous runs
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
        if (entry.is_regular_file() && entry.path().filename().string().starts_with("result_")) {
            fs::remove(entry);
        }
    }
    
    Server server;
    for (int i = 1; i <= 15; ++i) {
        server.add_task({i, "Param_A=10;Param_B=20"});
    }
    server.run();
    return 0;
}