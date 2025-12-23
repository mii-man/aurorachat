import socket
import ssl
import struct
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
MAX_CONNECTIONS_PER_IP = 2

CERTFILE = "server.pem"
KEYFILE = "server.key"

# --- Global State ---
clients = []               # list of SSLSocket objects
connections_per_ip = {}    # dict: ip -> set of sockets
rate_limit = {}            # maps SSLSocket -> last_msg_ts_ms
clients_lock = threading.Lock()

# --- Helper Functions ---


def process_chat_message(client_socket, message, client_ip):
    SERVER_IPS = {"127.0.0.1", "104.236.25.60"} # Server IP (these can be public but any admin's IPs cannot) if one of these goes rogue then remove them from this list and restart the aurorachat server.
    message_strip = message.strip()
    if not message_strip:
        return

    if client_ip in SERVER_IPS:
        broadcast(f"{message_strip}\n")
        return

    # Termination Trigger Check
    if TERMINATION_TRIGGER in message_strip:
        try:
            client_socket.sendall(
                b"No no, we aren't doxxers here. Restart if you wanna come back\n"
            )
        except Exception:
            pass
        print("lmaooo fleetway detected get this guy OUT")
        raise ConnectionResetError("Forced disconnect due to Fleetway name")

    if len(message_strip) > MAX_MESSAGE_LENGTH:
        try:
            client_socket.sendall(
                f"Message too large! Max length is {MAX_MESSAGE_LENGTH} characters.\n".encode()
            )
        except Exception:
            pass 
        return

    # Rate Limiting Check
    now = int(time.time() * 1000)
    with clients_lock:
        last_msg = rate_limit.get(client_socket, 0)
        if now - last_msg < RATE_LIMIT_MS:
            try:
                client_socket.sendall(b"Spam detected! Wait 2 seconds.\n")
            except Exception:
                pass
            return
        rate_limit[client_socket] = now

    censored_msg = profanity.censor(message_strip, '*')
    print(f"Message Processed: {repr(censored_msg)[1:-1]}")

    broadcast(f"{censored_msg}\n")


def broadcast(message):
    """
    Send message to all clients. Remove any dead clients encountered.
    """
    dead = []
    # snapshot clients to avoid holding lock while sending
    with clients_lock:
        snapshot = list(clients)

    for c in snapshot:
        try:
            data = message.encode("utf-8")
            pkt = len(data).to_bytes(4, "big") + data
            c.sendall(pkt)
        except Exception:
            dead.append(c)

    # cleanup dead sockets
    for c in set(dead):
        remove_client(c)


def remove_client(client_socket):
    client_ip = getattr(client_socket, "client_ip", None)

    with clients_lock:
        if client_socket in clients:
            clients.remove(client_socket)
        rate_limit.pop(client_socket, None)

        if client_ip:
            ip_set = connections_per_ip.get(client_ip)
            if ip_set:
                ip_set.discard(client_socket)
                if not ip_set:
                    del connections_per_ip[client_ip]

    # Attempt a polite TLS shutdown to send close_notify.
    try:
        # If it's an SSLSocket, unwrap to send close_notify.
        if hasattr(client_socket, "unwrap"):
            try:
                raw = client_socket.unwrap()  # sends close_notify
                try:
                    raw.shutdown(socket.SHUT_RDWR)
                except Exception:
                    pass
                try:
                    raw.close()
                except Exception:
                    pass
            except ssl.SSLError:
                # unwrap failed; try shutting down the SSLSocket and closing
                try:
                    client_socket.shutdown(socket.SHUT_RDWR)
                except Exception:
                    pass
                try:
                    client_socket.close()
                except Exception:
                    pass
        else:
            # Not an SSLSocket — just try normal shutdown/close
            try:
                client_socket.shutdown(socket.SHUT_RDWR)
            except Exception:
                pass
            try:
                client_socket.close()
            except Exception:
                pass
    except Exception:
        # Last-resort close
        try:
            client_socket.close()
        except Exception:
            pass

    with clients_lock:
        remaining = len(clients)
    print(f"Client disconnected. {remaining} client(s) remaining.")


def handle_client(ssl_sock, addr):
    client_ip = addr[0]
    ssl_sock.client_ip = client_ip
    buffer = b""

    try:
        ssl_sock.settimeout(1.0)

        while True:
            try:
                data = ssl_sock.recv(4096)
                if not data:
                    break

                buffer += data

                while len(buffer) >= 4:
                    msg_len = struct.unpack("!I", buffer[:4])[0]

                    if msg_len > MAX_MESSAGE_LENGTH:
                        raise ConnectionResetError("Message too large")

                    if len(buffer) < 4 + msg_len:
                        break  # wait for more data

                    msg_bytes = buffer[4:4 + msg_len]
                    buffer = buffer[4 + msg_len:]

                    try:
                        message = msg_bytes.decode("utf-8", errors="replace")
                    except Exception:
                        continue

                    process_chat_message(ssl_sock, message, client_ip)

            except socket.timeout:
                continue

    except Exception as e:
        print("Handler exception:", repr(e), file=sys.stderr)

    finally:
        remove_client(ssl_sock)


def tls_wrap_nonblocking(context, raw_sock, addr, timeout=5.0):
    """
    Wraps a TCP socket in TLS without freezing the server.
    Returns a fully handshaken ssl_sock or None on failure.
    After handshake succeeds, the returned ssl_sock is blocking
    with a short recv timeout for disconnect detection.
    """
    raw_sock.settimeout(5)
    try:
        first = raw_sock.recv(1, socket.MSG_PEEK)
    except Exception as e:
        print("No data from client → rejected:", e)
        raw_sock.close()
        return None

    if not first:
        print("No data from client → rejected")
        raw_sock.close()
        return None

    try:
        ssl_sock = context.wrap_socket(
            raw_sock,
            server_side=True,
            do_handshake_on_connect=False
        )
    except Exception as e:
        print("wrap_socket failed:", e)
        raw_sock.close()
        return None

    # Blocking for handshake loop and subsequent I/O
    ssl_sock.setblocking(True)

    start = time.time()
    while True:
        try:
            ssl_sock.do_handshake()
            # Handshake done — set a short recv timeout so recv() doesn't hang forever
            ssl_sock.settimeout(1.0)
            return ssl_sock

        except ssl.SSLWantReadError:
            if time.time() - start > timeout:
                print("Handshake timeout → dropped")
                try:
                    ssl_sock.close()
                except Exception:
                    pass
                return None
            time.sleep(0.05)
            continue

        except ssl.SSLError as e:
            print("Handshake error:", e)
            try:
                ssl_sock.close()
            except Exception:
                pass
            return None

# --- Main Server Logic ---

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

    try:
        while True:
            try:
                raw_client, addr = server.accept()

                ssl_client = tls_wrap_nonblocking(context, raw_client, addr)
                if not ssl_client:
                    continue  # handshake failed or not TLS

                client_ip = addr[0]

                with clients_lock:
                    ip_set = connections_per_ip.get(client_ip, set())

                    if len(ip_set) >= MAX_CONNECTIONS_PER_IP:
                        print(f"Rejecting connection from {client_ip} (limit reached)")
                        try:
                            ssl_client.sendall(b"Too many connections from your IP\n")
                        except Exception:
                            pass
                        ssl_client.close()
                        continue
                    
                    ip_set.add(ssl_client)
                    connections_per_ip[client_ip] = ip_set
                    clients.append(ssl_client)

                    total = len(clients)

                print(f"Client connected. Total clients: {total}")

                t = threading.Thread(target=handle_client, args=(ssl_client, addr))
                t.daemon = True
                t.start()

            except KeyboardInterrupt:
                print("\nShutting down...")
                break

            except Exception as e:
                print("Server error:", e)
    finally:
        # Graceful shutdown: close server and all clients
        print("Server shutting down: closing server socket and clients...")
        try:
            server.close()
        except Exception:
            pass

        # remove_client will attempt polite TLS shutdown for each client
        with clients_lock:
            remaining_clients = list(clients)
        for c in remaining_clients:
            try:
                remove_client(c)
            except Exception:
                pass

if __name__ == "__main__":
    start_server()
