#ifndef TEXT_TO_SPEECH_WRAPPER_H  
#define TEXT_TO_SPEECH_WRAPPER_H  
#include <string>  
#include <functional>  
class TextToSpeechWrapper {  
public:  
    TextToSpeechWrapper();  
    ~TextToSpeechWrapper();  
    void speak(const std::string& text, std::function<void()> completionCallback);  
private:  
    class Impl;  
    Impl* pImpl;  
};  

#endif // TEXT_TO_SPEECH_WRAPPER_H