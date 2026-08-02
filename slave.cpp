#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    // 1. Read parameters
    if (argc < 3) {
        std::cerr << "Usage: slave <task_id> <parameters>\n";
        return 1;
    }

    std::string task_id = argv[1];
    std::string params = argv[2];

    // 2. Start the task (Simulate work)
    int progress = 0;
    while (progress < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        progress += 10;
    }

    // 3. Return results via file
    std::string filename = "result_" + task_id + ".txt";
    std::ofstream out(filename);

    if (!out.is_open()) {
        return 1; // Failed to write
    }

    // Simulating an error on task 5
    if (task_id == "5") {
        out << "ERROR: Internal computation failed for " << params;
        return 1; 
    } 

    out << "SUCCESS: Processed data [" << params << "] successfully.";
    return 0;
}