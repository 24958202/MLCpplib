import os  
import shutil  

def prepare_yolo_dataset(dataset_path, output_path):  
    """  
    Prepare a YOLOv8-compatible dataset by organizing images and labels.  

    Args:  
        dataset_path (str): Path to the dataset folder (e.g., "train").  
        output_path (str): Path to the output folder (e.g., "train_prepared").  
    """  
    # Get class names from subfolder names  
    class_names = sorted([d for d in os.listdir(dataset_path) if os.path.isdir(os.path.join(dataset_path, d))])  

    # Create a mapping of class names to class IDs  
    class_to_id = {class_name: i for i, class_name in enumerate(class_names)}  

    # Create output directories for images and labels  
    images_folder = os.path.join(output_path, "images")  
    labels_folder = os.path.join(output_path, "labels")  
    os.makedirs(images_folder, exist_ok=True)  
    os.makedirs(labels_folder, exist_ok=True)  

    # Iterate through each class folder  
    for class_name, class_id in class_to_id.items():  
        class_folder = os.path.join(dataset_path, class_name)  

        # Iterate through all images in the class folder  
        for image_file in os.listdir(class_folder):  
            if image_file.endswith((".jpg", ".jpeg", ".png")):  # Check for valid image files  
                # Copy image to the images folder  
                src_image_path = os.path.join(class_folder, image_file)  
                dst_image_path = os.path.join(images_folder, image_file)  
                shutil.copy(src_image_path, dst_image_path)  

                # Create a corresponding label file  
                label_file = os.path.splitext(image_file)[0] + ".txt"  
                label_path = os.path.join(labels_folder, label_file)  

                # Generate a default bounding box (entire image)  
                # YOLO format: <class_id> <x_center> <y_center> <width> <height>  
                default_bbox = f"{class_id} 0.5 0.5 1.0 1.0\n"  

                # Write the label to the file  
                with open(label_path, "w") as f:  
                    f.write(default_bbox)  

    # Generate dataset.yaml file  
    yaml_path = os.path.join(output_path, "dataset.yaml")  
    with open(yaml_path, "w") as f:  
        f.write(f"path: {output_path}\n")  
        f.write("train: images\n")  
        f.write("val: images\n")  
        f.write("names:\n")  
        for i, class_name in enumerate(class_names):  
            f.write(f"  {i}: {class_name}\n")  

    print("YOLOv8 dataset prepared successfully!")  
    print(f"Classes found: {', '.join(class_names)}")  
    print(f"Dataset.yaml file generated at: {yaml_path}")  


# Example usage  
if __name__ == "__main__":  
    # Replace these paths with your dataset and output folder  
    dataset_path = "/home/ronnieji/ronnieji/kaggle/train"  # Path to your dataset with subfolders  
    output_path = "/home/ronnieji/ronnieji/kaggle/output_forYOLO"  # Path to the prepared dataset  
    prepare_yolo_dataset(dataset_path, output_path)
