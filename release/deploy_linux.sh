#!/bin/bash
# 1. Setup paths
RELEASE_DIR="../release"
BUILD_DIR="../build"
mkdir -p "$RELEASE_DIR"

# 2. Build the project
cmake -S .. -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target clean
cmake --build "$BUILD_DIR" -j$(nproc)

# 3. Create the .deb Installer
cd "$BUILD_DIR"
if cpack -G DEB; then
    mv *.deb "$RELEASE_DIR/"
    echo "Successfully created .deb package."
else
    echo "CPack failed to create the package."
    exit 1
fi

# 4. Create the Portable Version (.tar.gz)
cd "$RELEASE_DIR"
tar -cvzf AspeQt-2K26-linux-portable.tar.gz -C "$BUILD_DIR" AspeQt
