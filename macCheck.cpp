// mac_secure.cpp -- C++23, macOS only.
// This is inbound hardening + process AUDITING, not an outbound app firewall.
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
#include <format>
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
#include <pwd.h>
#include <spawn.h>
#include <sys/event.h>
#include <sys/stat.h>
#include <sys/time.h>
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
        const auto line = std::format("[{}] {}\n", timestamp(), message);
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
// Uses native kqueue to sleep the thread without polling overhead.
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

    Fd kq(kqueue());
    if (kq.value < 0) {
        ::kill(pid, SIGKILL);
        while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {}
        throw std::runtime_error("kqueue failed");
    }

    std::array<struct kevent, 2> changes{};
    EV_SET(&changes[0], input.value, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    EV_SET(&changes[1], pid, EVFILT_PROC, EV_ADD | EV_CLEAR, NOTE_EXIT, 0, nullptr);

    if (kevent(kq.value, changes.data(), 2, nullptr, 0, nullptr) == -1) {
        ::kill(pid, SIGKILL);
        while (waitpid(pid, nullptr, 0) < 0 && errno == EINTR) {}
        throw std::runtime_error("kevent registration failed");
    }

    std::string text;
    std::array<char, 4096> buffer{};
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    bool exited = false, eof = false, interrupted = false;

    while (!exited || !eof) {
        if (stopping) {
            interrupted = true;
            break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            interrupted = true;
            break;
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now);
        struct timespec ts{};
        ts.tv_sec = static_cast<time_t>(remaining.count() / 1000);
        ts.tv_nsec = static_cast<long>((remaining.count() % 1000) * 1000000);

        std::array<struct kevent, 2> events{};
        const int nev = kevent(kq.value, nullptr, 0, events.data(), static_cast<int>(events.size()), &ts);

        if (nev < 0) {
            if (errno == EINTR) continue;
            break; // Unexpected error, treat as interrupted/failed
        }
        if (nev == 0) {
            interrupted = true; // Timeout reached
            break;
        }

        for (int i = 0; i < nev; ++i) {
            if (events[i].filter == EVFILT_READ) {
                while (true) {
                    const auto n = ::read(input.value, buffer.data(), buffer.size());
                    if (n > 0) {
                        constexpr std::size_t limit = 1024 * 1024;
                        if (text.size() < limit)
                            text.append(buffer.data(), std::min(static_cast<std::size_t>(n), limit - text.size()));
                    } else if (n == 0) {
                        eof = true;
                        break;
                    } else if (errno == EINTR) {
                        continue;
                    } else { // EAGAIN or error
                        break;
                    }
                }
                if (events[i].flags & EV_EOF) eof = true;
            }
            if (events[i].filter == EVFILT_PROC && (events[i].fflags & NOTE_EXIT)) {
                exited = true;
                while (waitpid(pid, &status, WNOHANG) < 0 && errno == EINTR) {}
            }
        }
    }

    if (interrupted || (!exited && eof)) {
        ::kill(pid, SIGKILL);
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
        if (interrupted) {
            return {124, text + "\nCommand timed out or was interrupted"};
        }
    }

    if (WIFEXITED(status)) return {WEXITSTATUS(status), text};
    return {128 + (WIFSIGNALED(status) ? WTERMSIG(status) : 0), text};
}

bool command(Logger& log, std::vector<std::string> args) {
    std::string description;
    for (const auto& arg : args) description += arg + " ";
    log.write(std::format("[CMD] {}", description));
    auto result = run(std::move(args));
    if (!result.output.empty()) log.write(result.output);
    if (result.status != 0) log.write(std::format("[ERROR] Exit status {}", result.status));
    return result.status == 0;
}

struct Options {
    bool configure = false, pf = false, once = false;
    unsigned interval = 5;
    fs::path whitelist = "process_whitelist.txt";
    fs::path log = "/var/log/mac_security.log";
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
                         "Requires a preinstalled, active mac_security PF hook and enabled PF.\n";
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
             // Qiqi.app (Chinese app name). Kept as UTF-8 escapes so that this
             // source file stays pure ASCII; the runtime path is unchanged.
             "/Applications/Muse.app",
             "/Applications/\xE4\xB8\x83\xE4\xB8\x83.app"}) {
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
        if (first != std::string::npos && line.substr(first, last - first + 1) == "mac_security")
            installed = true;
        pos = end + 1;
    }
    if (hooks.status != 0 || !installed) {
        log.write("[PF] Active mac_security anchor missing. Install/load its main-ruleset hook first; no PF changes made.");
        return false;
    }
    auto status = run({"/sbin/pfctl", "-s", "info"});
    if (status.status != 0 || status.output.find("Status: Enabled") == std::string::npos) {
        log.write("[PF] PF is not enabled. Enable it explicitly with pfctl -E; no PF changes made.");
        return false;
    }
    // Secure temporary file in the destination directory; validate before rename.
    std::string pattern = "/etc/pf.anchors/.mac_security.XXXXXX";
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
    if (!command(log, {"/sbin/pfctl", "-n", "-a", "mac_security", "-f", pattern})) return false;
    const fs::path destination = "/etc/pf.anchors/mac_security";
    const fs::path backup = "/etc/pf.anchors/mac_security.before-mac-secure";
    if (fs::exists(destination)) {
        if (!fs::is_regular_file(fs::symlink_status(destination)))
            throw std::runtime_error("Existing PF anchor must be a regular file, not a symlink");
        // Keep the first backup; do not overwrite an earlier recovery copy.
        if (!fs::exists(backup)) fs::copy_file(destination, backup);
    }
    // Load the validated temporary rules; preserve the disk version if loading fails.
    if (!command(log, {"/sbin/pfctl", "-a", "mac_security", "-f", pattern})) return false;
    if (::rename(pattern.c_str(), destination.c_str()) != 0)
        throw std::runtime_error("PF rules loaded, but saving anchor failed; inspect live PF state");
    cleanup.path.clear();
    return command(log, {"/sbin/pfctl", "-a", "mac_security", "-sr"});
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}



// ---------------------------------------------------------------------------
// Whitelist matching policy -- MUST stay in sync with macbook_protect.sh v4.
//
// Evaluation order (mirrors the shell enforcer's awk stage):
//   1. SystemPath    Apple system paths are always allowed.
//   2. TransientTool name matches the transient-tool table AND the executable
//                    lives inside a trusted toolchain directory.
//   3. ExactPath     whitelist entry equals the full executable path.
//   4. ExactName     whitelist entry equals the basename.
//   5. DirPrefix     whitelist entry ends with '/': any executable under that
//                    directory is allowed (covers every helper inside a .app).
//   6. None          no rule matched -> reported as UNKNOWN.
// ---------------------------------------------------------------------------

enum class Rule { SystemPath, TransientTool, ExactPath, ExactName, DirPrefix, None };

[[maybe_unused]] constexpr std::string_view rule_text(Rule rule) {
    switch (rule) {
        case Rule::SystemPath:    return "system path";
        case Rule::TransientTool: return "transient tool inside trusted toolchain";
        case Rule::ExactPath:     return "whitelist exact match (full path)";
        case Rule::ExactName:     return "whitelist exact match (basename)";
        case Rule::DirPrefix:     return "directory prefix match";
        case Rule::None:          return "no match";
    }
    return "unknown";
}

constexpr std::string_view kSystemPrefixes[]{
    "/System/", "/usr/libexec/", "/usr/sbin/", "/usr/bin/", "/sbin/", "/bin/",
    "/Library/Apple/", "/System/Cryptexes/", "/System/Volumes/Preboot/"};

constexpr std::string_view kTransientNames[]{
    "awk", "ps", "sed", "sort", "tail", "grep", "sleep", "cat", "head", "wc",
    "bash", "sh", "zsh", "cut", "tr", "find", "xargs", "date", "tee", "kill",
    "pkill", "launchctl", "sudo", "man", "less", "more", "echo", "printf", "read",
    "true", "false", "test", "expr", "bc", "ssh", "scp", "rsync", "curl", "wget",
    "git", "npm", "node", "python3", "python", "ruby", "perl", "lua", "env",
    "which", "open", "osascript", "lsof", "netstat", "pfctl", "systemsetup",
    "launchd", "mdfind", "plutil", "ditto", "mktemp", "dirname", "basename",
    "uname", "hostname"};

// Fixes the sudo execution trap: identifies actual developer's home
// rather than defaulting to /var/root which kills local toolchains.
std::string derive_user_home(const fs::path& whitelist) {
    if (const char* sudoUser = std::getenv("SUDO_USER"); sudoUser != nullptr && *sudoUser != '\0') {
        if (const auto* pw = ::getpwnam(sudoUser); pw != nullptr && pw->pw_dir != nullptr) {
            return pw->pw_dir;
        }
    }
    struct stat st{};
    if (::stat(whitelist.c_str(), &st) == 0) {
        if (const auto* pw = ::getpwuid(st.st_uid); pw != nullptr && pw->pw_dir != nullptr && *pw->pw_dir != '\0')
            return pw->pw_dir;
    }
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') return home;
    return {};
}

std::vector<std::string> build_toolchains(const std::string& user_home) {
    std::vector<std::string> dirs{"/opt/homebrew/", "/usr/local/", "/Library/Developer/"};
    std::error_code ec;
    if (!user_home.empty()) {
        for (const char* relative : {".nvm", ".pyenv", ".cargo", ".local", ".workbuddy",
                                     "miniconda3", "anaconda3"}) {
            const std::string dir = user_home + "/" + relative;
            if (fs::is_directory(dir, ec)) dirs.push_back(dir + "/");
        }
    }
    for (const char* absolute : {"/opt/anaconda3", "/opt/local"})
        if (fs::is_directory(absolute, ec)) dirs.emplace_back(std::string(absolute) + "/");
    return dirs;
}

struct Whitelist {
    std::set<std::string> exact;         // full paths and bare basenames
    std::vector<std::string> prefixes;   // entries ending with '/': directory prefixes
    std::vector<std::string> toolchain;  // trusted toolchain directories
};

Whitelist load_whitelist(const fs::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read whitelist: " + path.string());
    Whitelist list;
    for (std::string line; std::getline(file, line);) {
        line = trim(std::move(line));
        if (line.empty() || line[0] == '#') continue;
        if (line.back() == '/') list.prefixes.push_back(std::move(line));
        else list.exact.insert(std::move(line));
    }
    if (!file.eof()) throw std::runtime_error("Error reading whitelist");
    list.toolchain = build_toolchains(derive_user_home(path));
    return list;
}

Rule classify(const Whitelist& list, const std::string& executable) {
    for (const auto prefix : kSystemPrefixes)
        if (executable.starts_with(prefix)) return Rule::SystemPath;

    const auto name = fs::path(executable).filename().string();
    bool transient = false;
    for (const auto candidate : kTransientNames)
        if (name == candidate) { transient = true; break; }

    // A matching name is not enough: the executable must also live inside a
    // trusted toolchain directory, otherwise renaming malware defeats the check.
    if (transient)
        for (const auto& dir : list.toolchain)
            if (executable.starts_with(dir)) return Rule::TransientTool;

    if (list.exact.contains(executable)) return Rule::ExactPath;
    if (list.exact.contains(name)) return Rule::ExactName;
    for (const auto& prefix : list.prefixes)
        if (executable.starts_with(prefix)) return Rule::DirPrefix;
    return Rule::None;
}

using ProcessKey = std::pair<pid_t, std::uint64_t>;
bool audit(Logger& log, const fs::path& path, std::set<ProcessKey>& previous) {
    // Reload every pass. A missing/unreadable whitelist is an error, not "all safe".
    const auto list = load_whitelist(path);
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
        
        // Pass the explicit length to bypass O(N) strlen inside string constructor.
        std::string executable(buffer.data(), length);
        
        // Decision goes through classify(): system path / toolchain transient /
        // exact path / exact basename / directory prefix. 
        if (classify(list, executable) != Rule::None) continue;
        
        ++unknown;
        const std::uint64_t started = info.pbi_start_tvsec * 1000000ULL + info.pbi_start_tvusec;
        const ProcessKey key{pid, started};
        current.insert(key);
        
        if (!previous.contains(key)) {
            log.write(std::format("[PROC] UNKNOWN pid={} uid={} executable={}", pid, info.pbi_uid, executable));
        }
    }
    previous = std::move(current);
    log.write(std::format("[PROC] Unknown={}; unavailable/exited={}; no processes killed.", unknown, unavailable));
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
                log.write("[AUDIT] lsof found no matching sockets or failed; consult output above.");
            }
            command(log, {"/usr/sbin/lsof", "-nP", "-iUDP"});
        }
        
        std::set<ProcessKey> previous;
        do {
            if (stopping) break;
            try { audit(log, options.whitelist, previous); }
            catch (const std::exception& error) {
                log.write(std::format("[AUDIT ERROR] {}", error.what()));
                okay = false;
            }
            if (options.once) break;
            
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(options.interval);
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