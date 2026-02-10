import os
from flask import Flask, make_response

app = Flask(__name__)

@app.route('/file', methods=['GET'])
def get_file():
    file_path = "textfile.txt"
    
    if not os.path.exists(file_path):
        return "ERROR: FILE NOT FOUND", 404

    try:
        # 1. Read the local file
        # 'r' mode handles Windows (\r\n) vs Linux (\n) automatically
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # 2. Convert to Atari EOL (155 / 0x9B)
        # We assume the PC file uses standard newlines (\n)
        atari_content = content.replace('\n', chr(155))
        
        # Ensure it ends with an EOL
        if not atari_content.endswith(chr(155)):
            atari_content += chr(155)

        # 3. Encode to Latin-1 (Raw Bytes 0-255)
        response = make_response(atari_content.encode('latin-1', errors='replace'))
        response.headers['Content-Type'] = 'text/plain'
        
        print(f"--- Served {file_path} ({len(atari_content)} bytes) ---")
        return response

    except Exception as e:
        print(f"Error: {e}")
        return "SERVER ERROR", 500

if __name__ == '__main__':
    print("--- ATARI FILE SERVER (REAL FILE) STARTED ---")
    print("Make sure 'testfile.txt' is in this folder.")
    app.run(host='0.0.0.0', port=5000)
