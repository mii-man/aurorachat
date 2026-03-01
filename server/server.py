# ascii art cuz why not
#    _                             ____
#   / \  _   _ _ __ ___  _ __ __ _/ ___|  ___ _ ____   _____ _ __
#  / _ \| | | | '__/ _ \| '__/ _` \___ \ / _ \ '__\ \ / / _ \ '__|
# / ___ \ |_| | | | (_) | | | (_| |___) |  __/ |   \ V /  __/ |
#/_/   \_\__,_|_|  \___/|_|  \__,_|____/ \___|_|    \_/ \___|_|
# the main server for aurorachat
# made using flask and socket
from flask import Flask, jsonify, request
from flask import session, redirect, url_for
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

import syscmd

from flask import session, redirect, url_for
from flask_socketio import SocketIO, emit
from flask_cors import CORS

# --- Configuration ---
HOST = '0.0.0.0'
PORT = 8961
LATEST_VERSION = "4.3"
RATE_LIMIT_MS = 1998  # one more millisecond of grace # another millisecond of grace
MAX_MESSAGE_LENGTH = 457  # holy yappery #one extra letter of yap
TERMINATION_TRIGGER = "Fleetway"
FLASK_SECRET_KEY = "[redacted]" # MAKE SURE TO REDACT BEFORE COMMITTING!!
PANEL_PASSWORD = "[redacted]"
RAWCHAT_KEY = "[redacted]"

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACCOUNT_DIR = os.path.join(SCRIPT_DIR, "accounts")
BANNEDUSR_DIR = os.path.join(SCRIPT_DIR, "bannedusers")
BANNEDIP_DIR = os.path.join(SCRIPT_DIR, "bannedips")
ADMIN_DIR = os.path.join(SCRIPT_DIR, "admins")
KNOWNUSR_DIR = os.path.join(SCRIPT_DIR, "knownusers") # at some point this should be merged with the normal account files

os.makedirs(ACCOUNT_DIR, exist_ok=True)
os.makedirs(BANNEDUSR_DIR, exist_ok=True)
os.makedirs(ADMIN_DIR, exist_ok=True)
os.makedirs(KNOWNUSR_DIR, exist_ok=True)
# -- TCP Sockets --
tcp_clients = []

def broadcast(message):
    for client in tcp_clients[:]:
        try:
            client.sendall(message.encode('utf-8'))
        except:
            tcp_clients.remove(client)
            
    socketio.emit('message', message)

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

# --- Global State ---
clients = {}
rate_limit = {}
connection_times = {}
latestMsg = ""
msg_lock = threading.Lock()

profanity.load_censor_words(whitelist_words=['yaoi', 'gay', 'lamo', 'frick', 'crap', 'fuck', 'god', 'shit', 'heck', 'hell', 'ass', 'stupid', 'murder', 'uzi', 'weed', 'piss', 'kill'])
profanity.add_censor_words(["67"])

app = Flask(__name__)

app.secret_key = FLASK_SECRET_KEY

CORS(app)

socketio = SocketIO(app, cors_allowed_origins="*")

socketio.start_background_task(start_tcp_server)

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
                 return {'data':"BANNED"}

def processMessage(client,data):
      global latestMsg
        if data['cmd'] == "RAWCHAT":
            if data['rawkey'] == RAWCHAT_KEY: #put whatever you want for the key
                broadcast(data['content'])
                return {'data':"MSG_SENT"}
            else:
                print("dont try to rawchat without the key")
                return {'data':"WRONG_KEY"}
        else:
          if (client.loggedin):
    
                filepath2 = os.path.join(BANNEDUSR_DIR, client.username)
                if os.path.exists(filepath2):
                     return {'data':"BANNED"}
    
                if data['cmd'] == "CHAT":
                      typeIdentifier = "<>"
                elif data['cmd'] == "BOTCHAT":
                      typeIdentifier = "[]"
                elif data['cmd'] == "INTCHAT":
                      typeIdentifier = "{}"
                usernameWithIdentifier = f"{typeIdentifier[0]}{client.username}{typeIdentifier[1]}"
                censored_msg = profanity.censor(data['content'].strip(), '*')
                now = int(time.time() * 1000)
                last_msg = rate_limit.get(client, 0)
                if now - last_msg < RATE_LIMIT_MS:
                      return {'data':"SPAM",'limit':str(RATE_LIMIT_MS)}
                rate_limit[client] = now
                if len(censored_msg) > MAX_MESSAGE_LENGTH:
                      return {'data':"TOOLONG",'limit':str(MAX_MESSAGE_LENGTH)}
                latestMsg = f"{usernameWithIdentifier}: {data['content']}\n"
                broadcast(f"{usernameWithIdentifier}: {censored_msg}\n")
                cmdResult = syscmd.checkCmd(client.username,data['content'],[ACCOUNT_DIR,BANNEDUSR_DIR,BANNEDIP_DIR,ADMIN_DIR,KNOWNUSR_DIR],broadcast)
                return {'data':"MSG_SENT"}
          else:
                return {'data':"NO_LOGIN"}

# --- Client Handling ---
def handleClient(address, data):
      filepath = os.path.join(BANNEDIP_DIR, address)
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
            if not LATEST_VERSION in data['version']:
                  print("Oudated Client connection established.")
                  return {'data':"CONNECT_OK", 'info':"OUTDATED"}
            if LATEST_VERSION in data['version']:
                 print("Client connection established")
                 return {'data':"CONNECT_OK"}
      else:
            print("A banned user tried to log in.")
            return {'data':"BANNED"}

@app.route('/api')
def error405():
      return "Please use POST instead.", 405

@app.route('/')
def rootRedirect():
      return redirect('/api')

# --- Main Server Logic ---

@app.route('/api', methods=['POST'])
def process_request():
    server_running.wait()
    global latestMsg
    request_json = request.get_json(silent=True)
    if not request_json:
      print(request.data)
      return {'data': 'NO_JSON'}, 400
    if request_json['cmd'] == "CONNECT":
        response = handleClient(sha256(request.remote_addr), request_json)
    elif request_json['cmd'] == "MAKEACC":
        response = makeAccount(clients[sha256(request.remote_addr)], request_json)
    elif request_json['cmd'] == "LOGINACC":
        response = loginAccount(clients[sha256(request.remote_addr)], request_json)
    elif request_json['cmd'] in ["CHAT", "BOTCHAT", "INTCHAT", "RAWCHAT"]:
        response = processMessage(clients[sha256(request.remote_addr)], request_json)
    else:
        print("Command not recognized.")
        response = {'data': "BADCMD"}
    return response




# This panel is simple and untested. I wouldn't suggest using it. Set a cryptographically random password so it can't be cracked.

@app.route('/admin/login', methods=['GET', 'POST'])
def admin_login():
    if request.method == 'POST':
        if os.path.isfile(request.form['username']):
            with open(os.path.join(ADMIN_DIR,request.form['username']) as f:
                if request.form['password'] == f.read():
                    session['admin'] = True
                    return redirect('/admin')
        return 'Invalid username or password', 401
    return '''
    <form method="POST">
        <input type="text" name="username" placeholder="Username" required>
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


@socketio.on('connect')
def handle_connect():
    print("WS Connection Established.")
    socketio.emit('message', 'Connected!')





if __name__ == "__main__":
      socketio.run(app, host="0.0.0.0", port=3072, use_reloader=False)
