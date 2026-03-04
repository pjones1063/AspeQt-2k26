#!/bin/bash
# 1. Setup paths
RELEASE_DIR="../release"
BUILD_DIR="../build"
mkdir -p "$RELEASE_DIR"

# 2. Build the project
cmake -S .. -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target clean
# sysctl -n hw.ncpu is the Mac equivalent of nproc
cmake --build "$BUILD_DIR" -j$(sysctl -n hw.ncpu)

# 3. Deploy and Bundle
# We point macdeployqt to the .app bundle. 
# The -executable flag is REQUIRED to bundle non-Qt libraries like libssh.
# The -dmg flag creates the installer, and -always-overwrite ensures a fresh build.
if macdeployqt "$BUILD_DIR/AspeQt.app" -dmg -executable="$BUILD_DIR/AspeQt.app/Contents/MacOS/AspeQt" -always-overwrite; then
    mv "$BUILD_DIR/AspeQt.dmg" "$RELEASE_DIR/AspeQt-2K26-macOS.dmg"
    echo "Successfully created macOS DMG."
else
    echo "macdeployqt failed. Ensure it is in your PATH."
    exit 1
fi
