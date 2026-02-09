import os
from flask import Flask, request, make_response

app = Flask(__name__)
NOTE_FILE = "atari_notes.txt"

@app.route('/note', methods=['GET', 'POST'])
def handle_note():
    # --- WRITE MODE (POST) ---
    if request.method == 'POST':
        # Decode the text from Atari
        content = request.data.decode('latin-1').strip()
        print(f"--- SAVING NOTE: {content} ---")
        
        # Save to PC hard drive
        with open(NOTE_FILE, 'w') as f:
            f.write(content)
        return "OK: SAVED"

    # --- READ MODE (GET) ---
    else:
        if os.path.exists(NOTE_FILE):
            with open(NOTE_FILE, 'r') as f:
                content = f.read()
            
            # Convert PC Newlines to Atari EOLs (155)
            content = content.replace('\n', chr(155))
            
            # Ensure it ends with EOL so Atari doesn't hang
            if not content.endswith(chr(155)): 
                content += chr(155)
            
            resp = make_response(content.encode('latin-1'))
            return resp
        else:
            # Default message if no file exists
            return "NO NOTES SAVED YET." + chr(155)

if __name__ == '__main__':
    print("--- ATARI CLOUD DRIVE ACTIVE ---")
    app.run(host='0.0.0.0', port=5000)
