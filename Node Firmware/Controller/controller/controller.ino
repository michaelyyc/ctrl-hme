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
 	Dec 21, 2013
 */

#include <Password.h>
#include <SPI.h>
#include <Ethernet.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <valueFromSerial.h>
#include <commandDefinitionsAlt.h>
#include <privatePasswordFile.h>


//Constants
#define oneWireBus1                   7 // Temperature Sensor
#define baud                       9600 // serial port baud rate
#define FWversion                  0.23 // FW version
#define tempMaximumAllowed         23.0// maximum temperature
#define tempMinimumAllowed         17.0 //minimum temperature
#define clientTimeoutLimit        90000 //time limit in ms for user input before the session is terminated by the server
//ASCII values
#define newLine                      10 // ASCII NEW LINE
#define carriageReturn               13 // ASCII Carriage return
#define tempOffset                -1.80 //offset in degrees C to make controller ambient temp agree with thermostat at centre of house


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


//Global Variables
char inputChar;	//store the value input from the serial port or telnet client
int i = 0; //for for loops
float tempAmbient = -127.0; //value used to store ambient temperature as measured by 1-wire sensor
double tempUpdateCountdown; //A countdown is used to update the temperature value periodically, not every loop
double tempUpdateDelay = 30; //Roughly the number of seconds between temp updates
double loopsPerSecond = 15000; //Roughly the number of times the main loop is processed each second
Password password = Password(MichaelPassword); //The password is contained in a private header file
float tempSetPoint = 21.0; //The setPoint temperature
float tempHysterisis = 0.25; //The values applied as +/- to the set point to avoid chattering the furnace control
bool maintainTemperature = false; //Furnace ON/Off commands are only sent if this is true
bool furnaceStatus = false; //This is true if Furnace ON commands are being sent
bool validPassword = false; //This is true any time a valid user is logged in
bool commandSent = false; //this tracks whether the controller has already relayed the command to the network, prevents sending it twice
unsigned long timeOfLastInput = 0;//this is used to determine if the client interaction has timed out


//Global variables from the basement node
float basementTempAmbient= -127.0; //value used to store basement temperature as measured from the basement node
bool basementFurnaceStatus = false;

//Global variables from Garage Node
float garageFirmware = -1; // track the garage node firmware version
float garageTempOutdoor = -127.0;  //value used to store outdoor temperature as received from the garage node
float garageTempAmbient = -127.0; //value used to store garage temperature as measured from the garage node
bool garageDoorStatus = false;
bool garageDoorError = false; //track whether garage door error flag is set
bool garageLastDoorActivation = false; //keep track of whether the last door activation was to open or close. 0 = open, 1 = close
bool garageAutoCloseStatus = false; // track whether auto close is enabled or not
bool garageRelay120V1 = 0; //track whether the 120V outlet is on, 1 = on, 0 = off





/* SETUP - RUN ONCE
 	Setup function initializes some variables and sets up the serial and ethernet ports
 	this takes about 1 second to run
 */
void setup()
{

  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  //MAC Address
  byte ip[] = { 192, 168, 1, 230 };   // IP Address
  byte gateway[] = { 10, 0, 0, 1 }; //Gateway (optional, not used)
  byte subnet[] = { 255, 255, 0, 0 }; //Subnet

  // Initialize the counter for periodic sensor updates
  tempUpdateCountdown = tempUpdateDelay * loopsPerSecond;

  // initialize the 1-wire bus
  sensors1.begin();

  //initialize the serioal port  
  Serial.begin(9600);

  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);

  // start listening for clients
  server.begin();

  //get the initial temperature value
  tempAmbient = readAmbientTemp();

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
          furnaceStatus = false; //set this false to require a new temperature comparison before sending first ON command to furnace
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
          basementFurnaceStatus = !boolFromSerial(); //The return is inverter because the basement node returns '1' as successfully disabled furnace
          if(!basementFurnaceStatus)
          {
            server.print(F("Furnace disabled - not maintaining temperature"));
            server.write(newLine);//new line
            server.write(carriageReturn);
          }
          else
          {
            server.print(F("No reply from basement node to furnace OFF command"));
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
            server.write(newLine);//new line
            server.write(carriageReturn);
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
            server.write(newLine);//new line
            server.write(carriageReturn);
          }
          else
            server.print(F("Minimum Temperature Already Reached"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }

        if(inputChar == ctrl_logOff)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Goodbye!"));
          server.write(newLine);//new line
          server.write(carriageReturn);
          client.stop();
          validPassword = false;
          break; //end the while loop
        }

        if(inputChar == ctrl_listCommands)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
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
          if(millis() < 60000)
          {
             server.print(millis(), DEC);
             server.print(F(" ms"));
          }
          else if(millis() < 120000)
          {
             server.print(millis()/60000);
             server.print(F(" minute"));
          } 
          else if(millis() < 3600000)
          {
             server.print(millis()/60000);
             server.print(F(" minutes"));
          }
          else if(millis() < 7200000)
          {
             server.print(millis()/3600000);
             server.print(F(" hour"));
          }
          else if(millis() < 86400000)
          {
             server.print(millis()/3600000);
             server.print(F(" hours"));
          }
          else
          {
             server.print(millis()/86400000);
             server.print(F(" days"));
          }                    
          server.write(carriageReturn);
          server.write(newLine);
          server.write(carriageReturn);
          server.write(newLine);

          
          server.print(F("You can use these commands...."));
          server.write(newLine);
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("GARAGE:"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("0 : Request FW Version"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("1 : Request Door Status"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("2 : Request Inside Temperature"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("3 : Activate Door"));   
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("4 : Request Auto Close Status"));   
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("5 : Disable Auto Close"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("6 : Enable Auto Close"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("7 : Turn on Block Heater"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("8 : Turn off Block Heater"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("a : Request outside temperature"));  


          server.write(newLine);
          server.write(carriageReturn);
          server.write(newLine);
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("BASEMENT:"));   
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("A : Request FW Version"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("B : Request Furnace Status"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("C : Turn Furnace On"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("D : Turn Furnace Off"));   
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("E : Turn Fan On"));   
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("F : Turn Fan and Furnace Off"));  
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("G : Request Temperature"));  
          server.write(newLine);
          server.write(carriageReturn);

          server.write(newLine);
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("MAIN FLOOR:"));   
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("K : Request FW Version"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("L : Request Temperature"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("M : Maintain Temperature"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("N : Do Not Maintain Temperature"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("O : Increase Set Temperature")); 
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("P : Decrease Set Temperature"));         
          server.write(carriageReturn);
          server.write(newLine);    
          server.write(carriageReturn);
          server.write(newLine);   
          server.print(F("x : Log off"));
          server.write(carriageReturn);
          server.write(newLine); 
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
          }

        }



        if(inputChar == grge_requestActivate120V1)
        {
          Serial.print(grge_requestActivate120V1);
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
          Serial.print(grge_requestActivate120V1);
          commandSent = true; //set the commandSent variable true so it is't sent again this loop          
          garageRelay120V1 = boolFromSerial();
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
          Serial.print(grge_requestTempZone2); //relay the command to the serial port
          garageTempOutdoor = floatFromSerial('!');
          server.print(F("Outdoor Temperature is: "));
          server.print(garageTempOutdoor);
          server.print(F(" deg C"));
          server.write(newLine);
          server.write(carriageReturn);
        }

        if(inputChar == grge_requestTempZone1)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(grge_requestTempZone1);
          garageTempAmbient = floatFromSerial('!'); 
          server.print(F("Garage Temperature is: "));
          server.print(garageTempAmbient);
          server.print(F(" deg C"));
          server.write(newLine);
          server.write(carriageReturn);
        }

        if(inputChar == grge_requestDoorStatus)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(grge_requestDoorStatus);
          garageDoorStatus = boolFromSerial();
          server.print(F("Garage door is "));
          if(garageDoorStatus)
            server.print(F("closed"));
          else
            server.print(F("open"));
          server.write(newLine);
          server.write(carriageReturn);
        }



        //BASEMENT NODE COMMANDS

        if(inputChar == bsmt_requestTemp)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial.print(bsmt_requestTemp);
          basementTempAmbient = floatFromSerial('!');
          server.print(F("Basement Temperature is: "));
          server.print(basementTempAmbient);
          server.print(F(" deg C"));
          server.write(newLine);
          server.write(carriageReturn);
        }

        //ALL OTHER COMMANDS
        //RELAY THEM TO THE SERIAL PORT (XBEE NETWORK)
        if(inputChar > 47 && inputChar < 123 && !commandSent)//ignore anything not alpha-numeric
        {
          Serial.print(inputChar);
          server.print(F("Unknown command - sending to all nodes"));
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
      tempUpdateCountdown--; //decrement the periodic update timer
      if(tempUpdateCountdown < 1) //Update the temp sensor and send furnace commands if needed
      {
        tempAmbient = readAmbientTemp();
        tempUpdateCountdown = tempUpdateDelay * loopsPerSecond; //reset the countdown
        controlFurnace();   
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
  tempUpdateCountdown--; //decrement the periodic update timer
  if(tempUpdateCountdown < 1)
  {
    tempAmbient = readAmbientTemp();
    tempUpdateCountdown = tempUpdateDelay * loopsPerSecond; //reset the countdown
    controlFurnace();
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

/* controlFurnace()
 	This function sends ON/OFF commands to the basement node to control the furnace
 	This is doen via the serial port which is connected to the XBEE network.
 	This function relies on global variables and takes no inputs
 */

void controlFurnace()
{
  //Control furnace by sending commands to the basement node based on tempAmbient

  if(tempAmbient == (-127.0 + tempOffset) || tempAmbient == (0.0 + tempOffset)) //sensor error, disable furnace
  {
    Serial.print(bsmt_turnFurnaceOff);  
    server.print(F("Sensor ERROR - Sent OFF command to basement node"));  
    server.write(carriageReturn);
    server.write(newLine);       
    basementFurnaceStatus = boolFromSerial();   
    if(!basementFurnaceStatus)
    {
      server.print(F("Furnace disabled - not maintaining temperature"));
      server.write(newLine);//new line
      server.write(carriageReturn);
    }
    else
    {
      server.print(F("Error - No reply from basement node to furnace OFF command"));
      server.write(newLine);//new line
      server.write(carriageReturn);
    }


  }

  if(maintainTemperature && tempAmbient != (- 127.0 + tempOffset) && tempAmbient != (0.0 + tempOffset))//only act when good sensor data is present and maintainTemperature is enabled
  {       
    //turn the furnace off if necessary 
    if(tempAmbient > (tempSetPoint + tempHysterisis))
    {
      furnaceStatus = false;//prevent furnace from turning on in the next loop
      Serial.print(bsmt_turnFurnaceOff);     
      server.print(F("sent OFF command to basement node "));
      server.print(tempAmbient);
      server.print(F(" deg C"));
      server.write(carriageReturn);
      server.write(newLine);  
    }  
    // turn the furnace on if necessary
    if(furnaceStatus || (tempAmbient < (tempSetPoint - tempHysterisis)))
    {
      furnaceStatus = true;//used for hysterisis when the second part of the IF condition is no longer true
      Serial.print(bsmt_turnFurnaceOn);//this command must be repeated at least every 5 minutes or the furnace will automatically turn off (see firmware for basement node)
      server.print(F("sent ON command to basement node ")); 
      server.print(tempAmbient);
      server.print(F(" deg C")); 
      server.write(carriageReturn);
      server.write(newLine);            
    }

  }
  return;
}


/* This function returns true if the client has not made any input in the amount of time specified in the function call
 */
bool didClientTimeout()
{
  unsigned long now;
  now = millis();
  if((now - timeOfLastInput) > (90000))
    return true;

  else
    return false;
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

