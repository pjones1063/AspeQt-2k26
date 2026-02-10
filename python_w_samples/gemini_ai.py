from flask import Flask, request
from google import genai
import threading

# --- CONFIGURATION ---
# Get an API key from - https://aistudio.google.com/
API_KEY = "xxxxxxxxxxxxxxxxxxxxxxx"
client = genai.Client(api_key=API_KEY)

app = Flask(__name__)

# --- SYNCHRONIZATION ---
# This acts as a traffic light. Red = Wait, Green = Answer Ready.
new_message_event = threading.Event()
last_response = "READY."

@app.route('/chat', methods=['POST'])
def receive_input():
    global last_response
    try:
        # 1. Clear the flag (Set light to Red)
        new_message_event.clear()
        
        user_text = request.data.decode('utf-8', errors='ignore').strip()
        print(f"\n[Atari]: {user_text}")

        if user_text:
            # 2. Call Gemini
            # CHANGED PROMPT: Removed "Atari" roleplay, focused on formatting.
            response = client.models.generate_content(
                model="gemini-pro-latest",
                contents=f"Format the answer for a 40-character wide screen. Be concise. User asks: {user_text}"
            )
            
            clean_text = response.text.replace('*', '').replace('#', '').replace('\n', ' ')
            last_response = clean_text[:250]
            print(f"[Gemini]: {last_response}")
            
            # 3. Signal Ready (Set light to Green)
            new_message_event.set()
            
        return "OK"
    except Exception as e:
        print(f"Error: {e}")
        last_response = "ERROR"
        new_message_event.set() # Release the lock so Atari doesn't hang
        return "ERROR"

@app.route('/chat', methods=['GET'])
def send_response():
    # Atari is asking for data. 
    # WAIT here until the POST thread says the answer is ready.
    # Timeout after 10 seconds so the Atari doesn't freeze forever.
    msg_ready = new_message_event.wait(timeout=10.0)
    
    if msg_ready:
        return last_response + "\n"
    else:
        return "TIMEOUT: AI TOOK TOO LONG\n"

if __name__ == '__main__':
    print("--- ATARI GEMINI GATEWAY (SYNCED) STARTED ---")
    app.run(host='0.0.0.0', port=5000)

