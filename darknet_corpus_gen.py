import os
import random

# Define paths
train_folder = "/Users/dengfengji/ronnieji/kaggle/train"  # Path to the folder containing the subfolders for each class
output_folder = "/Users/dengfengji/ronnieji/kaggle/voc"  # Path where the train.txt file will be saved
classes = ["no_tumor", "pituitary_tumor"]  # Class names

# Ensure output folder exists
os.makedirs(output_folder, exist_ok=True)

# Create train.txt
train_file_path = os.path.join(output_folder, "train.txt")
with open(train_file_path, "w") as train_file:
    for class_name in classes:
        class_path = os.path.join(train_folder, class_name)
        if not os.path.exists(class_path):
            print(f"Warning: Class folder '{class_path}' does not exist. Skipping.")
            continue
        for image_file in os.listdir(class_path):
            if image_file.endswith((".jpg", ".jpeg", ".png")):  # Add more extensions if needed
                image_path = os.path.abspath(os.path.join(class_path, image_file))
                train_file.write(image_path + "")

# Shuffle the train.txt file
with open(train_file_path, "r") as train_file:
    lines = train_file.readlines()
random.shuffle(lines)
with open(train_file_path, "w") as train_file:
    train_file.writelines(lines)

# Create voc.names
voc_names_path = os.path.join(output_folder, "voc.names")
with open(voc_names_path, "w") as voc_names_file:
    for class_name in classes:
        voc_names_file.write(class_name + "")

# Create voc.data
voc_data_path = os.path.join(output_folder, "voc.data")
with open(voc_data_path, "w") as voc_data_file:
    voc_data_file.write(f"classes = {len(classes)}")
    voc_data_file.write(f"train = {os.path.abspath(train_file_path)}")
    voc_data_file.write(f"names = {os.path.abspath(voc_names_path)}")
    voc_data_file.write(f"backup = /Users/dengfengji/ronnieji/kaggle/backup/")  # Change this path as needed

print("Files generated successfully:")
print(f"1. train.txt: {train_file_path}")
print(f"2. voc.names: {voc_names_path}")
print(f"3. voc.data: {voc_data_path}")
