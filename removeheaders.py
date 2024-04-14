#This script can remove x = 26 numbers of lines from the top and y = 351 lines from the bottom of all *.txt files in the folder ./books

import os

def remove_lines(file_path, x, y):
    with open(file_path, 'r') as file:
        lines = file.readlines()
    
    with open(file_path, 'w') as file:
        file.writelines(lines[x:-y])

folder_path = './books'
x = 26  # number of lines to remove from the top
y = 351  # number of lines to remove from the bottom

for file_name in os.listdir(folder_path):
    if file_name.endswith('.txt'):
        file_path = os.path.join(folder_path, file_name)
        remove_lines(file_path, x, y)

print("Done!");

