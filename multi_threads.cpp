
/*
    multi-thread 1
*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>

void searchInLines(const std::string& filename, const std::string& searchTerm, int startLine, int endLine) {
    std::ifstream inputFile(filename);
    std::string line;
    int lineNum = 0;

    while (std::getline(inputFile, line) && lineNum <= endLine) {
        if (lineNum >= startLine && line.find(searchTerm) != std::string::npos) {
            std::cout << "Search term found in line " << lineNum << ": " << line << std::endl;
        }
        ++lineNum;
    }

    inputFile.close();
}

int main() {
    const std::string filename = "large_file.txt";
    const std::string searchTerm = "search_term";
    const int numThreads = 4; // Number of threads to use

    std::ifstream inputFile(filename);
    int totalLines = std::count(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>(), '\n');
    inputFile.close();

    int linesPerThread = totalLines / numThreads;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        int startLine = i * linesPerThread;
        int endLine = (i == numThreads - 1) ? totalLines : (i + 1) * linesPerThread - 1;
        threads.emplace_back(searchInLines, filename, searchTerm, startLine, endLine);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}

/*
    multi-thread 2
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
std::queue<std::string> linesQueue;
bool finished = false;

void producer(const std::string& filename) {
    std::ifstream inputFile(filename);
    std::string line;

    while (std::getline(inputFile, line)) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            linesQueue.push(line);
        }
        cv.notify_one();
    }

    finished = true;
    cv.notify_all();
}

void consumer(const std::string& searchTerm) {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return !linesQueue.empty() || finished; });

        if (linesQueue.empty() && finished) {
            break;
        }

        std::string line = linesQueue.front();
        linesQueue.pop();
        lock.unlock();

        if (line.find(searchTerm) != std::string::npos) {
            std::cout << "Search term found: " << line << std::endl;
        }
    }
}

int main() {
    const std::string filename = "large_file.txt";
    const std::string searchTerm = "search_term";
    const int numThreads = std::thread::hardware_concurrency(); // Use the number of available cores

    std::vector<std::thread> threads;
    threads.emplace_back(producer, filename);

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(consumer, searchTerm);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return 0;
}
/*
    multi-thread 3
*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

std::atomic<bool> foundResult(false);

void searchInFile(const std::string& filename, const std::string& searchTerm) {
    std::ifstream inputFile(filename);
    std::string line;

    while (std::getline(inputFile, line)) {
        if (line.find(searchTerm) != std::string::npos) {
            std::cout << "Search term found in file " << filename << ": " << line << std::endl;
            foundResult = true;
            break;
        }
    }

    inputFile.close();
}

int main() {
    const std::vector<std::string> files = {"file1.txt", "file2.txt", "file3.txt", "file4.txt", "file5.txt",
                                            "file6.txt", "file7.txt", "file8.txt", "file9.txt", "file10.txt"};
    const std::string searchTerm = "search_term";
    const int numThreads = files.size();

    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(searchInFile, files[i], searchTerm);
    }

    for (auto& thread : threads) {
        if (foundResult) {
            break; // Terminate all threads if result is found
        }
        thread.join();
    }

    return 0;
}
