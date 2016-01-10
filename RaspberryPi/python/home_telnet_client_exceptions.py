import sys
import telnetlib
from time import sleep

HOST = "192.168.1.230"

tn = telnetlib.Telnet()

while 1:
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


            #write the details to an HTML file (just a quick and dirty way to get something viewable quickly
            myfile = "/var/www/html/index.html"

            text_file = open(myfile, "w")
            text_file.write("<HTML>" + '\n' + "<HEAD>" + '\n' + "<TITLE>Hello World</TITLE>" + '\n' + "</HEAD>" + '\n' + "<BODY>")
            text_file.write('\n' + "Date-time: " + dateTime + '\n' + "<BR>")
            text_file.write("Furnace is: ")
            if(furnaceStatus == '0'):
                text_file.write("OFF")
            else:
                text_file.write("ON")
            text_file.write("<BR>Main Floor Temperature: " + mainFloorAvgTemp + " Set Point: " + tempSetPoint + '\n' + "<BR>")
            text_file.write("Living Room Temperature: " + livingRoomTemperature + '\n' + "<BR>")
            text_file.write("Master Bedroom Temperature: " + masterBedroomTemperature + " Set Point: " + masterBedroomTemperatureSetPoint + " HEATER: ")
            if bedroomHeaterStatus == '1':
                text_file.write("ON")
            else:
                text_file.write("OFF")
            text_file.write('\n' + "<BR>")
            text_file.write("Back Bedroom Temperature: " + backBedroomTemperature + '\n' + "<BR>")
            text_file.write("Basement Temperature: " + basementTempAmbient + '\n' + "<BR>")
            text_file.write("Garage Temperature: " + garageTempAmbient + '\n' + "<BR>")
            text_file.write("Outdoor Temperature: " + garageTempOutdoor + '\n' + "<BR>")
            if garageDoorStatus == '0':
                text_file.write("Garage Door is OPEN" + '\n' + "<BR>")
            else:
                    text_file.write("Garage Door is CLOSED" + '\n' + "<BR>")
            text_file.write("</BODY>" + '\n' + "</HTML>")
            text_file.close()
            print("Web Page Updated at " + dateTime),

        else:
            print("Telnet Server Timeout")

    else:
        print("Telnet connection failed")

    tn.close()
    print("Disconnected")
    if response =="CONNECT":
        delay = 60
    else: #only wait 10 seconds for re-try if previous attempt failed
        delay = 10
    while delay > 0:
        if delay % 20 == 0:
            print delay
        delay = delay - 1
        sleep(1)



