// SpeechRecognitionWrapper.h  
#ifndef SPEECH_RECOGNITION_WRAPPER_H  
#define SPEECH_RECOGNITION_WRAPPER_H  
#include <string>  
#include <functional>
class SpeechRecognitionWrapper {  
public:  
    SpeechRecognitionWrapper();  
    ~SpeechRecognitionWrapper();  
    void startRecognition(std::function<void(const std::string&)> callback);
private:  
    class Impl;  
    Impl* pImpl;  
};  
#endif // SPEECH_RECOGNITION_WRAPPER_H