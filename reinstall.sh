#!/bin/bash
#run chmod +x reinstall.sh before running the script
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
    vim \
    gdb \
    valgrind

# Clean apt cache to free up space
sudo apt-get clean

# System locale configuration
export LC_ALL=en_US.UTF-8
export LANG=en_US.UTF-8
export LANGUAGE=en_US.UTF-8
sudo locale-gen en_US.UTF-8

# Build and install Boost
# Assuming the Boost library source code is already available in ./libs/boost_1_84_0
cd ./libs/boost_1_84_0 && \
./bootstrap.sh --prefix=/usr/local && \
./b2 install

# Install Eigen
# Assuming the Eigen library source code is already available in ./libs/eigen-3.4.0
cd ../eigen-3.4.0 && \
mkdir -p build && cd build && \
cmake .. && \
make && \
sudo make install

# Install Google Gumbo Parser
# Assuming the Google Gumbo Parser source code is already available in ./libs/google-gumbo-parser-3973c58
cd ../../google-gumbo-parser-3973c58 && \
./autogen.sh && \
./configure && \
make && \
sudo make install

# Install cpp-httplib
# Assuming the cpp-httplib source code is already available in ./libs/cpp-httplib-master
cd ../../cpp-httplib-master && \
mkdir -p build && cd build && \
cmake .. && \
make && \
sudo make install

# Install OpenCV
# Assuming the OpenCV source code needs to be downloaded
cd ../../opencv && \
wget -O /opencv.zip https://github.com/opencv/opencv/archive/4.x.zip && \
unzip /opencv.zip -d / && \
mkdir -p /build-opencv && cd /build-opencv && \
cmake /opencv-4.x && \
cmake --build .

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