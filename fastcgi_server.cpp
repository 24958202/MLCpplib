#include <fcgiapp.h>
#include <mysql/mysql.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <cstring>

// ── Database Configuration ──
constexpr const char* DB_HOST = "localhost";
constexpr const char* DB_USER = "cpp_user";
constexpr const char* DB_PASS = "STRONGPASSWORD";
constexpr const char* DB_NAME = "phonebot_db";
constexpr unsigned int DB_PORT = 3306;

// ── Configuration & Queue Definitions ──
struct Config {
    std::size_t num_acceptors  = 4;
    std::size_t num_workers    = std::thread::hardware_concurrency();
    std::size_t max_queue_size = 20'000;
};

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t max_size) : max_size_(max_size) {}
    bool push(T item, std::stop_token st) {
        std::unique_lock lock(mutex_);
        cv_not_full_.wait(lock, st, [this] { return queue_.size() < max_size_ || shutdown_; });
        if (shutdown_ || st.stop_requested()) return false;
        queue_.push(std::move(item));
        cv_not_empty_.notify_one();
        return true;
    }
    bool pop(T& item, std::stop_token st) {
        std::unique_lock lock(mutex_);
        cv_not_empty_.wait(lock, st, [this] { return !queue_.empty() || shutdown_; });
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one();
        return true;
    }
    void shutdown() {
        { std::lock_guard lock(mutex_); shutdown_ = true; }
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable_any cv_not_empty_;
    std::condition_variable_any cv_not_full_;
    std::size_t max_size_;
    bool shutdown_ = false;
};

struct FcgxRequest {
    FCGX_Request req{};
    FcgxRequest()  { FCGX_InitRequest(&req, 0, 0); }
    ~FcgxRequest() { FCGX_Free(&req, 1); }
    FcgxRequest(const FcgxRequest&) = delete;
    FcgxRequest& operator=(const FcgxRequest&) = delete;
    FcgxRequest(FcgxRequest&&) = default;
};

// ── Logging Utility ──
class Logger {
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
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::cerr << std::format("[{}] [{}] {}\n", 
            std::format("{:%H:%M:%S}", std::chrono::system_clock::to_time_t(now)),
            level,
            std::format(fmt, std::forward<Args>(args)...));
    }
};

// ── Helpers ──
std::string url_decode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            char hex[3] = { in[i+1], in[i+2], 0 };
            out.push_back(static_cast<char>(std::strtol(hex, nullptr, 16)));
            i += 2;
        } else if (in[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(in[i]);
        }
    }
    return out;
}

std::map<std::string, std::string> parse_form_data(const std::string& body) {
    std::map<std::string, std::string> params;
    size_t start = 0, end = 0;
    while (end != std::string::npos) {
        end = body.find('&', start);
        std::string pair = body.substr(start, end == std::string::npos ? std::string::npos : end - start);
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = url_decode(pair.substr(0, eq));
            std::string val = url_decode(pair.substr(eq + 1));
            params[key] = val;
        }
        start = end + 1;
    }
    return params;
}

// ── RAII MySQL Wrapper (Persistent & Auto-reconnect) ──
class MySQLDB {
public:
    MySQLDB() {
        connect();
    }
    
    ~MySQLDB() { 
        if (conn_) mysql_close(conn_); 
    }
    
    void connect() {
        if (conn_) mysql_close(conn_);
        conn_ = mysql_init(nullptr);
        
        bool reconnect = true;
        mysql_options(conn_, MYSQL_OPT_RECONNECT, &reconnect);
        
        if (!mysql_real_connect(conn_, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, nullptr, 0)) {
            last_error_ = mysql_error(conn_);
            Logger::error("MySQL connection failed: {}", last_error_);
        } else {
            last_error_ = "";
        }
    }

    MYSQL* get() { 
        if (conn_) {
            if (mysql_ping(conn_) != 0) {
                last_error_ = mysql_error(conn_);
                Logger::warn("MySQL ping failed, reconnecting: {}", last_error_);
                connect(); 
            }
        } else {
            connect();
        }
        return conn_; 
    }
    
    std::string error() const { return last_error_; }

private:
    MYSQL* conn_ = nullptr;
    std::string last_error_;
};

std::string execute_prepared(MYSQL* conn, const std::string& query, const std::vector<std::string>& params) {
    if (!conn) return "Database connection is not available.";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) return "Failed to initialize statement.";

    if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return "Prepare failed: " + err;
    }

    std::vector<MYSQL_BIND> binds(params.size());
    std::memset(binds.data(), 0, sizeof(MYSQL_BIND) * binds.size());

    for (size_t i = 0; i < params.size(); ++i) {
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = (char*)params[i].c_str();
        binds[i].buffer_length = static_cast<unsigned long>(params[i].size()); // Fixed size cast
    }

    if (!params.empty() && mysql_stmt_bind_param(stmt, binds.data()) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return "Bind failed: " + err;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        std::string err = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        return "Execution failed: " + err;
    }

    mysql_stmt_close(stmt);
    return "SUCCESS";
}

// ── Business Logic: User Manager Class ──
class UserManager {
public:
    explicit UserManager(MYSQL* conn) : db_conn_(conn) {}

    std::string add_user(const std::string& name, const std::string& email, const std::string& phone) {
        if (!db_conn_) return "No database connection.";
        
        std::string query = "REPLACE INTO user_profile (email, name, phone) VALUES (?, ?, ?);";
        std::string result = execute_prepared(db_conn_, query, {email, name, phone});
        
        if (result == "SUCCESS") return std::format("User {} successfully added/created.", email);
        return result;
    }

    std::string update_user(const std::string& name, const std::string& email, const std::string& phone) {
        if (!db_conn_) return "No database connection.";

        std::string query = "UPDATE user_profile SET name=?, phone=? WHERE email=?;";
        std::string result = execute_prepared(db_conn_, query, {name, phone, email});
        
        if (result == "SUCCESS") return std::format("User {} successfully updated.", email);
        return result;
    }

    std::string delete_user(const std::string& email) {
        if (!db_conn_) return "No database connection.";

        std::string query = "DELETE FROM user_profile WHERE email=?;";
        std::string result = execute_prepared(db_conn_, query, {email});
        
        if (result == "SUCCESS") return std::format("User {} successfully deleted.", email);
        return result;
    }

private:
    MYSQL* db_conn_;
};

// ── Request Processing ──
void process_request(FCGX_Request& req, MYSQL* db_conn) {
    const char* method = FCGX_GetParam("REQUEST_METHOD", req.envp);
    const char* clen_str = FCGX_GetParam("CONTENT_LENGTH", req.envp);
    
    std::string response_msg = "Invalid Request";
    constexpr int MAX_PAYLOAD_SIZE = 8192; // 8KB payload limit to prevent DoS OOM
    
    if (method && std::string(method) == "POST") {
        int clen = clen_str ? std::atoi(clen_str) : 0;
        
        if (clen > MAX_PAYLOAD_SIZE) {
            response_msg = "Error: Payload too large.";
        } else if (clen > 0) {
            std::string post_data(clen, '\0');
            FCGX_GetStr(post_data.data(), clen, req.in);
            
            auto params = parse_form_data(post_data);
            std::string action = params["action"];
            std::string email = params["email"];
            std::string name = params["name"];
            std::string phone = params["phone"];

            UserManager um(db_conn);
            if (action == "add_user") {
                response_msg = um.add_user(name, email, phone);
            } else if (action == "update_user") {
                response_msg = um.update_user(name, email, phone);
            } else if (action == "delete_user") {
                response_msg = um.delete_user(email);
            } else {
                response_msg = "Unknown action requested.";
            }
        } else {
            response_msg = "Empty POST body.";
        }
    }

    // Explicit Status code and CORS headers added here
    FCGX_FPrintF(req.out,
        "Status: 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
        "\r\n",
        response_msg.size()
    );
    FCGX_PutStr(response_msg.c_str(), static_cast<int>(response_msg.size()), req.out);
    
    FCGX_FFlush(req.out);
}

// ── Global Stop Source for Signal Handling ──
std::stop_source g_stop_source;

void signal_handler(int signal) {
    Logger::info("Received signal {} - initiating graceful shutdown", signal);
    g_stop_source.request_stop();
}

// ── Main Loop ──
int main() {
    const Config cfg;
    
    if (mysql_library_init(0, nullptr, nullptr) != 0) {
        Logger::error("Failed to initialize MySQL library");
        return 1;
    }

    if (FCGX_Init() != 0) {
        Logger::error("FCGX_Init() failed");
        return 1;
    }

    // Set up signal handlers for graceful shutdown
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);
    Logger::info("Signal handlers registered for SIGTERM and SIGINT");

    // Ensure table exists on startup using a temporary connection
    {
        MySQLDB startup_db;
        if (startup_db.get()) {
            const char* table_sql = "CREATE TABLE IF NOT EXISTS user_profile ("
                                    "email VARCHAR(255) PRIMARY KEY, "
                                    "name VARCHAR(255), "
                                    "phone VARCHAR(50));";
            mysql_query(startup_db.get(), table_sql);
            Logger::info("Database table verification complete");
        } else {
            Logger::warn("Could not connect to DB on startup to create table");
        }
    }

    BoundedQueue<std::unique_ptr<FcgxRequest>> queue(cfg.max_queue_size);
    
    // Atomic counter to track how many acceptors have exited
    std::atomic<int> acceptors_exited(0);
    
    Logger::info("Starting worker pool with {} workers and {} acceptors", 
        cfg.num_workers, cfg.num_acceptors);

    // ── Start Worker Threads ──
    std::vector<std::jthread> workers;
    workers.reserve(cfg.num_workers);

    for (std::size_t i = 0; i < cfg.num_workers; ++i) {
        workers.emplace_back([&queue, i](std::stop_token st) {
            mysql_thread_init(); // CRITICAL FIX: Initialize MySQL for this worker thread
            Logger::info("Worker {} started", i);
            
            MySQLDB thread_db; 
            
            int requests_processed = 0;
            while (true) {
                std::unique_ptr<FcgxRequest> item;
                if (!queue.pop(item, st)) break; 
                
                process_request(item->req, thread_db.get());
                FCGX_Finish_r(&item->req);
                requests_processed++;
            }
            
            Logger::info("Worker {} exiting after processing {} requests", i, requests_processed);
            mysql_thread_end(); // CRITICAL FIX: Cleanup MySQL thread resources
        });
    }

    // ── Start Acceptor Threads ──
    std::vector<std::jthread> acceptors;
    acceptors.reserve(cfg.num_acceptors);

    for (std::size_t i = 0; i < cfg.num_acceptors; ++i) {
        acceptors.emplace_back([&queue, &acceptors_exited, cfg, i](std::stop_token st) {
            Logger::info("Acceptor {} started", i);
            int requests_accepted = 0;
            
            while (!st.stop_requested()) {
                auto item = std::make_unique<FcgxRequest>();
                
                if (FCGX_Accept_r(&item->req) < 0) {
                    Logger::warn("Acceptor {}: FCGX_Accept_r failed", i);
                    break;
                }
                
                requests_accepted++;
                if (!queue.push(std::move(item), st)) {
                    Logger::warn("Acceptor {}: Failed to push request to queue (queue shutting down)", i);
                    break;
                }
            }
            
            // Only the last acceptor to exit should call queue.shutdown()
            int prev_count = acceptors_exited.fetch_add(1);
            if (prev_count + 1 == static_cast<int>(cfg.num_acceptors)) {
                Logger::info("Last acceptor {} exiting - signaling queue shutdown", i);
                queue.shutdown();
            } else {
                Logger::info("Acceptor {} exiting after accepting {} requests", i, requests_accepted);
            }
        });
    }

    Logger::info("All threads started. Waiting for shutdown signal...");

    // Main thread waits for stop signal
    {
        std::stop_callback callback(g_stop_source.get_token(), [&]() {
            Logger::info("Shutdown initiated - requesting stop tokens");
        });
        
        // Sleep until stop is requested
        std::unique_lock lock(std::mutex{});
        std::condition_variable_any cv;
        cv.wait(lock, g_stop_source.get_token());
    }

    Logger::info("Waiting for acceptors to finish accepting requests...");
    acceptors.clear();  // jthread destructor waits for completion
    
    Logger::info("Waiting for workers to finish processing queued requests...");
    workers.clear();    // jthread destructor waits for completion
    
    Logger::info("All threads joined successfully");
    
    mysql_library_end();
    Logger::info("MySQL library cleanup complete - exiting");
    
    return 0;
}
