import json
import sys
from pathlib import Path

import numpy as np
import torch
from PIL import Image, ImageDraw
from transformers import (
    AutoImageProcessor,
    Mask2FormerForUniversalSegmentation,
)


MODEL_NAME = "/Users/jidengfeng/Downloads/models/mask2former_liv" #"facebook/mask2former-swin-tiny-coco-instance"

# COCO's 80 category colors are generated deterministically below.
def make_color(class_id: int) -> tuple[int, int, int]:
    # A simple deterministic palette with reasonably distinct colors.
    return (
        (class_id * 67 + 37) % 256,
        (class_id * 131 + 83) % 256,
        (class_id * 197 + 149) % 256,
    )


def get_label(model, class_id: int) -> str:
    id2label = getattr(model.config, "id2label", {}) or {}
    return str(id2label.get(class_id, id2label.get(str(class_id), class_id)))


def colorize_instance_map(
    instance_map: np.ndarray,
    segments_info: list[dict],
) -> Image.Image:
    height, width = instance_map.shape
    result = np.zeros((height, width, 3), dtype=np.uint8)

    for segment in segments_info:
        instance_id = int(segment["id"])
        class_id = int(segment["label_id"])
        result[instance_map == instance_id] = make_color(class_id)

    return Image.fromarray(result, mode="RGB")


def make_overlay(image, instance_map, segments_info):
    color_mask = Image.new("RGBA", image.size, (0, 0, 0, 0))
    pixels = np.asarray(color_mask).copy()

    for segment in segments_info:
        instance_id = segment["id"]
        category_id = segment["label_id"]

        color = (
            (37 * category_id + 71) % 256,
            (17 * category_id + 151) % 256,
            (97 * category_id + 29) % 256,
        )

        pixels[instance_map == instance_id, :3] = color
        pixels[instance_map == instance_id, 3] = 100

    color_mask = Image.fromarray(pixels, mode="RGBA")
    return Image.alpha_composite(image.convert("RGBA"), color_mask)

def get_label_from_segments(segment: dict) -> str:
    return str(segment.get("label", f"class_{segment['label_id']}"))


def main() -> None:
    if len(sys.argv) not in (4,):
        print(
            "Usage: python segment_mask2former.py "
            "<input_image> <output_prefix> <model_directory>",
            file=sys.stderr,
        )
        sys.exit(1)

    input_path = Path(sys.argv[1])
    output_prefix = Path(sys.argv[2])
    model_directory = Path(sys.argv[3])

    if not input_path.exists():
        raise FileNotFoundError(f"Input image does not exist: {input_path}")
    if not (model_directory / "config.json").exists():
        raise FileNotFoundError(f"Missing config.json in {model_directory}")

    device = "cuda" if torch.cuda.is_available() else "cpu"
    dtype = torch.float16 if device == "cuda" else torch.float32
    print(f"Loading {MODEL_NAME} from {model_directory} on {device}...")

    processor = AutoImageProcessor.from_pretrained(str(model_directory), local_files_only=True)
    model = Mask2FormerForUniversalSegmentation.from_pretrained(
        str(model_directory),
        local_files_only=True,
        torch_dtype=dtype,
    ).to(device)
    model.eval()

    image = Image.open(input_path).convert("RGB")
    inputs = processor(images=image, return_tensors="pt")
    inputs = {name: value.to(device) for name, value in inputs.items()}

    with torch.inference_mode():
        outputs = model(**inputs)

    processed = processor.post_process_instance_segmentation(
        outputs,
        target_sizes=[(image.height, image.width)],
        threshold=0.5,
        mask_threshold=0.5,
        overlap_mask_area_threshold=0.8,
    )[0]

    instance_map = processed["segmentation"].cpu().numpy().astype(np.int32)
    segments_info = processed["segments_info"]

    # Add human-readable labels to the metadata.
    for segment in segments_info:
        segment["label"] = get_label(model, int(segment["label_id"]))

    instance_ids_path = output_prefix.with_name(
        output_prefix.name + "_instance_ids.png"
    )
    color_path = output_prefix.with_name(output_prefix.name + "_colored.png")
    overlay_path = output_prefix.with_name(output_prefix.name + "_overlay.png")
    json_path = output_prefix.with_name(output_prefix.name + "_segments.json")

    # A 32-bit PNG preserves instance IDs. Do not use mode 'L' here.
    Image.fromarray(instance_map, mode="I").save(instance_ids_path)
    colorize_instance_map(instance_map, segments_info).save(color_path)
    make_overlay(image, instance_map, segments_info).save(overlay_path)

    metadata = {
        "model": MODEL_NAME,
        "input": str(input_path),
        "image_width": image.width,
        "image_height": image.height,
        "segments": segments_info,
    }
    json_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    print(f"Detected {len(segments_info)} instances:")
    for segment in segments_info:
        print(
            f"  id={segment['id']}, "
            f"class={segment['label']} ({segment['label_id']}), "
            f"score={segment.get('score', 0.0):.3f}"
        )

    print(f"Instance-ID mask: {instance_ids_path}")
    print(f"Colored mask:     {color_path}")
    print(f"Overlay:          {overlay_path}")
    print(f"Metadata:         {json_path}")


if __name__ == "__main__":
    main()