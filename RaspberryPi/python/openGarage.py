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
    #request all data
    tn.write('@')
    dateTime = tn.read_until(',',0.5) [:-1]
    uptimeSeconds = tn.read_until(',',0.1)[:-1]
    furnaceRuntimeNow = tn.read_until(',',0.1)[:-1]
    furnaceRuntimeToday = tn.read_until(',',0.1)[:-1]
    furnaceRuntimeSinceReboot = tn.read_until(',',0.1)[:-1]
    garageDoorStatus = tn.read_until(',',0.1)[:-1]
    mainFloorAvgTemp = tn.read_until(',',0.1)[:-1]
    livingRoomTemperature = tn.read_until(',',0.1)[:-1]
    tempSetPoint = tn.read_until(',',0.1)[:-1]
    maintainTemperature = tn.read_until(',',0.1)[:-1]
    programmableThermostatEnabled = tn.read_until(',',0.1)[:-1]
    furnaceStatus = tn.read_until(',',0.1)[:-1]
    ventFanForceOn = tn.read_until(',',0.1)[:-1]
    ventFanAutoEnabled = tn.read_until(',',0.1)[:-1]
    ventFanStatus = tn.read_until(',',0.1)[:-1]
    backBedroomTemperature = tn.read_until(',',0.1)[:-1]
    masterBedroomTemperature = tn.read_until(',',0.1)[:-1]
    masterBedroomTemperatureSetPoint = tn.read_until(',',0.1)[:-1]
    bedroomMaintainTemp = tn.read_until(',',0.1)[:-1]
    bedroomHeaterStatus = tn.read_until(',',0.1)[:-1]
    bedroomHeaterAutoOffHour = tn.read_until(',',0.1)[:-1]
    basementTempAmbient = tn.read_until(',',0.1)[:-1]
    garageTempAmbient = tn.read_until(',',0.1)[:-1]
    garageTempOutdoor = tn.read_until(',',0.1)[:-1]
    blockHeaterEnabled = tn.read_until(',',0.1)[:-1]
    blockHeaterStatus = tn.read_until(',',0.1)[:-1]
    blockHeaterOffHour = tn.read_until(',',0.1)[:-1]
    blockHeaterOnHour = tn.read_until(',',0.1)[:-1]
    blockHeaterMaxTemp = tn.read_until(',',0.1)[:-1]
    validPassword = tn.read_until(',',0.1)[:-1]

    if(garageDoorStatus == '1'):
        tn.write('3\r')
        response = tn.read_until("door",1)
        print(response)
    else:
        print("Door already open")

    

tn.close()
print("Disconnected")




