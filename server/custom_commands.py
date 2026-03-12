import os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
KNOWNUSR_DIR = os.path.join(SCRIPT_DIR, "knownusers") # at some point this should be merged with the normal account files

broadcast = None
msg = None

def send(msg,prefix="~System Message~ [SYSTEM]: "): # Message wrapper
   global broadcast
   broadcast(prefix+msg)

def info():
   send("Type /rules to see the rules,")
   send("and /discord to get a link to our Discord server.", prefix="")

import random
def rules(user):
   global KNOWNUSR_DIR
   filepath = os.path.join(KNOWNUSR_DIR, user) # 0% chance of silly rules on first time
   with open(filepath, "r+") as f:
      data = f.read()
      f.seek(0)
      f.write('knownrules')
      f.truncate()
   silly = False
   if data == 'knownrules':
      if random.randint(1, 100) <= 10:
         silly = True
   if silly == False:
      send("Rules: press Y to read them. Be nice.")
   else:
      send("Rules (silly): no")

def discord():
   send("Want access to the rest of Unitendo? Join our official Discord server here:")
   send("https://discord.gg/dCSgz7KERv", prefix="")

def cmd(cmd,func,args=[],kwargs={}): # Command wrapper
   global msg
   if msg.startswith(cmd):
      return func(*args, **kwargs)

# Custom commands (other than /ban, /unban, and the welcome message) go here
def cmdSetup(user,msgarg,broadcastarg):
   global broadcast,msg
   broadcast = broadcastarg
   msg = msgarg

   cmd("/info", info)
   cmd("/rules", rules, args=[user])
   cmd("/discord", discord)
