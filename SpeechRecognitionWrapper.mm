// SpeechRecognitionWrapper.mm  
#include "SpeechRecognitionWrapper.h"  
#include <iostream>  
#include <string>
#include <functional> 
#import <Foundation/Foundation.h>  
#import <Speech/Speech.h>  

class SpeechRecognitionWrapper::Impl {  
public:  
    Impl() : recognizedText(""),regCount(0) {}  
    ~Impl() {}  
    void startRecognition(std::function<void(const std::string&)> callback) {  
        std::cout << "Starting recognition..." << std::endl;  
        @autoreleasepool {  
            dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);  
            __block SFSpeechRecognizerAuthorizationStatus authStatus;  
            [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {  
                authStatus = status;  
                if (authStatus == SFSpeechRecognizerAuthorizationStatusAuthorized) {  
                    std::cout << "Authorization successful." << std::endl;  // Debugging statement  
                } else {  
                    std::cerr << "Speech recognition not authorized." << std::endl;  
                }  
                dispatch_semaphore_signal(semaphore);  
            }];  
            // Wait for the authorization to complete  
            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);  
            // // Proceed only if authorized  
            if (authStatus != SFSpeechRecognizerAuthorizationStatusAuthorized) {  
                std::cerr << "authStatus != SFSpeechRecognizerAuthorizationStatusAuthorized : exit program." << std::endl;
                return;  
            }  
            NSLocale *locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en-US"];  
            // Create and configure the speech recognizer  
            SFSpeechRecognizer *speechRecognizer = [[SFSpeechRecognizer alloc] initWithLocale:locale];  
            if (!speechRecognizer) {  
                std::cerr << "Speech recognizer not available for the specified locale." << std::endl;  
                return;  
            }  
            std::cout << "Speech recognizer created." << std::endl;  // Debugging statement 
            SFSpeechAudioBufferRecognitionRequest *request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
                // Create and configure the audio engine  
                AVAudioEngine *audioEngine = [[AVAudioEngine alloc] init];  
                AVAudioInputNode *inputNode = [audioEngine inputNode];  
                AVAudioFormat *recordingFormat = [inputNode outputFormatForBus:0];  
                [inputNode installTapOnBus:0 bufferSize:1024 format:recordingFormat block:^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {  
                    [request appendAudioPCMBuffer:buffer];  
                }];  
                [audioEngine prepare];  
                NSError *error;  
                if (![audioEngine startAndReturnError:&error]) {  
                    std::cerr << "Audio engine couldn't start: " << error.localizedDescription.UTF8String << std::endl;  
                    return;  
                }  
            std::cout << "Audio engine started." << std::endl;  // Debugging statement  
            // Start the recognition task  
            __block BOOL shouldContinue = YES;  
            [speechRecognizer recognitionTaskWithRequest:request resultHandler:^(SFSpeechRecognitionResult *result, NSError *error) {  
                if (result) {  
                    recognizedText = result.bestTranscription.formattedString.UTF8String;  
                    //std::cout << "Recognized Text: " << recognizedText << std::endl; 
                    regCount++; 
                    if (result.isFinal) {  
                        //std::cout << "Final recognized text: " << recognizedText << std::endl;  
                        shouldContinue = NO;  
                        regCount=60;
                    }  
                    if (callback) {  
                        callback(recognizedText);  
                    }  
                }  
                if (error) {  
                    std::cerr << "Recognition error: " << error.localizedDescription.UTF8String << std::endl;  
                    shouldContinue = NO;  
                }  
            }];  
            // Run the loop to keep listening  
            while (shouldContinue) {  
                [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];  
            }  
            // Clean up resources  
            [audioEngine stop];  
            [inputNode removeTapOnBus:0];  
            // Restart recognition if needed  
            if (regCount>=60) {  
                regCount=0;
                startRecognition(callback); // Restart the recognition process  
            }  
        }  
    }
private:  
    std::string recognizedText;  
    int regCount = 0;
};  
SpeechRecognitionWrapper::SpeechRecognitionWrapper() : pImpl(new Impl()) {}  
SpeechRecognitionWrapper::~SpeechRecognitionWrapper() { delete pImpl; }  
void SpeechRecognitionWrapper::startRecognition(std::function<void(const std::string&)> callback) {  
    pImpl->startRecognition(callback);  
}  
