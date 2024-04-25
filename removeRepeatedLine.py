# Open the file in read mode
with open('/home/ronnieji/lib/db_tools/updateLink/u_booklist.txt', 'r') as file:
    lines = file.readlines()

# Create a set to store unique lines
unique_lines = set()

# Create a list to store lines that are not repeated
new_lines = []

# Iterate through each line in the file
for line in lines:
    # Check if the line is already in the set
    if line not in unique_lines:
        # If not, add it to the set and the new lines list
        unique_lines.add(line)
        new_lines.append(line)

# Open the file in write mode and write the new lines
with open('/home/ronnieji/lib/db_tools/updateLink/u_booklist_new.txt', 'w') as file:
    file.writelines(new_lines)

print("Repeated lines removed successfully.")
