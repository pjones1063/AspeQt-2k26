#!/usr/bin/env python3
import sys

def convert_to_atascii(filename):
    try:
        # Read the PC text file
        with open(filename, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 1. Normalize line endings to just \n (handles Windows CRLF)
        content = content.replace('\r\n', '\n')
        
        # 2. Convert to bytes (ASCII)
        # Atari BASIC requires keywords in UPPERCASE. 
        # Our chat code was uppercase, but let's force it just in case.
        content = content.upper()
        
        # 3. Create byte array
        data = bytearray()
        for char in content:
            # Map Newline ($0A) to ATASCII EOL ($9B)
            if char == '\n':
                data.append(0x9B)
            else:
                # Standard ASCII (0-127) maps 1:1 to ATASCII for basic code
                # (except for control chars, but basic alphanumerics are fine)
                data.append(ord(char))
                
        # 4. Append a final EOL if missing
        if data[-1] != 0x9B:
            data.append(0x9B)

        # Write it back (or to a new file)
        output_name = filename # Overwrite
        with open(output_name, 'wb') as f:
            f.write(data)
            
        print(f"Successfully converted {filename} to ATASCII format.")

    except Exception as e:
        print(f"Error converting file: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 txt2atari.py <filename>")
    else:
        convert_to_atascii(sys.argv[1])

