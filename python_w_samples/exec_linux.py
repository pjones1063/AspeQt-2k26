import subprocess
import os
from flask import Flask

app = Flask(__name__)

# --- CONFIGURATION ---
# If you are running this via SSH, you might need to force the display.
# If running locally in a terminal, os.environ is usually fine.
#
#  sudo apt update
#  sudo apt install zenity gnome-calculator gedit
#
# 
 
ENV_VARS = os.environ.copy()
ENV_VARS["DISPLAY"] = ":0"
# Force X11 backend if Wayland is being fussy (Optional but helpful)
ENV_VARS["GDK_BACKEND"] = "x11" 

@app.route('/cmd/<action>', methods=['GET'])
def handle_command(action):
    print(f"--- ATARI COMMAND RECEIVED: {action} ---")
    
    try:
        if action == "calc":
            # Launch Calculator (Fire and Forget)
            # Try 'gnome-calculator' (Gnome) or 'kcalc' (KDE)
            subprocess.Popen(['gnome-calculator'], env=ENV_VARS)
            return "OK: CALCULATOR LAUNCHED"
            
        elif action == "editor":
            # Launch Text Editor
            # Try 'gedit', 'mousepad', or 'pluma' depending on your desktop
            subprocess.Popen(['gedit'], env=ENV_VARS)
            return "OK: EDITOR LAUNCHED"
            
        elif action == "hello":
            # Show a Message Box (NON-BLOCKING)
            # We use Popen so Python replies to Atari immediately!
            subprocess.Popen([
                'zenity', '--info', 
                '--text=HELLO FROM THE ATARI!', 
                '--title=Message from 1979',
                '--width=300'
            ], env=ENV_VARS)
            
            return "OK: POPUP SENT"
            
        else:
            return "ERROR: UNKNOWN COMMAND"

    except FileNotFoundError:
        return "ERROR: APP NOT INSTALLED"
    except Exception as e:
        print(f"Error: {e}")
        return f"ERROR: {str(e)[:20]}"

if __name__ == '__main__':
    print("--- DEBIAN REMOTE CONTROL ACTIVE ---")
    print("Listening on Port 5000...")
    app.run(host='0.0.0.0', port=5000)
