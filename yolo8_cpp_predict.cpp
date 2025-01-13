#include <torch/script.h>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Mat> preprocessImage(const cv::Mat& img) {
    cv::Mat resized_img, float_img;
    cv::resize(img, resized_img, cv::Size(640, 640)); // Resize to 640x640
    resized_img.convertTo(float_img, CV_32F, 1.0 / 255.0); // Normalize to [0, 1]

    // Convert color from BGR to RGB
    cv::cvtColor(float_img, float_img, cv::COLOR_BGR2RGB);

    // Add a batch dimension
    std::vector<cv::Mat> batch = {float_img};
    return batch;
}

int main() {
    // Load the TorchScript model
    std::shared_ptr<torch::jit::script::Module> model;
    try {
        model = torch::jit::load("yolo8_traced.pt");
    } catch (const c10::Error& e) {
        std::cerr << "Error loading the model";
        return -1;
    }

    // Load an image using OpenCV
    cv::Mat img = cv::imread("path/to/your/image.jpg");
    if (img.empty()) {
        std::cerr << "Image not found!";
        return -1;
    }

    // Preprocess the image
    auto inputs = preprocessImage(img);

    // Convert to tensor
    auto input_tensor = torch::from_blob(inputs[0].data, {1, 3, 640, 640}, torch::kFloat).clone(); // Clone to maintain tensor ownership
    input_tensor = input_tensor.permute({0, 2, 3, 1}); // Change to NHWC format if needed
    input_tensor = input_tensor.contiguous();
    
    // Run inference
    std::vector<torch::jit::IValue> torch_inputs;
    torch_inputs.push_back(input_tensor);

    at::Tensor output = model->forward(torch_inputs).toTensor();

    // Process the output
    std::cout << "Output:" << output << std::endl;

    // Additional parsing of the output is needed to extract bounding boxes, classes, and scores

    return 0;
}
