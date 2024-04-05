# Use a base image with Ubuntu
FROM ubuntu:latest

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Update package lists and install necessary tools and libraries
RUN apt-get update && apt-get install -y \
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
    vim \
    gdb \
    valgrind && \
    apt-get clean

# System locale
# Important for UTF-8
ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8

# Install Boost
# https://www.boost.org/doc/libs/1_80_0/more/getting_started/unix-variants.html
RUN cd /tmp && \
    BOOST_VERSION_MOD=$(echo $BOOST_VERSION | tr . _) && \
    wget https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_MOD}.tar.bz2 && \
    tar --bzip2 -xf boost_${BOOST_VERSION_MOD}.tar.bz2 && \
    cd boost_${BOOST_VERSION_MOD} && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 install && \
    rm -rf /tmp/*
 
# Install Eigen
RUN cd ./lib/eigen-3.4.0 && \
    mkdir -p build && cd build && \
    cmake .. && \
    make && \
    make install .. 

# Install Google Gumbo Parser
RUN cd ./lib/google-gumbo-parser-3973c58 && \
    ./autogen.sh && \
    ./configure && \
    make && \
    make install ..

# Install cpp-httplib
RUN cd ./lib/cpp-httplib-master && \
    mkdir -p build && cd build && \
    cmake .. && \
    make && \
    make install ..
# Copy your C++20 application source code to the container
#COPY . /app

#install opencv
RUN cd ./lib/opencv && \
# Download and unpack OpenCV sources
# Note: It's better to do this in one RUN command to reduce image layers
RUN wget -O /opencv.zip https://github.com/opencv/opencv/archive/4.x.zip && \
    unzip /opencv.zip -d / && \
    mkdir -p /build-opencv && cd /build-opencv && \
    cmake /opencv-4.x && \
    cmake --build . && \
    cd /

#build foilders in dbtools
RUN mkdir -p /home/ronnieji/lib/db_tools/log && \
    /home/ronnieji/lib/db_tools/res && \
    /home/ronnieji/lib/db_tools/web && \
    /home/ronnieji/lib/db_tools/webUrls && \
    /home/ronnieji/lib/db_tools/wikiLog && \
    /home/ronnieji/corpus/wiki_catag && \
    /home/ronnieji/lib/lib/res

#lock these folders
RUN chmod u-w /home/ronnieji/lib/db_tools/log && \
    /home/ronnieji/lib/db_tools/res && \
    /home/ronnieji/lib/db_tools/web && \
    /home/ronnieji/lib/db_tools/webUrls && \
    /home/ronnieji/lib/db_tools/wikiLog && \
    /home/ronnieji/corpus/wiki_catag && \
    /home/ronnieji/lib/lib/res
# Set the working directory to /home/ronnieji/lib/db_tools
# Note: If this directory depends on content from the host, ensure it's copied into the image
WORKDIR /home/ronnieji/lib/db_tools
COPY ./lib /home/ronnieji/lib/lib

# Copy the necessary files from the host machine to the Docker image
# This command seems to copy from host to image, but Docker can't copy from outside its context (the build directory). Adjust as necessary.
# COPY /home/ronnieji/lib/lib /home/ronnieji/lib/lib

# Set the entry point for the Docker container
CMD ["./webcrawler_wiki_class_keyword"]
