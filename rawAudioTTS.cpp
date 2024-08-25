#include <portaudio.h>  
#include <sndfile.h>  
#include <vector>  
#include <iostream>  

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
    Pa_OpenStream(&stream, &inputParameters, nullptr, 44100, 256, paClipOff, recordCallback, &recordedSamples);  
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
    sfinfo.samplerate = 44100;  
    sfinfo.channels = 1;  
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;  
    SNDFILE *outfile = sf_open("/Users/ronnieji/MLCpplib-main/output.wav", SFM_WRITE, &sfinfo);  
    if (!outfile) {  
        std::cerr << "Error: Could not open output WAV file." << std::endl;  
        return 1;  
    }  
    sf_write_float(outfile, recordedSamples.data(), recordedSamples.size());  
    sf_close(outfile);  
    std::cout << "Audio recorded and saved to output.wav" << std::endl;  
    return 0;  
}
/*

*/