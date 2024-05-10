/*
     grep :
     To find a line with content in a very large *.txt or *.binary file
*/
#include <iostream>
#include <cstdlib>  // For popen and pclose
#include <string>
#include <cstdio>   // For FILE, fgets, etc.

int main() {
    std::string bookToFind = "2834.txt.utf-8";  // Can be a regex expression
    std::string filePath = "/home/ronnieji/lib/db_tools/booklist.txt";

    // Prepare the command
    std::string command = "grep \"" + bookToFind + "\" " + filePath;

    // Execute the command
    int bookFound = std::system(command.c_str());

    // Check result
    if (bookFound == 0) {
        std::cout << "Book found!." << std::endl;
    } else {
        std::cout << "Book not found or error occurred." << std::endl;
    }

    // Execute the command and capture the output
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error opening pipe!" << std::endl;
        return 1;
    }

    char buffer[128];
    std::string result;

    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }

    pclose(pipe);

    // Print the result
    std::cout << "Result: " << result << std::endl;

    return 0;
}
