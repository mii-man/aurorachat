# really quick code to encode str to utf8
# made to reset a user's password
username = input("uname: ")
password = input("enc pass: ")
import os.path
import os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ACCOUNT_DIR = os.path.join(SCRIPT_DIR, "accounts")
filepath = os.path.join(ACCOUNT_DIR, username)
with open(filepath, 'wb') as f:
    f.write(password.encode('utf-8'))
print('done')
