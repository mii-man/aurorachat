# really quick code to encode str to utf8
# made to set an admin's password for the panel

adminname = input("uname: ")
adminpass = input("enc pass: ")
import os.path
import os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ADMIN_DIR = os.path.join(SCRIPT_DIR, "admins")
filepath = os.path.join(ADMIN_DIR, adminname)
with open(filepath, 'wb') as f:
    f.write(adminpass.encode('utf-8'))
print('done')
