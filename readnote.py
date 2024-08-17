import json  

def read_json_and_process_text(file_path, output_file):  
    # Read the JSON file  
    try:  
        with open(file_path, 'r') as json_file:  
            data = json.load(json_file)  
    except Exception as e:  
        print(f"Error reading the JSON file: {e}")  
        return []  

    # Debug: print the loaded data to check its structure  
    print("Loaded data:", data)  

    all_text_lines = []  
    
    # Check if the top-level data is a dictionary and has expected keys  
    for group, items in data.items():  
        if isinstance(items, list):  
            for item in items:  
                # Extract the "text" content  
                text_content = item.get('text', '')  
                print("Extracted text content:", text_content)  # Debug print  
                
                # Split the text content based on newline character '\n'  
                text_lines = text_content.split('\n')  
                print("Split text lines:", text_lines)  # Debug print  
                
                # Add these lines to the complete list  
                all_text_lines.extend(text_lines)  
                all_text_lines.extend(" ")
        else:  
            print(f"Warning: Expected list for group '{group}', got {type(items)} instead.")  
    
    # Write each line into the output file  
    print("Writing to output file:", output_file)  # Debug print  
    try:  
        with open(output_file, 'w') as file:  
            for line in all_text_lines:  
                file.write(line + '\n')  
    except Exception as e:  
        print(f"Error writing to file: {e}")  

    return all_text_lines  

# Define the file paths  
input_json_file = "/Users/ronnieji/MLCpplib-main/notes.json"  # replace with your JSON file path  
output_file = "/Users/ronnieji/MLCpplib-main/notes_output.txt"  # replace with your desired output file path  

# Process the file and get the list of lines  
text_lines = read_json_and_process_text(input_json_file, output_file)  

# Output the list of lines to the console  
print(text_lines)