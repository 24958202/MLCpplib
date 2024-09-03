// #include <iostream>  
// #include "SpeechRecognitionWrapper.h"  
// #include "TextToSpeechWrapper.h"  
// #include <fstream>  
// #include <string>  
// #include <vector>  
// #include <future>  
// // Function to split text into smaller chunks  

// void MacSpeck(const std::string& input_str) {  
//     TextToSpeechWrapper ttsWrapper;  
//     std::promise<void> speechCompleted;  
//     auto future = speechCompleted.get_future();  
//     // Define a callback function to be called when speech synthesis is complete  
//     auto onSpeechComplete = [&speechCompleted]() {  
//         std::cout << "Speech synthesis complete!" << std::endl;  
//         speechCompleted.set_value();  
//     };  
//     // Start the text-to-speech process  
//     ttsWrapper.speak(input_str, onSpeechComplete);  
//     // Wait for the speech to complete  
//     future.wait();  
// }  
// void SpeechRecog() {  
//     std::cout << "Welcome to the C++ Speech Recognition Simulation!" << std::endl;  
//     SpeechRecognitionWrapper recognizer;  
//     recognizer.startRecognition([](const std::string& str_response) {  
//         std::cout << "Speech recognition: " << str_response << std::endl;  
//     });  
// }  
// int main() {  
//     std::ifstream iFile("/Users/dengfengji/ronnieji/corpus/english_ebooks/pg696.txt");  
//     std::vector<std::string> rBook;
//     if (iFile.is_open()) {  
//         std::string line;  
//         std::string strBook;  
//         while (std::getline(iFile, line)){  
//             if(!line.empty()){
//                 rBook.push_back(line);
//             }
//         }  
//         if(!rBook.empty()){
//             for(const auto& rb : rBook){
//                 MacSpeck(rb);
//             }
//         }
//     }  
//     return 0;  
// }
/*
    clang++ -std=c++20 -c /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.mm -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.o -framework Foundation -framework AVFoundation -framework Speech  

    clang++ -std=c++20 -c /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech.cpp -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech.o

    g++ -std=c++20 /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech.cpp -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.a /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/TextToSpeechWrapper.a -framework Foundation -framework AVFoundation -framework Speech
*/
// #include <iostream>  
// #include "SpeechRecognitionWrapper.h"  
// #include "TextToSpeechWrapper.h"  
// #include <fstream>  
// #include <string>  
// #include <vector>  
// #include <future>  

// // Function to split text into smaller chunks  
// std::vector<std::string> splitText(const std::string& text, size_t maxChunkSize) {  
//     std::vector<std::string> chunks;  
//     size_t start = 0;  
//     while (start < text.size()) {  
//         size_t end = start + maxChunkSize;  
//         if (end >= text.size()) {  
//             end = text.size();  
//         } else {  
//             // Ensure we don't split in the middle of a word  
//             end = text.find_last_of(" ", end);  
//             if (end == std::string::npos || end <= start) {  
//                 end = start + maxChunkSize; // Force split if no space found  
//             }  
//         }  
//         chunks.push_back(text.substr(start, end - start));  
//         start = end + 1; // Move past the space  
//     }  
//     return chunks;  
// }  

// void MacSpeck(const std::string& input_str) {  
//     TextToSpeechWrapper ttsWrapper;  
//     auto promise = std::make_shared<std::promise<void>>();  
//     auto future = promise->get_future();  

//     // Define a callback function to be called when speech synthesis is complete  
//     auto onSpeechComplete = [promise]() {  
//         std::cout << "Speech synthesis complete!" << std::endl;  
//         promise->set_value();  
//     };  

//     // Start the text-to-speech process  
//     ttsWrapper.speak(input_str, onSpeechComplete);  

//     // Wait for the speech to complete  
//     future.wait();  
// }
// void MacSpeck(TextToSpeechWrapper& ttsWrapper, const std::string& input_str) {  
//     std::promise<void> speechCompleted;  
//     auto future = speechCompleted.get_future();  

//     // Define a callback function to be called when speech synthesis is complete  
//     auto onSpeechComplete = [&speechCompleted]() {  
//         std::cout << "Speech synthesis complete!" << std::endl;  
//         speechCompleted.set_value();  
//     };  

//     // Start the text-to-speech process  
//     ttsWrapper.speak(input_str, onSpeechComplete);  

//     // Wait for the speech to complete  
//     future.wait();  
// }  
// void SpeechRecog() {  
//     std::cout << "Welcome to the C++ Speech Recognition Simulation!" << std::endl;  
//     SpeechRecognitionWrapper recognizer;  
//     recognizer.startRecognition([](const std::string& str_response) {  
//         std::cout << "Speech recognition: " << str_response << std::endl;  
//     });  
// }  
//int main() {  
    // std::ifstream iFile("/Users/dengfengji/ronnieji/corpus/test/pg1.txt");  
    // if (iFile.is_open()) {  
    //     std::string line;  
    //     std::string strBook;  
    //     while (std::getline(iFile, line)) {  
    //         if (!line.empty()) {  
    //             strBook += line + " ";  
    //         }  
    //     }  

    //     // Split the book into smaller chunks  
    //     const size_t maxChunkSize = 1000; // Adjust this size as needed  
    //     std::vector<std::string> chunks = splitText(strBook, maxChunkSize);  

    //     // Speak each chunk  
    //     for (const auto& chunk : chunks) {  
    //         if (!chunk.empty()) {  
    //             std::cout << chunk << std::endl;
    //             MacSpeck(chunk);  
    //         }  
    //     }  
    // } else {  
    //     std::cerr << "Failed to open the file." << std::endl;  
    // }  
//}
#include <iostream>  
#include <vector>  
#include <future>  
#include <mutex>
#include <thread>
#include <chrono>
#include <fstream>
#include "SpeechRecognitionWrapper.h"  
#include "TextToSpeechWrapper.h"  

void MacSpeck(TextToSpeechWrapper& ttsWrapper, const std::string& input_str) {  
    std::promise<void> speechCompleted;  
    auto future = speechCompleted.get_future();  

    // Define a callback function to be called when speech synthesis is complete  
    auto onSpeechComplete = [&speechCompleted]() {  
        std::cout << "Speech synthesis complete!" << std::endl;  
        speechCompleted.set_value();  
    };  

    // Start the text-to-speech process  
    ttsWrapper.speak(input_str, onSpeechComplete);  

    // Wait for the speech to complete  
    future.wait();  
}  

int main() {  
    std::vector<std::string> Sentences{  
        "This is a book.",  
        "How are you?",  
        "That is a very beautiful dog."  
    };  
    std::vector<std::string> Books;
    std::ifstream iFile("/Users/dengfengji/ronnieji/corpus/test/pg1.txt");  
    if (iFile.is_open()) {  
        std::string line;  
        while (std::getline(iFile, line)) {  
            if (!line.empty()) {  
                Books.push_back(line);
            }  
        }  
    }
    std::vector<std::future<void>> futures;  
    std::cout << "Listening..." << std::endl;  
    TextToSpeechWrapper ttsWrapper;  
    for (const auto& st : Books) {  
        futures.emplace_back(std::async(std::launch::async, [&ttsWrapper, st]() {  
            MacSpeck(ttsWrapper, st);  
        }));  
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }  
    // Wait for all speech synthesis operations to complete  
    for (auto& future : futures) {  
        future.wait();  
    }  
    std::cout << "All speech synthesis complete." << std::endl;  
    std::system("pause>0");  
    return 0;  
}
