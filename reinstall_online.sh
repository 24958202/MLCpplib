#!/bin/bash
# run chmod +x reinstall.sh before running the script
# Avoid prompts from apt
export DEBIAN_FRONTEND=noninteractive

# Update package lists and install necessary tools and libraries
sudo apt-get update && sudo apt-get install -y \
    build-essential \
    g++ \
    cmake \
    autoconf \
    pkg-config \
    ca-certificates \
    libssl-dev \
    git \
    wget \
    curl \
    unzip \
    language-pack-en \
    locales \
    locales-all \
    libboost-all-dev \
    libeigen3-dev \
    libsqlite3-dev \
    libmysqlclient-dev \
    libcurl4-openssl-dev \
    libgumbo-dev \
    libtool \
    m4 \
    automake \
    libicu-dev \
    libcpprest-dev \
    libmysqlclient-dev \
    libopencv-highgui4.5 \
    libopencv-dev \
    libopencv-video-dev \
    libopencv-imgcodecs-dev \
    vim \
    gdb \
    valgrind

sudo add-apt-repository ppa:savoury1/curl34 -y
sudo add-apt-repository ppa:savoury1/build-tools -y
sudo add-apt-repository ppa:savoury1/backports -y
sudo add-apt-repository ppa:savoury1/python -y
sudo add-apt-repository ppa:savoury1/encryption -y

sudo apt update && sudo apt-get install -y \
     curl \
     libcurlpp-dev \
     libcurlpp0 \
     expect \
     expect-lite \
     libexpected-dev \
     tcl-expect-dev 
# Update the linker cache
sudo ldconfig

# Set the library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Clean apt cache to free up space
sudo apt-get clean

# System locale configuration
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8
sudo locale-gen en_US.UTF-8

# Create directories for dbtools
mkdir -p /home/$USER/lib/db_tools/log \
         /home/$USER/lib/db_tools/res \
         /home/$USER/lib/db_tools/web \
         /home/$USER/lib/db_tools/webUrls \
         /home/$USER/lib/db_tools/wikiLog \
         /home/$USER/corpus/wiki_catag \
         /home/$USER/lib/lib/res

# Lock these directories
chmod u-w /home/$USER/lib/db_tools/log \
          /home/$USER/lib/db_tools/res \
          /home/$USER/lib/db_tools/web \
          /home/$USER/lib/db_tools/webUrls \
          /home/$USER/lib/db_tools/wikiLog \
          /home/$USER/corpus/wiki_catag \
          /home/$USER/lib/lib/res

echo "Setup complete."

