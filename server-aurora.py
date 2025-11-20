import socket
import ssl
import threading
import time
import sys
from better_profanity import profanity

# Initialize profanity filter
profanity.load_censor_words()

# --- Configuration ---
HOST = '0.0.0.0'
PORT = 8961
RATE_LIMIT_MS = 2000
MAX_MESSAGE_LENGTH = 456
TERMINATION_TRIGGER = "<Fleetway>"

CERTFILE = "cert.pem"
KEYFILE = "key.pem"

# --- Global State ---
clients = []
rate_limit = {}
connection_times = {}

# --- Helper Functions ---

def process_chat_message(client_socket, message):
    message_strip = message.strip()
    if not message_strip:
        return

    if TERMINATION_TRIGGER in message_strip:
        try:
            client_socket.sendall(
                b"No no, we aren't doxxers here. Restart if you wanna come back"
            )
        except:
            pass
        print("lmaooo fleetway detected get this guy OUT")
        raise ConnectionResetError("Forced disconnect due to Fleetway name")

    if len(message_strip) > MAX_MESSAGE_LENGTH:
        try:
            client_socket.sendall(
                f"Message too large! Max length is {MAX_MESSAGE_LENGTH} characters.\n".encode()
            )
        except:
            pass
        return

    now = int(time.time() * 1000)
    last_msg = rate_limit.get(client_socket, 0)
    if now - last_msg < RATE_LIMIT_MS:
        try:
            client_socket.sendall(b"Spam detected! Wait 2 seconds.\n")
        except:
            pass
        return

    rate_limit[client_socket] = now

    censored_msg = profanity.censor(message_strip, '*')
    print(f"Message Processed: {repr(censored_msg)[1:-1]}")
    broadcast(censored_msg + "\n")


def broadcast(message):
    dead = []
    for c in clients:
        try:
            c.sendall(message.encode("utf-8"))
        except:
            dead.append(c)

    for c in dead:
        remove_client(c)


def remove_client(client_socket):
    if client_socket in clients:
        clients.remove(client_socket)
    if client_socket in rate_limit:
        del rate_limit[client_socket]
    try:
        client_socket.close()
    except:
        pass
    print("Client disconnected.")


def handle_client(ssl_sock, addr):
    client_ip = addr[0]
    now_ms = int(time.time() * 1000)

    last = connection_times.get(client_ip, 0)
    if now_ms - last < RATE_LIMIT_MS:
        try:
            ssl_sock.close()
        except:
            pass
        return

    connection_times[client_ip] = now_ms

    clients.append(ssl_sock)
    print(f"Client connection established.")

    try:
        buffer = ""
        while True:
            data = ssl_sock.recv(4096)
            if not data:
                break
            chunk = data.decode("utf-8", errors="ignore")
            buffer += chunk

            msg = buffer.strip()
            buffer = ""

            if msg:
                process_chat_message(ssl_sock, msg)

    except Exception as e:
        # Treat "operation did not complete" as a normal disconnect
        if not "operation did not complete" in str(e):
            print(f"SSL socket error: {e}", file=sys.stderr)
    finally:
        remove_client(ssl_sock)

def tls_wrap_nonblocking(context, raw_sock, addr, timeout=5.0):
    """
    Wraps a TCP socket in TLS without freezing the server.
    Returns a fully handshaken ssl_sock or None on failure.
    """
    raw_sock.settimeout(5)
    first = raw_sock.recv(1, socket.MSG_PEEK)
    if not first:
        print(f"No data from client → rejected")
        raw_sock.close()
        return None

    try:
        ssl_sock = context.wrap_socket(
            raw_sock,
            server_side=True,
            do_handshake_on_connect=False
        )
    except Exception as e:
        print(f"wrap_socket failed:", e)
        raw_sock.close()
        return None

    ssl_sock.setblocking(False)

    start = time.time()
    while True:
        try:
            ssl_sock.do_handshake()
            return ssl_sock

        except ssl.SSLWantReadError:
            if time.time() - start > timeout:
                print(f"Handshake timeout → dropped")
                ssl_sock.close()
                return None
            time.sleep(0.05)
            continue

        except ssl.SSLError as e:
            print(f"Handshake error:", e)
            ssl_sock.close()
            return None

def start_server():
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)

    try:
        context.load_cert_chain(certfile=CERTFILE, keyfile=KEYFILE)
    except Exception as e:
        print("Failed to load cert/key:", e)
        sys.exit(1)

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(5)

    print(f"aurorachat server running on port {PORT}")

    while True:
        try:
            raw_client, addr = server.accept()

            ssl_client = tls_wrap_nonblocking(context, raw_client, addr)

            if not ssl_client:
                continue  # handshake failed or not TLS

            t = threading.Thread(target=handle_client, args=(ssl_client, addr))
            t.daemon = True
            t.start()

        except KeyboardInterrupt:
            print("\nShutting down...")
            break

        except Exception as e:
            print("Server error:", e)

    server.close()
    for c in list(clients):
        try: c.close()
        except: pass

if __name__ == "__main__":
    start_server()
