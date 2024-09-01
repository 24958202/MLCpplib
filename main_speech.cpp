// main.cpp  
#include <iostream>  
#include "SpeechRecognitionWrapper.h"  
int main() {  
    std::cout << "Welcome to the C++ Speech Recognition Simulation!" << std::endl;  
    SpeechRecognitionWrapper recognizer;  
    recognizer.startRecognition();  
    return 0;  
}
/*
    clang++ -std=c++20 -c /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.mm -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.o
    clang++ /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/SpeechRecognitionWrapper.o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech.o -o /Users/dengfengji/ronnieji/MLCpplib-main/MacSpeechReg/MacCommonLineTool_speech/MacCommonLineTool_speech/main_speech -framework Foundation -framework AVFoundation -framework Speech
*/