#include "TextToSpeechWrapper.h"  
#include <iostream>  
#include <functional>  
#import <Foundation/Foundation.h>  
#import <AVFoundation/AVFoundation.h>  

// Define the synthesizer delegate  
@interface SynthesizerDelegate : NSObject <AVSpeechSynthesizerDelegate>  
- (instancetype)initWithCallback:(std::function<void()> *)callback;  
@end  

@implementation SynthesizerDelegate {  
    std::function<void()> *_completionCallback;  
}  

- (instancetype)initWithCallback:(std::function<void()> *)callback {  
    self = [super init];  
    if (self) {  
        _completionCallback = callback;  
    }  
    return self;  
}  

- (void)speechSynthesizer:(AVSpeechSynthesizer *)synthesizer didFinishSpeechUtterance:(AVSpeechUtterance *)utterance {  
    if (_completionCallback && *_completionCallback) {  
        (*_completionCallback)();  
    }  
}  

- (void)dealloc {  
    delete _completionCallback;  
}  

@end  

// Implementation of the actual wrapper  
class TextToSpeechWrapper::Impl {  
public:  
    Impl() {}  
    ~Impl() {}  

    void speak(const std::string& text, std::function<void()> completionCallback) {  
        @autoreleasepool {  
            AVSpeechSynthesizer *synthesizer = [[AVSpeechSynthesizer alloc] init];  
            AVSpeechUtterance *utterance = [[AVSpeechUtterance alloc] initWithString:[NSString stringWithUTF8String:text.c_str()]];  
            utterance.voice = [AVSpeechSynthesisVoice voiceWithLanguage:@"en-US"];  

            // Create a delegate and pass the address of the callback  
            SynthesizerDelegate *delegate = [[SynthesizerDelegate alloc] initWithCallback:new std::function<void()>(completionCallback)];  
            synthesizer.delegate = delegate;  // Assign delegate  
            [synthesizer speakUtterance:utterance];  
        }  
    }  
};  

TextToSpeechWrapper::TextToSpeechWrapper() : pImpl(new Impl()) {}  
TextToSpeechWrapper::~TextToSpeechWrapper() { delete pImpl; }  
void TextToSpeechWrapper::speak(const std::string& text, std::function<void()> completionCallback) {  
    pImpl->speak(text, completionCallback);  
}