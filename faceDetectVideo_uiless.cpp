/*
    Program to detect faces in a video and save unique face images.
	You need opencv and opencv contrib modules to run the program
	
	Adjust prameters as needed:
	const unsigned int MAX_FEATURES = 1000;   // Max number of features to detect
	const float RATIO_THRESH = 0.95f;          // Ratio threshold for matching
	const unsigned int DE_THRESHOLD = 10;      // Min matches to consider a face as existing
	
	Follow these steps to install OpenCV4 with OpenCV Contrib on your system:

    ### Step 1: Install Dependencies
    Before building OpenCV, ensure you have the required dependencies installed. Run the following commands:

    **For Ubuntu/Debian:**
    ```bash
    sudo apt update
    sudo apt install -y build-essential cmake git libgtk-3-dev libavcodec-dev libavformat-dev libswscale-dev \
        libv4l-dev libxvidcore-dev libx264-dev libjpeg-dev libpng-dev libtiff-dev gfortran openexr \
        libatlas-base-dev python3-dev python3-numpy libtbb2 libtbb-dev libdc1394-22-dev
    ```

    **For macOS:**
    ```bash
    brew install cmake pkg-config jpeg libpng libtiff openexr eigen tbb
    ```

    ### Step 2: Clone OpenCV and OpenCV Contrib Repositories
    Clone the OpenCV and OpenCV Contrib repositories from GitHub:
    ```bash
    git clone https://github.com/opencv/opencv.git
    git clone https://github.com/opencv/opencv_contrib.git
    ```

    ### Step 3: Create a Build Directory
    Create a build directory for OpenCV:
    ```bash
    mkdir -p opencv/build
    cd opencv/build
    ```

    ### Step 4: Configure the Build with CMake
    Run the following `cmake` command to configure the build. Make sure to specify the path to the OpenCV Contrib modules:
    ```bash
    cmake -D CMAKE_BUILD_TYPE=Release \
      -D CMAKE_INSTALL_PREFIX=/usr/local \
      -D OPENCV_EXTRA_MODULES_PATH=/Users/dengfengji/ronnieji/libs/opencv_contrib/modules \
      -D BUILD_opencv_python3=OFF \
      -D BUILD_opencv_python2=OFF \
      -D BUILD_TESTS=OFF \
      -D BUILD_PERF_TESTS=OFF \
      -D WITH_TBB=ON \
      -D WITH_OPENMP=ON \
      -D WITH_FFMPEG=ON \
      -D WITH_OPENGL=OFF \
      -D OPENCV_GENERATE_PKGCONFIG=ON -D CMAKE_CXX_STANDARD=20 ..
    ```
    ### Step 5: Build OpenCV
    Compile OpenCV using `make`. Adjust the `-j` flag to match the number of CPU cores on your system:
    ```bash
    make -j$(nproc)  # For Linux
    make -j$(sysctl -n hw.ncpu)  # For macOS
    
    compile parameters:
    -framework ApplicationServices //on mac
	sudo DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH /Users/dengfengji/ronnieji/lib/MLCpplib-main/faceDetectVideo

*/
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cstdint>  // For uint32_t
#include <stdexcept>
#include <algorithm>
#include <ranges>
#include <thread>
#include <chrono>
#include <ctime>
#include <cmath>
#include <sys/types.h>
#include <signal.h>
#include <csignal>
#include <stdlib.h>
#include <atomic>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/features2d.hpp> 
#include <opencv2/xfeatures2d.hpp>
// Atomic flag to keep the main loop running
std::atomic<bool> signal_running{true};