// SpeechRecognitionWrapper.h  
#ifndef SPEECH_RECOGNITION_WRAPPER_H  
#define SPEECH_RECOGNITION_WRAPPER_H  
#include <string>  
class SpeechRecognitionWrapper {  
public:  
    SpeechRecognitionWrapper();  
    ~SpeechRecognitionWrapper();  
    void startRecognition();  
    std::string getRecognizedText();  
private:  
    class Impl;  
    Impl* pImpl;  
};  
#endif // SPEECH_RECOGNITION_WRAPPER_H