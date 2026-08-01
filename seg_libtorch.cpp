/*
 * 
Use /Users/jidengfeng/Downloads/MLCpplib/seg/segment_mask2former.py to convert 
 /Users/jidengfeng/Downloads/models/mask2former_liv to a /Users/jidengfeng/Downloads/models/mask2former_liv/mask2former.pt
file.

-I/Users/jidengfeng/Downloads/libtorch/include \
  -I/Users/jidengfeng/Downloads/libtorch/include/torch/csrc/api/include \
  -I/opt/homebrew/include \
  -I/opt/homebrew/include/opencv5 \
  -L/Users/jidengfeng/Downloads/libtorch/lib \
  -L/opt/homebrew/lib \
  -ltorch -ltorch_cpu -lc10 \
  -lopencv_core -lopencv_imgproc -lopencv_imgcodecs \
  -Wl,-rpath,/Users/jidengfeng/Downloads/libtorch/lib
   * 
   * 
   * 
Re-registerlib torch:
1. Remove the quarantine flag from LibTorch
This tells macOS to stop treating the downloaded LibTorch folder as an untrusted internet download.

xattr -dr com.apple.quarantine /Users/jidengfeng/Downloads/libtorch

2. Re-sign the LibTorch dynamic libraries
Apply a local ad-hoc signature to all .dylib files inside LibTorch:

codesign --force --deep --sign - /Users/jidengfeng/Downloads/libtorch/lib/*.dylib

3. Re-sign your compiled binary
Apply the same local signature to your executable:

codesign --force --deep --sign - /Users/jidengfeng/Downloads/MLCpplib/seg/seg_libtorch


Fixed run:

export KMP_DUPLICATE_LIB_OK=TRUE

/Users/jidengfeng/Downloads/MLCpplib/seg/seg_libtorch \
  /Users/jidengfeng/Downloads/IMG_6957.jpg \
  /Users/jidengfeng/Downloads/models/mask2former_liv/mask2former.pt \
  /Users/jidengfeng/Downloads/models/mask2former_liv \
  /Users/jidengfeng/output_test

 * 
*/
// mask2former_libtorch.cpp
// C++20 Mask2Former instance segmentation using a TorchScript/LibTorch model.
//
// Usage:
//   mask2former_libtorch input.jpg mask2former.pt model_directory output_prefix

#include <torch/script.h>
#include <torch/torch.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct ProcessorConfig {
    std::array<float, 3> mean{0.485F, 0.456F, 0.406F};
    std::array<float, 3> stddev{0.229F, 0.224F, 0.225F};
};

struct FixedInputShape {
    int height = 0;
    int width = 0;
};

struct Segment {
    int id;
    int label_id;
    float score;
};

struct PreprocessedImage {
    torch::Tensor pixel_values; // Float32, [1, 3, H, W], RGB normalized.
    torch::Tensor pixel_mask;   // Int64, [1, H, W], 1 valid / 0 padding.
};

[[noreturn]] static void fail(const std::string& message) {
    throw std::runtime_error(message);
}

static json read_json(const fs::path& path) {
    std::ifstream file(path);
    if (!file) fail("Cannot open: " + path.string());
    json result;
    file >> result;
    return result;
}

static std::array<float, 3> read_rgb_triplet(
    const json& j, const char* key, std::array<float, 3> fallback) {
    if (!j.contains(key)) return fallback;
    const auto& values = j.at(key);
    if (values.is_number()) {
        const float value = values.get<float>();
        return {value, value, value};
    }
    if (!values.is_array() || values.size() != 3) {
        fail(std::string(key) + " must contain three values");
    }
    return {values[0].get<float>(), values[1].get<float>(), values[2].get<float>()};
}

static ProcessorConfig load_processor_config(const fs::path& model_dir) {
    const fs::path path = model_dir / "preprocessor_config.json";
    if (!fs::exists(path)) fail("Missing: " + path.string());

    const json j = read_json(path);
    ProcessorConfig cfg;
    cfg.mean = read_rgb_triplet(j, "image_mean", cfg.mean);
    cfg.stddev = read_rgb_triplet(j, "image_std", cfg.stddev);
    return cfg;
}

static FixedInputShape load_fixed_input_shape(const fs::path& torchscript_path) {
    fs::path metadata_path = torchscript_path;
    metadata_path.replace_extension(".json");
    if (!fs::exists(metadata_path)) {
        fail("Missing TorchScript metadata file: " + metadata_path.string() +
             ". Re-run export_mask2former_torchscript.py.");
    }

    const json metadata = read_json(metadata_path);
    if (!metadata.contains("input") ||
        !metadata.at("input").contains("pixel_values_shape")) {
        fail("Invalid TorchScript metadata: pixel_values_shape is missing");
    }

    const auto& shape = metadata.at("input").at("pixel_values_shape");
    if (!shape.is_array() || shape.size() != 4 ||
        shape[0].get<int>() != 1 || shape[1].get<int>() != 3) {
        fail("Expected pixel_values_shape [1, 3, H, W] in " + metadata_path.string());
    }

    FixedInputShape result{shape[2].get<int>(), shape[3].get<int>()};
    if (result.height <= 0 || result.width <= 0) fail("Invalid fixed input dimensions");
    return result;
}

static std::vector<std::string> load_labels(const fs::path& model_dir) {
    const json j = read_json(model_dir / "config.json");
    if (!j.contains("id2label")) return {};

    const auto& map = j.at("id2label");
    int largest_id = -1;
    for (auto it = map.begin(); it != map.end(); ++it) {
        largest_id = std::max(largest_id, std::stoi(it.key()));
    }

    std::vector<std::string> labels(static_cast<size_t>(largest_id + 1));
    for (auto it = map.begin(); it != map.end(); ++it) {
        labels[static_cast<size_t>(std::stoi(it.key()))] = it.value().get<std::string>();
    }
    return labels;
}

// The TorchScript model was traced at exactly fixed_shape. For arbitrary input
// images, preserve aspect ratio, resize to fit the static canvas, and pad the
// right/bottom edge with zeros. The pixel mask marks only image pixels as valid.
static PreprocessedImage preprocess(const cv::Mat& bgr, const ProcessorConfig& cfg,
                                    const FixedInputShape fixed_shape) {
    if (bgr.empty()) fail("Cannot decode input image");

    const float scale = std::min(
        static_cast<float>(fixed_shape.width) / static_cast<float>(bgr.cols),
        static_cast<float>(fixed_shape.height) / static_cast<float>(bgr.rows));
        
    const int resized_w = std::min(fixed_shape.width, std::max(1, static_cast<int>(std::round(bgr.cols * scale))));
    const int resized_h = std::min(fixed_shape.height, std::max(1, static_cast<int>(std::round(bgr.rows * scale))));

    cv::Mat rgb, resized;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    cv::resize(rgb, resized, cv::Size(resized_w, resized_h), 0.0, 0.0, cv::INTER_LINEAR);

    auto pixels = torch::zeros(
        {1, 3, fixed_shape.height, fixed_shape.width},
        torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
    auto mask = torch::zeros(
        {1, fixed_shape.height, fixed_shape.width},
        torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU));

    auto pixel_data = pixels.accessor<float, 4>();
    auto mask_data = mask.accessor<int64_t, 3>();
    for (int y = 0; y < resized_h; ++y) {
        const auto* row = resized.ptr<cv::Vec3b>(y);
        for (int x = 0; x < resized_w; ++x) {
            for (int channel = 0; channel < 3; ++channel) {
                const float value = static_cast<float>(row[x][channel]) / 255.0F;
                pixel_data[0][channel][y][x] = (value - cfg.mean[channel]) / cfg.stddev[channel];
            }
            mask_data[0][y][x] = 1;
        }
    }
    return {pixels, mask};
}

static float sigmoid(float x) {
    if (x >= 0.0F) return 1.0F / (1.0F + std::exp(-x));
    const float e = std::exp(x);
    return e / (1.0F + e);
}

static std::vector<float> softmax(const float* values, int count) {
    const float maximum = *std::max_element(values, values + count);
    std::vector<float> result(static_cast<size_t>(count));
    float sum = 0.0F;
    for (int i = 0; i < count; ++i) {
        result[static_cast<size_t>(i)] = std::exp(values[i] - maximum);
        sum += result[static_cast<size_t>(i)];
    }
    for (float& value : result) value /= sum;
    return result;
}

static std::string label_for(const std::vector<std::string>& labels, int class_id) {
    if (class_id >= 0 && class_id < static_cast<int>(labels.size()) && !labels[class_id].empty()) {
        return labels[class_id];
    }
    return "class_" + std::to_string(class_id);
}

static std::pair<cv::Mat, std::vector<Segment>> postprocess_instance(
    const float* class_logits, int queries, int classes_with_null,
    const float* mask_logits, int mask_h, int mask_w,
    int output_h, int output_w, FixedInputShape fixed_shape, float threshold = 0.5F) {

    const int classes = classes_with_null - 1;
    if (classes <= 0) fail("Class output must contain a null class");

    struct Candidate { float class_score; int query; int class_id; };
    std::vector<Candidate> candidates;
    candidates.reserve(static_cast<size_t>(queries) * classes);
    for (int q = 0; q < queries; ++q) {
        const auto probabilities = softmax(
            class_logits + static_cast<size_t>(q) * classes_with_null, classes_with_null);
        for (int c = 0; c < classes; ++c) candidates.push_back({probabilities[c], q, c});
    }

    const int keep = std::min(queries, static_cast<int>(candidates.size()));
    std::partial_sort(candidates.begin(), candidates.begin() + keep, candidates.end(),
        [](const Candidate& a, const Candidate& b) { return a.class_score > b.class_score; });
    candidates.resize(static_cast<size_t>(keep));

    cv::Mat ids(output_h, output_w, CV_32SC1, cv::Scalar(-1));
    std::vector<Segment> segments;
    int next_id = 0;

    for (const Candidate& candidate : candidates) {
        cv::Mat binary(mask_h, mask_w, CV_8UC1);
        const float* source = mask_logits + static_cast<size_t>(candidate.query) * mask_h * mask_w;
        float probability_sum = 0.0F;
        int foreground_count = 0;

        for (int y = 0; y < mask_h; ++y) {
            auto* row = binary.ptr<uint8_t>(y);
            for (int x = 0; x < mask_w; ++x) {
                const float logit = source[static_cast<size_t>(y) * mask_w + x];
                row[x] = logit > 0.0F ? 255 : 0;
                if (row[x] != 0) {
                    probability_sum += sigmoid(logit);
                    ++foreground_count;
                }
            }
        }
        if (foreground_count == 0) continue;

        const float score = candidate.class_score *
            probability_sum / (static_cast<float>(foreground_count) + 1e-6F);
        if (score < threshold) continue;

        // 1. Calculate the scaled dimensions used during preprocessing
        const float scale = std::min(
            static_cast<float>(fixed_shape.width) / static_cast<float>(output_w),
            static_cast<float>(fixed_shape.height) / static_cast<float>(output_h));
        
        const int resized_w = std::min(fixed_shape.width, std::max(1, static_cast<int>(std::round(output_w * scale))));
        const int resized_h = std::min(fixed_shape.height, std::max(1, static_cast<int>(std::round(output_h * scale))));

        // 2. Map those dimensions to the model's mask output scale (e.g., H/4, W/4)
        const int valid_mask_w = std::min(mask_w, std::max(1, static_cast<int>(std::round(resized_w * (static_cast<float>(mask_w) / fixed_shape.width)))));
        const int valid_mask_h = std::min(mask_h, std::max(1, static_cast<int>(std::round(resized_h * (static_cast<float>(mask_h) / fixed_shape.height)))));

        // 3. Crop out the padding, THEN resize back to the original image size
        cv::Mat resized_mask;
        cv::Rect valid_roi(0, 0, valid_mask_w, valid_mask_h);
        cv::resize(binary(valid_roi), resized_mask, cv::Size(output_w, output_h), 0.0, 0.0, cv::INTER_NEAREST);
        
        for (int y = 0; y < output_h; ++y) {
            const auto* source_row = resized_mask.ptr<uint8_t>(y);
            auto* id_row = ids.ptr<int>(y);
            for (int x = 0; x < output_w; ++x) {
                if (source_row[x] != 0) id_row[x] = next_id;
            }
        }
        segments.push_back({next_id++, candidate.class_id, score});
    }
    return {ids, segments};
}

static cv::Vec3b color_for(int class_id) {
    const int r = (class_id * 67 + 37) % 256;
    const int g = (class_id * 131 + 83) % 256;
    const int b = (class_id * 197 + 149) % 256;
    return {static_cast<uint8_t>(b), static_cast<uint8_t>(g), static_cast<uint8_t>(r)};
}

static cv::Vec3b overlay_color_for(int class_id) {
    const int r = (37 * class_id + 71) % 256;
    const int g = (17 * class_id + 151) % 256;
    const int b = (97 * class_id + 29) % 256;
    return {static_cast<uint8_t>(b), static_cast<uint8_t>(g), static_cast<uint8_t>(r)};
}

static void write_outputs(const fs::path& prefix, const cv::Mat& original_bgr,
                          const cv::Mat& ids, const std::vector<Segment>& segments,
                          const std::vector<std::string>& labels, const fs::path& input_path,
                          const fs::path& model_path) {
    std::vector<int> id_to_class(segments.size(), -1);
    for (const auto& segment : segments) id_to_class[static_cast<size_t>(segment.id)] = segment.label_id;

    cv::Mat colored(ids.rows, ids.cols, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::Mat overlay = original_bgr.clone();
    for (int y = 0; y < ids.rows; ++y) {
        const auto* id_row = ids.ptr<int>(y);
        auto* color_row = colored.ptr<cv::Vec3b>(y);
        auto* overlay_row = overlay.ptr<cv::Vec3b>(y);
        for (int x = 0; x < ids.cols; ++x) {
            const int id = id_row[x];
            if (id < 0 || id >= static_cast<int>(id_to_class.size())) continue;
            const int class_id = id_to_class[static_cast<size_t>(id)];
            color_row[x] = color_for(class_id);
            const cv::Vec3b overlay_color = overlay_color_for(class_id);
            for (int c = 0; c < 3; ++c) {
                overlay_row[x][c] = static_cast<uint8_t>(
                    (155 * overlay_row[x][c] + 100 * overlay_color[c]) / 255);
            }
        }
    }

    // PNG value is instance_id + 1; zero remains the background value.
    cv::Mat ids_u16(ids.rows, ids.cols, CV_16UC1, cv::Scalar(0));
    for (int y = 0; y < ids.rows; ++y) {
        const auto* source = ids.ptr<int>(y);
        auto* destination = ids_u16.ptr<uint16_t>(y);
        for (int x = 0; x < ids.cols; ++x) {
            destination[x] = static_cast<uint16_t>(std::max(0, source[x] + 1));
        }
    }

    const fs::path ids_path = prefix.string() + "_instance_ids.png";
    const fs::path colored_path = prefix.string() + "_colored.png";
    const fs::path overlay_path = prefix.string() + "_overlay.png";
    const fs::path metadata_path = prefix.string() + "_segments.json";

    if (!cv::imwrite(ids_path.string(), ids_u16) ||
        !cv::imwrite(colored_path.string(), colored) ||
        !cv::imwrite(overlay_path.string(), overlay)) {
        fail("Could not write output PNG files");
    }

    json metadata;
    metadata["model"] = model_path.string();
    metadata["input"] = input_path.string();
    metadata["image_width"] = original_bgr.cols;
    metadata["image_height"] = original_bgr.rows;
    metadata["instance_id_encoding"] = "PNG value = segment id + 1; zero means background";
    metadata["segments"] = json::array();
    for (const auto& segment : segments) {
        metadata["segments"].push_back({
            {"id", segment.id},
            {"label_id", segment.label_id},
            {"label", label_for(labels, segment.label_id)},
            {"score", segment.score},
            {"was_fused", false}
        });
    }

    std::ofstream metadata_file(metadata_path);
    if (!metadata_file) fail("Cannot write: " + metadata_path.string());
    metadata_file << metadata.dump(2) << '\n';

    std::cout << "Detected " << segments.size() << " instances:\n";
    for (const auto& segment : segments) {
        std::cout << "  id=" << segment.id
                  << ", class=" << label_for(labels, segment.label_id)
                  << " (" << segment.label_id << ")"
                  << ", score=" << segment.score << '\n';
    }
    std::cout << "Instance-ID mask: " << ids_path
              << "\nColored mask:     " << colored_path
              << "\nOverlay:          " << overlay_path
              << "\nMetadata:         " << metadata_path << '\n';
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 5) {
            std::cerr << "Usage: " << argv[0]
                      << " <input_image> <mask2former.pt> <model_directory> <output_prefix>\n";
            return 1;
        }

        const fs::path input_path = fs::absolute(argv[1]);
        const fs::path torchscript_path = fs::absolute(argv[2]);
        const fs::path model_dir = fs::absolute(argv[3]);
        const fs::path output_prefix = fs::absolute(argv[4]);

        if (!fs::exists(input_path)) fail("Input image does not exist: " + input_path.string());
        if (!fs::exists(torchscript_path)) fail("TorchScript model does not exist: " + torchscript_path.string());
        if (!fs::exists(model_dir / "config.json")) fail("Missing config.json in: " + model_dir.string());
        if (output_prefix.has_parent_path()) fs::create_directories(output_prefix.parent_path());

        torch::NoGradGuard no_grad;
        torch::set_num_threads(std::max(1U, std::thread::hardware_concurrency()));

        const cv::Mat image = cv::imread(input_path.string(), cv::IMREAD_COLOR);
        if (image.empty()) fail("Cannot read input image: " + input_path.string());

        const auto processor_config = load_processor_config(model_dir);
        const auto fixed_shape = load_fixed_input_shape(torchscript_path);
        const auto labels = load_labels(model_dir);
        const auto input = preprocess(image, processor_config, fixed_shape);

        std::cout << "Loading TorchScript model: " << torchscript_path << '\n';
        auto model = torch::jit::load(torchscript_path.string(), torch::kCPU);
        model.eval();

        std::cout << "Running Mask2Former at fixed input size "
                  << fixed_shape.height << "x" << fixed_shape.width << "...\n";
        const torch::jit::IValue result = model.forward({input.pixel_values, input.pixel_mask});
        if (!result.isTuple()) fail("TorchScript model output must be a tuple");

        const auto& elements = result.toTuple()->elements();
        if (elements.size() != 2 || !elements[0].isTensor() || !elements[1].isTensor()) {
            fail("TorchScript output must be (class_queries_logits, masks_queries_logits)");
        }

        auto class_logits = elements[0].toTensor().to(torch::kCPU).contiguous().to(torch::kFloat32);
        auto mask_logits = elements[1].toTensor().to(torch::kCPU).contiguous().to(torch::kFloat32);
        if (class_logits.dim() != 3 || mask_logits.dim() != 4 ||
            class_logits.size(0) != 1 || mask_logits.size(0) != 1 ||
            class_logits.size(1) != mask_logits.size(1)) {
            fail("Unexpected output shapes; expected [1,Q,C+1] and [1,Q,H,W]");
        }

        const auto [ids, segments] = postprocess_instance(
            class_logits.data_ptr<float>(), static_cast<int>(class_logits.size(1)),
            static_cast<int>(class_logits.size(2)), mask_logits.data_ptr<float>(),
            static_cast<int>(mask_logits.size(2)), static_cast<int>(mask_logits.size(3)),
            image.rows, image.cols, fixed_shape);
        
        write_outputs(output_prefix, image, ids, segments, labels, input_path, torchscript_path);
        return 0;
    } catch (const c10::Error& error) {
        std::cerr << "LibTorch error: " << error.what() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
    }
    return 1;
}