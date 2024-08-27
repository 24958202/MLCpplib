/*
     g++ /Users/dengfengji/ronnieji/MLCpplib-main/trainvideo.cpp -o /Users/dengfengji/ronnieji/MLCpplib-main/trainvideo -I/opt/homebrew/Cellar/ffmpeg/7.0.2/include -L/opt/homebrew/Cellar/ffmpeg/7.0.2/lib -lavformat -lavcodec -lavutil -lswresample -lswscale -lpthread `pkg-config --cflags --libs avformat` -std=c++20
*/
#include <iostream>  
#include <fstream>  
#include <stdexcept>  
#include <vector>  
#include <memory>  
extern "C" {  
#include <libavformat/avformat.h>  
#include <libavcodec/avcodec.h>  
#include <libavutil/avutil.h>  
#include <libavutil/timestamp.h>  
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

    } catch (const std::exception& e) {  
        std::cerr << "Error: " << e.what() << "\n";  
        return 1;  
    }  
    return 0;  
}