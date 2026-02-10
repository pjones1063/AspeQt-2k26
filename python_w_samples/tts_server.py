import subprocess
from flask import Flask, request

app = Flask(__name__)

# Global variable to store the last result for the Atari to read
last_status = "READY"

@app.route('/speak', methods=['GET', 'POST'])
def speak_text():
    global last_status
    
    # --- 1. HANDLE READ (GET) ---
    # The Atari calls this to check if the last speech worked.
    if request.method == 'GET':
        return last_status

    # --- 2. HANDLE WRITE (POST) ---
    # The Atari calls this to send the text.
    try:
        text = request.data.decode('latin-1').strip()
        print(f"--- ATARI SAYS: {text} ---")
        
        if not text:
            last_status = "ERROR: EMPTY TEXT"
            return "ERROR"

        # Fire and Forget (Non-blocking speech)
        # -s 150 = Speed
        # -p 50  = Pitch
        subprocess.Popen([
            'espeak', 
            '-v', 'en-us', 
            '-s', '150', 
            '-p', '50', 
            text
        ])
        
        last_status = "OK: SPOKEN"
        return "OK"

    except FileNotFoundError:
        last_status = "ERROR: NO ESPEAK"
        return "ERROR"
    except Exception as e:
        last_status = f"ERROR: {str(e)[:15]}"
        return "ERROR"

if __name__ == '__main__':
    print("--- TALKING ATARI SERVER RUNNING ---")
    app.run(host='0.0.0.0', port=5000)
    
