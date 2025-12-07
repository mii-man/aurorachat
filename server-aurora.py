import socket
import threading
import time
import sys
from better_profanity import profanity

# Initialize the profanity filter
profanity.load_censor_words()

# --- Configuration ---
HOST = '0.0.0.0'
PORT = 8961
RATE_LIMIT_MS = 2000
MAX_MESSAGE_LENGTH = 456
TERMINATION_TRIGGER = "<Fleetway>" 

# --- Global State ---
clients = []
rate_limit = {}
connection_times = {}

# --- Helper Functions ---
def process_chat_message(client_socket, message, client_ip):
    SERVER_IPS = {"127.0.0.1", "104.236.25.60"} # Server IP (these can be public but any admin's IPs cannot) if one of these goes rogue then remove them from this list and restart the aurorachat server.

    message_strip = message.strip()
    if not message_strip:
        return

    if client_ip in SERVER_IPS:
        broadcast(f"{message_strip}\n")
        return

    if TERMINATION_TRIGGER in message_strip:
        try:
            farewell_message = "No no, we aren't doxxers here. Restart if you wanna come back"
            client_socket.sendall(farewell_message.encode('utf-8'))
        except Exception:
            pass 
        print(f"lmaooo fleetway detected get this guy OUT")
        raise ConnectionResetError("Forced disconnect due to client's name being Fleetway.")
    
    if len(message_strip) > MAX_MESSAGE_LENGTH:
        try:
            reject_message = f'Message too large! Max length is {MAX_MESSAGE_LENGTH} characters.\n'
            client_socket.sendall(reject_message.encode('utf-8'))
        except Exception:
            pass 
        return

    now = int(time.time() * 1000)
    last_msg = rate_limit.get(client_socket, 0)
    if now - last_msg < RATE_LIMIT_MS:
        try:
            client_socket.sendall('Spam detected! Wait 2 seconds.\n'.encode('utf-8'))
        except Exception:
            pass 
        return 
        
    rate_limit[client_socket] = now
    
    censored_msg = profanity.censor(message_strip, '*')
    print(f"Message Processed: {repr(censored_msg)[1:-1]}")
        
    broadcast(f"{censored_msg}\n")

def broadcast(message):
    sockets_to_remove = []
    for client in clients:
        try:
            client.sendall(message.encode('utf-8'))
        except socket.error:
            sockets_to_remove.append(client)
        except Exception:
            sockets_to_remove.append(client)
    for client in sockets_to_remove:
        remove_client(client)
        
def remove_client(client_socket):
    if client_socket in clients:
        clients.remove(client_socket)
    if client_socket in rate_limit:
        del rate_limit[client_socket]
    try:
        client_socket.close()
    except Exception:
        pass
    print(f"Some sort of 'client' seems to have 'disconnected.'")

def handle_client(client_socket, client_address):
    client_ip = client_address[0]
    now_ms = int(time.time() * 1000)
    last_connection = connection_times.get(client_ip, 0)
    if now_ms - last_connection < RATE_LIMIT_MS:
        try:
            client_socket.close()
        except Exception:
            pass
        return
    connection_times[client_ip] = now_ms

    clients.append(client_socket)
    print(f"Client connection established.")

    try:
        buffer = ""
        while True:
            data = client_socket.recv(4096)
            if not data:
                break
            received_chunk = data.decode('utf-8', errors='ignore')
            buffer += received_chunk
            message_to_process = buffer.strip()
            buffer = "" 
            if message_to_process:
                process_chat_message(client_socket, message_to_process, client_ip) 
    except ConnectionResetError:
        print("Connection error from someplace: Connection reset or forced disconnect.")
    except socket.timeout:
        print("Connection error from something: Timeout.")
    except Exception as err:
        print(f"Connection error from... you tell me because I forgot. OOOH I FORGOT I FORGOT I FORGOT I FORGOT I FORGOT: {err}", file=sys.stderr)
    finally:
        remove_client(client_socket)

# --- Main Server Logic ---
def start_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        server.bind((HOST, PORT))
        server.listen(5)
    except Exception as e:
        print(f"Could not bind to port {PORT}: {e}", file=sys.stderr)
        sys.exit(1)
    print(f'aurorachat server v0.0.3 running on port {PORT}.')

    while True:
        try:
            client_socket, client_address = server.accept()
            client_handler = threading.Thread(target=handle_client, args=(client_socket, client_address))
            client_handler.daemon = True
            client_handler.start()
        except KeyboardInterrupt:
            print("\nShutting down server...")
            server.close()
            for client in clients:
                try:
                    client.close()
                except Exception:
                    pass
            sys.exit(0)
        except Exception as e:
            print(f"Server error: {e}", file=sys.stderr)

if __name__ == "__main__":
    start_server()
