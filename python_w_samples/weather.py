from flask import Flask, make_response
import socket
import requests

app = Flask(__name__)

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

print(f"--- ATARI SMART GATEWAY ---")
print(f"Running on: http://{get_ip()}:5000")
print(f"---------------------------")

@app.route('/weather', methods=['GET'])
def get_weather():
    # 1. Fetch Real Weather (London, Ontario for example)
    # format=3 gives "Location: Condition Temp"
    try:
        r = requests.get('http://wttr.in/Lincoln_Ontario?format=3')
        text = r.text.strip()
        
        # 2. SANITIZE for Atari (The 6502 hates Emojis)
        # Simple ascii conversion, ignoring errors
        clean_text = text.encode('ascii', 'ignore').decode('ascii')
        
        # 3. Uppercase looks better on Atari
        clean_text = clean_text.upper()
        
        print(f"[GATEWAY] Weather Fetch: {clean_text}")
        
        # 4. Return with standard Newline (AspeQt will handle the translation!)
        response = make_response(clean_text + "\n")
        response.headers['Content-Type'] = 'text/plain'
        return response
        
    except Exception as e:
        return f"ERR: {str(e)}\n"

@app.route('/time', methods=['GET'])
def get_time():
    from datetime import datetime
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    return f"SERVER TIME: {now}\n"

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
