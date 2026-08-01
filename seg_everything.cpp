#include <onnxruntime_cxx_api.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

struct FixedInputShape {
    int height = 1008; // Updated to match model requirement
    int width = 1008;  // Updated to match model requirement
};

[[noreturn]] static void fail(const std::string& message) {
    throw std::runtime_error(message);
}

static std::vector<uint8_t> preprocess_to_uint8_nchw(const cv::Mat& bgr, const FixedInputShape fixed_shape) {
    if (bgr.empty()) fail("Cannot decode input image");

    const float scale = std::min(
        static_cast<float>(fixed_shape.width) / static_cast<float>(bgr.cols),
        static_cast<float>(fixed_shape.height) / static_cast<float>(bgr.rows));
        
    const int resized_w = std::min(fixed_shape.width, std::max(1, static_cast<int>(std::round(bgr.cols * scale))));
    const int resized_h = std::min(fixed_shape.height, std::max(1, static_cast<int>(std::round(bgr.rows * scale))));

    cv::Mat rgb, resized;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    cv::resize(rgb, resized, cv::Size(resized_w, resized_h), 0.0, 0.0, cv::INTER_LINEAR);

    // Create canvas padded with zeroes (black)
    cv::Mat canvas = cv::Mat::zeros(fixed_shape.height, fixed_shape.width, CV_8UC3);
    resized.copyTo(canvas(cv::Rect(0, 0, resized_w, resized_h)));

    // Allocate flat array for 3-channel NCHW format: [Channels=3, Height, Width]
    std::vector<uint8_t> input_tensor_values(3 * fixed_shape.height * fixed_shape.width, 0);

    const int stride = fixed_shape.height * fixed_shape.width;
    for (int y = 0; y < fixed_shape.height; ++y) {
        const auto* row = canvas.ptr<cv::Vec3b>(y);
        for (int x = 0; x < fixed_shape.width; ++x) {
            for (int channel = 0; channel < 3; ++channel) {
                input_tensor_values[channel * stride + y * fixed_shape.width + x] = row[x][channel];
            }
        }
    }
    return input_tensor_values;
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <input_image>\n";
            return 1;
        }

        const fs::path input_path = fs::absolute(argv[1]);
        if (!fs::exists(input_path)) fail("Input image does not exist: " + input_path.string());

        const fs::path model_dir = "/Users/jidengfeng/Downloads/models/segment-anything/sam3_vit_h_extracted";
        const fs::path encoder_path = model_dir / "sam3_image_encoder.onnx";

        if (!fs::exists(encoder_path)) fail("Encoder ONNX model missing: " + encoder_path.string());

        // --- 1. INITIALIZE ONNX RUNTIME ---
        std::cout << "Initializing ONNX Runtime Environment...\n";
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "SAM3_Encoder");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(std::max(1U, std::thread::hardware_concurrency()));
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        
        std::cout << "Loading SAM3 Image Encoder: " << encoder_path << '\n';
        Ort::Session session(env, encoder_path.c_str(), session_options);

        Ort::AllocatorWithDefaultOptions allocator;
        auto input_name = session.GetInputNameAllocated(0, allocator);
        auto output_name = session.GetOutputNameAllocated(0, allocator);
        
        std::vector<const char*> input_node_names = {input_name.get()};
        std::vector<const char*> output_node_names = {output_name.get()};

        // --- 2. PREPROCESS IMAGE ---
        std::cout << "Reading input image: " << input_path << '\n';
        const cv::Mat image = cv::imread(input_path.string(), cv::IMREAD_COLOR);
        if (image.empty()) fail("Cannot read input image: " + input_path.string());

        FixedInputShape fixed_shape{1008, 1008}; 
        
        std::cout << "Preprocessing to 1008x1008 NCHW uint8 format...\n";
        std::vector<uint8_t> input_tensor_values = preprocess_to_uint8_nchw(image, fixed_shape);

        // --- 3. CREATE ONNX TENSORS ---
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        // Changed rank from 4 ({1, 3, H, W}) to rank 3 ({3, H, W}) matching model expectation
        std::vector<int64_t> input_node_dims = {3, fixed_shape.height, fixed_shape.width};

        Ort::Value input_tensor = Ort::Value::CreateTensor<uint8_t>(
            memory_info, 
            input_tensor_values.data(), 
            input_tensor_values.size(), 
            input_node_dims.data(), 
            input_node_dims.size()
        );

        // --- 4. RUN INFERENCE ---
        std::cout << "Executing SAM3 Image Encoder Graph...\n";
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr}, 
            input_node_names.data(), 
            &input_tensor, 1, 
            output_node_names.data(), 1
        );

        if (output_tensors.empty()) fail("Failed to generate image embeddings.");

        // --- 5. EXTRACT EMBEDDINGS ---
        auto& output_tensor = output_tensors.front();
        auto type_info = output_tensor.GetTensorTypeAndShapeInfo();
        auto output_shape = type_info.GetShape();
        
        const float* embedding_data = output_tensor.GetTensorData<float>();
        const size_t embedding_count = type_info.GetElementCount();

        std::cout << "\n[SUCCESS] Image Embeddings Generated!\n";
        std::cout << "Embedding Tensor Shape: [";
        for (size_t i = 0; i < output_shape.size(); ++i) {
            std::cout << output_shape[i] << (i == output_shape.size() - 1 ? "" : ", ");
        }
        std::cout << "]\n";
        std::cout << "Total Elements: " << embedding_count << '\n';
        std::cout << "First embedding value: " << embedding_data[0] << '\n';

        return 0;
    } catch (const Ort::Exception& error) {
        std::cerr << "ONNX Runtime error: " << error.what() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
    }
    return 1;
}