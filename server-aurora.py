from flask import Flask, jsonify, request
import threading
import time
import sys
from better_profanity import profanity
import os.path
import os
import bcrypt
import hashlib
import threading
import socket
from flask import session, redirect, url_for


# -- TCP Sockets --
tcp_clients = []

def broadcast(message):
    for client in tcp_clients[:]:
        try:
            client.sendall(message.encode('utf-8'))
        except:
            tcp_clients.remove(client)

def handle_tcp_client(client_socket):
    try:
        while True:
            data = client_socket.recv(1024)
            if not data:
                break
    except:
        pass
    finally:
        if client_socket in tcp_clients:
            tcp_clients.remove(client_socket)
        client_socket.close()

def start_tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('0.0.0.0', 4040))
    server.listen(5)
    print("AuroraTCP running on port 4040")

    while True:
        client_sock, addr = server.accept()
        print("Client connected through TCP")
        tcp_clients.append(client_sock)
        t = threading.Thread(target=handle_tcp_client, args=(client_sock,), daemon=True)
        t.start()

# Start TCP server in background so that the Flask server doesn't explode
threading.Thread(target=start_tcp_server, daemon=True).start()

# --- Configuration ---
HOST = '0.0.0.0'
PORT = 8961
RATE_LIMIT_MS = 1999  # one more millisecond of grace
MAX_MESSAGE_LENGTH = 456  # holy yappery
TERMINATION_TRIGGER = "Fleetway"

# --- Global State ---
clients = {}
rate_limit = {}
connection_times = {}
latestMsg = ""
msg_lock = threading.Lock()

# Account file init
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACCOUNT_DIR = os.path.join(SCRIPT_DIR, "accounts")
BANNEDUSR_DIR = os.path.join(SCRIPT_DIR, "bannedusers")
ADMIN_DIR = os.path.join(SCRIPT_DIR, "admins")

os.makedirs(ACCOUNT_DIR, exist_ok=True)
os.makedirs(BANNEDUSR_DIR, exist_ok=True)
os.makedirs(ADMIN_DIR, exist_ok=True)

# Profanity init
profanity.load_censor_words()

app = Flask(__name__)

app.secret_key = "put a random key here"

# --- Helper Functions ---
def sha256(data):
      sha256Obj = hashlib.sha256(data.encode('utf-8'))
      finalHash = sha256Obj.hexdigest()
      return finalHash

class Client:
      def __init__(self):
            self.username = None
            self.loggedin = False

# --- Command Parsing ---
def makeAccount(client,data):
      if all(key in data for key in ['username','password']): # Make sure username and password exist
            username = data['username']
            password = data['password']
            filepath = os.path.join(ACCOUNT_DIR, username)
            if not os.path.exists(filepath):
                  hashed = bcrypt.hashpw(password.encode('utf-8'), bcrypt.gensalt())
                  with open(filepath, 'wb') as f: # welcome back ahh
                        f.write(hashed)
                  print("Account created what a W")
                  return {'data':"USR_CREATED"}
            else:
                  return {'data':"USR_IN_USE"}
                  print("Account exists bro get out")

def loginAccount(client,data):
      if all(key in data for key in ['username','password']): # Make sure username and password exist
            username = data['username']
            password = data['password']
            if TERMINATION_TRIGGER in username:
                  try:
                        return {'data':TERMINATION_TRIGGER.lower()}
                  except Exception:
                        pass
                  print(f"lmaooo {TERMINATION_TRIGGER.lower()} detected get this guy OUT")
                  raise ConnectionResetError(f"Forced disconnect due to client's name being {username}.")
            filepath2 = os.path.join(BANNEDUSR_DIR, username)
            if not os.path.exists(filepath2):
                  filepath = os.path.join(ACCOUNT_DIR, username)
                  if os.path.exists(filepath):
                        with open(filepath, 'rb') as f:
                              stored_hash = f.read()
                        if bcrypt.checkpw(password.encode('utf-8'), stored_hash):
                              print("User logged in!")
                              client.username = username
                              client.loggedin = True
                              return {'data':"LOGIN_OK"}
                        else:
                              print("Invalid password.")
                              return {'data':"LOGIN_WRONG_PASS"}
                  else:
                        print("Account not found.")
                        return {'data':"LOGIN_FAKE_ACC"}
            else:
                 print("you're BANNED. LOSER")
                 return {'data':"frick you you're BANNED"}

def processMessage(client,data):
      global latestMsg
      if (client.loggedin):

            filepath2 = os.path.join(BANNEDUSR_DIR, client.username)
            if os.path.exists(filepath2):
                 return {'data':"youre BANNED, LOSER"}

            if data['cmd'] == "CHAT":
                  typeIdentifier = "<>"
            elif data['cmd'] == "BOTCHAT":
                  typeIdentifier = "[]"
            elif data['cmd'] == "INTCHAT":
                  typeIdentifier = "{}"
            usernameWithIdentifier = f"{typeIdentifier[0]}{client.username}{typeIdentifier[1]}"
            censored_msg = profanity.censor(data['content'].strip(), '*')
            if data['content'].startswith('/ban '):
                  parts = data['content'].split(' ', 1)
                  username_to_ban = parts[1].strip()

                  filepath3 = os.path.join(ADMIN_DIR, client.username)
                  if not os.path.exists(filepath3):
                       return {'data':"not admin"}

                  filepath = os.path.join(ACCOUNT_DIR, username_to_ban)
                  if os.path.exists(filepath):
                        with open(os.path.join(BANNEDUSR_DIR, username_to_ban), 'w') as f:
                              f.write('banned')
            
            if data['content'].startswith('/unban '):
                  parts = data['content'].split(' ', 1)
                  username_to_ban = parts[1].strip()

                  filepath3 = os.path.join(ADMIN_DIR, client.username)
                  if not os.path.exists(filepath3):
                       return {'data':"not admin"}

                  filepath = os.path.join(ACCOUNT_DIR, username_to_ban)
                  if os.path.exists(filepath):
                        os.remove(os.path.join(BANNEDUSR_DIR, username_to_ban))
            now = int(time.time() * 1000)
            last_msg = rate_limit.get(client, 0)
            if now - last_msg < RATE_LIMIT_MS:
                  return {'data':"SPAM",'limit':str(RATE_LIMIT_MS)}
            rate_limit[client] = now
            if len(censored_msg) > MAX_MESSAGE_LENGTH:
                  return {'data':"TOOLONG",'limit':str(MAX_MESSAGE_LENGTH)}
            latestMsg = f"{usernameWithIdentifier}: {data['content']}\n"
            print(f"Received: {data} and parsed {censored_msg}")
            broadcast(f"{usernameWithIdentifier}: {censored_msg}\n")
            return {'data':"MSG_SENT"}
      else:
            return {'data':"NO_LOGIN"}

# --- Client Handling ---
def handleClient(address):
      filepath = os.path.join(BANNEDUSR_DIR, address)
      if not os.path.exists(filepath):
            client = Client()
            now_ms = int(time.time() * 1000)
            last_connection = connection_times.get(address, 0)
            if now_ms - last_connection < RATE_LIMIT_MS:
                  try:
                        with open(filepath, 'wb') as f:
                              f.write("Too many connections at once")
                  except Exception:
                        pass
                  return
            connection_times[address] = now_ms
            clients[address] = client
            print("Client connection established.")
            return {'data':"CONNECT_OK"}
      else:
            print("A banned user tried to log in.")
            return {'data':"BANNED"}

@app.route('/api')
def error405():
      return "Please use POST instead.", 405

# --- Main Server Logic ---

@app.route('/api', methods=['POST'])
def process_request():
    global latestMsg
    request_json = request.get_json(silent=True)
    if not request_json:
      print(request.data)
      return {'data': 'NO_JSON'}, 400
    if request_json['cmd'] == "CONNECT":
        response = handleClient(sha256(request.remote_addr))
    elif request_json['cmd'] == "MAKEACC":
        response = makeAccount(clients[sha256(request.remote_addr)], request_json)
    elif request_json['cmd'] == "LOGINACC":
        response = loginAccount(clients[sha256(request.remote_addr)], request_json)
    elif request_json['cmd'] in ["CHAT", "BOTCHAT", "INTCHAT"]:
        response = processMessage(clients[sha256(request.remote_addr)], request_json)
    else:
        print("Command not recognized.")
        response = {'data': "BADCMD"}
    return response




# This panel is simple and untested. I wouldn't suggest using it. Set a cryptographically random password so it can't be cracked.

@app.route('/admin/login', methods=['GET', 'POST'])
def admin_login():
    if request.method == 'POST':
        if request.form['password'] == 'password here':
            session['admin'] = True
            return redirect('/admin')
        return 'Invalid password', 401
    return '''
    <form method="POST">
        <input type="password" name="password" placeholder="Password" required>
        <button type="submit">Login</button>
    </form>
    '''

@app.route('/admin')
def admin():
    if not session.get('admin'):
        return redirect('/admin/login')
    return '''
    <form method="POST" action="/ban">
        <input name="username" placeholder="Username to ban" required>
        <button>Ban User</button>
    </form>
    '''

@app.route('/ban', methods=['POST'])
def ban_user():
    if not session.get('admin'):
        return 'Unauthorized', 403
    username = request.form['username']
    with open(os.path.join(BANNEDUSR_DIR, username), 'w') as f:
        f.write('banned')
    return redirect('/admin')





if __name__ == "__main__":
      app.run(host="0.0.0.0", port=3072, threaded=True, use_reloader=False)
