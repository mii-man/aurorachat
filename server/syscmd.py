import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACCOUNT_DIR = os.path.join(SCRIPT_DIR, "accounts")
BANNEDUSR_DIR = os.path.join(SCRIPT_DIR, "bannedusers")
BANNEDIP_DIR = os.path.join(SCRIPT_DIR, "bannedips")
ADMIN_DIR = os.path.join(SCRIPT_DIR, "admins")
STAFF_DIR = os.path.join(SCRIPT_DIR, "staff")
KNOWNUSR_DIR = os.path.join(SCRIPT_DIR, "knownusers") # at some point this should be merged with the normal account files

os.makedirs(ACCOUNT_DIR, exist_ok=True)
os.makedirs(BANNEDUSR_DIR, exist_ok=True)
os.makedirs(ADMIN_DIR, exist_ok=True)
os.makedirs(KNOWNUSR_DIR, exist_ok=True)
os.makedirs(STAFF_DIR, exist_ok=True)

msg = None
broadcast = None
sysprefix = "~System Message~ [SYSTEM]: "

def send(msg): # Message wrapper
   global sysprefix, broadcast
   broadcast(sysprefix+msg)

def checkAdmin(user):
   global ADMIN_DIR
   filepath = os.path.join(ADMIN_DIR, user)
   if os.path.exists(filepath):
      return True
   return False

def ban(user,msg):
   global ACCOUNT_DIR,BANNEDUSR_DIR,BANNEDIP_DIR
   if not checkAdmin(user):
      send("Invalid permissions.")
   parts = msg.split(' ', 3)
   userToBan = parts[2].strip()
   usrOrIp = parts[1].strip()
   if usrOrIp == "user":
      filepath = os.path.join(ACCOUNT_DIR, userToBan)
      if os.path.exists(filepath):
         filepath2 = os.path.join(BANNEDUSR_DIR, userToBan)
         if os.path.exists(filepath2):
            send(f"{userToBan} is already banned.")
         else:
            with open(filepath2, 'w') as f:
               f.write('banned')
            send(f"{userToBan} has been banned.")
      else:
         send(f"{userToBan} doesn't exist.")
   elif usrOrIp == "ip":
      filepath = os.path.join(BANNEDIP_DIR, userToBan)
      if os.path.exists(filepath):
         send("IP already banned.")
      else:
         with open(filepath, 'w') as f:
            f.write('banned')
         send("IP banned successfully.")
   else:
      send("Invalid syntax.")
def unban(user,msg):
   if not checkAdmin(user):
      send("Invalid permissions.")
   parts = msg.split(' ', 3)
   userToUnban = parts[2].strip()
   usrOrIp = parts[1].strip()
   if usrOrIp == "user":
      filepath = os.path.join(ACCOUNT_DIR, userToUnban)
      if os.path.exists(filepath):
            filepath2 = os.path.join(BANNEDUSR_DIR, userToUnban)
            if os.path.exists(filepath2):
                  os.remove(filepath2)
                  send(f"{userToUnban} has been unbanned.")
            else:
                  send(f"{userToUnban} isn't banned.")
      else:
            send(f"{userToUnban} not found.")
   elif usrOrIp == "ip":
      filepath2 = os.path.join(BANNEDIP_DIR, userToUnban)
      if os.path.exists(filepath2):
         os.remove(filepath2)
         send("IP unbanned successfully.")
      else:
         send("IP not banned.")
   else:
      send("Invalid syntax.")

def welcomeMsg(user):
   filepath = os.path.join(KNOWNUSR_DIR, user)
   if not os.path.exists(filepath):
      with open(filepath, 'w') as f:
         f.write('known')
      send(f"{user}, type /info if you're new!")

def cmd(cmd,callback,args=[],kwargs={}): # Command wrapper
   global msg
   if msg.startswith(cmd):
      return callback(*args, **kwargs)

def checkCmd(user,msgarg,broadcastarg): # add extra imports if needed
   global broadcast, msg
   broadcast = broadcastarg
   msg = msgarg

   welcomeMsg(user)

   cmd("/ban", ban, args=[user,msg])
   cmd("/unban", unban, args=[user,msg])

   import custom_commands
   custom_commands.cmdSetup(user,msg,broadcast)
