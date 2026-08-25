#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <boost/json.hpp> // Include Boost JSON

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

    // 2. Convert the C++ map into a Boost JSON Object
    boost::json::object json_payload;
    for (const auto& [key, vec] : compute_results) {
        boost::json::array json_arr;
        for (const auto& item : vec) {
            json_arr.emplace_back(item); // Add items to the JSON array
        }
        json_payload[key] = json_arr;    // Attach array to the JSON object
    }
    //
    // 3. Serialize to string and send to stdout
    std::cout << boost::json::serialize(json_payload);

    return 0;
}