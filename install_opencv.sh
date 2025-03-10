#!/bin/bash

# Exit on error
set -e

# Log all output
exec > >(tee -a build_opencv.log) 2>&1

# Check for dependencies
DEPENDENCIES=("git" "cmake" "make" "g++" "wget")
for dep in "${DEPENDENCIES[@]}"; do
    if ! command -v "$dep" &> /dev/null; then
        echo "Error: $dep is not installed. Please install it first."
        exit 1
    fi
done

# Create build directory
BUILD_DIR="$HOME/opencv_build"
echo "Step 1/5: Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Clone OpenCV and OpenCV Contrib
echo "Step 2/5: Cloning OpenCV repositories..."
if ! git clone https://github.com/opencv/opencv.git; then
    echo "Error: Failed to clone OpenCV repository."
    exit 1
fi
if ! git clone https://github.com/opencv/opencv_contrib.git; then
    echo "Error: Failed to clone OpenCV Contrib repository."
    exit 1
fi

# Create and enter build directory
BUILD_SUBDIR="$BUILD_DIR/build"
echo "Step 3/5: Creating build subdirectory: $BUILD_SUBDIR"
mkdir -p "$BUILD_SUBDIR"
cd "$BUILD_SUBDIR"

# Configure OpenCV with CMake
echo "Step 4/5: Configuring OpenCV with CMake..."
cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -D ENABLE_NEON=ON \
    -D ENABLE_VFPV3=ON \
    -D BUILD_TESTS=OFF \
    -D INSTALL_PYTHON_EXAMPLES=OFF \
    -D OPENCV_ENABLE_NONFREE=ON \
    -D CMAKE_SHARED_LINKER_FLAGS="-latomic" \
    -D WITH_OPENMP=ON \
    -D WITH_OPENCL=OFF \
    -D WITH_GTK=ON \
    -D WITH_GUI=ON \
    -D WITH_GTK=ON \
    -D WITH_QT=OFF \
    -D BUILD_EXAMPLES=OFF \
    -D OPENCV_ENABLE_NONFREE=ON ..

# Build OpenCV
echo "Step 5/5: Building OpenCV..."
make -j$(nproc)

# Install OpenCV
echo "Installing OpenCV..."
sudo make install

# Update shared library cache
echo "Updating shared library cache..."
sudo ldconfig

echo "OpenCV installation completed successfully!"
