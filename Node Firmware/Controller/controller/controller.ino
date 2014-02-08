/*
	This code is part of my home control project which includes multiple nodes
 	This is the code for the controller node located inside the house
 	This node is connected to an Ethernet Shield and 1-wire temperature sensor
 	and an XBEE module connected to the the rest of the nodes via a wireless network
 	
 	The main function is to run a telnet server which, in this version, works as a 
 	relay of text commands from the telnet client. Some commands are handled directly
 	by the controller node, mainly the command listing and reading the temperature sensor
 	
 	The XBEE module on this node is configured as coordinator and connected to the serial
 	port. Anything sent to the serial port on this node will be received at all other nodes.
 	Data received at this node's serial port can be from any other node.
 	
 	The furnace (basement node) can be controlled by the "MaintainTemp" and "tempSetPoint"
 	variables in this node. These variables can be set by the telnet client
 
 
 	Michael Bladon
 	Dec 21, 2013 - Feb 8, 2014
 */

#include <Password.h>
#include <SPI.h>
#include <Ethernet.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
//#include <SD.h>
#include <RTClib.h>
#include <valueFromSerial.h>
#include <commandDefinitionsAlt.h>
#include <privatePasswordFile.h>


//Constants
#define oneWireBus1                   7 // Temperature Sensor
#define baud                       9600 // serial port baud rate
#define FWversion                  0.34 // FW version
#define tempMaximumAllowed         23.0// maximum temperature
#define tempMinimumAllowed         15.0 //minimum temperature
#define bedroomHeaterAutoOffHour     10 //automatically turn off the bedroom heater at this time
#define blockHeaterOnHour             5 //hour in the morning to automatically turn on block heater
#define blockHeaterOffHour           11 //hour to turn the block heater off automatically
#define blockHeaterMaxTemp           -5 //maximum temperature INSIDE garage for block heater use
#define garageDoorStatusPin           9 //This pin is high when the garage door is closed
#define tempUpdateDelay              30 //number of seconds to wait before requesting another update from sensors
#define clientTimeoutLimit        90000 //number of seconds before assuming the client has disconnected

//ASCII values
#define newLine                      10 // ASCII NEW LINE
#define carriageReturn               13 // ASCII Carriage return
#define tempOffset                -0.80 //offset in degrees C to make controller ambient temp agree with thermostat at centre of house
//#define chipSelect                  4 //Chip select for the SD card used to store log data


/* NETWORK CONFIGURATION
 These variables are required to configure the Ethernet shield
 The shield must be assigned a local static IP address that doesn't conflict
 on the network. It DOES NOT have DHCP capabilities for Dynamic IP 
 
 To reach this server from outside the local network, port forwarding
 or VPN must be setup on the router
 */


EthernetServer server = EthernetServer(23); //Set up the ethernet server on port 23 (standard telnet port)

  
  
/* 1-WIRE Configuration 
 A DS18B20 1-wire temperature sensor is connected on pin defined as OneWireBus1
 These commands setup the OneWire instance and pass the reference to the Dallas Temperature
 class that will be used to read the temperature from the device
 */
OneWire oneWire1(oneWireBus1);  
DallasTemperature sensors1(&oneWire1);

//Realtime clock Configuration
RTC_DS1307 rtc; //the realtime clock object

//Global Variables
DateTime now; //variable to store the current date & time from the RTC
DateTime startup; //store the power-on time for calculating up-time
DateTime lastUpdateTime;
char inputChar;	//store the value input from the serial port or telnet client
int i = 0; //for for loops
float tempAmbient = -127.0; //value used to store ambient temperature as measured by 1-wire sensor
Password password = Password(MichaelPassword); //The password is contained in a private header file
float tempSetPoint = 21.0; //The setPoint temperature
float tempHysterisis = 0.25; //The values applied as +/- to the set point to avoid chattering the furnace control
bool maintainTemperature = false; //Furnace ON/Off commands are only sent if this is true
bool furnaceStatus = false; //This is true if Furnace ON commands are being sent
bool blockHeaterEnabled = false; //If this is true, blockheater automatically turns on and off with temperature and time
bool validPassword = false; //This is true any time a valid user is logged in
bool commandSent = false; //this tracks whether the controller has already relayed the command to the network, prevents sending it twice
//bool SDCardLog = false; //We only attempt to write to the SD card if this is true
unsigned long timeOfLastInput = 0;//this is used to determine if the client interaction has timed out


//Global variables from the basement node
float basementTempAmbient= -127.0; //value used to store basement temperature as measured from the basement node

//Global variables from Garage Node
float garageFirmware = -1; // track the garage node firmware version
float garageTempOutdoor = -127.0;  //value used to store outdoor temperature as received from the garage node
float garageTempAmbient = -127.0; //value used to store garage temperature as measured from the garage node
bool garageDoorStatus = false;
bool garageDoorError = false; //track whether garage door error flag is set
bool garageLastDoorActivation = false; //keep track of whether the last door activation was to open or close. 0 = open, 1 = close
bool garageAutoCloseStatus = false; // track whether auto close is enabled or not
bool garageRelay120V1 = 0; //track whether the 120V outlet is on, 1 = on, 0 = off

//Global variables from the bedroom node
float bedroomFirmware = -1;
float bedroomTemperature = -127.0;
float bedroomTemperatureSetPoint = -127.0;
bool bedroomMaintainTemp = false;

/* SETUP - RUN ONCE
 	Setup function initializes some variables and sets up the serial and ethernet ports
 	this takes about 1 second to run
 */
void setup()
{
  //Initialize the IO on this node
  pinMode(garageDoorStatusPin, OUTPUT);

  //Initialize the realtime clock
  Wire.begin();
  rtc.begin();
  
  //Initialize the SD card
/*
  pinMode(10, OUTPUT);
  if (SD.begin(chipSelect))
    {
      SDCardLog = true;
    }
 */ 
  
 
  
  //Initialize the Ethernet Adapter
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  //MAC Address
  byte ip[] = { 192, 168, 1, 230 };   // IP Address
  byte gateway[] = { 10, 0, 0, 1 }; //Gateway (optional, not used)
  byte subnet[] = { 255, 255, 0, 0 }; //Subnet

  // initialize the 1-wire bus
  sensors1.begin();

  //initialize the serioal port  
  Serial.begin(9600);

  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);

  // start listening for clients
  server.begin();

  //get the initial periodic update values
  getPeriodicUpdates();
  
  //Because the first value requested from network usually times out, request it again during the startup sequence
  Serial.print(grge_requestTempZone2); //relay the command to the serial port
  garageTempOutdoor = floatFromSerial('!');  
  //saveToSDCard();
  //Initialize the RTC
  
  now = rtc.now();//update the time from the RTC
  startup = now; //store the current time as the startup time
}

void loop()
{
  // This loop waits for a client to connect. While waiting, the temperature is updated
  // Periodically and the furnace control function is called to maintain temperature if
  // appropriately configured.


  // clear the valid password flag from any previous user
  validPassword = false; // Should already be false, but just in case the last session didn't end cleanly

  // if an incoming client connects, there will be bytes available to read:

  EthernetClient client = server.available();
  if (client == true) 
  {

    //Found that the some telnet clients send 24-26 bytes of data once connection is established.
    // Client trying to negotiate connecrtion? This section just ignores the first 250ms of data
    delay(250); //wait 250ms for incoming data from the client
    while(client.available())
    {
      inputChar = client.read();// read in the data but don't do anything with it.
      timeOfLastInput = millis();//set the time of last Input to NOW
    }

    // Prompt for password and compare to the actual value
    // The user must terminate password with '?' 
    // Typing errors can be cleared by sending '!'
    // If the password is wrong, the client will be disconnected and server will wait
    // for ~30 seconds before allowing new connections
    server.print(F("Enter password: "));
    while(!validPassword && client.connected())
    {

      if(client.available())
      {
        inputChar = client.read();
        timeOfLastInput = millis();//set the time of last Input to NOW
        switch (inputChar){
        case '!': //reset password
          password.reset();
          //  server.print(F("\tPassword is reset!"));
          break;
        case '?': //evaluate password
          if (password.evaluate())
          {
            server.print(F("Logged in successfully"));
            server.write(newLine);
            server.write(carriageReturn);
            password.reset();
            validPassword = true;

          }
          else
          {
            server.print(F("Login Failed - Disconnecting"));
            client.stop();
            password.reset();
            delay(30000); //Wait a short while before allowing any more connections
            timeOfLastInput = millis();//set the time of last Input to NOW

        }
          break;
        default: //append any ascii input characters that are not a '!' nor a '?' to the currently entered password.
          if(inputChar > 47 && inputChar < 123)
          {
            password << inputChar;
          }
        }
      }

      if(didClientTimeout())
      {
        server.print(F("Session Timeout - Disconnecting"));
        server.write(newLine);
        server.write(carriageReturn);
        
        
        server.print(F("Time of last input: "));
        server.print(timeOfLastInput);
        server.write(newLine);
        server.write(carriageReturn);
        server.print(F("Now: "));
        server.print(millis());
        server.write(newLine);
        server.write(carriageReturn);   
        
        client.stop();
        password.reset();
        timeOfLastInput = millis();    
      }

    }//End of while(!validPassword)


    //WELCOME MESSAGE - Displayed only at start of session
    if(validPassword)
    {
      server.print(F("Welcome! Type '?' for a list of commands"));
      server.write(newLine);
      server.write(carriageReturn);
    } 

    //Connected Loop - this is repeated as long as the client is connected
    //The loop relays data between the telnet client and the serial port (XBEE network)
    //Certain commands are handed by this node directly to the telnet client without any serial communication
    //Only ascii data is relayed from the telnet client to the serial port
    while(validPassword && client.connected())
    {
      if(client.available()) //This is true if the telnet client has sent any data
      {
        inputChar = client.read(); //read in the client data
        timeOfLastInput = millis();//set the time of last Input to NOW


        // COMMANDS HANDLED BY THIS NODE  
        if(inputChar == ctrl_requestFWVer)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Controller FW version: "));
          server.print(FWversion);
          server.write(newLine);//new line
          server.write(carriageReturn);
        }

        if(inputChar == ctrl_requestTemp)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Main Floor Temperature: "));
          server.print(tempAmbient);
          server.print(F(" deg C"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }

        if(inputChar == ctrl_enableFurnace)

        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          maintainTemperature = true;
          server.print(F("Furnace enabled, maintain "));
          server.print(tempSetPoint);
          server.print(F(" deg C at upstairs sensor"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }

        if(inputChar == ctrl_disableFurnace)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          maintainTemperature = false;
          furnaceStatus = false;
          Serial.print(bsmt_turnFurnaceOff); 
          if(!boolFromSerial())
          {
                      server.print(F("Error - No reply from basement node"));
          }
          else
          {
                      server.print(F("Not maintaining temperautre on main floor"));
                      server.write(newLine);//new line
                      server.write(carriageReturn);
          }
          
            
        }

        if(inputChar == ctrl_increaseTempSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          if(tempSetPoint < tempMaximumAllowed)
          {
            tempSetPoint = tempSetPoint + 0.25;
            server.print(F("Increased temp set point to: "));
            server.print(tempSetPoint);
            server.print(F(" deg C"));
          }
          else
            server.print(F("Maximum Temperature already Set"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }

        if(inputChar == ctrl_decreaseTempSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          if(tempSetPoint > tempMinimumAllowed)
          {
            tempSetPoint = tempSetPoint - 0.25;
            server.print(F("Decreased temp set point to: "));
            server.print(tempSetPoint);
            server.print(F(" deg C"));
          }
          else
            server.print(F("Minimum Temperature Already Reached"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }
        
        if(inputChar == ctrl_enableBlockHeater)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          blockHeaterEnabled = true;
          server.print(F("Block Heater will turn on after "));
          server.print(blockHeaterOnHour);          
          server.print(F(":00h if Temperature is below ")); 
          server.print(blockHeaterMaxTemp);
          server.print(F(" deg C"));         
          server.write(newLine);//new line
          server.write(carriageReturn);         
        }
        
        if(inputChar == ctrl_disableBlockHeater)
        {
          inputChar = grge_requestDeactivate120V1; //do this so the request is sent to turn off heater immediately
          blockHeaterEnabled = false;
          server.print(F("Block Heater will not turn on automatically"));         
          server.write(newLine);//new line
          server.write(carriageReturn);           
          
        }

         if(inputChar == ctrl_statusReport)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          sendStatusReport();
        }


        if(inputChar == ctrl_logOff)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Goodbye!"));
          server.write(newLine);//new line
          server.write(carriageReturn);
          client.stop();
          validPassword = false;
          break; //end the while loop to make the system request a password again.
        }

        if(inputChar == ctrl_listCommands)
        {
          commandSent = true;
          sendCommandList();
        }  


        //Defined commands not handled by this node
        //relay them and store results in variables in the controller
        if(inputChar == grge_requestFWVer)
        {
          Serial.print(grge_requestFWVer);//send the command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageFirmware = floatFromSerial('!');
          if(garageFirmware > 0)
          {
            server.print(F("Garage node firmware version: "));
            server.print(garageFirmware);
            server.write(carriageReturn);
            server.write(newLine);    
          }
          else
          {
            server.print(F("Error reading garage firmware version"));
            server.write(carriageReturn);
            server.write(newLine);  
          }
        }


        if(inputChar == grge_requestActivateDoor)
        {
            Serial.print(grge_requestActivateDoor);
            commandSent = true; //set the commandSent variable true so it is't sent again this loop          
            garageLastDoorActivation = boolFromSerial();
            if(garageLastDoorActivation)
            {
              server.print(F("Closing Garage Door"));
              server.write(carriageReturn);
              server.write(newLine); 
            }
            else
            {
              server.print(F("Opening Garage Door"));
              server.write(carriageReturn);
              server.write(newLine); 
              garageDoorStatus = false;
              digitalWrite(garageDoorStatusPin, garageDoorStatus);
            }
          }

        if(inputChar == grge_requestActivate120V1)
        {
          Serial.print(grge_requestActivate120V1);
          blockHeaterEnabled == false; //This is necessary to prevent block heater from turning off right away if time or temp is out of range
          commandSent = true; //set the commandSent variable true so it is't sent again this loop          
          garageRelay120V1 = boolFromSerial();
          if(garageRelay120V1)
          {
            server.print(F("120V outlet is now ON"));
            server.write(carriageReturn);
            server.write(newLine); 
          }
          else
          {
            server.print(F("ERROR: 120V outlet is OFF"));
            server.write(carriageReturn);
            server.write(newLine); 
          }

        }

        if(inputChar == grge_requestDeactivate120V1)
        {
          Serial.print(grge_requestDeactivate120V1);
          blockHeaterEnabled == false; //This is necessary to prevent block heater from turning back on right away if time and temp are in range
          commandSent = true; //set the commandSent variable true so it is't sent again this loop          
          garageRelay120V1 = !boolFromSerial();
          if(garageRelay120V1)
          {
            server.print(F("ERROR: 120V outlet is now ON"));
            server.write(carriageReturn);
            server.write(newLine); 
          }
          else
          {
            server.print(F("120V outlet is OFF"));
            server.write(carriageReturn);
            server.write(newLine); 
          }

        }

        if(inputChar == grge_requestAutoCloseStatus)
        {
          Serial.print(grge_requestAutoCloseStatus);//send command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageAutoCloseStatus = boolFromSerial(); 
          if(garageAutoCloseStatus)
          {
            server.print(F("Auto-close is enabled"));
            server.write(carriageReturn);
            server.write(newLine);     
          } 
          else
          {
            server.print(F("Auto-close is disabled"));
            server.write(carriageReturn);
            server.write(newLine); 
          }  

        }

        if(inputChar == grge_requestDisableAutoClose)
        {
          Serial.print(grge_requestDisableAutoClose);//send command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageAutoCloseStatus = boolFromSerial(); 
          if(garageAutoCloseStatus)
          {
            server.print(F("Auto-close is enabled"));
            server.write(carriageReturn);
            server.write(newLine);     
          } 
          else
          {
            server.print(F("Auto-close is disabled"));
            server.write(carriageReturn);
            server.write(newLine); 
          }     
        }

        if(inputChar == grge_requestEnableAutoClose)
        {
          Serial.print(grge_requestEnableAutoClose);//send command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop          
          garageAutoCloseStatus = boolFromSerial(); 
          if(garageAutoCloseStatus)
          {
            server.print(F("Auto-close is enabled"));
            server.write(carriageReturn);
            server.write(newLine);     
          } 
          else
          {
            server.print(F("Auto-close is disabled"));
            server.write(carriageReturn);
            server.write(newLine); 
          }         

        }

        if(inputChar == grge_requestClearErrorFlag)
        {
          Serial.print(grge_requestClearErrorFlag);//send command to node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageDoorError = !boolFromSerial(); //if the flag is cleared, garage node will send a '1' (successfully cleared)
          if(!garageDoorError)
          {
            server.print(F("Garage door error flag is cleared"));
            server.write(carriageReturn);
            server.write(newLine);
          }
          else
          {
            server.print(F("ERROR: Couldn't reset garage door error flag"));
            server.write(carriageReturn);
            server.write(newLine);       
          }


        }


        if(inputChar == grge_requestTempZone2)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Outdoor Temperature is: "));
          server.print(garageTempOutdoor);
          server.print(F(" deg C"));
          server.write(newLine);
          server.write(carriageReturn);
        }

        if(inputChar == grge_requestTempZone1)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Garage Temperature is: "));
          server.print(garageTempAmbient);
          server.print(F(" deg C"));
          server.write(newLine);
          server.write(carriageReturn);
        }

        if(inputChar == grge_requestDoorStatus)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          requestAndUpdateGarageDoorStatus();
        }



        //BASEMENT NODE COMMANDS

        if(inputChar == bsmt_requestTemp)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Basement Temperature is: "));
          server.print(basementTempAmbient);
          server.print(F(" deg C"));
          server.write(newLine);
          server.write(carriageReturn);
        }



        //BEDROOM NODE COMMANDS
         if(inputChar == bdrm_requestFWVer)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bdrm_requestFWVer);
          bedroomFirmware = floatFromSerial('!');
          server.print(F("Bedroom node FW Version: "));
          server.print(bedroomFirmware);
          server.write(newLine);
          server.write(carriageReturn);
        }       


         if(inputChar == bdrm_requestTemp)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bdrm_requestTemp);
          bedroomTemperature = floatFromSerial('!');
          server.print(F("Bedroom Temperature: "));
          server.print(bedroomTemperature);
          server.write(newLine);
          server.write(carriageReturn);
        }
        
        if(inputChar == bdrm_increaseSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bdrm_increaseSetPoint);
          bedroomTemperatureSetPoint = floatFromSerial('!');
          server.print(F("Bedroom Set Point Increased to: "));
          server.print(bedroomTemperatureSetPoint);
          server.write(newLine);
          server.write(carriageReturn);
        }
        
        if(inputChar == bdrm_decreaseSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bdrm_decreaseSetPoint);
          bedroomTemperatureSetPoint = floatFromSerial('!');
          server.print(F("Bedroom Set Point Decreased to: "));
          server.print(bedroomTemperatureSetPoint);
          server.write(newLine);
          server.write(carriageReturn);
        }   
                
        if(inputChar == bdrm_requestSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bdrm_requestSetPoint);
          bedroomTemperatureSetPoint = floatFromSerial('!');
          server.print(F("Bedroom Set Point: "));
          server.print(bedroomTemperatureSetPoint);
          server.write(newLine);
          server.write(carriageReturn);
        }   
        
        if(inputChar == bdrm_maintainSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bdrm_maintainSetPoint);
          bedroomMaintainTemp = boolFromSerial();
          if(bedroomMaintainTemp)
          {
            server.print(F("Maintaining Bedroom Set Point: "));
            server.print(bedroomTemperatureSetPoint);
            server.write(newLine);
            server.write(carriageReturn);
            server.print(F("Heater will disable automatically at "));
            server.print(bedroomHeaterAutoOffHour);
            server.print(F(":00"));
          }
          else
          {
            server.print(F("Error - Command not acknowledged by bedroom node"));
            server.write(newLine);
            server.write(carriageReturn);
          }
          
        }
        
        if(inputChar == bdrm_dontMaintainSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bdrm_dontMaintainSetPoint);
          bedroomMaintainTemp = !boolFromSerial();
          if(!bedroomMaintainTemp)
          {
            server.print(F("Bedroom Heater Disabled"));
            server.write(newLine);
            server.write(carriageReturn);
          }
          else
          {
            server.print(F("Error - Command not acknowledged by bedroom node"));
            server.write(newLine);
            server.write(carriageReturn);
          }
          
        }     
                
    
        

        //ALL OTHER COMMANDS
        //RELAY THEM TO THE SERIAL PORT (XBEE NETWORK)
        if(inputChar > 47 && inputChar < 123 && !commandSent)//ignore anything not alpha-numeric
        {
          server.print(F("Unknown command"));
          server.write(newLine);
          server.write(carriageReturn);
        }




        //Reset the CommandSent tracking variable
        commandSent = false;
      }

      //RELAY SERIAL DATA TO TELNET CLIENT
      if(Serial.available())
      {
        inputChar = Serial.read();//read the serial input
        server.write(inputChar);//output the serial input to the telnet client
      }
      now = rtc.now();//get time from the RTC
      if((now.unixtime() - lastUpdateTime.unixtime()) > tempUpdateDelay) //Update the temp sensors and send furnace commands if needed
      {
        getPeriodicUpdates();//Update the temperature and other sensors
        //saveToSDCard();
       
     
      }
      if(didClientTimeout())
      {
        server.print(F("Session Timeout - Disconnecting"));
        server.write(newLine);
        server.write(carriageReturn);
        
        
        server.print(F("Time of last input: "));
        server.print(timeOfLastInput);
        server.write(newLine);
        server.write(carriageReturn);
        server.print(F("Now: "));
        server.print(millis());
        server.write(newLine);
        server.write(carriageReturn);   
        
        client.stop();
        password.reset();
        timeOfLastInput = millis();      
        
      }
      
    }  //end of while(client.connected())
  }//end of if(client == true)



  //See if it's time to update the periodic sensors
      now = rtc.now();//update the time from the RTC
      if((now.unixtime() - lastUpdateTime.unixtime()) > tempUpdateDelay) //Update the temp sensors and send furnace commands if needed
      {
        getPeriodicUpdates();//Update the temperature and other sensors
        //saveToSDCard();

        
     
      }
      
  //If there is no telnet client connected, just read and ignore the serial port
  //This is to prevent the buffer from filling up when the is no telnet client but nodes are sending data
  //Only one byte is ignored per loop but the serial port at 9600 baud shouldn't be faster than the main loop
  if(Serial.available())
  {
    Serial.read();
  }

}// END OF THE MAIN LOOP FUNCTION


/* readAmbientTemp()
 	This function collects a temperature reading in degrees C from the 1-wire
 	sensor and returns it as a float
 */

float readAmbientTemp()
{
  float temp;
  sensors1.requestTemperatures();
  temp = sensors1.getTempCByIndex(0) + tempOffset;   
  return temp;
}

void disableBedroomHeaterOnTimer()//Automatically turns the bedroom heater off if called within 5 minutes of bedroomHeaterAutoOffHour
{
    now = rtc.now();
    if(now.hour() == bedroomHeaterAutoOffHour && now.minute() <= 5)
    {
      Serial.print(bdrm_dontMaintainSetPoint);
      bedroomMaintainTemp = !boolFromSerial();
    }
}

/* controlFurnace()
 	This function sends ON/OFF commands to the basement node to control the furnace
 	This is doen via the serial port which is connected to the XBEE network.
 	This function relies on global variables and takes no inputs
 */

void controlFurnace()
{
  //Control furnace by sending commands to the basement node based on tempAmbient

  if(tempAmbient == (-127.0 + tempOffset) || tempAmbient == (0.0 + tempOffset))
  {
      return;//Don't act on bad values - do nothing
  }
      //turn the furnace off if necessary 
      if(tempAmbient > (tempSetPoint + tempHysterisis))
      {
        furnaceStatus = false;//prevent furnace from turning on in the next loop
        Serial.print(bsmt_turnFurnaceOff);   
        if(!boolFromSerial())
        {
 //         server.print(F("ERROR: Basement node didn't acknowledge furnace off command"));
 //         server.write(carriageReturn);
 //         server.write(newLine);        
        }        
      }  
      
      
      // turn the furnace on if necessary
      if(furnaceStatus || (tempAmbient < (tempSetPoint - tempHysterisis)))
      {
        furnaceStatus = true;//used for hysterisis when the second part of the IF condition is no longer true
        Serial.print(bsmt_turnFurnaceOn);//this command must be repeated at least every 5 minutes or the furnace will automatically turn off (see firmware for basement node)
//        server.print(F("sent ON command to basement node ")); 
//        server.print(tempAmbient);
//        server.print(F(" deg C ")); 
        if(boolFromSerial())
          {
//            server.print(F("ACK"));
//            server.write(carriageReturn);
//            server.write(newLine);        
          }
   
        else
          {
 //           server.print(F("ERROR: Basement node didn't acknowledge furnace on command"));
 //           server.write(carriageReturn);
 //           server.write(newLine);        
          }
        }
    return;
  
}


//controlBlockHeater()
//This function turns on the block heater if the hour is greater than the specified constant 
//and if the temperature is below the specified constant
void controlBlockHeater()
{
  now = rtc.now();//update the time
  if(now.hour() >= blockHeaterOnHour && now.hour() <= blockHeaterOffHour && garageTempAmbient < (blockHeaterMaxTemp - 0.25))
  {
          Serial.print(grge_requestActivate120V1);
          garageRelay120V1 = boolFromSerial();
  }
  
  if(now.hour() < blockHeaterOnHour  || now.hour() >= blockHeaterOffHour || garageTempAmbient > (blockHeaterMaxTemp + 0.25))
  {
          Serial.print(grge_requestDeactivate120V1);
          garageRelay120V1 = !boolFromSerial();
  }
  return;  
}



/* This function returns true if the client has not made any input in the amount of time specified in the function call
 */
bool didClientTimeout()
{
  unsigned long temp;
  temp = millis();
  if((temp - timeOfLastInput) > clientTimeoutLimit)
    return true;

  else
    return false;
}


void sendCommandList()
{
   
          server.print(F("You can use these commands...."));
          server.write(newLine);
          server.write(carriageReturn);
          
          server.print(F("S : Status Report"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("x : Log off"));
          server.write(carriageReturn);
          server.write(newLine);
          
          
          server.write(newLine);
          server.print(F("MAIN FLOOR:                           MASTER BEDROOM:"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("E : Turn Fan On                       k : Increase Set Temperature"));   
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("F : Turn Fan and Furnace Off          l : Decrease Set Temperature"));   
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("M : Maintain Set Temperature          m : Maintain Temperature"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("N : Do Not Maintain Temperature       n : Do Not Maintain Temperature"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("O : Increase Set Temperature"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("P : Decrease Set Temperature"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.write(newLine);
          server.write(carriageReturn);

          server.print(F("GARAGE:"));   
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("3 : Activate Door                     7 : Turn on Block Heater Now"));   
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("4 : Request Auto Close Status         8 : Turn off Block Heater Now"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("5 : Disable Auto Close                Q : Enable Block Heater Timer"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("6 : Enable Auto Close                 R : Disable Block Heater Timer"));
          server.write(carriageReturn);
          server.write(newLine);


}

void sendStatusReport()
{
      server.print(F("Main Floor Temperature:     "));
      server.print(tempAmbient);
      server.print(F(" deg C"));
      server.write(newLine);//new line
      server.write(carriageReturn);

      server.print(F("Bedroom Temperature:        "));
      server.print(bedroomTemperature);
      server.print(F(" deg C"));
      server.write(newLine);//new line
      server.write(carriageReturn);
      
      server.print(F("Basement Temperature:       "));
      server.print(basementTempAmbient);
      server.print(F(" deg C"));
      server.write(newLine);
      server.write(carriageReturn);

      server.print(F("Garage Temperature:         "));
      server.print(garageTempAmbient);
      server.print(F(" deg C"));
      server.write(newLine);
      server.write(carriageReturn);

      server.print(F("Outdoor Temperature:        "));  
      server.print(garageTempOutdoor);
      server.print(F(" deg C"));
      server.write(newLine);
      server.write(carriageReturn);
      server.write(newLine);
      server.write(carriageReturn);
      
     if(bedroomMaintainTemp)
        {    
          server.print(F("Bedroom Thermostat ON - Set:"));
          server.print(bedroomTemperatureSetPoint);
        }
        
      else
        {
          server.print(F("Bedroom Thermostat OFF Set: "));
          server.print(bedroomTemperatureSetPoint);
   
        }
        
        server.print(F(" Deg C in Bedroom"));
        server.write(newLine);
        server.write(carriageReturn);  
  
          
     if(maintainTemperature)
        {    
          server.print(F("Main Thermostat ON - Set:   "));
          server.print(tempSetPoint);
        }
        
      else
        {
          server.print(F("Main Thermostat OFF Set:    "));
          server.print(tempSetPoint);
  
        }

        server.print(F(" Deg C on Main Floor"));
        server.write(newLine);//new line
        server.write(carriageReturn); 
        
        if(blockHeaterEnabled)
        {
          server.print(F("Block Heater will turn on after "));
          server.print(blockHeaterOnHour);          
          server.print(F(":00h if Temperature is below ")); 
          server.print(blockHeaterMaxTemp);
          server.print(F(" deg C"));         
          server.write(newLine);//new line
          server.write(carriageReturn);     
        }
        
        else
        {
         server.print(F("Block heater will not turn on automatically")); 
         server.write(newLine);//new line
         server.write(carriageReturn);     
        }
        
        server.write(newLine);
        server.write(carriageReturn);   
  

        
        server.print(F("Furnace is currently "));
        
        if(furnaceStatus)
        {
          server.print(F("ON"));
          server.write(newLine);
          server.write(carriageReturn);
        }
        else
        {
          server.print(F("OFF"));
          server.write(newLine);
          server.write(carriageReturn);
        }
        
        
        server.print(F("Bedroom Heater is "));

        if(bedroomMaintainTemp && (bedroomTemperature < (bedroomTemperatureSetPoint - 0.25)))
        {
          server.print(F("ON"));
          server.write(newLine);
          server.write(carriageReturn);
        }
        
        else
        {
          server.print(F("OFF"));
          server.write(newLine);
          server.write(carriageReturn);
        }
        

        {
          server.print(F("Block Heater is "));

        }       
        
        if(garageRelay120V1)
        {    
          server.print(F("ON"));
          server.write(newLine);
          server.write(carriageReturn);    
        }

        else
        {    
          server.print(F("OFF"));
          server.write(newLine);
          server.write(carriageReturn);    
        }
        
        
        requestAndUpdateGarageDoorStatus();
        server.write(newLine);
        server.write(carriageReturn);          
        
          server.print(F("Controller FW Version: "));
          server.print(FWversion);
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("Memory Used: "));
          server.print(2048 - freeRam());
          server.print(F(" / 2048 Bytes"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("Uptime: "));
          sendUptime();
          
       now = rtc.now();//update the time from the RTC
      
       server.print(F("System Time: "));
       server.print(now.year(), DEC);
       server.print(F("/"));
       if(now.month() < 10)
          server.print(F("0"));
       server.print(now.month(), DEC);
       server.print(F("/"));
       if(now.day() < 10)
          server.print(F("0"));
       server.print(now.day(), DEC);
       server.print(F(" "));
       
       server.print(now.hour(), DEC);
       server.print(F(":"));
       if(now.minute() < 10)
          server.print(F("0"));
       server.print(now.minute(), DEC);
       server.print(F(":"));
       if(now.second() < 10)
          server.print(F("0"));
       server.print(now.second(), DEC);
       server.print(F(" "));
       server.write(carriageReturn);
       server.write(newLine);    
               
}

void sendUptime()
{
  now = rtc.now();
  DateTime uptime = now.unixtime() - startup.unixtime();

  long uptimeSeconds = uptime.unixtime();
          if(uptimeSeconds < 60)
          {
             server.print(uptimeSeconds);
             server.print(F(" seconds"));
          }
          else if(uptimeSeconds < 120)
          {
             server.print(uptimeSeconds/60);
             server.print(F(" minute"));
          } 
          else if(uptimeSeconds < 3600)
          {
             server.print(uptimeSeconds/60);
             server.print(F(" minutes"));
          }
          else if(uptimeSeconds < 7200)
          {
             server.print(uptimeSeconds/3600);
             server.print(F(" hour"));
          }
          else if(uptimeSeconds < 86400)
          {
             server.print(uptimeSeconds/3600);
             server.print(F(" hours"));
          }
          else
          {
             server.print(uptimeSeconds/86400);
             server.print(F(" days"));
          }                    
          server.write(carriageReturn);
          server.write(newLine);

}

void requestAndUpdateGarageDoorStatus()
{
          Serial.print(grge_requestDoorStatus);
          garageDoorStatus = boolFromSerial();
          digitalWrite(garageDoorStatusPin, garageDoorStatus);
          
          server.print(F("Garage door is "));
          if(garageDoorStatus)
            server.print(F("CLOSED"));
          else
            server.print(F("OPEN"));
          server.write(newLine);
          server.write(carriageReturn);
}

void getPeriodicUpdates()
{
    //Get a reading from the controller's built in 1-wire temperature sensor
    tempAmbient = readAmbientTemp();
    
    
    //Get updates from the garage node
    Serial.print(grge_requestTempZone2); //relay the command to the serial port
    garageTempOutdoor = floatFromSerial('!');  
    Serial.print(grge_requestTempZone1);
    garageTempAmbient = floatFromSerial('!'); 
    Serial.print(grge_requestDoorStatus);
    garageDoorStatus = boolFromSerial();
    digitalWrite(garageDoorStatusPin, garageDoorStatus);

    //Get updates from the basement node
    Serial.print(bsmt_requestTemp);
    basementTempAmbient = floatFromSerial('!');    
    
    //Get updates from the bedroom node
    Serial.print(bdrm_requestTemp);
    bedroomTemperature = floatFromSerial('!');
    Serial.print(bdrm_requestSetPoint);
    bedroomTemperatureSetPoint = floatFromSerial('!');
    
    disableBedroomHeaterOnTimer();
    
    if(maintainTemperature)
          controlFurnace();
       
    if(blockHeaterEnabled)
          controlBlockHeater();
 
    now = rtc.now();//read time from the RTC   
    
    lastUpdateTime = now.unixtime();//update the timer for periodic updates
    
}

//This function saves all the data to an SD card in a CSV file format
void saveToSDCard()
{
/*
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
  if(!dataFile)
  {
    SDCardLog = false;
    return;
  }
  
  
  //make a string for assembling the data to log
  String dataString = "";
  dataString += now.year();
  dataString += now.month();
  dataString += now.day();
  dataString += ",";
  dataString += now.hour();
  dataString += now.minute();
  dataString += now.second();
  dataString += ",";
  //add data variables here
  dataString += String(garageDoorStatus);
  
  dataFile.println(dataString);
  dataFile.close(); 
  */
}



/*
freeRam() function returns the number of free bytes available in RAM from
http://www.controllerprojects.com/2011/05/23/determining-sram-usage-on-arduino/
*/
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

