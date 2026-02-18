from flask import Flask, request
import socket

app = Flask(__name__)

# Helper to find your PC's IP address
def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # Doesn't need to be reachable
        s.connect(('10.255.255.255', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

print(f"--- ATARI MULTI-DEVICE SERVER ---")
print(f"Server running on: http://{get_ip()}:5000")
print(f"---------------------------------")

# Endpoint for W1
@app.route('/device1', methods=['POST'])
def handle_one():
    data = request.data.decode('utf-8')
    print(f"\n[W1] RECEIVED: {data}")
    return "OK From 1"

# Endpoint for W2
@app.route('/device2', methods=['POST'])
def handle_two():
    data = request.data.decode('utf-8')
    print(f"\n[W2] RECEIVED: {data}")
    return "OK From 2"

@app.route('/trigger', methods=['POST'])
def handle_trigger():
    data = request.data.decode('utf-8')
    # FORCE FLUSH
    print(f"\n[XIO] TRIGGERED: {data}") 
    return "OK"

if __name__ == '__main__':
    # host='0.0.0.0' allows external connections from the Atari
    app.run(host='0.0.0.0', port=5000)
    
