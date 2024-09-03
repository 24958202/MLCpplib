// main.cpp  
#include <iostream>  
#include "SpeechRecognitionWrapper.h"  
int main() {  
    std::cout << "Welcome to the C++ Speech Recognition Simulation!" << std::endl;  
    std::string str_response;
    SpeechRecognitionWrapper recognizer;  
    recognizer.startRecognition([](const std::string& str_response){
        std::cout << "Speech recognition: " << str_response << std::endl;
        return;
    });
    return 0;  
}
/*
    clang++ -std=c++20 -c /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.mm -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.o -framework Foundation -framework AVFoundation -framework Speech  

    clang++ -std=c++20 -c /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech.cpp -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech.o

    clang++ -std=c++20 /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech.o -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech -framework Foundation -framework AVFoundation -framework Speech
*/