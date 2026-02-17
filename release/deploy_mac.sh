#!/bin/bash
mkdir -p ../release
cmake -S .. -B ../build -DCMAKE_BUILD_TYPE=Release
cmake --build ../build --target clean
cmake --build ../build -j$(sysctl -n hw.ncpu)
# Create DMG
macdeployqt ../build/AspeQt.app -dmg
mv ../build/AspeQt.dmg ../release/AspeQt-2k26.dmg

