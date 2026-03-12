from flask import Flask, jsonify, request, send_from_directory
from flask import render_template
from flask import session, redirect, url_for
from flask_socketio import SocketIO, emit
from flask_cors import CORS
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
from dotenv import load_dotenv

import syscmd

# --- Configuration ---
load_dotenv()
HOST = os.getenv('HOST')
HTTP_PORT = int(os.getenv('AUC_HTTP_PORT'))
TCP_PORT = int(os.getenv('AUC_TCP_PORT'))
LATEST_VERSION = os.getenv('AUC_LATEST_VERSION')
RATE_LIMIT_MS = int(os.getenv('AUC_RATE_LIMIT_MS'))
MAX_MESSAGE_LENGTH = int(os.getenv('AUC_MAX_MESSAGE_LENGTH'))
USERNAME_MAX_CHARS = int(os.getenv('AUC_USERNAME_MAX_CHARS'))
PASSWORD_MAX_CHARS = int(os.getenv('AUC_PASSWORD_MAX_CHARS'))
TERMINATION_TRIGGER = os.getenv('AUC_TERMINATION_TRIGGER')
FLASK_SECRET_KEY = os.getenv('AUC_FLASK_SECRET_KEY')
RAWCHAT_KEY = os.getenv('AUC_RAWCHAT_KEY')

# i think at some point these should all be merged, for example one acc file with password/is banned/is admin/is known/tag on separate lines
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACCOUNT_DIR = os.path.join(SCRIPT_DIR, "accounts")
BANNEDUSR_DIR = os.path.join(SCRIPT_DIR, "bannedusers")
ACCOUNTIPS_DIR = os.path.join(SCRIPT_DIR, "accountips")
BANNEDIP_DIR = os.path.join(SCRIPT_DIR, "bannedips")
ADMIN_DIR = os.path.join(SCRIPT_DIR, "admins")
KNOWNUSR_DIR = os.path.join(SCRIPT_DIR, "knownusers")
TAG_DIR = os.path.join(SCRIPT_DIR, "usertags")

os.makedirs(ACCOUNT_DIR, exist_ok=True)
os.makedirs(BANNEDUSR_DIR, exist_ok=True)
os.makedirs(BANNEDIP_DIR, exist_ok=True)
os.makedirs(ACCOUNTIPS_DIR, exist_ok=True)
os.makedirs(ADMIN_DIR, exist_ok=True)
os.makedirs(KNOWNUSR_DIR, exist_ok=True)
os.makedirs(TAG_DIR, exist_ok=True)

# virt u put this globalization here and its horrifying (-orstando)
# global userCount
userCount = 0

# -- TCP Sockets --
tcp_clients = []

def broadcast(message):
    for client in tcp_clients[:]:
        try:
            client.sendall(message.encode('utf-8'))
        except:
            global userCount
            userCount -= 1
            tcp_clients.remove(client)
            
    socketio.emit('message', message)

def systembroadcast(message):
    for client in tcp_clients[:]:
        try:
            client.sendall("(Server) *[SYSTEM]*: ".encode('utf-8') + message.encode('utf-8'))
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
    server.bind(('0.0.0.0', TCP_PORT))
    server.listen(5)
    print("AuroraTCP running on port 4040")

    while True:
        client_sock, addr = server.accept()
        print("Client connected through TCP")
        tcp_clients.append(client_sock)
        global userCount
        userCount += 1
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

def checkAdmin(user):
   filepath = os.path.join(ADMIN_DIR, user)
   if os.path.exists(filepath):
      return True
   return False

# --- Command Parsing ---

def makeAccount(client,data):
      if all(key in data for key in ['username','password']): # Make sure username and password exist
            username = data['username']
            password = data['password']
            if "\\" in username or "/" in username or " " in username or len(username) > USERNAME_MAX_CHARS:
                 return {'data':'ILLEGAL'}
            if len(password) > PASSWORD_MAX_CHARS:
                 return {'data':'ILLEGAL'}
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

def loginAccount(client,data, ip):
      if all(key in data for key in ['username','password']): # Make sure username and password exist
            username = data['username']
            password = data['password']
            if TERMINATION_TRIGGER in username:
                  try:
                        return {'data':"UNAMETRIGGER"}
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
                              with open(os.path.join(ACCOUNTIPS_DIR, username), 'wb') as f:
                                    f.write(ip.encode('utf-8'))
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
            if data['rawkey'] == RAWCHAT_KEY:
                broadcast(f"{data['content']}\n")
                return {'data':"MSG_SENT"}
            else:
                print("dont try to rawchat without the key")
                return {'data':"WRONG_KEY"}
      else:
          if (client.loggedin):
                
                if (len(data['platform']) > 30):
                     return {'data':"ILLEGAL"}
    
                filepath2 = os.path.join(BANNEDUSR_DIR, client.username)
                if os.path.exists(filepath2):
                     return {'data':"BANNED"}
    
                if data['cmd'] == "CHAT":
                      typeIdentifier = "<>"
                elif data['cmd'] == "BOTCHAT":
                      typeIdentifier = "[]"
                elif data['cmd'] == "INTCHAT":
                      typeIdentifier = "{}"
                tag = " "
                if os.path.exists(os.path.join(TAG_DIR, client.username)):
                      with open(os.path.join(TAG_DIR, client.username)) as f:
                            tag = f" ~{f.read().strip()}~ "
                elif checkAdmin(client.username):
                      tag = " ~Moderator~ "
                usernameWithIdentifier = f"({data['platform']}){tag}{typeIdentifier[0]}{client.username}{typeIdentifier[1]}"
                
                censored_msg = profanity.censor(data['content'].strip(), '*')
                if not checkAdmin(client.username):
                  now = int(time.time() * 1000)
                  last_msg = rate_limit.get(client, 0)
                  if now - last_msg < RATE_LIMIT_MS:
                        return {'data':"SPAM",'limit':str(RATE_LIMIT_MS)}
                  rate_limit[client] = now

                if len(censored_msg) > MAX_MESSAGE_LENGTH:
                      return {'data':"TOOLONG",'limit':str(MAX_MESSAGE_LENGTH)}
                latestMsg = f"{usernameWithIdentifier}: {data['content']}\n"
                broadcast(f"{usernameWithIdentifier}: {censored_msg}\n")
                syscmd.checkCmd(client.username,data['content'],broadcast)
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
    global latestMsg
    request_json = request.get_json(silent=True)
    if not request_json:
      print(request.data)
      return {'data': 'NO_JSON'}, 400
    if request_json['cmd'] == "CONNECT":
        response = handleClient(request.remote_addr, request_json)
    elif request_json['cmd'] == "MAKEACC":
        response = makeAccount(clients[request.remote_addr], request_json)
    elif request_json['cmd'] == "LOGINACC":
        response = loginAccount(clients[request.remote_addr], request_json, request.remote_addr)
    elif request_json['cmd'] in ["CHAT", "BOTCHAT", "INTCHAT", "RAWCHAT"]:
        response = processMessage(clients[request.remote_addr], request_json)
    else:
        print("Command not recognized.")
        response = {'data': "BADCMD"}
    return response

# Grab auroraweb
@app.route("/web")
def auoraweb():
    return render_template('auroraweb.html')


@app.route('/admin/login', methods=['GET', 'POST'])
def admin_login():
    if request.method == 'POST':
        if os.path.isfile(os.path.join(ADMIN_DIR,request.form['username'])):
            with open(os.path.join(ADMIN_DIR,request.form['username']), 'rb') as f:
                if bcrypt.checkpw(request.form['password'].encode('utf-8'), f.read()):
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
        <input name="value" id="value" placeholder="Username to ban" required><br>
        <input name="reason" id="reason" placeholder="Ban reason"><br id="reasonbr">
        <select name="type" id="type" onchange="change()">
            <option value="user">User</option>
            <option value="ip">IP</option>
        </select>
        <select name="mode" id="mode" onchange="change()">
            <option value="ban">Ban</option>
            <option value="unban">Unban</option>
        </select><br>
        <button id="submit">Ban User</button>
    </form>
    <script>
    function change() {
        if (document.getElementById("type").value == "user") {
            if (document.getElementById("mode").value == "ban") {
                document.getElementById("submit").innerHTML = "Ban User";
                document.getElementById("value").setAttribute('placeholder','Username to ban');
                document.getElementById("reason").style.display="inline";
                document.getElementById("reasonbr").style.display="inline"
            } else if (document.getElementById("mode").value == "unban") {
                document.getElementById("submit").innerHTML = "Unban User";
                document.getElementById("value").setAttribute('placeholder','Username to unban');
                document.getElementById("reason").style.display="none";
                document.getElementById("reasonbr").style.display="none"
            }
        } else if (document.getElementById("type").value == "ip") {
            if (document.getElementById("mode").value == "ban") {
                document.getElementById("submit").innerHTML = "Ban IP";
                document.getElementById("value").setAttribute('placeholder','IP to ban');
                document.getElementById("reason").style.display="inline";
                document.getElementById("reasonbr").style.display="inline"
            } else if (document.getElementById("mode").value == "unban") {
                document.getElementById("submit").innerHTML = "Unban IP";
                document.getElementById("value").setAttribute('placeholder','IP to unban');
                document.getElementById("reason").style.display="none";
                document.getElementById("reasonbr").style.display="inline"
            }
        }
    }
    </script>
    '''

@app.route('/ban', methods=['POST'])
def ban_user():
    if not session.get('admin'):
        return 'Unauthorized', 403
    value = request.form['value']
    type = request.form['type'] # user / ip
    mode = request.form['mode'] # ban / unban
    if type == 'user':
        dir = BANNEDUSR_DIR
    elif type == 'ip':
        dir = BANNEDIP_DIR
    if mode == 'ban':
        with open(os.path.join(dir, value), 'w') as f:
            f.write(request.form['reason'])
    elif mode == 'unban':
        os.remove(os.path.join(dir, value))
    return redirect('/admin')


@socketio.on('connect')
def handle_connect():
    print("WS Connection Established.")
    socketio.emit('message', 'Connected!')





if __name__ == "__main__":
      socketio.run(app, host="0.0.0.0", port=HTTP_PORT, use_reloader=False)
