import logging
from flask import Flask, request, make_response

# 1. SETUP LOGGING
# Minimize console noise (Werkzeug logs) so we can see our custom XIO hits
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)

# 2. GLOBAL STORAGE
# We use a dictionary to store buffers for each "Client IP + Port" combination.
# This ensures W1, W2, W3, and W4 get their own private data streams.
client_buffers = {}

@app.route('/stress', methods=['GET', 'POST'])
def handle_stress():
    # Identify the specific connection (IP address)
    client_id = request.remote_addr
    
    # ---------------------------------------------------------
    # WRITE MODE (ATARI SENDING DATA)
    # ---------------------------------------------------------
    if request.method == 'POST':
        try:
            # 1. Capture Raw Data
            data = request.data
            
            # 2. Decode for Logging (Safe Mode)
            try:
                text_payload = data.decode('utf-8', errors='ignore')
            except:
                text_payload = "<BINARY>"

            # 3. STORAGE LOGIC
            # If the Atari sends data, we overwrite the previous buffer for this client.
            # This works for both "Echo" (Test 1) and "Streaming" (Test 2/3).
            client_buffers[client_id] = data

            # 4. LOGGING LOGIC (Smart Filtering)
            # Don't spam console for the big 1-500 stream (Test 2)
            # if len(data) > 100:
            #    print(f"[POST] {client_id} -> Received Stream: {len(data)} bytes")
            # Don't spam console for rapid XIO hits (Test 4)
            if "W1-PING" in text_payload:
                 # Print dots for XIO hopping to show it's alive without flooding
                 print("*", end="", flush=True)
            elif "W2-PING" in text_payload:
                 # Print dots for XIO hopping to show it's alive without flooding
                 print("+", end="", flush=True)     
            else:
                # Normal Packet (Test 1)
                 print(f"[POST] {client_id} -> Received Stream: {len(data)} bytes")

            return "OK", 200

        except Exception as e:
            print(f"!!! POST ERROR: {e}")
            return "ERROR", 500

    # ---------------------------------------------------------
    # READ MODE (ATARI ASKING FOR DATA)
    # ---------------------------------------------------------
    else: # GET
        try:
            # 1. Retrieve Data for this Client
            response_data = client_buffers.get(client_id, b"NO_DATA_YET")
            
            # 2. LOGGING
            if len(response_data) > 100:
                 print(f"[GET]  {client_id} <- Serving Stream: {len(response_data)} bytes")
            elif response_data == b"NO_DATA_YET":
                 print(f"[GET]  {client_id} <- EMPTY (No previous POST)")
            else:
                 try:
                    txt = response_data.decode('utf-8')
                    print(f"[GET]  {client_id} <- Echoing: {txt}")
                 except:
                    print(f"[GET]  {client_id} <- Echoing Binary: {len(response_data)} bytes")

            # 3. SEND RESPONSE
            # Return raw bytes exactly as received (Crucial for Binary Mode tests)
            response = make_response(response_data)
            response.headers['Content-Type'] = 'text/plain'
            return response

        except Exception as e:
            print(f"!!! GET ERROR: {e}")
            return "ERROR", 500

if __name__ == '__main__':
    print("--- UNIVERSAL STRESS SERVER RUNNING ---")
    print("Supports: Echo, Streaming, Quad-Channel, and XIO")
    print("Listening on Port 5000...")
    # threaded=True is REQUIRED for Test 3 (Quad Channel) and Test 4 (XIO Speed)
    app.run(host='0.0.0.0', port=5000, threaded=True)
