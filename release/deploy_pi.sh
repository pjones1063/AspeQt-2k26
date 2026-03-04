#!/bin/bash
# 1. Setup paths
RELEASE_DIR="../release"
BUILD_DIR="../build"
mkdir -p "$RELEASE_DIR"

# 2. Build the project
# We use -j$(nproc) to use all cores on the Pi (4 or 5) for faster builds
cmake -S .. -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target clean
cmake --build "$BUILD_DIR" -j$(nproc)

# 3. Create the .deb Installer
cd "$BUILD_DIR"
if cpack -G DEB; then
    # On Pi 64-bit, this will usually produce an 'arm64' or 'aarch64' .deb
    mv *.deb "$RELEASE_DIR/"
    echo "Successfully created Pi .deb package."
else
    echo "CPack failed to create the .deb package."
    exit 1
fi

# 4. Create the Portable Version (.tar.gz)
cd "$RELEASE_DIR"
tar -cvzf AspeQt-2K26-pi.64-portable.tar.gz -C "$BUILD_DIR" AspeQt
