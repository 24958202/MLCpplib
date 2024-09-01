// main.cpp  
#include <iostream>  
#include "SpeechRecognitionWrapper.h"  
int main() {  
    std::cout << "Welcome to the C++ Speech Recognition Simulation!" << std::endl;  
    SpeechRecognitionWrapper recognizer;  
    recognizer.startRecognition();  
    return 0;  
}