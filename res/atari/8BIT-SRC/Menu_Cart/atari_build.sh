#!/bin/bash

# --- Configuration ---
MADS_BIN="/home/paul/Atari_Local/wudsn-ide-tools-main/ASM/MADS/mads.linux-x86-64"
CARTCONV="/usr/bin/cartconv"
OUT_DIR="/home/paul/eclipse-workspace/AspeQt-2K26-master/res/atari/8BIT-SRC/Menu_Cart"

mkdir -p "$OUT_DIR"

# ... (Configuration lines remain the same) ...

echo "--- Starting AspeQt-2k26 Build ---"

# 1. Build Executable (Keep as is)
$MADS_BIN menu_exe.asm -o:"$OUT_DIR/aspeqt_menu.xex"
echo "[✓] Created aspeqt_menu.xex"

# 2. Build Bootable Cartridge
# Compile with the filler (Result: 8198 bytes)
$MADS_BIN menu_crt_boot.asm -o:boot.tmp -fv:255 -b:'$A000'

if [ -f boot.tmp ]; then
    # STRIP THE 6-BYTE HEADER using dd
    # bs=1 (block size 1 byte), skip=6 (skip first 6 bytes)
    dd if=boot.tmp of=boot.bin bs=1 skip=6 status=none
    
    # Now boot.bin is exactly 8192 bytes. Create .CAR and .ROM
    $CARTCONV -t normal -i boot.bin -o "$OUT_DIR/aspeqt_boot.car"
    cp boot.bin "$OUT_DIR/aspeqt_boot.rom"
    echo "[✓] Created aspeqt_boot (.car and .rom)"
fi

# 3. Build Non-Bootable Cartridge
$MADS_BIN menu_crt_noboot.asm -o:noboot.tmp -fv:255 -b:'$A000'

if [ -f noboot.tmp ]; then
    # STRIP THE 6-BYTE HEADER
    dd if=noboot.tmp of=noboot.bin bs=1 skip=6 status=none
    
    # Create final files
    $CARTCONV -t normal -i noboot.bin -o "$OUT_DIR/aspeqt_noboot.car"
    cp noboot.bin "$OUT_DIR/aspeqt_noboot.rom"
    echo "[✓] Created aspeqt_noboot (.car and .rom)"
fi

# ... (Validation and Cleanup sections remain the same) ...
rm -f boot.tmp noboot.tmp boot.bin noboot.bin


# --- File Size Validation for Hardware ---
echo "--- Validating ROM Sizes for Ultimate Cart ---"
for file in "$OUT_DIR"/*.rom; do
    actual_size=$(stat -c%s "$file")
    if [ "$actual_size" -eq 8192 ]; then
        echo "[OK] $file is $actual_size bytes."
    else
        echo "[!!] WARNING: $file is $actual_size bytes (Expected 8192). This may fail on hardware!"
    fi
done

# --- Cleanup ---
rm -f boot.bin noboot.bin

echo "--- Build Complete! ---"
