import sys
import telnetlib
from time import sleep

HOST = "192.168.1.230"

tn = telnetlib.Telnet()

print("opening telnet connection to home controller... ")
failedAttempt = False

try:
    tn.open(HOST)
except:
    failedAttempt = True

if failedAttempt == False:
    sleep(0.1)
    tn.write('n')
    response = tn.read_until("CONNECT", 10)


if response == "CONNECT":
    print "Connected"
    
    tn.write('I')
    response = tn.read_until(':',1)
    print(response)
    tn.write("17\r")
    print("sent 17\r")
    response = tn.read_until("17.00", 1)
    print(response)
    

tn.close()
print("Disconnected")




