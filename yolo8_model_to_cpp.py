import torch
from ultralytics import YOLO

# Load your trained YOLOv8 model
model = YOLO('path/to/your/best.pt')  # Change this to your trained weights

# Export to TorchScript
model.eval()  # Set the model to evaluation mode
example_input = torch.randn(1, 3, 640, 640)  # Example input
traced_model = torch.jit.trace(model.forward, example_input)
traced_model.save('yolo8_traced.pt')
