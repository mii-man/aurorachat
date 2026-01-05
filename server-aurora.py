import socket
import threading
import time
import sys
from better_profanity import profanity
import os.path
import os
import bcrypt
import re
# Get directory of the auroraserver file
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACCOUNT_DIR = os.path.join(SCRIPT_DIR, "accounts")

os.makedirs(ACCOUNT_DIR, exist_ok=True)

# Initialize the profanity filter
profanity.load_censor_words()

# --- Configuration ---
HOST = '0.0.0.0'
PORT = 8961
RATE_LIMIT_MS = 1999  # one more millisecond of grace
MAX_MESSAGE_LENGTH = 456  # holy yappery
TERMINATION_TRIGGER = "<Fleetway>"

# --- Global State ---
clients = []
rate_limit = {}
connection_times = {}


# --- Helper Functions ---
class Client:
    def __init__(self, sock):
        self.socket = sock
        self.username = None
        self.logged_in = False

    def sendall(self, data):
        self.socket.sendall(data)

    def close(self):
        self.socket.close()

    def fileno(self):
        return self.socket.fileno()

    def __getattr__(self, attr):
        return getattr(self.socket, attr)


def process_chat_message(client, message, client_ip):
    SERVER_IPS = {"104.236.25.60"}  # Server IP (these can be public but any admin's IPs cannot) if one of these goes rogue then remove them from this list and restart the aurorachat server.

    message_strip = message.strip()
    if not message_strip:
        return

    if client_ip in SERVER_IPS:
        broadcast(f"{message_strip}\n")
        return
    if message_strip.startswith("CHAT,"):
        chat_body = message_strip[len("CHAT,"):]

        ip_regex = r"\b(?:25[0-5]|2[0-4]\d|1?\d?\d)(?:\.(?:25[0-5]|2[0-4]\d|1?\d?\d)){3}\b"

        if TERMINATION_TRIGGER in chat_body or re.search(ip_regex, chat_body):
            try:
                client.sendall(b"no doxxers here")
            except Exception:
                pass
            print("IP or previous doxxer Fleetway detected in chat message")
            raise ConnectionResetError(
                "IP address detected in chat message or previous doxxer Fleetway detected"
            )

    if len(message_strip) > MAX_MESSAGE_LENGTH:
        try:
            reject_message = f'Message too large! Max length is {MAX_MESSAGE_LENGTH} characters.\n'
            client.sendall(reject_message.encode('utf-8'))
        except Exception:
            pass
        return

    now = int(time.time() * 1000)
    last_msg = rate_limit.get(client, 0)
    if now - last_msg < RATE_LIMIT_MS:
        try:
            client.sendall('Spam detected! Wait 2 seconds.\n'.encode('utf-8'))
        except Exception:
            pass
        return

    rate_limit[client] = now

    censored_msg = profanity.censor(message_strip, '*')
    print(f"Message Processed: {repr(censored_msg)[1:-1]}")

    if message_strip.startswith("CHAT,"):
        if (client.logged_in == True):
            message_content = censored_msg[len("CHAT,"):]
            broadcast(f"<{client.username}>: {message_content}\n")
            print(f"Received: {message_strip} and broadcasted {message_content}")

    if message_strip.startswith("MAKEACC,"):
        parts = message_strip.split(",")
        if len(parts) >= 3:
            username = parts[1].strip()
            password = parts[2].strip()
            filepath = os.path.join(ACCOUNT_DIR, username)
            if not os.path.exists(filepath):
                client.sendall("USR_CREATED".encode('utf-8'))
                hashed = bcrypt.hashpw(password.encode('utf-8'), bcrypt.gensalt())
                with open(filepath, 'wb') as f: # welcome back ahh
                    f.write(hashed)
                print("Account created what a W")
                client.sendall("USR_CREATED".encode('utf-8'))
            else:
                client.sendall("USR_IN_USE".encode('utf-8')) # random but it only works with this here most of the time??? idk why
                print("Account exists bro get out")
                client.sendall("USR_IN_USE".encode('utf-8'))

    if message_strip.startswith("LOGINACC,"):
        parts = message_strip.split(",")
        if len(parts) >= 3:
            username = parts[1].strip()
            password = parts[2].strip()
            filepath = os.path.join(ACCOUNT_DIR, username)
            if os.path.exists(filepath):
                with open(filepath, 'rb') as f:
                    stored_hash = f.read()
                if bcrypt.checkpw(password.encode('utf-8'), stored_hash):
                    print("User logged in!")
                    client.username = username
                    client.logged_in = True
                    client.sendall("LOGIN_OK".encode('utf-8'))
                    broadcast(f"* {client.username} joined the chat\n")
                else:
                    print("Invalid password.")
                    client.sendall("LOGIN_WRONG_PASS".encode('utf-8'))
            else:
                print("Account not found.")
                client.sendall("LOGIN_FAKE_ACC".encode('utf-8'))
    
    if message_strip.startswith("LOGGEDIN?"):
        client.sendall(str(client.logged_in).encode('utf-8'))

    # This exists to help prevent old clients from insecurely connecting and also helps block spammers
    # i am commenting this out because it is conflicting with ip detection and i will do something else
#    if not any(message_strip.startswith(cmd) for cmd in ["CHAT,", "MAKEACC,", "LOGINACC,", "LOGGEDIN?"]):
#        try:
#            farewell_message = "Your client is outdated. Update to use aurorachat."
#            client.sendall(farewell_message.encode('utf-8'))
#        except Exception:
#            pass
#        print(f"they're old")
#        raise ConnectionResetError("Forced disconnect due to outdated client.")

    # this is somethign else
    if not any(message_strip.startswith(cmd) for cmd in ["CHAT,", "MAKEACC,", "LOGINACC,", "LOGGEDIN?"]):
        client.sendall("our servers cannot process your message and will be ignoring it, please consider updating your client".encode('utf-8'))
        print(f"Ignoring unknown command: {message_strip!r}")
        return

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


def remove_client(client):
    if client in clients:
        clients.remove(client)
    if client in rate_limit:
        del rate_limit[client]
    try:
        client.close()
    except Exception:
        pass
    print(f"Some sort of 'client' seems to have 'disconnected.'")


def handle_client(client_socket, client_address):
    client = Client(client_socket)
    client_ip = client_address[0]
    now_ms = int(time.time() * 1000)
    last_connection = connection_times.get(client_ip, 0)
    if now_ms - last_connection < RATE_LIMIT_MS:
        try:
            client.close()
        except Exception:
            pass
        return
    connection_times[client_ip] = now_ms

    clients.append(client)
    print(f"Client connection established.")

    try:
        buffer = ""
        while True:
            data = client.recv(4096)
            if not data:
                break
            received_chunk = data.decode('utf-8', errors='ignore')
            buffer += received_chunk

            message_to_process = buffer.strip()
            buffer = ""

            if message_to_process:
                process_chat_message(client, message_to_process, client_ip)


    except ConnectionResetError:
        print("Connection error from someplace: Connection reset or forced disconnect.")
    except socket.timeout:
        print("Connection error from something: Timeout.")
    except Exception as err:
        print(f"Connection error from... you tell me because I forgot. OOOH I FORGOT I FORGOT I FORGOT I FORGOT I FORGOT: {err}", file=sys.stderr)
    finally:
        remove_client(client)


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
    print(f'auroraserver v0.0.4 running on port {PORT}.')

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
