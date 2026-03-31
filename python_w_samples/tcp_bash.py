import socket
import subprocess

HOST = '0.0.0.0'
PORT = 9090

def start_server():
    print(f"Starting Atari-to-PC TCP Bash Shell on port {PORT}...")
    
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen()
        
        print(f"Listening for Atari connections on {HOST}:{PORT}\n")
        
        while True:
            conn, addr = s.accept()
            print(f"Atari connected from {addr}")
            handle_connection(conn, addr)
            print("Connection closed. Waiting for next connection...\n")

def handle_connection(conn, addr):
    with conn:
        try:
            # Welcome banner must have \r\n for Atari INPUT to finish
            conn.sendall(b"Connected to Debian Bash Shell.\r\n")
        except Exception as e:
            print(f"Could not send welcome banner: {e}")
            return

        while True:
            try:
                data = conn.recv(1024)
                
                if not data:
                    print(f"Atari at {addr} closed the connection.")
                    break
                    
                raw_text = data.decode('utf-8', errors='ignore').strip()
                
                # BULLETPROOFING: If the 6502 driver recycled old read-buffer text 
                # during the 255-byte pad flush, the actual command the user 
                # just typed will always be the very last line.
                if '\n' in raw_text:
                    command = raw_text.split('\n')[-1].strip()
                else:
                    command = raw_text.strip()
                
                if not command:
                    conn.sendall(b"===EOF===\r\n")
                    continue
                    
                if command.lower() in ['exit', 'quit']:
                    print("Atari requested disconnect.")
                    break

                print(f"Executing: {command}")
                
                process = subprocess.Popen(
                    command, 
                    shell=True, 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.PIPE,
                    executable='/bin/bash'
                )
                
                stdout, stderr = process.communicate()
                output = stdout + stderr
                
                if not output:
                    output_str = "(Command executed silently)"
                else:
                    output_str = output.decode('utf-8', errors='replace')
                
                # THE FIX: Strip trailing spaces/newlines from the bash output,
                # then FORCE a clean line break before the EOF marker.
                output_str = output_str.strip()
                output_str += "\n===EOF===\n"
                
                # Translate to FujiNet/Atari W: format
                output_str = output_str.replace('\n', '\r\n')
                
                conn.sendall(output_str.encode('utf-8'))

            except ConnectionResetError:
                print(f"Connection reset: The Atari unexpectedly dropped the connection.")
                break
            except Exception as e:
                print(f"Internal error: {e}")
                error_msg = f"Error executing command: {e}\r\n===EOF===\r\n"
                try:
                    conn.sendall(error_msg.encode('utf-8'))
                except:
                    break

if __name__ == "__main__":
    try:
        start_server()
    except KeyboardInterrupt:
        print("\nShutting down server. Goodbye!")
