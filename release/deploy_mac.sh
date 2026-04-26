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
# ... (cmake build steps above) ...

# 3. Deploy and Bundle
if macdeployqt "$BUILD_DIR/AspeQt.app" -executable="$BUILD_DIR/AspeQt.app/Contents/MacOS/AspeQt" -always-overwrite; then
    echo "macdeployqt finished bundling."
else
    echo "macdeployqt failed."
    exit 1
fi

# 4. Repair the Signature (Ad-Hoc for Apple Silicon)
echo "Applying Ad-Hoc Code Signature..."
codesign --force --deep --sign - "$BUILD_DIR/AspeQt.app"

# 4. Create the DMG
echo "Creating DMG..."
hdiutil create -volname "AspeQt-2K26" -srcfolder "$BUILD_DIR/AspeQt.app" -ov -format UDZO "$RELEASE_DIR/AspeQt-2K26-macOS.dmg"

echo "Successfully created distributable macOS DMG."
