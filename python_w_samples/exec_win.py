import subprocess
import ctypes
from flask import Flask, request

app = Flask(__name__)

def popup_message(text):
    # Uses Windows API to show a message box (0 = OK button)
    # This pauses the Python script until you click OK on the PC!
    ctypes.windll.user32.MessageBoxW(0, text, "Message from Atari 800XL", 0x40 | 0x1)

@app.route('/cmd/<action>', methods=['GET'])
def handle_command(action):
    print(f"--- ATARI COMMAND: {action} ---")
    
    try:
        if action == "calc":
            # Launch Calculator (Non-blocking)
            subprocess.Popen('calc.exe')
            return "OK: CALCULATOR OPENED"
            
        elif action == "notepad":
            # Launch Notepad (Non-blocking)
            subprocess.Popen('notepad.exe')
            return "OK: NOTEPAD OPENED"
            
        elif action == "hello":
            # Show a Message Box (Blocking)
            # We run this in a separate way so it doesn't freeze the Flask server? 
            # Actually, for a simple test, blocking is fine.
            popup_message("HELLO FROM 1979!")
            return "OK: MESSAGE SEEN"
            
        else:
            return "ERROR: UNKNOWN COMMAND"

    except Exception as e:
        return f"ERROR: {e}"

if __name__ == '__main__':
    print("--- ATARI REMOTE CONTROL ACTIVE ---")
    app.run(host='0.0.0.0', port=5000)
