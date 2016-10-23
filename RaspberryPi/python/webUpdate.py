import sys
import telnetlib
import smtplib
from time import sleep

HOST = "192.168.1.230"

tn = telnetlib.Telnet()

#Variable to track garage door status for changes between iterations
lastGarageDoorStatus = '0'

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
            controllerTemperature = tn.read_until(',',0.1)[:-1]
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
            text_file.write("<HTML>" + '\n' + "<HEAD>" + '\n' + "<TITLE>Home Controller</TITLE>" + '\n' + "<meta http-equiv=\"refresh\" content=\"10\">" + '\n' + "</HEAD>" + '\n' + "<BODY bgcolor=\"#ACDBDA\">")
            text_file.write("<strong>Furnace</strong><BR>")
            text_file.write("Furnace is: ")
            if(furnaceStatus == '0'):
                text_file.write("OFF" + '\n' + "<BR>")
            else:
                text_file.write("ON" + '\n' + "<BR>")
                text_file.write(furnaceRuntimeNow + " minutes Now ")

#           Convert furnace minutes to hours
# 	    furnaceRuntimeTodayHrs = float(furnaceRuntimeToday) / 60
#	    furnaceRuntimeSinceRebootHrs = float(furnaceRuntimeSinceReboot) / 60
#	    furnaceRuntimeSinceReboot = str(furnaceRuntimeSinceRebootHrs)
#	    furnaceRuntimeToday = str(furnaceRuntimeTodayHrs)

            text_file.write(furnaceRuntimeToday + " minutes Today, " + furnaceRuntimeSinceReboot + " minutes since reboot" + '\n' + "<BR>")

                
            if(maintainTemperature == '1'):
            	text_file.write(" Set Point: " + tempSetPoint + '\n' + "<BR>")
            else:
        	text_file.write("Disabled" + '\n')
        		
            text_file.write("Thermostat Schedule: ")
            if(programmableThermostatEnabled == '1'):
        	text_file.write("Enabled" + '\n')
            else:
        	text_file.write("Disabled" + '\n')

            text_file.write("<BR><BR><strong>Main Floor</strong><BR>")		 
            text_file.write("Main Floor Temperature: " + mainFloorAvgTemp + "<BR>")
            text_file.write("Living Room Temperature: " + livingRoomTemperature + '\n' + "<BR>")
            text_file.write("Master Bedroom Temperature: " + masterBedroomTemperature + " Set Point: " + masterBedroomTemperatureSetPoint + " HEATER: ")
            if bedroomHeaterStatus == '1':
                text_file.write("ON")
            else:
                text_file.write("OFF")
            text_file.write('\n' + "<BR>")
            text_file.write("Back Bedroom Temperature: " + backBedroomTemperature + '\n' + "<BR><BR>")

            text_file.write("<strong>Basement</strong><BR>")		 
            text_file.write("Basement Temperature: " + basementTempAmbient + '\n' + "<BR>")
            text_file.write("Controller Temperature: " + controllerTemperature + '\n' + "<BR><BR>")

            text_file.write("<strong>Garage and Outdoor</strong><BR>")		 
            text_file.write("Garage Temperature: " + garageTempAmbient + '\n' + "<BR>")
            text_file.write("Outdoor Temperature: " + garageTempOutdoor + '\n' + "<BR>")

        
            if garageDoorStatus == '0':
                text_file.write("Garage Door is OPEN" + '\n' + "<BR>")
            else:
            	text_file.write("Garage Door is CLOSED" + '\n' + "<BR>")
            
            text_file.write("Block Heater: ")
            if(blockHeaterStatus == '1'):
        		text_file.write("ON")
	    else:
        	if(blockHeaterEnabled == '1'):
        	    text_file.write("Scheduled")
        	else:
        	    text_file.write("OFF")

            text_file.write("<BR><BR>" + '\n' + "<strong>Controls:</strong>" + '\n' + "<BR>")
            text_file.write("<a href=\"protected/openGarageDoor.html\">Open Garage Door</a>" + '\n' + "<BR>")
            text_file.write("<a href=\"protected/closeGarage.php\">Close Garage Door</a>" + '\n' + "<BR>")
            text_file.write("<a href=\"protected/homeMode.php\">Home Mode</a>" + '\n' + "<BR>")
            text_file.write("<a href=\"protected/awayMode.php\">Away Mode</a>" + '\n' + "<BR>")
            text_file.write("<a href=\"protected/setTempAuto.php\">Run Schedule</a>" + '\n' + "<BR>")
	    text_file.write("<a href=\"protected/wakeUpServer.php\">Wake Up Server</a>" + '\n' + "<BR>")
#           text_file.write("Maintain Bedroom Temp at 19'C" + '\n' + "<BR>")
#           text_file.write("Bedroom heater off" + '\n' + "<BR>")
#           text_file.write("Block Heater On" + '\n' + "<BR>")
#           text_file.write("Block Heater Off / Timer Off" + '\n' + "<BR>")
#           text_file.write("Block Heater Timer On" + '\n' + "<BR>")
#           text_file.write("Vent Fan Off" + '\n' + "<BR>")
#           text_file.write("Vent Fan On" + '\n' + "<BR>")

            text_file.write('\n' + "<BR><BR>" + "Last Update: " + dateTime + '\n' + "<BR><BR>")
                            
            text_file.write("</BODY>" + '\n' + "</HTML>")
            text_file.close()
            print("Web Page Updated at " + dateTime),

	    #check if the garage status changed from closed to open on this iteration
  	
	if(garageDoorStatus == '0' and lastGarageDoorStatus == '1'):
   		# Send alert email
       	    print("Garage door opened on this iteration")

	    sender = 'ctrl@bladon.ca'
	    receivers = ['4038130062@txt.bell.ca']
   	    message = "Garage Door is Open!"

	    try:
   		smtpObj = smtplib.SMTP('mail.shaw.ca')
   		smtpObj.sendmail(sender, receivers, message)         
   		print "Successfully sent email"
	    except:
   		print "Error: unable to send email"
	
    	#else:
        	#print("No change in garage stauts")    	

	if(garageDoorStatus == '1' and lastGarageDoorStatus == '0'):
   		# Send alert email
       	    print("Garage door Closed on this iteration")

	    sender = 'ctrl@bladon.ca'
	    receivers = ['4038130062@txt.bell.ca']
   	    message = "Garage Door is Closed"

	    try:
   		smtpObj = smtplib.SMTP('mail.shaw.ca')
   		smtpObj.sendmail(sender, receivers, message)         
   		print "Successfully sent email"
	    except:
   		print "Error: unable to send email"
	
    	#else:
        	#print("No change in garage stauts")    	

    
    	lastGarageDoorStatus = garageDoorStatus


#        else:
#            print("Telnet Server Timeout")

    else:
        print("Telnet connection failed")

    tn.close()

    print("Disconnected")
    

    

    if response =="CONNECT": #if the last attempt was successful, wait 10 seconds to do it again
            delay = 10
    else: #only wait 2 seconds for re-try if previous attempt failed
            delay = 2
    while delay > 0:
            if delay % 2 == 0:
                print delay
            delay = delay - 1
            sleep(1)



