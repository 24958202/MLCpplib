// mac_secure.cpp -- C++23, macOS only.
// This is inbound hardening + process AUDITING, not an outbound app firewall.
/*
	Usage:
	b3sum /Users/jidengfeng/Downloads/MLCpplib/macCheck
    sudo /Users/jidengfeng/Downloads/MLCpplib/macCheck --whitelist /Users/jidengfeng/macbook_protect/process_whitelist.txt --interval 5
*/
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <libproc.h>
#include <poll.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace {
volatile std::sig_atomic_t stopping = 0;
void onSignal(int) { stopping = 1; }

struct Fd {
    int value;
    explicit Fd(int fd) : value(fd) {}
    ~Fd() { if (value >= 0) ::close(value); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
};

std::string timestamp() {
    const auto now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);
    std::array<char, 32> text{};
    std::strftime(text.data(), text.size(), "%Y-%m-%d %H:%M:%S", &tm);
    return text.data();
}

// Never follow a log-file symlink when running as root.
class Logger {
    Fd fd_;
public:
    explicit Logger(const fs::path& path)
        : fd_(::open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND |
                                 O_CLOEXEC | O_NOFOLLOW, 0600)) {
        if (fd_.value < 0) throw std::runtime_error("Cannot open log: " + path.string());
        struct stat st{};
        if (fstat(fd_.value, &st) != 0 || !S_ISREG(st.st_mode) ||
            st.st_uid != geteuid() || (st.st_mode & 0022) != 0)
            throw std::runtime_error("Log must be a regular, owned, non-group/world-writable file");
    }
    void write(std::string_view message) {
        const auto line = "[" + timestamp() + "] " + std::string(message) + "\n";
        std::cout << line << std::flush;
        std::size_t offset = 0;
        while (offset < line.size()) {
            const auto n = ::write(fd_.value, line.data() + offset, line.size() - offset);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) throw std::runtime_error("Cannot write log");
            offset += static_cast<std::size_t>(n);
        }
    }
};

struct Result { int status; std::string output; };

// Absolute executable paths and argv, never a shell. Bounded output and runtime.
Result run(std::vector<std::string> args, std::chrono::seconds timeout = 30s) {
    if (args.empty() || args.front().empty() || args.front()[0] != '/')
        throw std::runtime_error("An absolute executable path is required");
    int pipefd[2];
    if (::pipe(pipefd) != 0) throw std::runtime_error("pipe failed");
    Fd input(pipefd[0]), output(pipefd[1]);
    for (int fd : pipefd)
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
            throw std::runtime_error("fcntl failed");
    Fd nullInput(::open("/dev/null", O_RDONLY | O_CLOEXEC));
    if (nullInput.value < 0) throw std::runtime_error("Cannot open /dev/null");

    posix_spawn_file_actions_t actions;
    int error = posix_spawn_file_actions_init(&actions);
    if (error) throw std::runtime_error(std::strerror(error));
    struct ActionsGuard {
        posix_spawn_file_actions_t* actions;
        ~ActionsGuard() { posix_spawn_file_actions_destroy(actions); }
    } guard{&actions};
    auto check = [](int code) {
        if (code) throw std::runtime_error(std::strerror(code));
    };
    check(posix_spawn_file_actions_adddup2(&actions, nullInput.value, STDIN_FILENO));
    check(posix_spawn_file_actions_adddup2(&actions, output.value, STDOUT_FILENO));
    check(posix_spawn_file_actions_adddup2(&actions, output.value, STDERR_FILENO));
    check(posix_spawn_file_actions_addclose(&actions, input.value));

    std::vector<char*> argv;
    for (auto& arg : args) argv.push_back(arg.data());
    argv.push_back(nullptr);
    // Do not pass an arbitrary root caller's environment to privileged tools.
    char pathEnv[] = "PATH=/usr/bin:/bin:/usr/sbin:/sbin";
    char localeEnv[] = "LC_ALL=C";
    char* environment[] = {pathEnv, localeEnv, nullptr};
    pid_t pid{};
    error = posix_spawn(&pid, argv[0], &actions, nullptr, argv.data(), environment);
    if (error) return {127, std::strerror(error)};
    ::close(output.value);
    output.value = -1;
    if (fcntl(input.value, F_SETFL, O_NONBLOCK) == -1) {
        ::kill(pid, SIGKILL);
        while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {}
        throw std::runtime_error("fcntl failed");
    }

    std::string text;
    std::array<char, 4096> buffer{};
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    bool exited = false, eof = false, interrupted = false;
    while (!exited || !eof) {
        // Bound each drain too: a continuously writing child cannot defeat timeout.
        for (int i = 0; i < 32 && !eof; ++i) {
            const auto n = ::read(input.value, buffer.data(), buffer.size());
            if (n > 0) {
                constexpr std::size_t limit = 1024 * 1024;
                if (text.size() < limit)
                    text.append(buffer.data(), std::min(static_cast<std::size_t>(n), limit - text.size()));
            } else if (n == 0) { eof = true; break; }
            else if (errno == EINTR) continue;
            else if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else { eof = true; break; }
        }
        if (!exited) {
            const auto waited = waitpid(pid, &status, WNOHANG);
            if (waited == pid) exited = true;
            else if (waited < 0 && errno != EINTR) {
                exited = true;
                interrupted = true;
            }
        }
        if (exited && eof) break;
        if (stopping || std::chrono::steady_clock::now() >= deadline) {
            if (!exited) {
                ::kill(pid, SIGKILL);
                while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
            }
            interrupted = true;
            break;
        }
        pollfd pfd{input.value, POLLIN, 0};
        ::poll(&pfd, 1, 50);
    }
    if (interrupted) return {124, text + "\nCommand timed out or was interrupted"};
    if (WIFEXITED(status)) return {WEXITSTATUS(status), text};
    return {128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0), text};
}

bool command(Logger& log, std::vector<std::string> args) {
    std::string description;
    for (const auto& arg : args) description += arg + " ";
    log.write("[CMD] " + description);
    auto result = run(std::move(args));
    if (!result.output.empty()) log.write(result.output);
    if (result.status != 0) log.write("[ERROR] Exit status " + std::to_string(result.status));
    return result.status == 0;
}

struct Options {
    bool configure = false, pf = false, once = false;
    unsigned interval = 5;
    fs::path whitelist = "process_whitelist.txt";
    fs::path log = "/var/log/macCheck.log";
};

Options parse(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (++i >= argc) throw std::runtime_error("Missing value for " + arg);
            return argv[i];
        };
        if (arg == "--configure") options.configure = true;
        else if (arg == "--pf") options.pf = true;
        else if (arg == "--once") options.once = true;
        else if (arg == "--whitelist") options.whitelist = value();
        else if (arg == "--log") options.log = value();
        else if (arg == "--interval") {
            const auto text = value();
            std::size_t consumed = 0;
            const auto n = std::stoul(text, &consumed);
            if (consumed != text.size() || n < 1 || n > 86400)
                throw std::runtime_error("Interval must be 1..86400 seconds");
            options.interval = static_cast<unsigned>(n);
        } else if (arg == "--help") {
            std::cout << "Usage: mac_secure [--configure] [--pf] [--once]\n"
                         "                  [--interval SECONDS] [--whitelist PATH] [--log PATH]\n"
                         "Default: audit processes every 5 seconds; never kill them.\n"
                         "--configure: configure ALF and disable remote/sharing services once.\n"
                         "--pf: validate and load the dedicated PF anchor once.\n"
                         "Requires a preinstalled, active macCheck PF hook and enabled PF.\n";
            std::exit(0);
        } else throw std::runtime_error("Unknown option: " + arg);
    }
    return options;
}

bool configure(Logger& log) {
    bool okay = true;
    auto execute = [&](std::vector<std::string> args) {
        if (stopping) { okay = false; return; }
        if (!command(log, std::move(args))) okay = false;
    };
    const std::string alf = "/usr/libexec/ApplicationFirewall/socketfilterfw";
    for (auto [option, value] : std::array{
             std::pair{"--setglobalstate", "on"},
             std::pair{"--setstealthmode", "on"},
             std::pair{"--setblockall", "off"}})
        execute({alf, option, value});
    // Logging options vary across macOS releases; do not hide an unsupported option.
    execute({alf, "--setloggingmode", "on"});
    // Prevent the automatic signed-application exception where supported.
    execute({alf, "--setallowsigned", "off"});
    execute({alf, "--setallowsignedapp", "off"});
    for (const auto* app : {
             "/Applications/Safari.app", "/System/Applications/Safari.app",
             "/Applications/Google Chrome.app", "/Applications/WorkBuddy.app",
             "/Applications/Muse.app", "/Applications/七七.app"}) {
        if (!fs::is_directory(app)) continue;
        execute({alf, "--add", app});
        execute({alf, "--unblockapp", app});
    }
    // Removing an ALF entry does not block an app. Explicitly block this bundle.
    const std::string universal = "/System/Library/CoreServices/UniversalControl.app";
    if (fs::is_directory(universal)) {
        execute({alf, "--add", universal});
        execute({alf, "--blockapp", universal});
    }
    // CLI scripts (including OpenClaw) are not meaningful ALF app identities.
    execute({"/usr/sbin/systemsetup", "-f", "-setremotelogin", "off"});
    execute({"/System/Library/CoreServices/RemoteManagement/ARDAgent.app/Contents/Resources/kickstart",
             "-deactivate", "-stop"});
    // Disable future starts; boot out currently loaded jobs as well.
    for (const auto* service : {"system/com.apple.InternetSharing",
                                "system/com.apple.screensharing"}) {
        execute({"/bin/launchctl", "disable", service});
        if (!stopping && run({"/bin/launchctl", "print", service}).status == 0)
            execute({"/bin/launchctl", "bootout", service});
    }
    execute({alf, "--getglobalstate"});
    execute({alf, "--getstealthmode"});
    execute({alf, "--listapps"});
    return okay;
}

constexpr std::string_view pfRules = R"PF(# Managed by mac_secure; inbound filtering only.
# No inbound HTTP/HTTPS exceptions: browsers initiate OUTBOUND connections.
pass quick on lo0 all
# Keep DHCP and essential IPv6 control traffic working.
pass in quick inet proto udp from any port 67 to any port 68
pass in quick inet6 proto udp from any port 547 to any port 546
pass in quick inet6 proto ipv6-icmp icmp6-type { 1, 2, 3, 4 }
pass in quick inet6 proto ipv6-icmp from fe80::/10 icmp6-type { 133, 134, 135, 136 }
block in quick all
pass out all keep state
)PF";

bool configurePf(Logger& log) {
    // Only update our anchor. Never replace/reload the system's main ruleset,
    // disable PF, flush states, or change another product's anchors.
    auto hooks = run({"/sbin/pfctl", "-s", "Anchors"});
    bool installed = false;
    std::size_t pos = 0;
    while (pos < hooks.output.size()) {
        auto end = hooks.output.find('\n', pos);
        if (end == std::string::npos) end = hooks.output.size();
        auto line = hooks.output.substr(pos, end - pos);
        const auto first = line.find_first_not_of(" \t\r");
        const auto last = line.find_last_not_of(" \t\r");
        if (first != std::string::npos && line.substr(first, last - first + 1) == "macCheck")
            installed = true;
        pos = end + 1;
    }
    if (hooks.status != 0 || !installed) {
        log.write("[PF] Active macCheck anchor missing. Install/load its main-ruleset hook first; no PF changes made.");
        return false;
    }
    auto status = run({"/sbin/pfctl", "-s", "info"});
    if (status.status != 0 || status.output.find("Status: Enabled") == std::string::npos) {
        log.write("[PF] PF is not enabled. Enable it explicitly with pfctl -E; no PF changes made.");
        return false;
    }
    // Secure temporary file in the destination directory; validate before rename.
    std::string pattern = "/etc/pf.anchors/.macCheck.XXXXXX";
    Fd temp(::mkstemp(pattern.data()));
    if (temp.value < 0) throw std::runtime_error("Cannot create PF temporary file");
    struct Cleanup {
        std::string path;
        ~Cleanup() { if (!path.empty()) ::unlink(path.c_str()); }
    } cleanup{pattern};
    if (fchmod(temp.value, 0600) != 0) throw std::runtime_error("Cannot protect PF file");
    std::size_t offset = 0;
    while (offset < pfRules.size()) {
        auto n = ::write(temp.value, pfRules.data() + offset, pfRules.size() - offset);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) throw std::runtime_error("Cannot write PF rules");
        offset += static_cast<std::size_t>(n);
    }
    if (::fsync(temp.value) != 0) throw std::runtime_error("Cannot sync PF rules");
    if (!command(log, {"/sbin/pfctl", "-n", "-a", "macCheck", "-f", pattern})) return false;
    const fs::path destination = "/etc/pf.anchors/macCheck";
    const fs::path backup = "/etc/pf.anchors/macCheck.before-mac-secure";
    if (fs::exists(destination)) {
        if (!fs::is_regular_file(fs::symlink_status(destination)))
            throw std::runtime_error("Existing PF anchor must be a regular file, not a symlink");
        // Keep the first backup; do not overwrite an earlier recovery copy.
        if (!fs::exists(backup)) fs::copy_file(destination, backup);
    }
    // Load the validated temporary rules; preserve the disk version if loading fails.
    if (!command(log, {"/sbin/pfctl", "-a", "macCheck", "-f", pattern})) return false;
    if (::rename(pattern.c_str(), destination.c_str()) != 0)
        throw std::runtime_error("PF rules loaded, but saving anchor failed; inspect live PF state");
    cleanup.path.clear();
    return command(log, {"/sbin/pfctl", "-a", "macCheck", "-sr"});
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

std::set<std::string> readWhitelist(const fs::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read whitelist: " + path.string());
    std::set<std::string> entries;
    for (std::string line; std::getline(file, line);) {
        line = trim(std::move(line));
        if (!line.empty() && line[0] != '#') entries.insert(std::move(line));
    }
    if (!file.eof()) throw std::runtime_error("Error reading whitelist");
    return entries;
}

using ProcessKey = std::pair<pid_t, std::uint64_t>;
bool audit(Logger& log, const fs::path& path, std::set<ProcessKey>& previous) {
    // Reload every pass. A missing/unreadable whitelist is an error, not "all safe".
    auto allowed = readWhitelist(path);
    std::vector<pid_t> pids;
    int bytes = proc_listpids(PROC_ALL_PIDS, 0, nullptr, 0);
    if (bytes <= 0) throw std::runtime_error("proc_listpids failed");
    // Grow and retry if the process table outgrew our snapshot buffer.
    for (;;) {
        pids.resize(static_cast<std::size_t>(bytes) / sizeof(pid_t) + 128);
        const auto capacity = pids.size() * sizeof(pid_t);
        if (capacity > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::runtime_error("Process table too large");
        bytes = proc_listpids(PROC_ALL_PIDS, 0, pids.data(), static_cast<int>(capacity));
        if (bytes <= 0) throw std::runtime_error("proc_listpids failed");
        if (static_cast<std::size_t>(bytes) < capacity) break;
    }
    pids.resize(static_cast<std::size_t>(bytes) / sizeof(pid_t));
    std::set<ProcessKey> current;
    unsigned unknown = 0, unavailable = 0;
    for (const auto pid : pids) {
        if (pid <= 0 || pid == getpid()) continue;
        proc_bsdinfo info{};
        if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) != sizeof(info)) {
            ++unavailable;
            continue; // Exited, or access denied. Do not infer that it is safe.
        }
        std::array<char, PROC_PIDPATHINFO_MAXSIZE> buffer{};
        const auto length = proc_pidpath(pid, buffer.data(), static_cast<uint32_t>(buffer.size()));
        if (length <= 0) { ++unavailable; continue; }
        std::string executable(buffer.data());
        const auto name = fs::path(executable).filename().string();
        // Exact whole-entry matching; no substring grep, command-line splitting,
        // or blanket interpreter/network-tool exemptions.
        if (allowed.contains(executable) || allowed.contains(name)) continue;
        ++unknown;
        const std::uint64_t started = info.pbi_start_tvsec * 1000000ULL + info.pbi_start_tvusec;
        const ProcessKey key{pid, started};
        current.insert(key);
        if (!previous.contains(key))
            log.write("[PROC] UNKNOWN pid=" + std::to_string(pid) +
                      " uid=" + std::to_string(info.pbi_uid) + " executable=" + executable);
    }
    previous = std::move(current);
    log.write("[PROC] Unknown=" + std::to_string(unknown) +
              "; unavailable/exited=" + std::to_string(unavailable) + "; no processes killed.");
    return true;
}
} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse(argc, argv);
        if (geteuid() != 0) throw std::runtime_error("Run using sudo (root required).");
        if (fcntl(STDIN_FILENO, F_GETFD) == -1 || fcntl(STDOUT_FILENO, F_GETFD) == -1 ||
            fcntl(STDERR_FILENO, F_GETFD) == -1)
            throw std::runtime_error("Standard file descriptors must be open");
        struct sigaction action{};
        action.sa_handler = onSignal;
        sigemptyset(&action.sa_mask);
        if (sigaction(SIGINT, &action, nullptr) != 0 || sigaction(SIGTERM, &action, nullptr) != 0)
            throw std::runtime_error("Cannot install signal handlers");
        Logger log(options.log);
        log.write("=== mac_secure started: INBOUND hardening and process AUDIT only ===");
        bool okay = true;
        if (options.configure && !configure(log)) okay = false;
        if (!stopping && options.pf && !configurePf(log)) okay = false;
        if (!stopping) {
            if (!command(log, {"/usr/sbin/lsof", "-nP", "-iTCP", "-sTCP:LISTEN"})) {
                // lsof also returns 1 when there are no matching sockets.
                log.write("[AUDIT] lsof found no matching sockets or failed; consult output above.");
            }
            command(log, {"/usr/sbin/lsof", "-nP", "-iUDP"});
        }
        std::set<ProcessKey> previous;
        do {
            if (stopping) break;
            try { audit(log, options.whitelist, previous); }
            catch (const std::exception& error) {
                log.write(std::string("[AUDIT ERROR] ") + error.what());
                okay = false;
            }
            if (options.once) break;
            const auto deadline = std::chrono::steady_clock::now() +
                                  std::chrono::seconds(options.interval);
            while (!stopping && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(100ms);
        } while (!stopping);
        log.write(okay ? "=== Finished without reported setup/audit errors ==="
                       : "=== Finished WITH errors; review the log ===");
        return okay ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}