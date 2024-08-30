/*
     g++ /Users/dengfengji/ronnieji/MLCpplib-main/trainvideo.cpp \
    -o /Users/dengfengji/ronnieji/MLCpplib-main/trainvideo \
    -I/opt/homebrew/Cellar/fftw/3.3.10_1/include -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lavformat -lavcodec -lavutil -lswresample -lswscale -lpthread -lfftw3f \
    -std=c++20
*/
#include <iostream>  
#include <fstream>  
#include <stdexcept>  
#include <vector>  
#include <memory>  
#include <complex>  
#include <fftw3.h>  
extern "C" {  
#include <libavformat/avformat.h>  
#include <libavcodec/avcodec.h>  
#include <libavutil/avutil.h>  
#include <libavutil/timestamp.h>  
}  
struct WAVHeader {  
    char riff[4] = {'R', 'I', 'F', 'F'};  
    uint32_t chunkSize;  
    char wave[4] = {'W', 'A', 'V', 'E'};  
    char fmt[4] = {'f', 'm', 't', ' '};  
    uint32_t subchunk1Size = 16;  
    uint16_t audioFormat = 1;  
    uint16_t numChannels = 2;  
    uint32_t sampleRate = 44100;  
    uint32_t byteRate;  
    uint16_t blockAlign;  
    uint16_t bitsPerSample = 16;  
    char subchunk2ID[4] = {'d', 'a', 't', 'a'};  
    uint32_t subchunk2Size;  
    WAVHeader(int numChannels, int sampleRate, int bitsPerSample, uint32_t dataSize) {  
        this->numChannels = numChannels;  
        this->sampleRate = sampleRate;  
        this->bitsPerSample = bitsPerSample;  
        byteRate = sampleRate * numChannels * bitsPerSample / 8;  
        blockAlign = numChannels * bitsPerSample / 8;  
        subchunk2Size = dataSize;  
        chunkSize = 36 + subchunk2Size;  
    }  
};  
// Function to perform FFT using FFTW  
void performFFT(std::vector<std::complex<float>>& data, bool inverse = false) {  
    int N = data.size();  
    // Allocate memory for FFTW input/output  
    fftwf_complex* input = reinterpret_cast<fftwf_complex*>(data.data());  
    fftwf_complex* output = reinterpret_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * N));  
    // Choose direction for FFT  
    int direction = inverse ? FFTW_BACKWARD : FFTW_FORWARD;  
    // Create plan  
    fftwf_plan plan = fftwf_plan_dft_1d(N, input, output, direction, FFTW_ESTIMATE);  
    // Execute plan  
    fftwf_execute(plan);  
    // Scale output if inverse transform  
    if (inverse) {  
        for (int i = 0; i < N; ++i) {  
            output[i][0] /= N;  
            output[i][1] /= N;  
        }  
    }  
    // Copy result back to original data vector  
    for (int i = 0; i < N; ++i) {  
        data[i] = std::complex<float>(output[i][0], output[i][1]);  
    }  
    // Cleanup  
    fftwf_destroy_plan(plan);  
    fftwf_free(output);  
}  
// Moving Average Filter  
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
// Evaluation Function  
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
// Function to convert raw audio to WAV with noise reduction  
void convertRawToWav(const char* rawFilename, const char* wavFilename, int numChannels, int sampleRate, int bitsPerSample, size_t windowSize) {  
    std::ifstream rawFile(rawFilename, std::ios::binary);  
    if (!rawFile) {  
        throw std::runtime_error("Could not open raw audio input file.");  
    }  
    // Determine raw data size  
    rawFile.seekg(0, std::ios::end);  
    uint32_t dataSize = rawFile.tellg();  
    rawFile.seekg(0, std::ios::beg);  
    // Read raw PCM data into a vector of floats for processing  
    std::vector<int16_t> pcmData(dataSize / sizeof(int16_t));  
    rawFile.read(reinterpret_cast<char*>(pcmData.data()), dataSize);  
    rawFile.close();  
    // Convert to float for processing  
    std::vector<std::complex<float>> audioData(pcmData.begin(), pcmData.end());  
    // Apply FFT  
    performFFT(audioData);  
    // Here we can perform noise reduction in frequency domain  
    // Example: naive noise reduction just zeroes out small magnitude components  
    float noiseThreshold = 10.0f; // Example threshold, adjust as necessary  
    for (auto& freq : audioData) {  
        if (std::abs(freq) < noiseThreshold) {  
            freq = 0;  
        }  
    }  
    // Apply inverse FFT  
    performFFT(audioData, true);  
    // Convert filtered audio back to PCM format  
    std::vector<int16_t> filteredPcmData(audioData.size());  
    std::transform(audioData.begin(), audioData.end(), filteredPcmData.begin(), [](std::complex<float> sample) {  
        return static_cast<int16_t>(std::clamp(sample.real(), -32768.0f, 32767.0f));  
    });  
    // Write WAV header and data  
    std::ofstream wavFile(wavFilename, std::ios::binary);  
    if (!wavFile) {  
        throw std::runtime_error("Could not open WAV output file.");  
    }  
    WAVHeader header(numChannels, sampleRate, bitsPerSample, filteredPcmData.size() * sizeof(int16_t));  
    wavFile.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));  
    wavFile.write(reinterpret_cast<const char*>(filteredPcmData.data()), filteredPcmData.size() * sizeof(int16_t));  
    wavFile.close();  
}  
void initializeFFmpeg() {  
    //av_register_all();  
    avformat_network_init();  
}  
AVFormatContext* openInputFile(const char* filename) {  
    AVFormatContext* fmt_ctx = nullptr;  
    if (avformat_open_input(&fmt_ctx, filename, nullptr, nullptr) < 0) {  
        throw std::runtime_error("Could not open input file");  
    }  
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {  
        avformat_close_input(&fmt_ctx);  
        throw std::runtime_error("Failed to retrieve input stream information");  
    }  
    return fmt_ctx;  
}  
int findStreamIndex(AVFormatContext* fmt_ctx, AVMediaType mediaType) {  
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {  
        if (fmt_ctx->streams[i]->codecpar->codec_type == mediaType) {  
            return i;  
        }  
    }  
    return -1;  
}  
void extractAudio(AVFormatContext* fmt_ctx, int audioStreamIndex, const char* outputFilename) {  
    const AVCodec* codec = nullptr;  
    AVCodecContext* codec_ctx = nullptr;  
    AVCodecParameters* codec_params = fmt_ctx->streams[audioStreamIndex]->codecpar;  
    codec = avcodec_find_decoder(codec_params->codec_id);  
    if (!codec) {  
        throw std::runtime_error("Failed to find codec for audio stream");  
    }  
    codec_ctx = avcodec_alloc_context3(codec);  
    avcodec_parameters_to_context(codec_ctx, codec_params);  
    avcodec_open2(codec_ctx, codec, nullptr);  
    AVPacket* packet = av_packet_alloc();  
    AVFrame* frame = av_frame_alloc();  
    std::ofstream outFile(outputFilename, std::ios::binary);
    /*  
    while (av_read_frame(fmt_ctx, &packet) >= 0) {  
        if (packet.stream_index == audioStreamIndex) {  
            avcodec_send_packet(codec_ctx, &packet);  
            while (avcodec_receive_frame(codec_ctx, frame) >= 0) {  
                outFile.write(reinterpret_cast<const char*>(frame->extended_data[0]), frame->linesize[0]);  
            }  
        }  
        av_packet_unref(&packet);  
    } 
    */ 
    while (av_read_frame(fmt_ctx, packet) >= 0) {  
        if (packet->stream_index == audioStreamIndex) {  
            avcodec_send_packet(codec_ctx, packet);  
            while (avcodec_receive_frame(codec_ctx, frame) >= 0) {  
                outFile.write(reinterpret_cast<const char*>(frame->extended_data[0]), frame->linesize[0]);  
            }  
        }  
        av_packet_unref(packet);  
    }  
    av_frame_free(&frame);  
    avcodec_free_context(&codec_ctx);  
    av_packet_free(&packet);
    outFile.close();  
}  
void extractSubtitles(AVFormatContext* fmt_ctx, int subtitleStreamIndex, const char* outputFilename) {  
    const AVCodec* codec = nullptr;  
    AVCodecContext* codec_ctx = nullptr;  
    AVCodecParameters* codec_params = fmt_ctx->streams[subtitleStreamIndex]->codecpar;  
    codec = avcodec_find_decoder(codec_params->codec_id);  
    if (!codec) {  
        throw std::runtime_error("Failed to find codec for subtitle stream");  
    }  
    codec_ctx = avcodec_alloc_context3(codec);  
    avcodec_parameters_to_context(codec_ctx, codec_params);  
    avcodec_open2(codec_ctx, codec, nullptr);  
    /*
    AVPacket* packet = av_packet_alloc(); 
    AVSubtitle subtitle;  
    std::ofstream outFile(outputFilename);  
    while (av_read_frame(fmt_ctx, &packet) >= 0) {  
        if (packet.stream_index == subtitleStreamIndex) {  
            avcodec_send_packet(codec_ctx, &packet);  
            while (avcodec_receive_subtitle2(codec_ctx, &subtitle, &packet) >= 0) {  
                // Process and write the subtitle data (simplified for example)  
                outFile << "Subtitle at " << packet.pts << ": " << subtitle.num_rects << " rects.\n";   
                avsubtitle_free(&subtitle);  
            }  
        }  
        av_packet_unref(&packet);  
    }  
    avcodec_free_context(&codec_ctx);  
    outFile.close();  
    */
    AVPacket* packet = av_packet_alloc();  
    std::ofstream outFile(outputFilename);  
    while (av_read_frame(fmt_ctx, packet) >= 0) {  
        if (packet->stream_index == subtitleStreamIndex) {  
            avcodec_send_packet(codec_ctx, packet);  
            AVSubtitle subtitle;  
            int response = avcodec_receive_frame(codec_ctx, reinterpret_cast<AVFrame*>(&subtitle));  
            if (response >= 0) {  
                // Simplified example of handling subtitle display  
                outFile << "Subtitle at " << packet->pts << ": " << subtitle.num_rects << " rects.\n";   
                avsubtitle_free(&subtitle);  
            }  
        }  
        av_packet_unref(packet);  
    }  
    avcodec_free_context(&codec_ctx);  
    av_packet_free(&packet);  
    outFile.close();  
}  
int main(int argc, char* argv[]) {  
    if (argc < 2) {  
        std::cerr << "Usage: " << argv[0] << " <input_video_file>\n";  
        return 1;  
    }  
    const char* inputFilename = argv[1];  
    const char* audioOutput = "/Users/dengfengji/ronnieji/MLCpplib-main/output_audio.raw";  
    const char* subtitleOutput = "/Users/dengfengji/ronnieji/MLCpplib-main/output_subtitles.txt";  
    try {  
        initializeFFmpeg();  
        AVFormatContext* fmt_ctx = openInputFile(inputFilename);  
        int audioStreamIndex = findStreamIndex(fmt_ctx, AVMEDIA_TYPE_AUDIO);  
        int subtitleStreamIndex = findStreamIndex(fmt_ctx, AVMEDIA_TYPE_SUBTITLE);  
        if (audioStreamIndex != -1) {  
            std::cout << "Extracting audio...\n";  
            extractAudio(fmt_ctx, audioStreamIndex, audioOutput);  
            std::cout << "Audio extracted to " << audioOutput << "\n";  
        } else {  
            std::cout << "No audio stream found.\n";  
        }  
        if (subtitleStreamIndex != -1) {  
            std::cout << "Extracting subtitles...\n";  
            extractSubtitles(fmt_ctx, subtitleStreamIndex, subtitleOutput);  
            std::cout << "Subtitles extracted to " << subtitleOutput << "\n";  
        } else {  
            std::cout << "No subtitle stream found.\n";  
        }  
        avformat_close_input(&fmt_ctx);  
        avformat_network_deinit();  
       try {  
            convertRawToWav("/Users/dengfengji/ronnieji/MLCpplib-main/output_audio.raw",  
                            "/Users/dengfengji/ronnieji/MLCpplib-main/output_audio.wav",  
                            2, 44100, 16, 1024); // Example window size  
            std::cout << "Conversion with FFT-based noise reduction complete.\n";  
        } catch (const std::exception& e) {  
            std::cerr << "Error: " << e.what() << std::endl;  
        }  
    } catch (const std::exception& e) {  
        std::cerr << "Error: " << e.what() << "\n";  
        return 1;  
    }  
    return 0;  
}