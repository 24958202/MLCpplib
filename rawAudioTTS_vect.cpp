#include <portaudio.h>  
#include <sndfile.h>  
#include <vector>  
#include <iostream>  
#include <cmath>
#include <algorithm>
#define SAMPLE_RATE 44100
#define RATE_PER_FRAME 520 //256
// Function to apply a moving average filter for noise reduction  
// std::vector<float> movingAverageFilter(const std::vector<float>& audio, size_t windowSize) {  
//     std::vector<float> filtered(audio.size(), 0.0f);  
//     auto halfWindow = windowSize / 2;  
//     for (size_t i = 0; i < audio.size(); ++i) {  
//         float sum = 0.0f;  
//         size_t count = 0;  
//         for (ssize_t j = static_cast<ssize_t>(i) - halfWindow; j <= static_cast<ssize_t>(i) + halfWindow; ++j) {  
//             if (j >= 0 && j < static_cast<ssize_t>(audio.size())) {  
//                 sum += audio[j];  
//                 count++;  
//             }  
//         }  
//         filtered[i] = sum / count;  
//     }  
//     return filtered;  
// }  
// Enhanced Moving Average Function with Evaluation  
std::vector<float> movingAverageFilter(const std::vector<float>& audio, size_t windowSize) {  
    std::vector<float> filtered(audio.size(), 0.0f);  
    size_t halfWindow = windowSize / 2;  
    for (size_t i = 0; i < audio.size(); ++i) {  
        float sum = 0.0f;  
        size_t count = 0;  
        for (size_t j = std::max(0, static_cast<int>(i) - static_cast<int>(halfWindow));  
             j <= std::min(audio.size() - 1, i + halfWindow); ++j) {  
            sum += audio[j];  
            count++;  
        }  
        filtered[i] = sum / count;  
    }  
    return filtered;  
}  
void evaluateFilterAccuracy(const std::vector<float>& original, const std::vector<float>& filtered) {  
    float sumSquaresOriginal = 0.0f;  
    float sumSquaresDifference = 0.0f;  
    for (size_t i = 0; i < original.size(); ++i) {  
        sumSquaresOriginal += original[i] * original[i];  
        sumSquaresDifference += (original[i] - filtered[i]) * (original[i] - filtered[i]);  
    }  
    float snrImprovement = 10 * log10(sumSquaresOriginal / sumSquaresDifference);  
    std::cout << "Signal-to-Noise Ratio Improvement: " << snrImprovement << " dB" << std::endl;  
}  
static int recordCallback(const void *inputBuffer, void *outputBuffer,  
                          unsigned long framesPerBuffer,  
                          const PaStreamCallbackTimeInfo *timeInfo,  
                          PaStreamCallbackFlags statusFlags, void *userData) {  
    auto *data = (std::vector<float> *)userData;  
    const float *rptr = (const float *)inputBuffer;  

    if (inputBuffer != nullptr) {  
        std::copy(rptr, rptr + framesPerBuffer, std::back_inserter(*data));  
    }  

    return paContinue;  
}  

int main() {  
    Pa_Initialize();  
    std::vector<float> recordedSamples;  
    // Set up input parameters  
    PaStreamParameters inputParameters = {};  
    inputParameters.device = Pa_GetDefaultInputDevice();  
    inputParameters.channelCount = 1;  
    inputParameters.sampleFormat = paFloat32;  
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;  
    inputParameters.hostApiSpecificStreamInfo = nullptr;  
    PaStream *stream;  
    Pa_OpenStream(&stream, &inputParameters, nullptr, SAMPLE_RATE, RATE_PER_FRAME, paClipOff, recordCallback, &recordedSamples);  
    Pa_StartStream(stream);  
    std::cout << "Recording. Press Enter to stop..." << std::endl;  
    std::cin.get();  
    Pa_StopStream(stream);  
    /*
        insert speech recognition
    */
    Pa_CloseStream(stream);  
    Pa_Terminate();  
    // Write audio data to a WAV file  
    SF_INFO sfinfo;  
    sfinfo.frames = recordedSamples.size();  
    sfinfo.samplerate = SAMPLE_RATE;  
    sfinfo.channels = 1;  
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;  
    SNDFILE *outfile = sf_open("/Users/dengfengji/ronnieji/MLCpplib-main/output.wav", SFM_WRITE, &sfinfo);  
    if (!outfile) {  
        std::cerr << "Error: Could not open output WAV file." << std::endl;  
        return 1;  
    }  
    sf_write_float(outfile, recordedSamples.data(), recordedSamples.size());  
    sf_close(outfile);  
    std::cout << "Audio recorded and saved to output.wav" << std::endl; 
    // /*
    //     put the file into a int16_t array
    // */ 
    // // Create a float buffer for the audio samples  
    // std::vector<float> audioBuffer;  
    // audioBuffer.resize(recordedSamples.size());  
    // // Copy the recorded samples directly to audioBuffer   
    // for (size_t i = 0; i < recordedSamples.size(); ++i) {  
    //     // Directly store the float samples without scaling  
    //     audioBuffer[i] = recordedSamples[i];  
    //     std::cout << audioBuffer[i] << std::endl;  
    // }
    // Smoothing the audio to reduce noise  
    int filterWindowSize = 3; // Choose an odd number for better center calculation  
    std::vector<float> smoothedSamples = movingAverageFilter(recordedSamples, filterWindowSize);
     /*
        Evaluate the improvement of noise reduction
    */
    evaluateFilterAccuracy(recordedSamples, smoothedSamples);  
    double timePerSample = 1.0 / SAMPLE_RATE;  
    // Adjust speech threshold till you achieve the desired detection performance  
    const float speechThreshold = 0.0010;  // Adjust this threshold as necessary  
    // Minimum silence duration in samples (e.g., 50 ms)  
    const int minSilenceSamples = 5 * SAMPLE_RATE / 1000; // 50 ms in samples  
    std::vector<float> wordDurations;  
    std::vector<float> silenceDurations;  
    bool inWord = false;  
    int wordStart = 0;  
    int silenceStart = 0;  
    for (size_t i = 0; i < smoothedSamples.size(); ++i) {  
        if (std::fabs(smoothedSamples[i]) > speechThreshold) {  
            if (!inWord) {  
                inWord = true;  
                wordStart = i;  
                if (silenceStart != 0 && (wordStart - silenceStart) > minSilenceSamples) {  
                    int silenceSamples = wordStart - silenceStart;  
                    silenceDurations.push_back(silenceSamples * timePerSample);  
                }  
            }  
        } else {  
            if (inWord) {  
                inWord = false;  
                int wordDurationSamples = i - wordStart;  
                wordDurations.push_back(wordDurationSamples * timePerSample);  
                silenceStart = i;  
            }  
        }  
    }  
    // Output detected word durations and silence durations for verification  
    std::cout << "Detected Word Durations (seconds):" << std::endl;  
    for (size_t i = 0; i < wordDurations.size(); ++i) {  
        std::cout << "Word " << (i + 1) << ": " << wordDurations[i] << " seconds" << std::endl;  
    }  
    std::cout << "Detected Silence Durations (seconds):" << std::endl;  
    for (size_t i = 0; i < silenceDurations.size(); ++i) {  
        std::cout << "Silence " << (i + 1) << ": " << silenceDurations[i] << " seconds" << std::endl;  
    }  
    return 0;  
}
/*

*/