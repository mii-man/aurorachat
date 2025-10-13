import socket
import threading

clients = []

# Function to handle each client
def handle_client(conn, addr):
    print(f"{addr} connected")
    while True:
        try:
            data = conn.recv(1024)
            if not data:
                break
            # Broadcast the message to all other clients
            for c in clients:
                if c != conn:
                    c.sendall(data)
        except:
            break
    print(f"{addr} disconnected")
    clients.remove(conn)
    conn.close()

# Create server socket
HOST = "0.0.0.0"  # listen on all interfaces
PORT = 5000       # pick a port
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen()
print(f"Server running on port {PORT}")

# Accept clients
while True:
    conn, addr = server.accept()
    clients.append(conn)
    threading.Thread(target=handle_client, args=(conn, addr)).start()
