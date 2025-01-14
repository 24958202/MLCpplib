import torch
from ultralytics import YOLO

# Load your trained YOLOv8 model
model = YOLO('path/to/your/best.pt')  # Change this to your trained weights

# Export to TorchScript
model.eval()  # Set the model to evaluation mode
example_input = torch.randn(1, 3, 640, 640)  # Example input
traced_model = torch.jit.trace(model.forward, example_input)
traced_model.save('yolo8_traced.pt')


/*
use c++20 
*/

#include <torch/torch.h>
#include <torch/script.h> // For TorchScript
#include <iostream>

struct SimpleYOLO : torch::nn::Module {
    SimpleYOLO() {
        // Define layers (for illustrative purposes; this needs to match YOLOv8 architecture)
        conv1 = register_module("conv1", torch::nn::Conv2d(3, 32, /*kernel_size=*/3));
        conv2 = register_module("conv2", torch::nn::Conv2d(32, 64, /*kernel_size=*/3));
        // Add additional layers as needed
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(conv1->forward(x));
        x = torch::max_pool2d(x, 2);
        x = torch::relu(conv2->forward(x));
        x = torch::max_pool2d(x, 2);
        // Add additional forward operations as needed
        return x;
    }

    torch::nn::Conv2d conv1{nullptr}, conv2{nullptr}; // Declare layers
};

int main() {
    // Create a model instance
    SimpleYOLO model;

    // Set the model to evaluation mode (not strictly necessary in C++)
    model.eval(); 

    // Create a dummy input tensor (batch size of 1, 3 channels, 640x640 resolution)
    auto example_input = torch::randn({1, 3, 640, 640});

    // Use tracing to convert to TorchScript
    torch::jit::script::Module traced_model;
    try {
        traced_model = torch::jit::trace(model, example_input);
        traced_model.save("yolo8_model.pt"); // Save as TorchScript format
        std::cout << "Model has been exported to TorchScript format." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error during model tracing: " << e.what() << std::endl;
    }

    return 0;
}
