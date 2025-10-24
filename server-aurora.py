import socket
import threading
import time
import sys

# --- Auto-install missing dependency ---
try:
    from better_profanity import profanity
except ImportError:
    print("Installing 'better_profanity' module...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "better_profanity"])
    from better_profanity import profanity

# Initialize the profanity filter
profanity.load_censor_words()

# --- Configuration ---
HOST = '0.0.0.0'
PORT = 8961
RATE_LIMIT_MS = 2000

# --- Global State ---
clients = []
# Banning system (banned_ips) has been REMOVED!
# Rate limit is now keyed only by the client_socket object.
rate_limit = {} 

# --- Helper Functions ---


def process_chat_message(client_socket, message):
    """
    Helper function to process a single message from a client.
    """
    if not message.strip():
        return

    # 1. Rate Limiting Check
    now = int(time.time() * 1000)
    # Rate limit key is the client_socket itself.
    last_msg = rate_limit.get(client_socket, 0)
    
    if now - last_msg < RATE_LIMIT_MS:
        try:
            client_socket.sendall('Spam detected! Wait 2 seconds.\n'.encode('utf-8'))
        except Exception:
            pass 
        return 
        
    # 2. Update Rate Limit (ONLY for valid messages)
    rate_limit[client_socket] = now
    
    # 3. Profanity Check
    censored_msg = profanity.censor(message.strip(), '*')

    print(f"Message Processed: {censored_msg}")
        
    # 5. Broadcast Message
    broadcast(f"{censored_msg}\n")


def broadcast(message):
    """
    Broadcast a message to all connected clients (including sender).
    """
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
    """
    Remove a client from global lists.
    """
        
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
    """
    Handles all communication with a single client in a separate thread.
    """

    print('Client connection established.')
    clients.append(client_socket)
    
    try:
        while True:
            # Receive data in chunks
            data = client_socket.recv(4096)
            if not data:
                break
            
            # Decode the entire received chunk
            received_message = data.decode('utf-8', errors='ignore')
            
            # Process the entire received chunk as a message
            message_to_process = received_message.strip()

            if message_to_process:
                process_chat_message(client_socket, message_to_process) 
            
    except ConnectionResetError:
        print("Connection error from someplace: Connection reset.")
    except socket.timeout:
        # LOGGING FIX: IP REMOVED
        print("Connection error from something: Timeout.")
    except Exception as err:
        print(f"Connection error from... you tell me because I forgot. OOOH I FORGOT I FORGOT I FORGOT I FORGOT I FORGOT: {err}", file=sys.stderr)
        
    finally:
        remove_client(client_socket)
        
# --- Main Server Logic ---

def start_server():
    """
    Initializes and starts the TCP server.
    """
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server.bind((HOST, PORT))
        server.listen(5)
    except Exception as e:
        print(f"Could not bind to port {PORT}: {e}", file=sys.stderr)
        sys.exit(1)
        
    print(f'aurorachat server v0.0.1 running on port {PORT}.') # Hi this is hacker tron here! This server is completely compatible with hbchat clients! cool

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
