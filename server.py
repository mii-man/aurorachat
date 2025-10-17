import socket
import threading

clients = []

def handle_client(conn, addr):
    print(f"{addr} connected")
    while True:
        try:
            data = conn.recv(1024)
            if not data:
                break
            for c in clients:
                if c != conn:
                    c.sendall(data)
        except:
            break
    print(f"{addr} disconnected")
    clients.remove(conn)
    conn.close()

HOST = "0.0.0.0"
PORT = 5000
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen()
print(f"Server running on port {PORT}")

while True:
    conn, addr = server.accept()
    clients.append(conn)
    threading.Thread(target=handle_client, args=(conn, addr)).start()
