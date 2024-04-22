#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <expect.h>

int main() {
    std::string processListFile = "process_list.txt"; // replace with your file name
    std::string sudoPwd = "your_sudo_password"; // replace with your sudo password

    std::vector<std::string> processList;
    {
        std::ifstream file(processListFile);
        if (!file) {
            std::cerr << "Failed to open process list file." << std::endl;
            return 1;
        }

        std::string line;
        while (std::getline(file, line)) {
            processList.push_back(line);
        }
    }

    while (true) {
        for (const auto& process : processList) {
            std::string cmd = "pidof " + process;
            int ret = system(cmd.c_str());
            if (ret != 0) {
                std::cout << "Process " << process << " is not running. Starting it..." << std::endl;
                cmd = "sudo " + process; // assume the process needs sudo privileges
                FILE *fp = popen(cmd.c_str(), "w"); // open a pipe to the command
                if (fp == nullptr) {
                    std::cerr << "Failed to open pipe." << std::endl;
                    continue;
                }

                // use libexpect to simulate keyboard input
                expect_t exp = expect_open(fp, EXPECT_FLAG_IGNORE_CASE);
                if (exp == nullptr) {
                    std::cerr << "Failed to open expect session." << std::endl;
                    pclose(fp);
                    continue;
                }

                expect_send(exp, sudoPwd.c_str()); // send the sudo password
                expect_send(exp, "\n"); // send a newline to confirm

                expect_close(exp);
                pclose(fp);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5)); // wait 5 seconds
    }

    return 0;
}
