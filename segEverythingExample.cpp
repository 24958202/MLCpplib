// segEverythingExample.cpp
//
// Segment all instances matching a text prompt, then blur their pixels.
//
// Usage:
//   ./segEverythingExample image_encoder.onnx language_encoder.onnx decoder.onnx \
//        prompt_tokens.txt input.jpg output.jpg [confidence_threshold]
//
// prompt_tokens.txt must contain exactly 32 int64 token IDs generated using
// SAM3's compatible CLIP tokenizer. Example prompt: "person".

#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct FloatTensor {
    std::vector<float> data;
    std::vector<int64_t> shape;
};

class Sam3Redactor {
public:
    Sam3Redactor(const std::string& imageEncoderPath,
                 const std::string& languageEncoderPath,
                 const std::string& decoderPath)
        : env_(ORT_LOGGING_LEVEL_WARNING, "sam3-cpp"),
          options_(),
          imageEncoder_(nullptr),
          languageEncoder_(nullptr),
          decoder_(nullptr) {
        options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        imageEncoder_ = Ort::Session(env_, imageEncoderPath.c_str(), options_);
        languageEncoder_ = Ort::Session(env_, languageEncoderPath.c_str(), options_);
        decoder_ = Ort::Session(env_, decoderPath.c_str(), options_);

        imageInputNames_ = getNames(imageEncoder_, true);
        imageOutputNames_ = getNames(imageEncoder_, false);
        languageInputNames_ = getNames(languageEncoder_, true);
        languageOutputNames_ = getNames(languageEncoder_, false);
        decoderInputNames_ = getNames(decoder_, true);
        decoderOutputNames_ = getNames(decoder_, false);

        printModelInfo("Image encoder", imageEncoder_);
        printModelInfo("Language encoder", languageEncoder_);
        printModelInfo("Decoder", decoder_);
    }

    cv::Mat redact(const cv::Mat& sourceBgr,
                   const std::vector<int64_t>& tokens,
                   float threshold) {
        if (sourceBgr.empty()) {
            throw std::runtime_error("The input image is empty or unreadable.");
        }
        if (tokens.size() != 32) {
            throw std::runtime_error("The token file must contain exactly 32 integers.");
        }
        if (threshold < 0.0f || threshold > 1.0f) {
            throw std::runtime_error("confidence_threshold must be in the range 0.0 to 1.0.");
        }

        const auto vision = runImageEncoder(sourceBgr);
        const auto language = runLanguageEncoder(tokens);
        auto outputs = runDecoder(sourceBgr.rows, sourceBgr.cols, vision, language);

        const Ort::Value* scoreTensor = findOutput(outputs, {"scores", "score"});
        const Ort::Value* maskTensor = findOutput(outputs, {"masks", "mask"});

        // Fall back to standard SAM3 output indices if names differ
        if (scoreTensor == nullptr && outputs.size() >= 2) {
            scoreTensor = &outputs[1];
        }
        if (maskTensor == nullptr && outputs.size() >= 3) {
            maskTensor = &outputs[2];
        }
        if (scoreTensor == nullptr || maskTensor == nullptr) {
            throw std::runtime_error("Could not find decoder score and mask outputs.");
        }

        cv::Mat combinedMask = mergeMasks(*maskTensor, *scoreTensor,
                                          sourceBgr.cols, sourceBgr.rows,
                                          threshold);

        cv::Mat blurred;
        cv::GaussianBlur(sourceBgr, blurred, cv::Size(61, 61), 0.0);

        cv::Mat result = sourceBgr.clone();
        blurred.copyTo(result, combinedMask);
        return result;
    }

private:
    Ort::Env env_;
    Ort::SessionOptions options_;
    Ort::Session imageEncoder_;
    Ort::Session languageEncoder_;
    Ort::Session decoder_;

    std::vector<std::string> imageInputNames_;
    std::vector<std::string> imageOutputNames_;
    std::vector<std::string> languageInputNames_;
    std::vector<std::string> languageOutputNames_;
    std::vector<std::string> decoderInputNames_;
    std::vector<std::string> decoderOutputNames_;

    static std::vector<std::string> getNames(const Ort::Session& session, bool input) {
        Ort::AllocatorWithDefaultOptions allocator;
        const size_t count = input ? session.GetInputCount() : session.GetOutputCount();
        std::vector<std::string> names;
        names.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            auto name = input ? session.GetInputNameAllocated(i, allocator)
                              : session.GetOutputNameAllocated(i, allocator);
            names.emplace_back(name.get());
        }
        return names;
    }

    static std::vector<const char*> cStrings(const std::vector<std::string>& names) {
        std::vector<const char*> result;
        result.reserve(names.size());
        for (const auto& name : names) {
            result.push_back(name.c_str());
        }
        return result;
    }

    static void printOneSide(const Ort::Session& session, bool input) {
        Ort::AllocatorWithDefaultOptions allocator;
        const size_t count = input ? session.GetInputCount() : session.GetOutputCount();

        for (size_t i = 0; i < count; ++i) {
            auto name = input ? session.GetInputNameAllocated(i, allocator)
                              : session.GetOutputNameAllocated(i, allocator);

            Ort::TypeInfo typeInfo = input ? session.GetInputTypeInfo(i)
                                           : session.GetOutputTypeInfo(i);
            const auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            const auto shape = tensorInfo.GetShape();

            std::cout << "  " << name.get() << "  shape: [";
            for (size_t j = 0; j < shape.size(); ++j) {
                std::cout << shape[j] << (j + 1 == shape.size() ? "" : ", ");
            }
            std::cout << "]  type: "
                      << static_cast<int>(tensorInfo.GetElementType()) << "\n";
        }
    }

    static void printModelInfo(const std::string& title, const Ort::Session& session) {
        std::cout << "\n" << title << " inputs:\n";
        printOneSide(session, true);
        std::cout << title << " outputs:\n";
        printOneSide(session, false);
    }

    static FloatTensor copyFloat(const Ort::Value& value) {
        const auto info = value.GetTensorTypeAndShapeInfo();
        if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            throw std::runtime_error("Expected a float tensor from the ONNX model.");
        }
        const float* ptr = value.GetTensorData<float>();
        const size_t count = info.GetElementCount();
        return {{ptr, ptr + count}, info.GetShape()};
    }

    static Ort::Value makeFloatTensor(const Ort::MemoryInfo& memory,
                                      const FloatTensor& tensor) {
        return Ort::Value::CreateTensor<float>(
            memory,
            const_cast<float*>(tensor.data.data()),
            tensor.data.size(),
            tensor.shape.data(),
            tensor.shape.size());
    }

    // Aspect-ratio preserving letterbox preprocessing for uint8 NCHW format
    static std::vector<uint8_t> preprocessLetterbox(const cv::Mat& bgr, int targetWidth, int targetHeight) {
        if (bgr.empty()) {
            throw std::runtime_error("Cannot decode input image for preprocessing.");
        }

        const float scale = std::min(
            static_cast<float>(targetWidth) / static_cast<float>(bgr.cols),
            static_cast<float>(targetHeight) / static_cast<float>(bgr.rows));

        const int resized_w = std::min(targetWidth, std::max(1, static_cast<int>(std::round(bgr.cols * scale))));
        const int resized_h = std::min(targetHeight, std::max(1, static_cast<int>(std::round(bgr.rows * scale))));

        cv::Mat rgb, resized;
        cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
        cv::resize(rgb, resized, cv::Size(resized_w, resized_h), 0.0, 0.0, cv::INTER_LINEAR);

        // Pad canvas with black (zeros)
        cv::Mat canvas = cv::Mat::zeros(targetHeight, targetWidth, CV_8UC3);
        resized.copyTo(canvas(cv::Rect(0, 0, resized_w, resized_h)));

        std::vector<uint8_t> chw(3 * targetHeight * targetWidth, 0);
        const int stride = targetHeight * targetWidth;
        for (int y = 0; y < targetHeight; ++y) {
            const auto* row = canvas.ptr<cv::Vec3b>(y);
            for (int x = 0; x < targetWidth; ++x) {
                chw[y * targetWidth + x]                     = row[x][0]; // R
                chw[stride + y * targetWidth + x]            = row[x][1]; // G
                chw[2 * stride + y * targetWidth + x]        = row[x][2]; // B
            }
        }
        return chw;
    }

    std::unordered_map<std::string, FloatTensor> runImageEncoder(const cv::Mat& bgr) {
        if (imageInputNames_.empty()) {
            throw std::runtime_error("This example expects at least one image-encoder input.");
        }

        Ort::TypeInfo typeInfo = imageEncoder_.GetInputTypeInfo(0);
        const auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        const auto shape = tensorInfo.GetShape();

        const int height = (shape.size() >= 2 && shape[1] > 0) ? static_cast<int>(shape[1]) : 1008;
        const int width = (shape.size() >= 3 && shape[2] > 0) ? static_cast<int>(shape[2]) : 1008;

        auto image = preprocessLetterbox(bgr, width, height);

        const std::array<int64_t, 3> inputShape{3, height, width};
        const auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input = Ort::Value::CreateTensor<uint8_t>(
            memory, image.data(), image.size(), inputShape.data(), inputShape.size());

        const auto inputNames = cStrings(imageInputNames_);
        const auto outputNames = cStrings(imageOutputNames_);
        auto outputValues = imageEncoder_.Run(Ort::RunOptions{nullptr},
                                             inputNames.data(), &input, 1,
                                             outputNames.data(), outputNames.size());

        if (outputValues.size() != imageOutputNames_.size()) {
            throw std::runtime_error("Image encoder returned an unexpected number of feature tensors.");
        }

        // Dynamically bind output tensor values to their actual runtime output names
        std::unordered_map<std::string, FloatTensor> result;
        for (size_t i = 0; i < imageOutputNames_.size(); ++i) {
            result.emplace(imageOutputNames_[i], copyFloat(outputValues[i]));
        }
        return result;
    }

    std::unordered_map<std::string, FloatTensor> runLanguageEncoder(
        const std::vector<int64_t>& tokens) {
        if (languageInputNames_.empty()) {
            throw std::runtime_error("This example expects at least one language-encoder input.");
        }

        const std::array<int64_t, 2> shape{1, 32};
        const auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input = Ort::Value::CreateTensor<int64_t>(
            memory, const_cast<int64_t*>(tokens.data()), tokens.size(),
            shape.data(), shape.size());

        const auto inputNames = cStrings(languageInputNames_);
        const auto outputNames = cStrings(languageOutputNames_);
        auto outputValues = languageEncoder_.Run(Ort::RunOptions{nullptr},
                                               inputNames.data(), &input, 1,
                                               outputNames.data(), outputNames.size());

        if (outputValues.size() < 3) {
            throw std::runtime_error("Language encoder returned fewer than three tensors.");
        }

        std::unordered_map<std::string, FloatTensor> result;
        result.emplace("language_features", copyFloat(outputValues[1]));
        result.emplace("language_embeds", copyFloat(outputValues[2]));

        const auto maskInfo = outputValues[0].GetTensorTypeAndShapeInfo();
        if (maskInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL ||
            maskInfo.GetElementCount() != 32) {
            throw std::runtime_error("Unexpected language attention-mask type or shape.");
        }
        const bool* mask = outputValues[0].GetTensorData<bool>();
        std::copy(mask, mask + 32, languageMask_.begin());
        return result;
    }

    std::vector<Ort::Value> runDecoder(
        int originalHeight,
        int originalWidth,
        const std::unordered_map<std::string, FloatTensor>& vision,
        const std::unordered_map<std::string, FloatTensor>& language) {
        const auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

        int64_t originalHeightValue = originalHeight;
        int64_t originalWidthValue = originalWidth;
        float boxCoords[] = {0.0f, 0.0f, 0.0f, 0.0f};
        int64_t boxLabels[] = {1};
        bool boxMasks[] = {true}; // true indicates a dummy/no geometric prompt

        const std::array<int64_t, 3> boxCoordsShape{1, 1, 4};
        const std::array<int64_t, 2> oneValueShape{1, 1};
        std::vector<Ort::Value> inputs;
        inputs.reserve(decoderInputNames_.size());

        for (const std::string& name : decoderInputNames_) {
            if (name == "original_height") {
                inputs.emplace_back(Ort::Value::CreateTensor<int64_t>(
                    memory, &originalHeightValue, 1, nullptr, 0));
            } else if (name == "original_width") {
                inputs.emplace_back(Ort::Value::CreateTensor<int64_t>(
                    memory, &originalWidthValue, 1, nullptr, 0));
            } else if (name == "box_coords") {
                inputs.emplace_back(Ort::Value::CreateTensor<float>(
                    memory, boxCoords, 4, boxCoordsShape.data(), boxCoordsShape.size()));
            } else if (name == "box_labels") {
                inputs.emplace_back(Ort::Value::CreateTensor<int64_t>(
                    memory, boxLabels, 1, oneValueShape.data(), oneValueShape.size()));
            } else if (name == "box_masks") {
                inputs.emplace_back(Ort::Value::CreateTensor<bool>(
                    memory, boxMasks, 1, oneValueShape.data(), oneValueShape.size()));
            } else if (name == "language_mask") {
                const std::array<int64_t, 2> maskShape{1, 32};
                inputs.emplace_back(Ort::Value::CreateTensor<bool>(
                    memory, languageMask_.data(), languageMask_.size(),
                    maskShape.data(), maskShape.size()));
            } else if (const auto visual = vision.find(name); visual != vision.end()) {
                inputs.emplace_back(makeFloatTensor(memory, visual->second));
            } else if (const auto text = language.find(name); text != language.end()) {
                inputs.emplace_back(makeFloatTensor(memory, text->second));
            } else {
                throw std::runtime_error("Unsupported required decoder input: " + name);
            }
        }

        const auto inputNames = cStrings(decoderInputNames_);
        const auto outputNames = cStrings(decoderOutputNames_);
        return decoder_.Run(Ort::RunOptions{nullptr},
                            inputNames.data(), inputs.data(), inputs.size(),
                            outputNames.data(), outputNames.size());
    }

    const Ort::Value* findOutput(const std::vector<Ort::Value>& outputs,
                                   const std::vector<std::string>& wanted) const {
        for (size_t i = 0; i < decoderOutputNames_.size() && i < outputs.size(); ++i) {
            for (const auto& name : wanted) {
                if (decoderOutputNames_[i] == name) {
                    return &outputs[i];
                }
            }
        }
        return nullptr;
    }

    static cv::Mat mergeMasks(const Ort::Value& masksValue,
                            const Ort::Value& scoresValue,
                            int outputWidth,
                            int outputHeight,
                            float threshold) {
        const auto maskInfo = masksValue.GetTensorTypeAndShapeInfo();
        const auto scoreInfo = scoresValue.GetTensorTypeAndShapeInfo();
        const auto maskShape = maskInfo.GetShape();

        if (maskInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL ||
            scoreInfo.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
            maskShape.size() != 4 || maskShape[1] != 1) {
            throw std::runtime_error("Unexpected SAM3 decoder mask or score tensor type/shape.");
        }

        const int count = static_cast<int>(maskShape[0]);
        const int maskHeight = static_cast<int>(maskShape[2]);
        const int maskWidth = static_cast<int>(maskShape[3]);
        const bool* masks = masksValue.GetTensorData<bool>();
        const float* scores = scoresValue.GetTensorData<float>();
        const size_t pixels = static_cast<size_t>(maskHeight) * maskWidth;

        cv::Mat combined(maskHeight, maskWidth, CV_8UC1, cv::Scalar(0));
        int accepted = 0;
        size_t foregroundPixels = 0;
        float minScore = count > 0 ? scores[0] : 0.0f;
        float maxScore = minScore;

        for (int n = 0; n < count; ++n) {
            minScore = std::min(minScore, scores[n]);
            maxScore = std::max(maxScore, scores[n]);
            if (scores[n] < threshold) {
                continue;
            }
            ++accepted;
            const bool* current = masks + static_cast<size_t>(n) * pixels;
            for (int y = 0; y < maskHeight; ++y) {
                auto* row = combined.ptr<uint8_t>(y);
                for (int x = 0; x < maskWidth; ++x) {
                    if (current[static_cast<size_t>(y) * maskWidth + x]) {
                        if (row[x] == 0) {
                            ++foregroundPixels;
                        }
                        row[x] = 255;
                    }
                }
            }
        }

        std::cout << "\nDecoder detections: " << count
                  << ", score range: [" << minScore << ", " << maxScore << "]"
                  << ", accepted at threshold " << threshold << ": " << accepted
                  << ", mask foreground pixels: " << foregroundPixels << "\n";

        if (combined.cols != outputWidth || combined.rows != outputHeight) {
            cv::resize(combined, combined, cv::Size(outputWidth, outputHeight),
                       0.0, 0.0, cv::INTER_NEAREST);
        }
        return combined;
    }

    std::array<bool, 32> languageMask_{};
};

static std::vector<int64_t> loadTokens(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open token file: " + path);
    }

    std::vector<int64_t> tokens;
    int64_t value = 0;
    while (file >> value) {
        tokens.push_back(value);
    }
    if (tokens.size() != 32) {
        throw std::runtime_error("Token file must contain 32 integers; found " +
                                 std::to_string(tokens.size()) + ".");
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    if (argc < 7 || argc > 8) {
        std::cerr << "Usage:\n  " << argv[0]
                  << " sam3_image_encoder.onnx sam3_language_encoder.onnx sam3_decoder.onnx"
                  << " prompt_tokens.txt input.jpg output.jpg [confidence_threshold]\n";
        return 1;
    }

    try {
        const float threshold = argc == 8 ? std::stof(argv[7]) : 0.5f;
        const auto tokens = loadTokens(argv[4]);
        const cv::Mat source = cv::imread(argv[5], cv::IMREAD_COLOR);
        if (source.empty()) {
            throw std::runtime_error(std::string("Cannot load input image: ") + argv[5]);
        }

        Sam3Redactor redactor(argv[1], argv[2], argv[3]);
        const cv::Mat result = redactor.redact(source, tokens, threshold);

        if (!cv::imwrite(argv[6], result)) {
            throw std::runtime_error(std::string("Cannot write output image: ") + argv[6]);
        }
        std::cout << "\nSaved: " << argv[6] << "\n";
        return 0;
    } catch (const Ort::Exception& e) {
        std::cerr << "\nONNX Runtime error:\n" << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "\nError:\n" << e.what() << "\n";
    }
    return 2;
}