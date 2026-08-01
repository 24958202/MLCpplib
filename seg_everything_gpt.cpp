#include <onnxruntime_cxx_api.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct InputShape {
    int height = 1008;
    int width = 1008;
};

struct Detection {
    int id = 0;
    int label_id = -1;
    std::string label;
    float score = 0.0f;
    cv::Mat mask; // CV_8UC1, original image size
};

[[noreturn]] static void fail(const std::string& message) {
    throw std::runtime_error(message);
}

static const std::vector<std::string> COCO_LABELS = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train",
    "truck", "boat", "traffic light", "fire hydrant", "stop sign",
    "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep",
    "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella",
    "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard",
    "sports ball", "kite", "baseball bat", "baseball glove", "skateboard",
    "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork",
    "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
    "couch", "pottedplant", "bed", "diningtable", "toilet", "tvmonitor",
    "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave",
    "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
    "scissors", "teddy bear", "hair drier", "toothbrush"
};

static std::vector<uint8_t> preprocess(const cv::Mat& bgr, const InputShape& shape) {
    if (bgr.empty()) fail("Input image is empty");

    const float scale = std::min(
        static_cast<float>(shape.width) / bgr.cols,
        static_cast<float>(shape.height) / bgr.rows);

    const int resized_w = std::max(1, std::min(shape.width,
        static_cast<int>(std::round(bgr.cols * scale))));
    const int resized_h = std::max(1, std::min(shape.height,
        static_cast<int>(std::round(bgr.rows * scale))));

    cv::Mat rgb, resized;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    cv::resize(rgb, resized, cv::Size(resized_w, resized_h), 0, 0, cv::INTER_LINEAR);

    cv::Mat canvas = cv::Mat::zeros(shape.height, shape.width, CV_8UC3);
    resized.copyTo(canvas(cv::Rect(0, 0, resized_w, resized_h)));

    std::vector<uint8_t> tensor_values(3ULL * shape.height * shape.width, 0);
    const int plane = shape.height * shape.width;

    for (int y = 0; y < shape.height; ++y) {
        const auto* row = canvas.ptr<cv::Vec3b>(y);
        for (int x = 0; x < shape.width; ++x) {
            for (int c = 0; c < 3; ++c) {
                tensor_values[c * plane + y * shape.width + x] = row[x][c];
            }
        }
    }
    return tensor_values;
}

static std::string label_name(int label_id) {
    if (label_id >= 0 && label_id < static_cast<int>(COCO_LABELS.size())) {
        return COCO_LABELS[label_id];
    }
    return "class_" + std::to_string(label_id);
}

static cv::Mat extract_mask(const Ort::Value& mask_tensor, size_t index,
                            int model_h, int model_w,
                            const cv::Size& original_size,
                            float threshold) {
    auto info = mask_tensor.GetTensorTypeAndShapeInfo();
    const auto shape = info.GetShape();
    const float* values = mask_tensor.GetTensorData<float>();

    cv::Mat model_mask(model_h, model_w, CV_8UC1);
    const size_t offset = index * static_cast<size_t>(model_h) * model_w;

    for (int y = 0; y < model_h; ++y) {
        auto* row = model_mask.ptr<uint8_t>(y);
        for (int x = 0; x < model_w; ++x) {
            row[x] = values[offset + static_cast<size_t>(y) * model_w + x] >= threshold
                ? 255 : 0;
        }
    }

    cv::Mat original_mask;
    cv::resize(model_mask, original_mask, original_size, 0, 0, cv::INTER_NEAREST);
    return original_mask;
}

static void draw_detection(cv::Mat& marked, const Detection& detection, const cv::Scalar& color) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(detection.mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat overlay = marked.clone();
    overlay.setTo(color, detection.mask);
    cv::addWeighted(overlay, 0.28, marked, 0.72, 0.0, marked);
    cv::drawContours(marked, contours, -1, color, 3, cv::LINE_AA);

    cv::Mat mask_u8;
    if (detection.mask.type() != CV_8UC1) {
        detection.mask.convertTo(mask_u8, CV_8UC1);
    } else {
        mask_u8 = detection.mask;
    }

    // Compute the mask bounding box without cv::boundingRect().
    // This avoids OpenCV header/version mismatches where that API is not exposed.
    cv::Rect box(0, 0, marked.cols, marked.rows);
    std::vector<cv::Point> mask_points;
    cv::findNonZero(mask_u8, mask_points);
    if (!mask_points.empty()) {
        int min_x = marked.cols;
        int min_y = marked.rows;
        int max_x = 0;
        int max_y = 0;
        for (const cv::Point& point : mask_points) {
            min_x = std::min(min_x, point.x);
            min_y = std::min(min_y, point.y);
            max_x = std::max(max_x, point.x);
            max_y = std::max(max_y, point.y);
        }
        box = cv::Rect(min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    }

    const std::string text = detection.label + " " +
        cv::format("%.2f", detection.score);

    int baseline = 0;
    const cv::Size text_size = cv::getTextSize(
        text, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseline);

    const int x = std::max(0, box.x);
    const int y = std::max(text_size.height + 8, box.y);
    cv::rectangle(marked,
        cv::Rect(x, y - text_size.height - 10,
                 text_size.width + 12, text_size.height + 12),
        color, cv::FILLED);
    cv::putText(marked, text, cv::Point(x + 6, y - 5),
        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
}

static json make_json(const cv::Mat& image, const fs::path& input_path,
                      const fs::path& model_path, const std::vector<Detection>& detections) {
    json output;
    output["image_height"] = image.rows;
    output["image_width"] = image.cols;
    output["input"] = fs::absolute(input_path).string();
    output["instance_id_encoding"] = "PNG value = segment id + 1; zero means background";
    output["model"] = fs::absolute(model_path).string();
    output["segments"] = json::array();

    for (const auto& detection : detections) {
        output["segments"].push_back({
            {"id", detection.id},
            {"label", detection.label},
            {"label_id", detection.label_id},
            {"score", detection.score},
            {"was_fused", false}
        });
    }
    return output;
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 3 || argc > 5) {
            std::cerr << "Usage: " << argv[0]
                      << " <input.jpg> <model.onnx> [output.json] [marked.png]\n";
            return 1;
        }

        const fs::path input_path = fs::absolute(argv[1]);
        const fs::path model_path = fs::absolute(argv[2]);
        const fs::path json_path = argc >= 4 ? fs::path(argv[3]) : fs::path("segments.json");
        const fs::path marked_path = argc >= 5 ? fs::path(argv[4]) : fs::path("marked.png");
        const fs::path instance_path = marked_path.parent_path() / "instances.png";

        if (!fs::exists(input_path)) fail("Input image does not exist: " + input_path.string());
        if (!fs::exists(model_path)) fail("ONNX model does not exist: " + model_path.string());

        cv::Mat image = cv::imread(input_path.string(), cv::IMREAD_COLOR);
        if (image.empty()) fail("Unable to decode image: " + input_path.string());

        InputShape input_shape;
        const auto input_values = preprocess(image, input_shape);

        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "InstanceSegmentation");
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(std::max(1U, std::thread::hardware_concurrency()));
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        Ort::Session session(env, model_path.c_str(), options);
        Ort::AllocatorWithDefaultOptions allocator;

        if (session.GetInputCount() < 1 || session.GetOutputCount() < 3) {
            fail("The ONNX model must have at least one input and three outputs: masks, labels, scores");
        }

        auto input_name = session.GetInputNameAllocated(0, allocator);
        std::vector<const char*> input_names = {input_name.get()};

        auto masks_name = session.GetOutputNameAllocated(0, allocator);
        auto labels_name = session.GetOutputNameAllocated(1, allocator);
        auto scores_name = session.GetOutputNameAllocated(2, allocator);
        std::vector<const char*> output_names = {
            masks_name.get(), labels_name.get(), scores_name.get()
        };

        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> input_dims = {3, input_shape.height, input_shape.width};
        Ort::Value input_tensor = Ort::Value::CreateTensor<uint8_t>(
            memory_info, const_cast<uint8_t*>(input_values.data()), input_values.size(),
            input_dims.data(), input_dims.size());

        auto outputs = session.Run(Ort::RunOptions{nullptr}, input_names.data(),
                                   &input_tensor, 1, output_names.data(), output_names.size());

        auto mask_info = outputs[0].GetTensorTypeAndShapeInfo();
        auto mask_shape = mask_info.GetShape();
        if (mask_shape.size() != 3) {
            fail("Expected masks output with shape [N,H,W]");
        }

        const size_t count = static_cast<size_t>(mask_shape[0]);
        const int mask_h = static_cast<int>(mask_shape[1]);
        const int mask_w = static_cast<int>(mask_shape[2]);
        const int64_t* labels = outputs[1].GetTensorData<int64_t>();
        const float* scores = outputs[2].GetTensorData<float>();

        constexpr float score_threshold = 0.50f;
        constexpr float mask_threshold = 0.50f;
        std::vector<Detection> detections;
        cv::Mat instance_map = cv::Mat::zeros(image.size(), CV_16UC1);
        cv::Mat marked = image.clone();

        std::mt19937 generator(12345);
        std::uniform_int_distribution<int> color_value(60, 255);

        for (size_t i = 0; i < count; ++i) {
            if (scores[i] < score_threshold) continue;

            Detection detection;
            detection.id = static_cast<int>(detections.size());
            detection.label_id = static_cast<int>(labels[i]);
            detection.label = label_name(detection.label_id);
            detection.score = scores[i];
            detection.mask = extract_mask(outputs[0], i, mask_h, mask_w,
                                          image.size(), mask_threshold);

            if (cv::countNonZero(detection.mask) == 0) continue;
            detections.push_back(detection);

            const uint16_t instance_value = static_cast<uint16_t>(detection.id + 1);
            instance_map.setTo(instance_value, detection.mask);

            const cv::Scalar color(color_value(generator), color_value(generator), color_value(generator));
            draw_detection(marked, detection, color);
        }

        json result = make_json(image, input_path, model_path, detections);
        std::ofstream json_file(json_path);
        if (!json_file) fail("Unable to write JSON file: " + json_path.string());
        json_file << result.dump(2) << '\n';

        if (!cv::imwrite(marked_path.string(), marked)) {
            fail("Unable to write marked image: " + marked_path.string());
        }
        if (!cv::imwrite(instance_path.string(), instance_map)) {
            fail("Unable to write instance PNG: " + instance_path.string());
        }

        std::cout << "Detected instances: " << detections.size() << '\n';
        std::cout << "JSON: " << json_path << '\n';
        std::cout << "Marked image: " << marked_path << '\n';
        std::cout << "Instance PNG: " << instance_path << '\n';
        return 0;
    }
    catch (const Ort::Exception& error) {
        std::cerr << "ONNX Runtime error: " << error.what() << '\n';
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
    }
    return 1;
}
