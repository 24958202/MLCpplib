// SpeechRecognitionWrapper.mm  
#include "SpeechRecognitionWrapper.h"  
#include <iostream>  
#import <Foundation/Foundation.h>  
#import <Speech/Speech.h>  

class SpeechRecognitionWrapper::Impl {  
public:  
    Impl() : recognizedText("") {}  
    ~Impl() {}  

    void startRecognition() {  
        @autoreleasepool {  
            __block bool authorized = false;  
            [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {  
                if (status == SFSpeechRecognizerAuthorizationStatusAuthorized) {  
                    authorized = true;  
                } else {  
                    std::cerr << "Speech recognition not authorized." << std::endl;  
                }  
            }];  
            if (!authorized) {  
                return;  
            }  
            NSLocale *locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en-US"];  
            SFSpeechRecognizer *speechRecognizer = [[SFSpeechRecognizer alloc] initWithLocale:locale];  
            if (!speechRecognizer) {  
                std::cerr << "Speech recognizer not available for the specified locale." << std::endl;  
                return;  
            }  
            SFSpeechAudioBufferRecognitionRequest *request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];  
            AVAudioEngine *audioEngine = [[AVAudioEngine alloc] init];  
            AVAudioInputNode *inputNode = [audioEngine inputNode];  
            AVAudioFormat *recordingFormat = [inputNode outputFormatForBus:0];  
            [inputNode installTapOnBus:0 bufferSize:1024 format:recordingFormat block:^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {  
                [request appendAudioPCMBuffer:buffer];  
                std::cout << "Audio buffer appended." << std::endl;  // Debugging statement  
            }];  
            [audioEngine prepare];  
            NSError *error;  
            if (![audioEngine startAndReturnError:&error]) {  
                std::cerr << "Audio engine couldn't start: " << error.localizedDescription.UTF8String << std::endl;  
                return;  
            }  
            std::cout << "Audio engine started." << std::endl;  // Debugging statement  
            [speechRecognizer recognitionTaskWithRequest:request resultHandler:^(SFSpeechRecognitionResult *result, NSError *error) {  
                if (result) {  
                    recognizedText = result.bestTranscription.formattedString.UTF8String;  
                    std::cout << "Recognized Text: " << recognizedText << std::endl;  
                }  
                if (error) {  
                    std::cerr << "Recognition error: " << error.localizedDescription.UTF8String << std::endl;  
                }  
                if (result.isFinal) {  
                    std::cout << "Final recognized text: " << recognizedText << std::endl;  
                    [audioEngine stop];  
                    [inputNode removeTapOnBus:0];  
                    CFRunLoopStop(CFRunLoopGetCurrent());  
                }  
            }];  
            std::cout << "Listening... Press Ctrl+C to stop." << std::endl;  
            CFRunLoopRun();  
        }  
    }
    std::string getRecognizedText() {  
        return recognizedText;  
    }  
private:  
    std::string recognizedText;  
};  
SpeechRecognitionWrapper::SpeechRecognitionWrapper() : pImpl(new Impl()) {}  
SpeechRecognitionWrapper::~SpeechRecognitionWrapper() { delete pImpl; }  
void SpeechRecognitionWrapper::startRecognition() { pImpl->startRecognition(); }  
std::string SpeechRecognitionWrapper::getRecognizedText() { return pImpl->getRecognizedText(); }