#include <curl/curl.h>
#include <iostream>
#include <string>
#include <string_view>
#include <format>
#include <stdexcept>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
// Global libcurl Initialization (RAII)
// ─────────────────────────────────────────────────────────────────────────────

struct CurlGlobalState {
    CurlGlobalState() {
        // Must be called exactly once per program execution
        if (curl_global_init(CURL_GLOBAL_ALL) != 0) {
            throw std::runtime_error("Failed to initialize libcurl");
        }
    }
    ~CurlGlobalState() {
        curl_global_cleanup();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Highly Efficient Reusable Client
// ─────────────────────────────────────────────────────────────────────────────

class FastCgiClient {
public:
    FastCgiClient(std::string target_url) : url_(std::move(target_url)) {
        curl_ = curl_easy_init();
        if (!curl_) {
            throw std::runtime_error("Failed to create CURL handle");
        }

        // Set persistent options (these don't change between requests)
        curl_easy_setopt(curl_, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        
        // Timeout configurations to prevent hanging
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 10L);       // 10 seconds max total
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5L); // 5 seconds max connection phase
    }

    ~FastCgiClient() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
    }

    // Disable copying (CURL* owns a socket)
    FastCgiClient(const FastCgiClient&) = delete;
    FastCgiClient& operator=(const FastCgiClient&) = delete;

    // ─────────────────────────────────────────────────────────────────────────
    // Core Request Execution
    // ─────────────────────────────────────────────────────────────────────────
    
    [[nodiscard]] 
    std::string send_request(const std::string& key, const std::string& request_payload) {
        std::string response_buffer;
        
        // 1. Tell curl where to write the data
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_buffer);

        // 2. Set the POST body (your 'request string')
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_payload.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, request_payload.length());

        // 3. Inject the Key as a custom header (e.g., X-Api-Key)
        // Alternatively, if matching your server code exactly, use: 
        // std::string cookie_header = std::format("Cookie: user_id={}", key);
        std::string auth_header = std::format("X-Api-Key: {}", key);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, auth_header.c_str());
        // Tell the server we are just sending raw text
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

        // 4. Perform the request
        CURLcode res = curl_easy_perform(curl_);

        // 5. Cleanup headers for this specific request
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            throw std::runtime_error(std::format("curl_easy_perform() failed: {}", 
                                                 curl_easy_strerror(res)));
        }

        return response_buffer; // Return the raw string without HTML
    }

private:
    CURL* curl_ = nullptr;
    std::string url_;

    // libcurl callback to append received data into our std::string
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t realsize = size * nmemb;
        auto* str = static_cast<std::string*>(userp);
        str->append(static_cast<char*>(contents), realsize);
        return realsize;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Main Example
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    // 1. Initialize libcurl globally once
    CurlGlobalState global_curl;

    try {
        // 2. Instantiate the client ONCE. 
        // Point this to the Nginx/Apache endpoint that proxies to your FastCGI C++ server.
        FastCgiClient client("http://127.0.0.1/api/process");

        // 3. Fire multiple requests reusing the SAME handle.
        // This is where you get maximum efficiency (TCP connection is kept alive).
        for (int i = 1; i <= 10; ++i) {
            std::string key = std::format("SECRET_KEY_{}", i);
            std::string payload = std::format("This is payload data block #{}", i);
            
            std::string response = client.send_request(key, payload);
            
            std::cout << std::format("Response {}: {}\n", i, response);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}