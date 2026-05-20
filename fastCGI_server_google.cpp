/**
 * High-Performance FastCGI Server — C++20
 *
 * Architecture:
 *   [N Acceptor Threads]  →  [Bounded Queue]  →  [M Worker Threads]
 *
 * - Acceptor threads call FCGX_Accept_r() concurrently (thread-safe).
 * - The bounded queue acts as a backpressure valve; if workers are
 *   overwhelmed, acceptors block instead of unboundedly allocating memory.
 * - Worker threads process requests and write responses.
 *
 * Build:
 *   g++ -std=c++20 -O2 -o fastcgi_app fastcgi_server.cpp -lfcgi -lpthread
 *
 * Nginx config snippet:
 *   location / {
 *       fastcgi_pass unix:/tmp/fastcgi.sock;
 *       include fastcgi_params;
 *   }
 */

#include <fcgiapp.h>       // FCGX_* thread-safe API

#include <atomic>
#include <condition_variable>
#include <format>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>      // C++20
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

struct Config {
    std::size_t num_acceptors  = 4;
    std::size_t num_workers    = std::thread::hardware_concurrency();
    std::size_t max_queue_size = 20'000;
};

// ─────────────────────────────────────────────────────────────────────────────
// Bounded Thread-Safe Queue (C++20: stop_token-aware)
// ─────────────────────────────────────────────────────────────────────────────

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t max_size) : max_size_(max_size) {}

    bool push(T item, std::stop_token st) {
        std::unique_lock lock(mutex_);
        cv_not_full_.wait(lock, st, [this] {
            return queue_.size() < max_size_ || shutdown_;
        });
        if (shutdown_ || st.stop_requested()) return false;
        queue_.push(std::move(item));
        cv_not_empty_.notify_one();
        return true;
    }

    bool pop(T& item, std::stop_token st) {
        std::unique_lock lock(mutex_);
        cv_not_empty_.wait(lock, st, [this] {
            return !queue_.empty() || shutdown_;
        });
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one();
        return true;
    }

    void shutdown() {
        {
            std::lock_guard lock(mutex_);
            shutdown_ = true;
        }
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }

private:
    std::queue<T>                   queue_;
    mutable std::mutex              mutex_;
    std::condition_variable_any     cv_not_empty_; 
    std::condition_variable_any     cv_not_full_;
    std::size_t                     max_size_;
    bool                            shutdown_ = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// RAII Wrapper for FCGX_Request
// ─────────────────────────────────────────────────────────────────────────────

struct FcgxRequest {
    FCGX_Request req{};

    FcgxRequest()  { FCGX_InitRequest(&req, 0, 0); }
    ~FcgxRequest() { FCGX_Free(&req, /*close_fd=*/1); }

    FcgxRequest(const FcgxRequest&)            = delete;
    FcgxRequest& operator=(const FcgxRequest&) = delete;
    FcgxRequest(FcgxRequest&&)                 = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

[[nodiscard]]
std::string make_request_id() {
    static std::atomic<std::uint64_t> counter{0};
    return std::format("REQ-{:016X}",
                       counter.fetch_add(1, std::memory_order_relaxed));
}

// Reads the raw POST body from the FastCGI input stream
[[nodiscard]]
std::string read_post_body(FCGX_Request& req) {
    const char* content_length_str = FCGX_GetParam("CONTENT_LENGTH", req.envp);
    if (!content_length_str) return {};

    std::size_t content_length = 0;
    try {
        content_length = std::stoull(content_length_str);
    } catch (...) {
        return {}; // Invalid content length
    }

    if (content_length == 0) return {};

    std::string body;
    body.resize(content_length);
    int bytes_read = FCGX_GetStr(body.data(), static_cast<int>(content_length), req.in);
    body.resize(bytes_read);
    
    return body;
}

// ─────────────────────────────────────────────────────────────────────────────
// Request Processing Logic
// ─────────────────────────────────────────────────────────────────────────────

void process_request(FCGX_Request& req) {
    // ── 1. Parse inputs ──────────────────────────────────────────────────────

    // Read the custom header sent by the cURL client
    const char* key_env = FCGX_GetParam("HTTP_X_API_KEY", req.envp);
    std::string key = key_env ? key_env : "MISSING_KEY";

    // Read the raw request string from the POST body
    std::string request_string = read_post_body(req);
    const std::string request_id = make_request_id();

    // ── 2. Build response body (No HTML, raw text only) ──────────────────────

    // You can replace this format string with your actual backend logic.
    const std::string body = std::format(
        "Processed successfully.\n"
        "ID: {}\n"
        "Key received: {}\n"
        "Request string length: {} bytes\n"
        "Request string content: {}\n",
        request_id, key, request_string.size(), request_string
    );

    // ── 3. Write HTTP response headers + body ────────────────────────────────

    FCGX_FPrintF(req.out,
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "X-Request-ID: %s\r\n"
        "\r\n",                             // blank line ends headers
        body.size(), request_id.c_str()
    );

    FCGX_PutStr(body.data(), static_cast<int>(body.size()), req.out);
}

// ─────────────────────────────────────────────────────────────────────────────
// Telemetry
// ─────────────────────────────────────────────────────────────────────────────

void maybe_log_stats(std::atomic<std::uint64_t>& counter) {
    constexpr std::uint64_t LOG_INTERVAL = 10'000;
    auto current = counter.fetch_add(1, std::memory_order_relaxed) + 1;
    if (current % LOG_INTERVAL == 0) {
        std::clog << std::format("[fastcgi] Processed {:L} requests\n", current);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    const Config cfg;

    if (FCGX_Init() != 0) {
        std::cerr << "FCGX_Init() failed\n";
        return 1;
    }

    BoundedQueue<std::unique_ptr<FcgxRequest>> queue(cfg.max_queue_size);
    std::atomic<std::uint64_t> total_processed{0};

    // ── Worker threads ───────────────────────────────────────────────────────
    std::vector<std::jthread> workers;
    workers.reserve(cfg.num_workers);

    for (std::size_t i = 0; i < cfg.num_workers; ++i) {
        workers.emplace_back([&](std::stop_token st) {
            while (true) {
                std::unique_ptr<FcgxRequest> item;
                if (!queue.pop(item, st)) break; 

                process_request(item->req);
                FCGX_Finish_r(&item->req);

                maybe_log_stats(total_processed);
            }
        });
    }

    // ── Acceptor threads ─────────────────────────────────────────────────────
    std::vector<std::jthread> acceptors;
    acceptors.reserve(cfg.num_acceptors);

    for (std::size_t i = 0; i < cfg.num_acceptors; ++i) {
        acceptors.emplace_back([&](std::stop_token st) {
            while (!st.stop_requested()) {
                auto item = std::make_unique<FcgxRequest>();

                if (FCGX_Accept_r(&item->req) < 0) {
                    break;
                }
                if (!queue.push(std::move(item), st)) break;
            }
            queue.shutdown();
        });
    }

    acceptors.clear();
    workers.clear();

    std::clog << std::format("[fastcgi] Exiting. Total requests served: {}\n",
                             total_processed.load());
    return 0;
}