import sys
import os
import struct
import argparse

def parse_xex_to_memory(filepath):
    """
    Parses an Atari executable (XEX/COM) file and maps its segments
    into a virtual 64KB memory array.
    """
    memory = bytearray(65536)
    
    with open(filepath, 'rb') as f:
        data = f.read()
        
    # Check for standard Atari DOS executable header
    if not data.startswith(b'\xff\xff'):
        return None 
        
    idx = 2
    while idx < len(data):
        # Skip optional repeating FF FF segment headers
        if data[idx:idx+2] == b'\xff\xff':
            idx += 2
            continue
            
        if idx + 4 > len(data):
            break # Reached EOF unexpectedly
            
        # Read the start and end address of the block (Little Endian)
        start_addr = struct.unpack_from('<H', data, idx)[0]
        end_addr = struct.unpack_from('<H', data, idx + 2)[0]
        idx += 4
        
        block_len = end_addr - start_addr + 1
        
        if idx + block_len > len(data):
            print(f"Warning: Unexpected end of file at block {hex(start_addr)}-{hex(end_addr)}")
            block_len = len(data) - idx
            
        # Copy block payload into our virtual memory map
        memory[start_addr:start_addr+block_len] = data[idx:idx+block_len]
        idx += block_len
        
    return memory

def convert_to_car(input_file, output_file):
    print(f"Processing '{input_file}'...")
    
    memory = parse_xex_to_memory(input_file)
    cart_type = 1   # Type 1 = Standard 8 KB cartridge ($A000-$BFFF)
    rom_size = 8192 # 8KB
    
    if memory is not None:
        print("Detected Atari Executable (XEX/COM) format. Extracting $A000-$BFFF memory block...")
        # Extract exactly the 8KB $A000-$BFFF chunk
        rom_data = memory[0xA000:0xC000]
    else:
        print("No XEX header detected. Assuming a flat raw binary ROM...")
        with open(input_file, 'rb') as f:
            rom_data = f.read()
            
        # Pad or truncate flat binaries to strictly 8KB
        if len(rom_data) < rom_size:
            print(f"Padding binary from {len(rom_data)} to {rom_size} bytes.")
            rom_data = rom_data.ljust(rom_size, b'\x00')
        elif len(rom_data) > rom_size:
            print(f"Truncating binary from {len(rom_data)} to {rom_size} bytes.")
            rom_data = rom_data[:rom_size]
            
    # Calculate checksum: Simple 32-bit sum of all bytes in the ROM
    checksum = sum(rom_data)
    
    # CAR Header Format (All 32-bit ints are Big Endian ">I" for Atari CAR format):
    # 4 Bytes: "CART"
    # 4 Bytes: Cart Type
    # 4 Bytes: Checksum
    # 4 Bytes: Reserved (0)
    header = struct.pack(">4sIII", b"CART", cart_type, checksum, 0)
    
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(rom_data)
        
    print(f"Success! Created {output_file}")
    print(f" -> Cartridge Type: {cart_type} (Standard 8KB)")
    print(f" -> Checksum:       {hex(checksum)}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert an Atari MADS .com/.xex or .bin to .CAR format")
    parser.add_argument("input", help="Input file (.com, .xex, or .bin)")
    parser.add_argument("-o", "--output", help="Output .car file (optional)", default=None)
    
    args = parser.parse_args()
    
    # Default to replacing the extension with .car if no output specified
    out_file = args.output if args.output else os.path.splitext(args.input)[0] + ".car"
    
    convert_to_car(args.input, out_file)

