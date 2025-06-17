#!/bin/bash

timeout=30
user_agent="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3"
delay=7
dest_dir="/home/ronnieji/lib/db_tools/updateLink"

set -e

# Ensure destination directory exists
mkdir -p "$dest_dir"

exists_list="exists_list.txt"
to_download_list="to_download_list.txt"

# Clear output files at the beginning
> "$exists_list"
> "$to_download_list"

# --- Reminder message before creating lists ---
echo "Creating exists list and to_download list by checking $dest_dir for existing files..."

# Stage 1: Check existence and split the lists
while IFS= read -r url; do
    # Skip empty lines
    [ -z "$url" ] && continue
    filename=$(basename "${url%%\?*}")
    dest_path="$dest_dir/$filename"
    if [ -f "$dest_path" ]; then
        echo "$url" >> "$exists_list"
    else
        echo "$url" >> "$to_download_list"
    fi
done < u_booklist.txt

echo "Stage 1 complete: Existing files listed in $exists_list, new files in $to_download_list."

# Stage 2: Download only new files, with delay
while IFS= read -r url; do
    [ -z "$url" ] && continue
    filename=$(basename "${url%%\?*}")
    dest_path="$dest_dir/$filename"
    
    # Check if already downloaded in previous run (paranoia check)
    if grep -Fxq "$url" "$exists_list"; then
        echo "File $filename already exists in $dest_dir. Skipping download."
        continue
    fi

    echo "Downloading $filename..."
    curl --connect-timeout "$timeout" --max-time "$timeout" -A "$user_agent" -o "$dest_path" -Lk "$url"
    if [ $? -ne 0 ]; then
        echo "Error downloading $url"
    fi

    sleep "$delay"
done < "$to_download_list"

echo "All done!"
