#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

#ifdef _WIN32
#define PYTHON_COMMAND "python"
#else
#define PYTHON_COMMAND "python3"
#endif

std::string shell_quote(const std::string& value)
{
#ifdef _WIN32
    std::string result = "\"";
    for (char c : value) {
        if (c == '"') {
            result += "\\\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
#else
    std::string result = "'";
    for (char c : value) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }
    result += "'";
    return result;
#endif
}

int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4) {
        std::cerr
            << "Usage: " << argv[0]
            << " <input_image> <output_prefix> [model_directory]\n\n"
            << "Example:\n"
            << "  " << argv[0]
            << " image.jpg result ./mask2former-swin-tiny-coco-instance\n";
        return EXIT_FAILURE;
    }

    const fs::path input_image = fs::absolute(argv[1]);
    const fs::path output_prefix = fs::absolute(argv[2]);
    const fs::path model_directory =
        argc == 4 ? fs::absolute(argv[3]) : fs::current_path();
    const fs::path python_script = fs::absolute("/Users/jidengfeng/Downloads/MLCpplib/seg/segment_mask2former.py");

    if (!fs::exists(input_image)) {
        std::cerr << "Input image does not exist: " << input_image << '\n';
        return EXIT_FAILURE;
    }

    if (!fs::exists(model_directory / "config.json")) {
        std::cerr << "Model directory does not contain config.json: "
                  << model_directory << '\n';
        return EXIT_FAILURE;
    }

    if (!fs::exists(python_script)) {
        std::cerr << "Missing Python script: " << python_script << '\n';
        return EXIT_FAILURE;
    }

    std::ostringstream command;
    command << PYTHON_COMMAND << ' '
            << shell_quote(python_script.string()) << ' '
            << shell_quote(input_image.string()) << ' '
            << shell_quote(output_prefix.string()) << ' '
            << shell_quote(model_directory.string());

    std::cout << "Running Mask2Former instance segmentation...\n";
    const int exit_code = std::system(command.str().c_str());

    if (exit_code != 0) {
        std::cerr << "Segmentation process failed with exit code "
                  << exit_code << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}