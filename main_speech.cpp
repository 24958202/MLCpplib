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


#include <iostream>  
#include <vector>  
#include <future>  
#include <mutex>
#include <thread>
#include <chrono>
#include <fstream>
#include "SpeechRecognitionWrapper.h"  
#include "TextToSpeechWrapper.h"  

int estimateDuration(const std::string& text) {  
    // Estimate duration based on the number of words and a speaking rate  
    int words = std::count(text.begin(), text.end(), ' ') + 1; // Count spaces to estimate words  
    int speakingRate = 130; // Average speaking rate in words per minute  
    int durationInSeconds = (words * 60) / speakingRate; // Convert to seconds  
    return durationInSeconds;  
}  
int main() {  
    std::vector<std::string> Sentences{  
        "This is a book.",  
        "How are you?",  
        "That is a very beautiful dog."  
    }; 
    std::vector<std::string> strBooks; 
    std::ifstream iFile("/Users/dengfengji/ronnieji/corpus/test/pg1.txt");  
    if (iFile.is_open()) {  
        std::string line;  
        std::string strBook;  
        while (std::getline(iFile, line)) {  
            if (!line.empty()) {  
                strBooks.push_back(line);
            }  
        }  
    }
    TextToSpeechWrapper ttsWrapper;  
    // Define the callback for when speech is complete  
    auto onSpeechComplete = []() {  
        std::cout << "Callback invoked." << std::endl;  
    };  
    for (const auto& item : Sentences) {
        // Estimate duration and wait  
        int duration = estimateDuration(item);      
        ttsWrapper.speak(item, onSpeechComplete);
        std::this_thread::sleep_for(std::chrono::seconds(duration));
    }  

    std::cout << "All speech synthesis complete." << std::endl;  
    std::system("pause>0");  
    return 0;  
}
