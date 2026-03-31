import socket
import psutil

HOST = '0.0.0.0'
PORT = 9090

def start_dashboard():
    print(f"Starting Debian Telemetry Server on port {PORT}...")
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen()
        
        while True:
            print(f"Waiting for Atari dashboard to connect...")
            conn, addr = s.accept()
            print(f"Atari connected from {addr}!")
            
            with conn:
                try:
                    # THE FIX: Send a dummy packet so the Atari's OPEN command finishes instantly!
                    conn.sendall(b"READY\n")
                    
                    while True:
                        # 1. Wait for Atari's PING
                        data = conn.recv(1024)
                        if not data:
                            print("Atari disconnected cleanly.")
                            break
                            
                        # 2. Read sensors (interval=1 acts as our 1-second pacing delay)
                        cpu = int(psutil.cpu_percent(interval=1))
                        ram = int(psutil.virtual_memory().percent)
                        disk = int(psutil.disk_usage('/').percent)
                        
                        # 3. Format and send
                        payload = f"{str(cpu).zfill(3)},{str(ram).zfill(3)},{str(disk).zfill(3)}\n"
                        conn.sendall(payload.encode('utf-8'))
                        
                except (ConnectionResetError, BrokenPipeError):
                    print("Atari disconnected. Resetting...")
                except Exception as e:
                    print(f"Error: {e}")

if __name__ == "__main__":
    try:
        start_dashboard()
    except KeyboardInterrupt:
        print("\nShutting down server. Goodbye!")
