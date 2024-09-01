// main.mm
#include <iostream>
#import <Foundation/Foundation.h>
#import <Speech/Speech.h>

void startSpeechRecognition() {
    @autoreleasepool {
        // Request authorization
        [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {
            switch (status) {
                case SFSpeechRecognizerAuthorizationStatusAuthorized:
                    std::cout << "Speech recognition authorized." << std::endl;
                    break;
                case SFSpeechRecognizerAuthorizationStatusDenied:
                    std::cout << "Speech recognition authorization denied." << std::endl;
                    return;
                case SFSpeechRecognizerAuthorizationStatusRestricted:
                    std::cout << "Speech recognition restricted on this device." << std::endl;
                    return;
                case SFSpeechRecognizerAuthorizationStatusNotDetermined:
                    std::cout << "Speech recognition not determined." << std::endl;
                    return;
            }
        }];

        // Set up the speech recognizer
        NSLocale *locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en-US"];
        SFSpeechRecognizer *speechRecognizer = [[SFSpeechRecognizer alloc] initWithLocale:locale];

        if (!speechRecognizer) {
            std::cout << "Speech recognizer not available for the specified locale." << std::endl;
            return;
        }

        // Create a recognition request
        SFSpeechAudioBufferRecognitionRequest *request = [[SFSpeechAudioBufferRecognitionRequest alloc] init];

        // Set up audio engine
        AVAudioEngine *audioEngine = [[AVAudioEngine alloc] init];
        AVAudioInputNode *inputNode = [audioEngine inputNode];
        AVAudioFormat *recordingFormat = [inputNode outputFormatForBus:0];
        [inputNode installTapOnBus:0 bufferSize:1024 format:recordingFormat block:^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {
            [request appendAudioPCMBuffer:buffer];
        }];

        // Start audio engine
        [audioEngine prepare];
        NSError *error;
        if (![audioEngine startAndReturnError:&error]) {
            std::cout << "Audio engine couldn't start: " << error.localizedDescription.UTF8String << std::endl;
            return;
        }

        // Start recognition task
        SFSpeechRecognitionTask *recognitionTask = [speechRecognizer recognitionTaskWithRequest:request resultHandler:^(SFSpeechRecognitionResult *result, NSError *error) {
            if (result) {
                std::string recognizedText = result.bestTranscription.formattedString.UTF8String;
                std::cout << "Recognized Text: " << recognizedText << std::endl;
            }

            if (error) {
                std::cout << "Recognition error: " << error.localizedDescription.UTF8String << std::endl;
            }

            if (result.isFinal) {
                [audioEngine stop];
                [inputNode removeTapOnBus:0];
            }
        }];

        // Keep the program running to listen for speech
        std::cout << "Listening... Press Ctrl+C to stop." << std::endl;
        [[NSRunLoop currentRunLoop] run];
    }
}

int main() {
    std::cout << "Welcome to the C++ Speech Recognition Simulation!" << std::endl;
    startSpeechRecognition();
    return 0;
}
