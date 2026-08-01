#!/usr/bin/env python3
"""
Export a local Hugging Face Mask2Former checkpoint to TorchScript for LibTorch C++.

Usage:
    python export_mask2former_torchscript.py \
      /Users/jidengfeng/Downloads/models/mask2former_liv \
      /Users/jidengfeng/Documents/image1.jpg \
      /Users/jidengfeng/Downloads/models/mask2former_liv/mask2former.pt
"""

import argparse
import json
from pathlib import Path

import torch
from PIL import Image
from transformers import AutoImageProcessor, Mask2FormerForUniversalSegmentation


class Mask2FormerInferenceWrapper(torch.nn.Module):
    """
    Produces the two tensors required for Mask2Former post-processing:

      class_queries_logits: [batch, num_queries, num_classes + 1]
      masks_queries_logits: [batch, num_queries, mask_height, mask_width]
    """

    def __init__(self, model: Mask2FormerForUniversalSegmentation):
        super().__init__()
        self.model = model

    def forward(
        self,
        pixel_values: torch.Tensor,
        pixel_mask: torch.Tensor,
    ) -> tuple[torch.Tensor, torch.Tensor]:
        outputs = self.model(
            pixel_values=pixel_values,
            pixel_mask=pixel_mask,
            return_dict=True,
        )

        return (
            outputs.class_queries_logits,
            outputs.masks_queries_logits,
        )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export a local Mask2Former model to TorchScript."
    )

    parser.add_argument(
        "model_directory",
        type=Path,
        help="Directory containing config.json and model weights.",
    )
    parser.add_argument(
        "example_image",
        type=Path,
        help="An image used to trace the model.",
    )
    parser.add_argument(
        "output_model",
        type=Path,
        help="Output TorchScript model path, normally mask2former.pt.",
    )

    return parser.parse_args()


def main() -> None:
    args = parse_arguments()

    model_directory = args.model_directory.expanduser().resolve()
    example_image_path = args.example_image.expanduser().resolve()
    output_model_path = args.output_model.expanduser().resolve()

    if not model_directory.is_dir():
        raise FileNotFoundError(
            f"Model directory does not exist: {model_directory}"
        )

    if not (model_directory / "config.json").is_file():
        raise FileNotFoundError(
            f"Missing config.json in model directory: {model_directory}"
        )

    if not example_image_path.is_file():
        raise FileNotFoundError(
            f"Example image does not exist: {example_image_path}"
        )

    output_model_path.parent.mkdir(parents=True, exist_ok=True)

    # Use CPU + FP32. This is the most portable option for LibTorch on macOS.
    device = torch.device("cpu")

    print(f"Loading image processor from: {model_directory}")
    processor = AutoImageProcessor.from_pretrained(
        str(model_directory),
        local_files_only=True,
    )

    print(f"Loading Mask2Former model from: {model_directory}")
    model = Mask2FormerForUniversalSegmentation.from_pretrained(
        str(model_directory),
        local_files_only=True,
    )

    model.to(device)
    model.float()
    model.eval()

    print(f"Reading example image: {example_image_path}")
    image = Image.open(example_image_path).convert("RGB")

    inputs = processor(
        images=image,
        return_tensors="pt",
    )

    if "pixel_values" not in inputs:
        raise RuntimeError("The processor did not produce pixel_values.")

    if "pixel_mask" not in inputs:
        raise RuntimeError(
            "The processor did not produce pixel_mask. "
            "This Mask2Former export requires pixel_mask."
        )

    pixel_values = inputs["pixel_values"].to(
        device=device,
        dtype=torch.float32,
    )

    pixel_mask = inputs["pixel_mask"].to(
        device=device,
        dtype=torch.int64,
    )

    print("Input tensor shapes:")
    print(f"  pixel_values: {tuple(pixel_values.shape)}")
    print(f"  pixel_mask:   {tuple(pixel_mask.shape)}")

    wrapper = Mask2FormerInferenceWrapper(model)
    wrapper.to(device)
    wrapper.eval()

    print("Testing eager PyTorch inference...")
    with torch.inference_mode():
        eager_class_logits, eager_mask_logits = wrapper(
            pixel_values,
            pixel_mask,
        )

    print("Eager output tensor shapes:")
    print(f"  class_queries_logits: {tuple(eager_class_logits.shape)}")
    print(f"  masks_queries_logits: {tuple(eager_mask_logits.shape)}")

    print("Tracing TorchScript model...")
    with torch.inference_mode():
        traced_model = torch.jit.trace(
            wrapper,
            (pixel_values, pixel_mask),
            strict=False,
            check_trace=False,
        )

    traced_model.eval()

    # Freeze and optimize the inference graph where possible.
    try:
        traced_model = torch.jit.freeze(traced_model)
        traced_model = torch.jit.optimize_for_inference(traced_model)
    except RuntimeError as error:
        print(f"Warning: TorchScript optimization skipped: {error}")

    print(f"Saving TorchScript model: {output_model_path}")
    traced_model.save(str(output_model_path))

    if not output_model_path.is_file():
        raise RuntimeError(
            f"TorchScript export did not create: {output_model_path}"
        )

    print("Reloading TorchScript model for validation...")
    loaded_model = torch.jit.load(
        str(output_model_path),
        map_location=device,
    )
    loaded_model.eval()

    with torch.inference_mode():
        scripted_outputs = loaded_model(pixel_values, pixel_mask)

    if not isinstance(scripted_outputs, tuple):
        raise RuntimeError(
            "Unexpected TorchScript output. Expected a tuple containing "
            "(class_queries_logits, masks_queries_logits)."
        )

    scripted_class_logits, scripted_mask_logits = scripted_outputs

    print("TorchScript output tensor shapes:")
    print(f"  class_queries_logits: {tuple(scripted_class_logits.shape)}")
    print(f"  masks_queries_logits: {tuple(scripted_mask_logits.shape)}")

    if eager_class_logits.shape != scripted_class_logits.shape:
        raise RuntimeError("Class-logit output shape differs after tracing.")

    if eager_mask_logits.shape != scripted_mask_logits.shape:
        raise RuntimeError("Mask-logit output shape differs after tracing.")

    # Write metadata that C++ can read to reproduce the traced input shape.
    metadata_path = output_model_path.with_suffix(".json")

    metadata = {
        "model_directory": str(model_directory),
        "example_image": str(example_image_path),
        "input": {
            "pixel_values_shape": list(pixel_values.shape),
            "pixel_mask_shape": list(pixel_mask.shape),
            "pixel_values_dtype": "float32",
            "pixel_mask_dtype": "int64",
        },
        "output": {
            "class_queries_logits_shape": list(scripted_class_logits.shape),
            "masks_queries_logits_shape": list(scripted_mask_logits.shape),
        },
    }

    metadata_path.write_text(
        json.dumps(metadata, indent=2),
        encoding="utf-8",
    )

    print("\nExport completed successfully.")
    print(f"TorchScript model: {output_model_path}")
    print(f"Metadata file:     {metadata_path}")
    print("\nImportant: C++ preprocessing must produce the same input tensor")
    print("shape and normalization as this processor/traced model.")


if __name__ == "__main__":
    main()