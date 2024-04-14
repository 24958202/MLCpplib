#!/bin/bash

# Set the timeout value in seconds
timeout=30
user_agent="Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3"
delay=10  # Time delay in seconds

# Read the url.txt file line by line
while IFS= read -r url; do
    # Download the txt file using curl with timeout, user-agent, and other options
    curl --connect-timeout $timeout --max-time $timeout -A "$user_agent" -O $url

    # Add a time delay between downloads
    sleep $delay
done < u_booklist.txt



