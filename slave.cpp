#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include "nlohmann/json.hpp" // Include nlohmann JSON

int main(int argc, char* argv[]) {
    std::string params = (argc > 1) ? argv[1] : "NoParams";

    // Simulate work
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 1. Create your complex C++ data structure
    std::map<std::string, std::vector<std::string>> compute_results = {
        {"ProcessedParams", {params}},
        {"Users", {"Alice", "Bob"}},
        {"Status", {"Active", "Completed"}},
        {"Warnings", {}} // Empty vector example
    };

    // 2. Convert the C++ map into a nlohmann JSON Object
    nlohmann::json json_payload;
    for (const auto& [key, vec] : compute_results) {
        nlohmann::json json_arr = nlohmann::json::array();
        for (const auto& item : vec) {
            json_arr.push_back(item); // Add items to the JSON array
        }
        json_payload[key] = json_arr;    // Attach array to the JSON object
    }
    //
    // 3. Serialize to string and send to stdout
    std::cout << json_payload.dump();

    return 0;
}