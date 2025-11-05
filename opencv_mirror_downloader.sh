
#!/usr/bin/env bash
#
# opencv_mirror_downloader.sh
# -----------------------------------------
# Automatically download blocked OpenCV third‑party resources
# (normally fetched from raw.githubusercontent.com) via the JSDelivr CDN.
#
# Run this inside your OpenCV build directory after a failed CMake run.
# It parses CMakeDownloadLog.txt for missing URLs and fetches them.
#
# Works well on Raspberry Pi OS Lite and other minimal systems.

set -e

BUILD_DIR=$(pwd)
LOG_FILE="$BUILD_DIR/CMakeDownloadLog.txt"
DOWNLOAD_DIR="$BUILD_DIR/downloads"

# Create downloads folder if needed
mkdir -p "$DOWNLOAD_DIR"

if [[ ! -f "$LOG_FILE" ]]; then
  echo "❌ Error: $LOG_FILE not found. Run CMake first so the log is generated."
  exit 1
fi

echo "📋 Reading failed download info from: $LOG_FILE"
echo "📁 Saving files into: $DOWNLOAD_DIR"
echo

grep "#do_download" "$LOG_FILE" | while read -r line; do
  # Extract key info
  url=$(echo "$line" | sed -E 's/.*https:/https:/; s/"//g' | awk '{print $1}')
  hash=$(echo "$line" | awk '{print $(NF-1)}' | tr -d '"')
  name=$(echo "$line" | awk '{print $NF}' | tr -d '"')

  if [[ -z "$url" || -z "$hash" || -z "$name" ]]; then
    echo "⚠️  Skipping malformed line:"
    echo "   $line"
    continue
  fi

  # Convert to JSDelivr mirror URL
  js_url=$(echo "$url" | sed 's|https://raw.githubusercontent.com/|https://cdn.jsdelivr.net/gh/|' | sed 's|/|@|' -1)

  echo "⬇️  Downloading $name ..."
  echo "   From: $js_url"

  # Download file
  if wget -q -O "$DOWNLOAD_DIR/$name" "$js_url"; then
    echo "$hash" > "$DOWNLOAD_DIR/$name.hash"
    echo "✅ Saved: $DOWNLOAD_DIR/$name"
  else
    echo "❌ Failed to download: $js_url"
    echo "   You can manually try another mirror:"
    echo "   https://gitclone.com/github.com/opencv/opencv_3rdparty/raw/... "
  fi

  echo
done

echo "🎉 All done. Re‑run 'cmake ..' and then 'make -j$(nproc)'"

