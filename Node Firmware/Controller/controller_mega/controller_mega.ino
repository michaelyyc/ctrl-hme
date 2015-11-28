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
 	Dec 21, 2013 - Feb 8, 2014 [Arduino Uno Version]

        Feb 16 - Change target device to Arduino Mega 2560
        Serial1 used for Xbee network
        Serial0 used for bootloading and diagnostics
 */

#include <Password.h>
#include <SPI.h>
#include <Ethernet.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Wire.h>
#include <SD.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <valueFromSerial1.h>
//#include <valueFromTelnet.h>
#include <commandDefinitionsAlt.h>
#include <XBEEDataIdentifiers.h>
#include <privatePasswordFile.h>
//#include "plotly_streaming_ethernet.h"


//Defines
//#define plotlyOn

//EEPROM Map
#define EEPROM_thermostatWeekdayTimePeriod1HourStart 			0
#define EEPROM_thermostatWeekdayTimePeriod1MinuteStart			1
#define EEPROM_thermostatWeekdayTimePeriod1SetPoint	          	2
#define EEPROM_thermostatWeekdayTimePeriod2HourStart			3
#define EEPROM_thermostatWeekdayTimePeriod2MinuteStart			4
#define EEPROM_thermostatWeekdayTimePeriod2SetPoint		        5
#define EEPROM_thermostatWeekdayTimePeriod3HourStart			6
#define EEPROM_thermostatWeekdayTimePeriod3MinuteStart			7
#define EEPROM_thermostatWeekdayTimePeriod3SetPoint			8
#define EEPROM_thermostatWeekdayTimePeriod4HourStart			9
#define EEPROM_thermostatWeekdayTimePeriod4MinuteStart 		       10
#define EEPROM_thermostatWeekdayTimePeriod4SetPoint		       11
#define EEPROM_thermostatWeekendTimePeriod1HourStart		       12
#define EEPROM_thermostatWeekendTimePeriod1MinuteStart		       13
#define EEPROM_thermostatWeekendTimePeriod1SetPoint		       14
#define EEPROM_thermostatWeekendTimePeriod2HourStart		       15
#define EEPROM_thermostatWeekendTimePeriod2MinuteStart		       16
#define EEPROM_thermostatWeekendTimePeriod2SetPoint		       17
#define EEPROM_thermostatWeekendTimePeriod3HourStart		       18
#define EEPROM_thermostatWeekendTimePeriod3MinuteStart                 19
#define EEPROM_thermostatWeekendTimePeriod3SetPoint                    20
#define EEPROM_thermostatWeekendTimePeriod4HourStart                   21
#define EEPROM_thermostatWeekendTimePeriod4MinuteStart                 22
#define EEPROM_thermostatWeekendTimePeriod4SetPoint                    23
#define EEPROM_maintainTemperature                                     24


//Constants
#define oneWireBus1                   7 // Main floor Temperature Sensor
#define baud                       9600 // serial port baud rate
#define FWversion                  0.81 // FW version
// NOTE **** line 1060 has been commented out to preven thr furnace from ever turning on... this is in place
// until a new main floor temperature sensor is installed, since the controller has now been moved into the basement ceiling

#define tempMaximumAllowed         23.0// maximum temperature
#define tempMinimumAllowed         15.0 //minimum temperature
//#define blockHeaterOnHour             4 //hour in the morning to automatically turn on block heater
//#define blockHeaterOffHour           20 //hour to turn the block heater off automatically
#define blockHeaterMaxTemp           -7 //maximum temperature INSIDE garage for block heater use
#define garageDoorStatusPin           9 //This pin is high when the garage door is closed
#define tempUpdateDelay              30 //number of seconds to wait before requesting another update from sensors when there is no telnet client connected
#define tempUpdateDelayLong          50 //number of seconds to wait before requesting another update from sensors when there IS a telnet client connected
#define clientTimeoutLimit        90000 //number of milliseconds before assuming the client has disconnected
#define tempHysterisis             0.35 //used for main floor thermostat control

//ASCII values
#define newLine                      10 // ASCII NEW LINE
#define carriageReturn               13 // ASCII Carriage return
#define tempMainFloorOffset          +0.9 //offset in degrees C to make controller ambient temp agree with thermostat at centre of house
#define chipSelect                   4 //Chip select for the SD card used to store log data


/* NETWORK CONFIGURATION
 These variables are required to configure the Ethernet shield
 The shield must be assigned a local static IP address that doesn't conflict
 on the network. It DOES NOT have DHCP capabilities for Dynamic IP

 To reach this server from outside the local network, port forwarding
 or VPN must be setup on the router
 */


EthernetServer server = EthernetServer(23); //Set up the ethernet server on port 23 (standard telnet port)
EthernetClient client = 0; //Ethernet client variable is global so it can be used by multiple functions

/* 1-WIRE Configuration
 A DS18B20 1-wire temperature sensor is connected on pin defined as OneWireBus1
 These commands setup the OneWire instance and pass the reference to the Dallas Temperature
 class that will be used to read the temperature from the device
 */
OneWire oneWire1(oneWireBus1);
DallasTemperature sensors1(&oneWire1);

//Realtime clock Configuration
RTC_DS1307 rtc; //the realtime clock object


#ifdef plotlyOn
//Plot.ly constants
#define nTraces 19
char *tokens[nTraces] = {"lk9ptjliw1","edlnwbgv76","1rq3kmwflp","yixcbzx6ln","ux0go17f8r","wz0qmd8uq7","w1cc14d5py","xtavfhzooi","bp7gfumana","b4dnrc5tml","mh8n1cetbz","x5qyhhz1tx","8qg4w9rg02","ygs2pprjiw","lubyhpbry6","2ewdc1sj33","0lil5m5c4v","xl05yr617u","nkpqmw98nl"};
plotly graph = plotly("michaelyyc", "hyv6ttj87t", tokens, "homeData", nTraces);
#endif


//Global Variables
DateTime now; //variable to store the current date & time from the RTC
DateTime startup; //store the power-on time for calculating up-time
DateTime lastUpdateTime;
int lastLogMinute; //used to keep track of SD card logging and log only once per minute
char inputChar;	//store the value input from the serial port or telnet client
int i = 0; //for for loops
float tempMainFloor = -127.0; //value used to store ambient temperature on the main floor
Password password = Password(MichaelPassword); //The password is contained in a private header file
float tempSetPoint = 21.0; //The setPoint temperature
//float tempHysterisis = 0.25; //The values applied as +/- to the set point to avoid chattering the furnace control
bool maintainTemperature; //Furnace ON/Off commands are only sent if this is true
bool furnaceStatus = false; //This is true if Furnace ON commands are being sent
bool programmableThermostatEnabled = true; // Allows user to disable programmable thermostat to force constant temperature
bool blockHeaterEnabled = false; //If this is true, blockheater automatically turns on and off with temperature and time
bool ventFanStatus = false; //stores the status of the furnace fan (true = on)
bool ventFanForceOn = false; //if this is true, the ventillation fan will run continuously
bool ventFanAutoEnabled = false; //If this is true, the status of the fan is controller based on thermostat settings
bool validPassword = false; //This is true any time a valid user is logged in
bool commandSent = false; //this tracks whether the controller has already relayed the command to the network, prevents sending it twice
bool SDCardLog = false; //We only attempt to write to the SD card if this is true
unsigned long timeOfLastInput = 0;//this is used to determine if the client interaction has timed out

//Variables for Automatic Thermostat [weekdays]
int thermostatWeekdayTimePeriod1msmStart; //system on time in the morning
float thermostatWeekdayTimePeriod1SetPoint;
int thermostatWeekdayTimePeriod2msmStart; //reduce temperature when leaving for work
float thermostatWeekdayTimePeriod2SetPoint;
int thermostatWeekdayTimePeriod3msmStart; //heat up before we get home from work
float thermostatWeekdayTimePeriod3SetPoint;
int thermostatWeekdayTimePeriod4msmStart; //turn down when going to bed
float thermostatWeekdayTimePeriod4SetPoint;
int bedroomHeaterAutoOffHour;      //automatically turn off the bedroom heater at this time
int blockHeaterOnHour = 4; //this updates based on thermostatWeekdayTimePeriod1msmStart
int blockHeaterOffHour = 9; //this updates based on thermostatWeekdayTimePeriod1msmStart

//Variables for the automatic Thermostat [weekends]
int thermostatWeekendTimePeriod1msmStart; //system on-time in the morning
float thermostatWeekendTimePeriod1SetPoint;
int thermostatWeekendTimePeriod2msmStart; //reduce temperature when leaving
float thermostatWeekendTimePeriod2SetPoint;
int thermostatWeekendTimePeriod3msmStart; //heat up before we get home
float thermostatWeekendTimePeriod3SetPoint;
int thermostatWeekendTimePeriod4msmStart; //turn down when going to bed
float thermostatWeekendTimePeriod4SetPoint;


//Global variables from the basement node
float basementTempAmbient= -127.0; //value used to store basement temperature as measured from the basement node
float livingRoomTemperature = -127.0;
//Global variables from Garage Node
float garageFirmware = -1; // track the garage node firmware version
float garageTempOutdoor = -127.0;  //value used to store outdoor temperature as received from the garage node
float garageTempAmbient = -127.0; //value used to store garage temperature as measured from the garage node
int garageDoorStatus = -1111; // 0 = open, 1 = closed, -1111 = timeout
bool garageDoorError = false; //track whether garage door error flag is set
bool garageLastDoorActivation = false; //keep track of whether the last door activation was to open or close. 0 = open, 1 = close
bool garageAutoCloseStatus = false; // track whether auto close is enabled or not
bool blockHeaterStatus = 0; //track whether the 120V outlet is on, 1 = on, 0 = off

//Global variables from the bedroom node
float bedroomFirmware = -1;
float masterBedroomTemperature = -127.0;
float masterBedroomTemperatureSetPoint = 17.0;
float backBedroomTemperature = -127.0;
bool bedroomMaintainTemp = false;
bool bedroomHeaterStatus = false;

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

  now = rtc.now();//update the time from the RTC
  startup = now; //store the current time as the startup time

  //Initialize the SD card

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);//Needed to enable SD card write
  if (SD.begin(chipSelect))
    {
      SDCardLog = true;
      Serial.println("SD Card Initialized");
      writeHeaderToSDCard();
    }
   else
      Serial.println("SD Card Failed to Initialize");

  // initialize the 1-wire bus
  sensors1.begin();

  //initialize the serioal ports
  Serial.begin(9600); //diagnostic output port
  Serial1.begin(9600); //XBEE network port

  //Restore settings from EEPROM
  programThermostatRestoreFromEEPROM();

  //get the initial periodic update values
  getPeriodicUpdates();

  //Because the first value requested from network usually times out, request it again during the startup sequence
  Serial1.print(grge_requestTempZone2); //relay the command to the serial port
  garageTempOutdoor = floatFromSerial1('!');

  lastLogMinute = now.minute() - 1; // initialize the last logged time so that the logging function will always save on the first call
  saveToSDCard();

    //Initialize the Ethernet Adapter
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  //MAC Address
  byte ip[] = { 192, 168, 1, 230 };   // IP Address
  byte dns[] = { 8, 8, 8, 8 };
  byte gateway[] = { 192, 168, 1, 100 }; //Gateway (optional, not used)
  byte subnet[] = { 255, 255, 255, 0 }; //Subnet

  // initialize the ethernet device
  Ethernet.begin(mac, ip, dns, gateway, subnet);

 #ifdef plotlyOn
  // connect to plotly server
  graph.fileopt="extend";
  graph.timezone="America/Denver";
  while(!graph.init()){
  if(graph.init()){
    break;
    }
  };
  graph.openStream(); 
  #endif

  // start listening for clients
  server.begin();


}

void loop()
{
  // This loop waits for a client to connect. While waiting, the temperature is updated
  // Periodically and the furnace control function is called to maintain temperature if
  // appropriately configured.

  // Check if any messages ahve arrived from the XBEE nodes
  readSerial();


  client = server.available();

  // clear the valid password flag from any previous user
  validPassword = false; // Should already be false, but just in case the last session didn't end cleanly

  // if an incoming client connects, there will be bytes available to read:

  client = server.available();
  if (client == true)
  {

    //Found that the some telnet clients send 24-26 bytes of data once connection is established.
    // Client trying to negotiate connecrtion? This section just ignores the first 250ms of data
    delay(250); //wait 250ms for incoming data from the client
    flushTelnet();//clear any inputs buffered from the telnet client

    // Prompt for password and compare to the actual value
    // The user must terminate password with '?'
    // Typing errors can be cleared by sending '!'
    // If the password is wrong, the client will be disconnected and server will wait
    // for ~30 seconds before allowing new connections
    server.print(F("Enter password: "));
    while(!validPassword && client.connected())
    {

// Check if any messages ahve arrived from the XBEE nodes
      readSerial();
      
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
            delay(10000); //Wait a short while before allowing any more connections
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
 
      // Check if any messages ahve arrived from the XBEE nodes
    readSerial();

    //Connected Loop - this is repeated as long as the client is connected
    //The loop relays data between the telnet client and the serial port (XBEE network)
    //Certain commands are handed by this node directly to the telnet client without any serial communication
    //Only ascii data is relayed from the telnet client to the serial port
    while(validPassword && client.connected())
    {
      // Check if any messages ahve arrived from the XBEE nodes
      readSerial();

      
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
        
        if(inputChar == ctrl_telnetSerialRelay)
        {
         commandSent = true; //set the commandSent variable true so it is't sent again this loop
         server.print(F("Starting telnet<->Serial Relay. Escape character is '!'")); 
         server.write(newLine);//new line
         server.write(carriageReturn);
         telnetSerialRelay('!');
         server.print(F("Relay session ended")); 
         server.write(newLine);//new line
         server.write(carriageReturn);
        }
        
        if(inputChar == ctrl_triggerUpdatesNow)
        {
         getPeriodicUpdates(); 
         commandSent = true; //set the commandSent variable true so it is't sent again this loop  
        }

 /*       if(inputChar == ctrl_requestTemp)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Main Floor Temperature: "));
          server.print(tempMainFloor);
          server.print(F(" 'C"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }
*/
        if(inputChar == ctrl_enableFurnace)

        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          maintainTemperature = true;
          EEPROM.write(EEPROM_maintainTemperature, 1);
          server.print(F("Main Thermostat On"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }

        if(inputChar == ctrl_disableFurnace)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          maintainTemperature = false;
          EEPROM.write(EEPROM_maintainTemperature, 0);
          furnaceStatus = false;
          Serial1.print(bsmt_turnFurnaceOff);
          if(!boolFromSerial1())
          {
                      server.print(F("Error - No reply from basement node"));
          }
          else
          {
                      server.print(F("Main Thermostat Off"));
                      server.write(newLine);//new line
                      server.write(carriageReturn);
          }


        }


        if(inputChar == ctrl_setTempSetPoint)
        {
            commandSent = true;
            tempSetPoint = programThermostatSetPoint(-1); //Get a new thermostat set point from telnet connection
            server.print(F("New main floor set point: "));
            server.print(tempSetPoint);
            server.print(F("' C"));

            if(!maintainTemperature)
            {
            server.print(F(" NOTE: Thermostat is NOT enabled"));
            }
            server.write(newLine);//new line
            server.write(carriageReturn);

            if(programmableThermostatEnabled)//diable the thermostat schedule if required
            {
            commandSent = false;
            inputChar = ctrl_toggleThermostatSchedule;
            }
        }

/*
        if(inputChar == ctrl_increaseTempSetPoint)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          if(tempSetPoint < tempMaximumAllowed)
          {
            tempSetPoint = tempSetPoint + 0.25;
            server.print(F("Increased temp set point to: "));
            server.print(tempSetPoint);
            server.print(F(" 'C"));
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
            server.print(F(" 'C"));
          }
          else
            server.print(F("Minimum Temperature Already Reached"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }
*/
        if(inputChar == ctrl_enableBlockHeater)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          blockHeaterEnabled = true;
          server.print(F("Block Heater will turn on between "));
          server.print(blockHeaterOnHour);
          server.print(F(":00h and "));
          server.print(blockHeaterOffHour);
          server.print(F(":00h if garage temp is below "));
          server.print(blockHeaterMaxTemp);
          server.print(F(" 'C"));
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

        if(inputChar == ctrl_programmableThermostat)
        {
           commandSent = true;
           programThermostat();
        }

        if(inputChar == ctrl_setBedroomSetPoint)
        {
           commandSent = true;
           masterBedroomTemperatureSetPoint = programThermostatSetPoint(-1); //Get a new thermostat set point from telnet connection
            server.print(F("New bedroom set point: "));
            server.print(masterBedroomTemperatureSetPoint);
            server.print(F("' C"));

            if(!bedroomMaintainTemp)
            {
            server.print(F(" NOTE: Bedroom thermostat is NOT enabled"));
            }
            server.write(newLine);//new line
            server.write(carriageReturn);
        }

        if(inputChar == ctrl_enableBedroomHeater)
        {
           commandSent = true;
           bedroomMaintainTemp = true;
           server.print(F("Maintaining "));
           server.print(masterBedroomTemperatureSetPoint);
           server.print(F(" 'C in bedroom until "));
           server.print(bedroomHeaterAutoOffHour);
           server.print(F(":00am"));
           server.write(newLine);//new line
           server.write(carriageReturn);
        }

        if(inputChar == ctrl_disableBedroomHeater)
        {
           commandSent = true;
           bedroomMaintainTemp = false;
           bedroomHeaterStatus = false;
           Serial1.print(bdrm_requestDeactivate120V1);

           server.print(F("Bedroom heater disabled"));
           server.write(newLine);//new line
           server.write(carriageReturn);
        }

        if(inputChar == ctrl_toggleThermostatSchedule)
        {
         commandSent = true;
         programmableThermostatEnabled = !programmableThermostatEnabled;
         server.print(F("Programmable Thermostat Schedule is "));
         if(programmableThermostatEnabled)
         server.print(F("En"));
         else
         server.print(F("Dis"));

         server.print(F("abled"));
         server.write(newLine);//new line
         server.write(carriageReturn);

        }

        if(inputChar == ctrl_turnFanOn)
        {
         commandSent = true;
	 ventFanForceOn = true;
      	 Serial1.print(bsmt_turnFanOn);
         ventFanStatus = boolFromSerial1();
         if(ventFanStatus)
         {
         	server.print(F("Furnace vent turned on"));
          }
		else
		{
			  	server.print(F("ERROR: No response"));
		}
			server.write(newLine);//new line
         	server.write(carriageReturn);
        }

		if(inputChar == ctrl_turnFanOff)
		{
		 commandSent = true;
	         ventFanForceOn = false;
		 Serial1.print(bsmt_turnFanOff);
		 ventFanStatus = !boolFromSerial1();
		 if(!ventFanStatus)
		 {
			server.print(F("Furnace vent turned off"));
		  }
		else
		{
				server.print(F("ERROR: No response"));
		}
			server.write(newLine);//new line
			server.write(carriageReturn);
		}

       if(inputChar == ctrl_fanAutoOn)
  {
        commandSent = true;
        ventFanAutoEnabled = true;
        server.print(F("Vent Fan automatic control enabled"));

	server.write(newLine);//new line
	server.write(carriageReturn);
  }
         if(inputChar == ctrl_fanAutoOff)
  {
        commandSent = true;
        ventFanAutoEnabled = false;
        server.print(F("Vent Fan automatic control disabled"));
      
	server.write(newLine);//new line
	server.write(carriageReturn);
  }

/*
        //Defined commands not handled by this node
        //relay them and store results in variables in the controller
        if(inputChar == grge_requestFWVer)
        {
          Serial1.print(grge_requestFWVer);//send the command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageFirmware = floatFromSerial1('!');
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
*/

        if(inputChar == grge_requestActivateDoor)
        {
            Serial1.print(grge_requestActivateDoor);
            commandSent = true; //set the commandSent variable true so it is't sent again this loop
            garageLastDoorActivation = boolFromSerial1();
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
              garageDoorStatus = 0;
              digitalWrite(garageDoorStatusPin, garageDoorStatus);
            }
          }

        if(inputChar == grge_requestActivate120V1)
        {
          Serial1.print(grge_requestActivate120V1);
          blockHeaterEnabled == false; //This is necessary to prevent block heater from turning off right away if time or temp is out of range
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          blockHeaterStatus = boolFromSerial1();
          if(blockHeaterStatus)
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
          Serial1.print(grge_requestDeactivate120V1);
          blockHeaterEnabled == false; //This is necessary to prevent block heater from turning back on right away if time and temp are in range
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          blockHeaterStatus = !boolFromSerial1();
          if(blockHeaterStatus)
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
          Serial1.print(grge_requestAutoCloseStatus);//send command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageAutoCloseStatus = boolFromSerial1();
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
          Serial1.print(grge_requestDisableAutoClose);//send command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageAutoCloseStatus = boolFromSerial1();
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
          Serial1.print(grge_requestEnableAutoClose);//send command to the garage node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageAutoCloseStatus = boolFromSerial1();
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
          Serial1.print(grge_requestClearErrorFlag);//send command to node
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          garageDoorError = !boolFromSerial1(); //if the flag is cleared, garage node will send a '1' (successfully cleared)
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

/*
        if(inputChar == grge_requestTempZone2)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Outdoor Temperature is: "));
          server.print(garageTempOutdoor);
          server.print(F(" 'C"));
          server.write(newLine);
          server.write(carriageReturn);
        }

        if(inputChar == grge_requestTempZone1)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Garage Temperature is: "));
          server.print(garageTempAmbient);
          server.print(F(" 'C"));
          server.write(newLine);
          server.write(carriageReturn);
        }
*/
        if(inputChar == grge_requestDoorStatus)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
 //         requestAndUpdateGarageDoorStatus();
           server.print(F("Garage door is "));
          if(garageDoorStatus == 1)
            server.print(F("CLOSED"));
          if(garageDoorStatus == 0)
            server.print(F("OPEN"));
          server.write(newLine);
          server.write(carriageReturn);
        }



        //BASEMENT NODE COMMANDS
/*
        if(inputChar == bsmt_requestTemp)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          server.print(F("Basement Temperature is: "));
          server.print(basementTempAmbient);
          server.print(F(" 'C"));
          server.write(newLine);
          server.write(carriageReturn);
        }
*/


        //BEDROOM NODE COMMANDS
/*
         if(inputChar == bdrm_requestFWVer)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial1.print(bdrm_requestFWVer);
          bedroomFirmware = floatFromSerial1('!');
          server.print(F("Bedroom node FW Version: "));
          server.print(bedroomFirmware);
          server.write(newLine);
          server.write(carriageReturn);
        }
*/
/*
         if(inputChar == bdrm_requestTemp)
        {
          commandSent = true; //set the commandSent variable true so it is't sent again this loop
          Serial1.print(bdrm_requestTemp);
          masterBedroomTemperature = floatFromSerial1('!');
          server.print(F("Bedroom Temperature: "));
          server.print(masterBedroomTemperature);
          server.write(newLine);
          server.write(carriageReturn);
        }
 */

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

      //RELAY SERIAL DATA TO TELNET CLIENT OR Serial0
      if(Serial1.available())
      {
        inputChar = Serial1.read();//read the serial input
        //server.write(inputChar);//output the serial input to the telnet client
        Serial.print(inputChar);
      }
      now = rtc.now();//get time from the RTC
      if((now.unixtime() - lastUpdateTime.unixtime()) > tempUpdateDelayLong) //Update the temp sensors and send furnace commands if needed
      {
        getPeriodicUpdates();//Update the temperature and other sensors
        saveToSDCard();
        sendToPlotly();

      }
      if(didClientTimeout())
      {
        server.print(F("Session Timeout - Disconnecting"));
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
        saveToSDCard();
        sendToPlotly();

      }

  //If there is no telnet client connected, just read and ignore the serial port
  //This is to prevent the buffer from filling up when the is no telnet client but nodes are sending data
  //Only one byte is ignored per loop but the serial port at 9600 baud shouldn't be faster than the main loop
  if(Serial1.available())
  {
    inputChar = Serial1.read();
    Serial.write(inputChar);
  }

}// END OF THE MAIN LOOP FUNCTION


// readAmbientTemp()
// 	This function collects a temperature reading in degrees C from the 1-wire
// 	sensor and returns it as a float
float readAmbientTemp()
{
  float temp;
  sensors1.requestTemperatures();
  temp = sensors1.getTempCByIndex(0);
  return temp;
}

void disableBedroomHeaterOnTimer()//Automatically turns the bedroom heater off if called within 5 minutes of bedroomHeaterAutoOffHour
{
    now = rtc.now();
    if(now.hour() == bedroomHeaterAutoOffHour && now.minute() <= 5)
    {
      bedroomMaintainTemp = false;
      bedroomHeaterStatus = false;
    }
}

/* controlFurnace()
 	This function sends ON/OFF commands to the basement node to control the furnace
 	This is doen via the serial port which is connected to the XBEE network.
 	This function relies on global variables and takes no inputs
 */

void controlFurnace()
{
  //Make sure tempSetPoint is reasonable. This is a safety feature to prevent house over-heating
  if(tempSetPoint < tempMinimumAllowed || tempSetPoint > tempMaximumAllowed)
  {
    Serial.println("ERROR: Temperature Setpoint of of range!");
    return;
  }


  //Control furnace by sending commands to the basement node based on tempMainFloor

  if(tempMainFloor == (-127.0 + tempMainFloorOffset) || tempMainFloor == (0.0 + tempMainFloorOffset))
  {
      return;//Don't act on bad values - do nothing
  }
      //turn the furnace off if necessary
      if(tempMainFloor > (tempSetPoint + tempHysterisis))
      {
        furnaceStatus = false;//prevent furnace from turning on in the next loop
        Serial1.print(bsmt_turnFurnaceOff);
        if(!boolFromSerial1())
        {
 //         server.print(F("ERROR: Basement node didn't acknowledge furnace off command"));
 //         server.write(carriageReturn);
 //         server.write(newLine);
        }
      }


      // turn the furnace on if necessary
      if(furnaceStatus || (tempMainFloor < (tempSetPoint - tempHysterisis)))
      {
        furnaceStatus = true;//used for hysterisis when the second part of the IF condition is no longer true
        Serial1.print(bsmt_turnFurnaceOn);//this command must be repeated at least every 5 minutes or the furnace will automatically turn off (see firmware for basement node)
//        server.print(F("sent ON command to basement node "));
//        server.print(tempMainFloor);
//        server.print(F(" 'C "));
        if(boolFromSerial1())
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

//This function sends commands to turn the furnace ventillation fan on or off as needed
void controlVentFan()
{
      if(ventFanForceOn)//remind the basement node to keep the fan on if this variable is set
      {
  	 Serial1.print(bsmt_turnFanOn);
         ventFanStatus = boolFromSerial1();
         return;
      }
      
      if(ventFanAutoEnabled && (tempMainFloor > tempSetPoint + tempHysterisis))
      {
        if((garageTempOutdoor - 1.0) < tempMainFloor)//the fan should only be on if it's colder outside than inside
        {
         Serial1.print(bsmt_turnFanOn);
         ventFanStatus = boolFromSerial1();  
         return;
        }
      }
      
      if(ventFanAutoEnabled && (tempMainFloor < tempSetPoint - tempHysterisis))
      {
       Serial1.print(bsmt_turnFanOff);
       ventFanStatus = !boolFromSerial1(); 
       return;
      }
      
      ventFanStatus = false;//If the command to turn on the vent fan hasn't been sent lately, set the status to false.
      Serial1.print(bsmt_turnFanOff);
      ventFanStatus = !boolFromSerial1();  
  return;
}


void controlBedroomHeater()
{
  //Make sure tempSetPoint is reasonable. This is a safety feature to prevent house over-heating
  if(masterBedroomTemperatureSetPoint < tempMinimumAllowed || masterBedroomTemperatureSetPoint > tempMaximumAllowed)
  {
    Serial.println("ERROR: Bedroom temperature Setpoint of of range!");
    return;
  }


  //Control furnace by sending commands to the bedroom node based on tempMainFloor

  if(masterBedroomTemperature == -127.0 || masterBedroomTemperature == 0.0)
  {
      return;//Don't act on bad values - do nothing
  }
      //turn the furnace off if necessary
      if(masterBedroomTemperature > (masterBedroomTemperatureSetPoint + tempHysterisis))
      {
        bedroomHeaterStatus = false;//prevent heater from turning on in the next loop
        Serial1.print(bdrm_requestDeactivate120V1);
        Serial.println("Bedroom heater turned off");
        Serial.print("Sent ");
        Serial.println(bdrm_requestDeactivate120V1);
        if(!boolFromSerial1())
        {
 //         server.print(F("ERROR: Bedroom node didn't acknowledge furnace off command"));
 //         server.write(carriageReturn);
 //         server.write(newLine);
        }
      }


      // turn the furnace on if necessary
      if(bedroomHeaterStatus || (masterBedroomTemperature < (masterBedroomTemperatureSetPoint - tempHysterisis)))
      {
        bedroomHeaterStatus = true;//used for hysterisis when the second part of the IF condition is no longer true
        Serial1.print(bdrm_requestActivate120V1);//this command must be repeated at least every 5 minutes or the heater will automatically turn off (see firmware for basement node)
        Serial.println("Bedroom heater turned ON");
        Serial.print("Sent ");
        Serial.println(bdrm_requestActivate120V1);
//        server.print(F("sent ON command to bedroom node "));
//        server.print(tempMainFloor);
//        server.print(F(" 'C "));
        if(boolFromSerial1())
          {
//            server.print(F("ACK"));
//            server.write(carriageReturn);
//            server.write(newLine);
          }

        else
          {
 //           server.print(F("ERROR: Bedroom node didn't acknowledge furnace on command"));
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
  if(now.hour() >= blockHeaterOnHour && now.hour() < blockHeaterOffHour && garageTempAmbient < (blockHeaterMaxTemp - 0.25))
  {
          Serial1.print(grge_requestActivate120V1);
          blockHeaterStatus = boolFromSerial1();
  }

  if(now.hour() < blockHeaterOnHour  || now.hour() >= blockHeaterOffHour || garageTempAmbient > (blockHeaterMaxTemp + 0.25))
  {
          Serial1.print(grge_requestDeactivate120V1);
          blockHeaterStatus = !boolFromSerial1();
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
          server.print(F("E : Turn Fan On                       B : Set Temperature"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("F : Turn Fan and Furnace Off          m : Maintain Temperature"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("M : Maintain Set Temperature          n : Do Not Maintain Temperature"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("O : Turn Fan On                       o : Turn Fan Off"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("P : Automatic Fan On                  p : Automatic Fan Off"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("N : Do Not Maintain Temperature"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("I : Set main floor temperature"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("T : Program Thermostat Schedule"));
          server.write(newLine);
          server.write(carriageReturn);
          server.print(F("t : Enabled/Disable Schedule"));
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
         server.print(F("Controller FW Version: "));
          server.print(FWversion);
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("Memory Used: "));
          server.print(8192 - freeRam());
          server.print(F(" / 8192 Bytes ["));
          server.print((freeRam()/(float(8192))*(float(100))));
          server.print(F("% free]"));
          server.write(carriageReturn);
          server.write(newLine);
          server.print(F("Uptime: "));
          sendUptime();
          if(!SDCardLog)
         {
          server.print(("NOT "));
         }
          server.print(F("Logging to SD Card"));
          server.write(carriageReturn);
          server.write(newLine);
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
       server.write(carriageReturn);
       server.write(newLine);
       server.write(carriageReturn);
       server.write(newLine);


      server.print(F("SETTINGS"));
      server.write(newLine);
      server.write(carriageReturn);
      server.print(F("M.Bedroom Thermostat: "));
      server.print(masterBedroomTemperatureSetPoint);
      server.print(F(" 'C"));


     if(bedroomMaintainTemp)
        {
          server.print(F(" ON"));
        }

      else
        {
          server.print(F(" OFF"));
        }

        server.write(newLine);//new line
        server.write(carriageReturn);

       server.print(F("Main Thermostat     : "));
       server.print(tempSetPoint);
       server.print(F(" 'C"));


     if(maintainTemperature)
        {
          server.print(F(" ON  "));
        }

      else
        {
          server.print(F(" OFF  "));
        }

        server.write(newLine);//new line
        server.write(carriageReturn);

       server.print(F("Automatic Vent      : "));
     if(ventFanAutoEnabled)
        {
          server.print(F("ENABLED"));
        }

      else
        {
          server.print(F("DISABLED"));
        }

        server.write(newLine);//new line
        server.write(carriageReturn);


        server.print(F("Block Heater will "));
        if(blockHeaterEnabled)
        {
          server.print(F("turn on between "));
          server.print(blockHeaterOnHour);
          server.print(F(":00h and "));
          server.print(blockHeaterOffHour);
          server.print(F(":00h if garage temp is below "));
          server.print(blockHeaterMaxTemp);
          server.print(F(" 'C"));
          server.write(newLine);//new line
          server.write(carriageReturn);
        }

        else
        {
         server.print(F("not turn on automatically"));
         server.write(newLine);//new line
         server.write(carriageReturn);
        }



         server.print(F("Programmable Thermostat Schedule is "));
         if(programmableThermostatEnabled)
         server.print(F("En"));
         else
         server.print(F("Dis"));

         server.print(F("abled"));
         server.write(newLine);//new line
         server.write(carriageReturn);
         server.write(newLine);
         server.write(carriageReturn);


         server.print(F("STATUS"));
           server.write(newLine);
          server.write(carriageReturn);

  
        server.print(F("Furnace is "));

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
  
        server.print(F("Ventillation Fan is "));

        if(ventFanStatus)
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

        if(bedroomHeaterStatus)
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

        if(blockHeaterStatus)
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


        server.print(F("Garage door is "));
          if(garageDoorStatus == 1)
            server.print(F("CLOSED"));
          if(garageDoorStatus == 0)
            server.print(F("OPEN"));
            server.write(newLine);
        server.write(carriageReturn);
            server.write(newLine);
        server.write(carriageReturn);

        server.print(F("TEMPERATURES"));
      server.write(newLine);//new line
      server.write(carriageReturn);
      server.print(F("Main Floor:     "));
      server.print(tempMainFloor);
      server.print(F(" 'C"));
      server.write(newLine);//new line
      server.write(carriageReturn);

      server.print(F("Back Bedroom:   "));
      server.print(backBedroomTemperature);
      server.print(F(" 'C"));
      server.write(newLine);//new line
      server.write(carriageReturn);
      server.print(F("Master Bedroom: "));
      server.print(masterBedroomTemperature);
      server.print(F(" 'C"));
      server.write(newLine);//new line
      server.write(carriageReturn);

      server.print(F("Basement:       "));
      server.print(basementTempAmbient);
      server.print(F(" 'C"));
      server.write(newLine);
      server.write(carriageReturn);

      server.print(F("Garage:         "));
      server.print(garageTempAmbient);
      server.print(F(" 'C"));
      server.write(newLine);
      server.write(carriageReturn);

      server.print(F("Outdoor:        "));
      server.print(garageTempOutdoor);
      server.print(F(" 'C"));
      server.write(newLine);
      server.write(carriageReturn);


}

void sendUptime()
{
  now = rtc.now();
 // DateTime uptime = now.unixtime() - startup.unixtime();
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
          Serial1.print(grge_requestDoorStatus);
          garageDoorStatus = boolIntFromSerial1();
          digitalWrite(garageDoorStatusPin, garageDoorStatus);

          server.print(F("Garage door is "));
          if(garageDoorStatus == 1)
            server.print(F("CLOSED"));
          if(garageDoorStatus == 0)
            server.print(F("OPEN"));
          if(garageDoorStatus == -1111)
            server.print(F("UNRESPONSIVE"));
          server.write(newLine);
          server.write(carriageReturn);
}

void getPeriodicUpdates()
{
    //Get a reading from the controller's built in 1-wire temperature sensor

    if(validPassword) //only output this if someone is logged in successfully
    server.print(F("Updating sensor data..."));
  
    Serial1.flush();//flush serial buffer

    //Get updates from the garage node
    Serial1.print(grge_requestTempZone2); //relay the command to the serial port
    garageTempOutdoor = floatFromSerial1('!');
    Serial1.print(grge_requestTempZone1);
    garageTempAmbient = floatFromSerial1('!');
    Serial1.print(grge_requestDoorStatus);
    garageDoorStatus = boolIntFromSerial1();
    if(garageDoorStatus == -1111)//no response from garage node
    {
     if(!client.connected())//if there is no client connected
      {
       Serial1.flush();//flush serial buffer
       Serial1.print(grge_requestDoorStatus);//and try getting garage door status one more time
       garageDoorStatus = boolIntFromSerial1();   
      }
    }
    digitalWrite(garageDoorStatusPin, garageDoorStatus);

    //Get updates from the basement node
    Serial1.print(bsmt_requestTemp);
    basementTempAmbient = floatFromSerial1('!');
    Serial1.print(bsmt_requestTempZone2);
    backBedroomTemperature = floatFromSerial1('!');
    Serial1.print(bsmt_requestTempZone3);
    tempMainFloor = floatFromSerial1('!') + tempMainFloorOffset;
    
    
    //Get updates from the bedroom node
    Serial1.print(bdrm_requestTemp);
    masterBedroomTemperature = floatFromSerial1('!');

    //Perform time-triggered actions
    disableBedroomHeaterOnTimer(); //Disable bedroom heater at pre-defined time
    automaticTempSetPoint(); //set the temperature set point based on day and time

    if(maintainTemperature)
          controlFurnace();

    if(blockHeaterEnabled)
          controlBlockHeater();

    if(bedroomMaintainTemp)
          controlBedroomHeater();
          
    controlVentFan();

    now = rtc.now();//read time from the RTC

    lastUpdateTime = now.unixtime();//update the timer for periodic updates
    
    
    if(validPassword) //only output this if someone is logged in successfully
  {
    server.print(F("Done"));
    server.write(newLine);
    server.write(carriageReturn);
  }
}

void automaticTempSetPoint()
{
  if(!programmableThermostatEnabled) // don't change the set point if the programmable thermostat is disabled
  {
    return;
  }

  now = rtc.now();//update the time from the RTC
  int nowMsm = minutesSinceMidnight(now.hour(), now.minute());

  if(now.dayOfWeek() > 0 && now.dayOfWeek() < 6) // Monday = 1, Friday = 5
  {

   if(nowMsm < thermostatWeekdayTimePeriod1msmStart)//if before start of first period
   {
     if(now.dayOfWeek() == 1)//if it's Monday
       tempSetPoint = thermostatWeekendTimePeriod4SetPoint; //use temperature from period 4 of Sunday night

    else
    tempSetPoint = thermostatWeekdayTimePeriod4SetPoint; //use temperature from period 4 of the previous weekday
   }


   if(nowMsm >= thermostatWeekdayTimePeriod1msmStart && nowMsm < thermostatWeekdayTimePeriod2msmStart)
     tempSetPoint = thermostatWeekdayTimePeriod1SetPoint;

   if(nowMsm >= thermostatWeekdayTimePeriod2msmStart && nowMsm < thermostatWeekdayTimePeriod3msmStart)
     tempSetPoint = thermostatWeekdayTimePeriod2SetPoint;

   if(nowMsm >= thermostatWeekdayTimePeriod3msmStart && nowMsm < thermostatWeekdayTimePeriod4msmStart)
     tempSetPoint = thermostatWeekdayTimePeriod3SetPoint;

   if(nowMsm >= thermostatWeekdayTimePeriod4msmStart)
     tempSetPoint = thermostatWeekdayTimePeriod4SetPoint;


  }
  if(now.dayOfWeek() == 0 || now.dayOfWeek() == 6) //sunday = 0, saturday = 6
  {
   if(nowMsm < thermostatWeekendTimePeriod1msmStart)//if before start of first period
    {
    if(now.dayOfWeek() == 6) //if it's saturday morning
      tempSetPoint = thermostatWeekdayTimePeriod4SetPoint;//use temperature from period 4 of Friday night

    else
          tempSetPoint = thermostatWeekendTimePeriod4SetPoint;//use temperature from period 4 of Saturday night
    }

   if(nowMsm >= thermostatWeekendTimePeriod1msmStart && nowMsm < thermostatWeekendTimePeriod2msmStart)
     tempSetPoint = thermostatWeekendTimePeriod1SetPoint;
     
   if(nowMsm >= thermostatWeekendTimePeriod2msmStart && nowMsm < thermostatWeekendTimePeriod3msmStart)
     tempSetPoint = thermostatWeekendTimePeriod2SetPoint;

   if(nowMsm >= thermostatWeekendTimePeriod3msmStart && nowMsm < thermostatWeekendTimePeriod4msmStart)
     tempSetPoint = thermostatWeekendTimePeriod3SetPoint;

   if(nowMsm >= thermostatWeekendTimePeriod4msmStart)
     tempSetPoint = thermostatWeekendTimePeriod4SetPoint;
  }
  return;
}

void programThermostat()
{
// display opening message to client
  server.print(F("Programmable Thermostat Adjustment"));
  server.write(newLine);
  server.write(carriageReturn);

// display currnet program and adjustments to client
  programThermostatDisplaySettings();
  server.print(F("Type the letter of the item you wish to change, or type 'x'"));
  server.write(newLine);
  server.write(carriageReturn);

  //read input from client
    while(client.connected())
    {
      //See if it's time to update the periodic sensors
      now = rtc.now();//update the time from the RTC
      if((now.unixtime() - lastUpdateTime.unixtime()) > tempUpdateDelayLong) //Update the temp sensors and send furnace commands if needed
      {
        getPeriodicUpdates();//Update the temperature and other sensors
        saveToSDCard();
        sendToPlotly();
      }

      //see if the telnet client is still alive
      if(didClientTimeout())
      {
        server.print(F("Session Timeout - Disconnecting"));
        server.write(newLine);
        server.write(carriageReturn);
        client.stop();
        password.reset();
        timeOfLastInput = millis();
        return;
      }

      if(client.available())
      {
        inputChar = client.read();
        timeOfLastInput = millis();//set the time of last Input to NOW
        switch (inputChar){
        case 'x': //return to main menu
          server.print(F("Returning to main menu"));
          server.write(newLine);
          server.write(carriageReturn);
          return;
          break;
          case 'a': //change weekday period 1
          thermostatWeekdayTimePeriod1msmStart = programThermostatmsm(EEPROM_thermostatWeekdayTimePeriod1HourStart, EEPROM_thermostatWeekdayTimePeriod1MinuteStart);
          thermostatWeekdayTimePeriod1SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekdayTimePeriod1SetPoint);
            bedroomHeaterAutoOffHour = EEPROM.read(EEPROM_thermostatWeekdayTimePeriod1HourStart);
            blockHeaterOnHour = bedroomHeaterAutoOffHour - 2;
            blockHeaterOffHour = bedroomHeaterAutoOffHour + 4;
            programThermostatDisplaySettings();

          break;

          case 'b': //change weekday period 2
          thermostatWeekdayTimePeriod2msmStart = programThermostatmsm(EEPROM_thermostatWeekdayTimePeriod2HourStart, EEPROM_thermostatWeekdayTimePeriod2MinuteStart);
          thermostatWeekdayTimePeriod2SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekdayTimePeriod2SetPoint);
            programThermostatDisplaySettings();

          break;

          case 'c': //change weekday period 3
          thermostatWeekdayTimePeriod3msmStart = programThermostatmsm(EEPROM_thermostatWeekdayTimePeriod3HourStart, EEPROM_thermostatWeekdayTimePeriod3MinuteStart);
          thermostatWeekdayTimePeriod3SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekdayTimePeriod3SetPoint);
            programThermostatDisplaySettings();
          break;

          case 'd': //change weekday period 4
          thermostatWeekdayTimePeriod4msmStart = programThermostatmsm(EEPROM_thermostatWeekdayTimePeriod4HourStart, EEPROM_thermostatWeekdayTimePeriod4MinuteStart);
          thermostatWeekdayTimePeriod4SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekdayTimePeriod4SetPoint);
          programThermostatDisplaySettings();
          break;

          case 'e': //change weekend period 1
          thermostatWeekendTimePeriod1msmStart = programThermostatmsm(EEPROM_thermostatWeekendTimePeriod1HourStart, EEPROM_thermostatWeekendTimePeriod1MinuteStart);
          thermostatWeekendTimePeriod1SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekendTimePeriod1SetPoint);
            programThermostatDisplaySettings();
          break;

          case 'f': //change weekend period 2
          thermostatWeekendTimePeriod2msmStart = programThermostatmsm(EEPROM_thermostatWeekendTimePeriod2HourStart, EEPROM_thermostatWeekendTimePeriod2MinuteStart);
          thermostatWeekendTimePeriod2SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekendTimePeriod2SetPoint);
            programThermostatDisplaySettings();
          break;

          case 'g': //change weekend period 3
          thermostatWeekendTimePeriod3msmStart = programThermostatmsm(EEPROM_thermostatWeekendTimePeriod3HourStart, EEPROM_thermostatWeekendTimePeriod3MinuteStart);
          thermostatWeekendTimePeriod3SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekendTimePeriod3SetPoint);
            programThermostatDisplaySettings();
          break;

          case 'h': //change weekend period 4
          thermostatWeekendTimePeriod4msmStart = programThermostatmsm(EEPROM_thermostatWeekendTimePeriod4HourStart, EEPROM_thermostatWeekendTimePeriod4MinuteStart);
          thermostatWeekendTimePeriod4SetPoint = programThermostatSetPoint(EEPROM_thermostatWeekendTimePeriod4SetPoint);
            programThermostatDisplaySettings();
          break;
        }
    }
  }
}

int programThermostatHour(int EEPROMLocation) // get an hour from the telnet client and return it as an integer
{
  int newHour = -1;
  flushTelnet(); //clear any inputs from the telnet client

  while(newHour < 0 || newHour > 23)
{
  server.print(F("Enter new hour: "));
  newHour = intFromTelnet(carriageReturn);

  //VALIDATE DATA
  if(newHour >= 0 && newHour <= 23)
  {
      //store the setpoint in EEPROM
    if(EEPROMLocation >= 0 && EEPROMLocation <= 4095)
    {
      EEPROM.write(EEPROMLocation, newHour); //divide it by 10 because it's stored as a single byte value 0-255
//      newHour = EEPROM.read(EEPROMLocation); //Debug line to make sure EEPROM write is accurate
    }

    return newHour;
  }

  server.print(F("Valid entry is 0 - 23, try again: "));

}
}

int programThermostatMinute(int EEPROMLocation)// get a minute from the telnet client and return it as an integer
{
  int newMinute = -1;
  flushTelnet(); // clear any inputs from the telnet client
  while(newMinute < 0 || newMinute > 23)
{
  server.print(F("Enter new minute: "));
  newMinute = intFromTelnet(carriageReturn);

  //VALIDATE DATA
  if(newMinute >= 0 && newMinute <= 59)
  {
        //store the setpoint in EEPROM
    if(EEPROMLocation >= 0 && EEPROMLocation <= 4095)
    {
      EEPROM.write(EEPROMLocation, newMinute); //divide it by 10 because it's stored as a single byte value 0-255
//      newMinute = EEPROM.read(EEPROMLocation); //Debugging line to make sure EEPROM store is accurate
    }
      return newMinute;
  }


  server.print(F("Valid entry is 0 - 59, try again: "));

}
}

float programThermostatSetPoint(int EEPROMLocation) // get a floating point temperature from the telnet client and return it as an integer
{
  float newTemperature = -1;
  flushTelnet(); // clear any inputs from the telnet client
  while(newTemperature < tempMinimumAllowed || newTemperature > tempMaximumAllowed)
{
  server.print(F("Enter new Temperature in Deg C: "));
  newTemperature = floatFromTelnet(carriageReturn);

  //VALIDATE DATA
  if(newTemperature >= tempMinimumAllowed && newTemperature <= tempMaximumAllowed)
  {
  //store the setpoint in EEPROM
  if(EEPROMLocation >= 0 && EEPROMLocation <= 4095)
  {
    EEPROM.write(EEPROMLocation, (newTemperature * 10)); //divide it by 10 because it's stored as a single byte value 0-255
  }

 //START OF DEBUGGING CODE
 /* newTemperature = EEPROM.read(EEPROMLocation); // restore the value just written into EEPROM and return it
  newTemperature = newTemperature / 10;
   */
    return newTemperature;
  }

  server.print(F("Valid entry is "));
  server.print(tempMinimumAllowed);
  server.print(F(" to "));
  server.print(tempMaximumAllowed);
  server.print(F(" deg C, try again: "));
}
}

int programThermostatmsm(int EEPROMLocation_hour, int EEPROMLocation_minute) // get an hour from the telnet client and return it as an integer
{
  int newHour = -1;
  int newMinute = -1;

  flushTelnet(); //clear any inputs from the telnet client

  while(newHour < 0 || newHour > 23)
  {
    server.print(F("Enter new hour: "));
    newHour = intFromTelnet(carriageReturn);

    //VALIDATE DATA
    if(newHour >= 0 && newHour <= 23)
    {
        //store the setpoint in EEPROM
      if(EEPROMLocation_hour >= 0 && EEPROMLocation_hour <= 4095)
      {
        EEPROM.write(EEPROMLocation_hour, newHour); //divide it by 10 because it's stored as a single byte value 0-255
      }
      break;
    }

    server.print(F("Valid entry is 0 - 23, try again: "));
  }

  while(newMinute < 0 || newMinute > 23)
  {
    server.print(F("Enter new minute: "));
    newMinute = intFromTelnet(carriageReturn);

    //VALIDATE DATA
    if(newMinute >= 0 && newMinute <= 59)
    {
          //store the setpoint in EEPROM
      if(EEPROMLocation_minute >= 0 && EEPROMLocation_minute <= 4095)
      {
        EEPROM.write(EEPROMLocation_minute, newMinute); //divide it by 10 because it's stored as a single byte value 0-255
  //      newMinute = EEPROM.read(EEPROMLocation); //Debugging line to make sure EEPROM store is accurate
      }
        break;
    }


  server.print(F("Valid entry is 0 - 59, try again: "));

}
  return minutesSinceMidnight(newHour, newMinute);
}



void   programThermostatDisplayLine(int periodNumber)
{
  server.print(F(" Period "));
  server.print(periodNumber);
  server.print(F(" Start Time "));
  return;
}

void   programThermostatDisplayHour(int hourToDisplay)
{
  if(hourToDisplay < 10)
    server.print(F("0"));

  server.print(hourToDisplay);
  server.print(F(":"));
  return;
}


void   programThermostatDisplayMinute(int minuteToDisplay)
{
  if(minuteToDisplay < 10)
    server.print(F("0"));

  server.print(minuteToDisplay);
  server.print(F("h"));
  return;
}

void   programThermostatDisplaySetPoint(float setPointToDisplay)
{
  server.print(F(" - Set Point: "));
  server.print(setPointToDisplay);
  server.print(F("' C"));
  server.write(newLine);
  server.write(carriageReturn);

  return;
}


void   programThermostatDisplaySettings()
{
  server.print(F("Weekday Program"));
  server.write(newLine);
  server.write(carriageReturn);
  server.print(F("a)"));
  programThermostatDisplayLine(1);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekdayTimePeriod1msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekdayTimePeriod1msmStart));
  programThermostatDisplaySetPoint(thermostatWeekdayTimePeriod1SetPoint);
  server.print(F("b)"));
  programThermostatDisplayLine(2);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekdayTimePeriod2msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekdayTimePeriod2msmStart));
  programThermostatDisplaySetPoint(thermostatWeekdayTimePeriod2SetPoint);
  server.print(F("c)"));
  programThermostatDisplayLine(3);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekdayTimePeriod3msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekdayTimePeriod3msmStart));
  programThermostatDisplaySetPoint(thermostatWeekdayTimePeriod3SetPoint);

  server.print(F("d)"));
  programThermostatDisplayLine(4);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekdayTimePeriod4msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekdayTimePeriod4msmStart));
  programThermostatDisplaySetPoint(thermostatWeekdayTimePeriod4SetPoint);
  server.write(newLine);
  server.write(carriageReturn);


  server.print(F("Weekend Program"));
  server.write(newLine);
  server.write(carriageReturn);
  server.print(F("e)"));
  programThermostatDisplayLine(1);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekendTimePeriod1msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekendTimePeriod1msmStart));
  programThermostatDisplaySetPoint(thermostatWeekendTimePeriod1SetPoint);
  server.print(F("f)"));
  programThermostatDisplayLine(2);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekendTimePeriod2msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekendTimePeriod2msmStart));
  programThermostatDisplaySetPoint(thermostatWeekendTimePeriod2SetPoint);
  server.print(F("g)"));
  programThermostatDisplayLine(3);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekendTimePeriod3msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekendTimePeriod3msmStart));
  programThermostatDisplaySetPoint(thermostatWeekendTimePeriod3SetPoint);

  server.print(F("h)"));
  programThermostatDisplayLine(4);
  programThermostatDisplayHour(hoursFrommsm(thermostatWeekendTimePeriod4msmStart));
  programThermostatDisplayMinute(minutesFrommsm(thermostatWeekendTimePeriod4msmStart));
  programThermostatDisplaySetPoint(thermostatWeekendTimePeriod4SetPoint);
  server.write(newLine);
  server.write(carriageReturn);
  return;
}


void programThermostatRestoreFromEEPROM()
{

  //make sure that the EEPROM doesn't have any blank values in the range where the
  //thermostat settings should be stored
  bedroomHeaterAutoOffHour = EEPROM.read(EEPROM_thermostatWeekdayTimePeriod1HourStart);
  blockHeaterOnHour = bedroomHeaterAutoOffHour - 2;
  blockHeaterOffHour = bedroomHeaterAutoOffHour + 4;
  
  thermostatWeekdayTimePeriod1msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekdayTimePeriod1HourStart), EEPROM.read(EEPROM_thermostatWeekdayTimePeriod1MinuteStart));
  thermostatWeekdayTimePeriod1SetPoint = EEPROM.read(EEPROM_thermostatWeekdayTimePeriod1SetPoint);
  thermostatWeekdayTimePeriod1SetPoint = thermostatWeekdayTimePeriod1SetPoint / 10;

  thermostatWeekdayTimePeriod2msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekdayTimePeriod2HourStart), EEPROM.read(EEPROM_thermostatWeekdayTimePeriod2MinuteStart));
  thermostatWeekdayTimePeriod2SetPoint = EEPROM.read(EEPROM_thermostatWeekdayTimePeriod2SetPoint);
  thermostatWeekdayTimePeriod2SetPoint = thermostatWeekdayTimePeriod2SetPoint / 10;

  thermostatWeekdayTimePeriod3msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekdayTimePeriod3HourStart), EEPROM.read(EEPROM_thermostatWeekdayTimePeriod3MinuteStart));
  thermostatWeekdayTimePeriod3SetPoint = EEPROM.read(EEPROM_thermostatWeekdayTimePeriod3SetPoint);
  thermostatWeekdayTimePeriod3SetPoint = thermostatWeekdayTimePeriod3SetPoint / 10;

  thermostatWeekdayTimePeriod4msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekdayTimePeriod4HourStart), EEPROM.read(EEPROM_thermostatWeekdayTimePeriod4MinuteStart));
  thermostatWeekdayTimePeriod4SetPoint = EEPROM.read(EEPROM_thermostatWeekdayTimePeriod4SetPoint);
  thermostatWeekdayTimePeriod4SetPoint = thermostatWeekdayTimePeriod4SetPoint / 10;

  thermostatWeekendTimePeriod1msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekendTimePeriod1HourStart), EEPROM.read(EEPROM_thermostatWeekendTimePeriod1MinuteStart));
  thermostatWeekendTimePeriod1SetPoint = EEPROM.read(EEPROM_thermostatWeekendTimePeriod1SetPoint);
  thermostatWeekendTimePeriod1SetPoint = thermostatWeekendTimePeriod1SetPoint / 10;

  thermostatWeekendTimePeriod2msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekendTimePeriod2HourStart), EEPROM.read(EEPROM_thermostatWeekendTimePeriod2MinuteStart));
  thermostatWeekendTimePeriod2SetPoint = EEPROM.read(EEPROM_thermostatWeekendTimePeriod2SetPoint);
  thermostatWeekendTimePeriod2SetPoint = thermostatWeekendTimePeriod2SetPoint / 10;

  thermostatWeekendTimePeriod3msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekendTimePeriod3HourStart), EEPROM.read(EEPROM_thermostatWeekendTimePeriod3MinuteStart));
  thermostatWeekendTimePeriod3SetPoint = EEPROM.read(EEPROM_thermostatWeekendTimePeriod3SetPoint);
  thermostatWeekendTimePeriod3SetPoint = thermostatWeekendTimePeriod3SetPoint / 10;

  thermostatWeekendTimePeriod4msmStart = minutesSinceMidnight(EEPROM.read(EEPROM_thermostatWeekendTimePeriod4HourStart), EEPROM.read(EEPROM_thermostatWeekendTimePeriod4MinuteStart));
  thermostatWeekendTimePeriod4SetPoint = EEPROM.read(EEPROM_thermostatWeekendTimePeriod4SetPoint);
  thermostatWeekendTimePeriod4SetPoint = thermostatWeekendTimePeriod4SetPoint / 10;

  maintainTemperature = EEPROM.read(EEPROM_maintainTemperature);


 return;
}

//This function uploads all the data to the Plot.ly server
void sendToPlotly()
{
    #ifdef plotlyOn
    if(garageDoorStatus != -1111)
      graph.plot(millis(), garageDoorStatus, tokens[0]);
    
    graph.plot(millis(), tempMainFloor, tokens[1]);
    graph.plot(millis(), tempSetPoint, tokens[2]);
    graph.plot(millis(), maintainTemperature, tokens[3]);    
    graph.plot(millis(), furnaceStatus, tokens[4]);
    graph.plot(millis(), ventFanForceOn, tokens[5]);
    graph.plot(millis(), ventFanAutoEnabled, tokens[6]);
    graph.plot(millis(), ventFanStatus, tokens[7]);   
    graph.plot(millis(), backBedroomTemperature, tokens[9]);
    graph.plot(millis(), masterBedroomTemperature, tokens[10]);    
    graph.plot(millis(), masterBedroomTemperatureSetPoint, tokens[11]);
    graph.plot(millis(), bedroomMaintainTemp, tokens[12]);
    graph.plot(millis(), bedroomHeaterStatus, tokens[13]);
    graph.plot(millis(), basementTempAmbient, tokens[14]);
    if(garageTempAmbient != -1111)
      graph.plot(millis(), garageTempAmbient, tokens[15]);
    if(garageTempOutdoor != -1111)
      graph.plot(millis(), garageTempOutdoor, tokens[16]);
    graph.plot(millis(), blockHeaterEnabled, tokens[17]);    
    graph.plot(millis(), blockHeaterStatus, tokens[18]);
    #endif
}




//This function saves all the data to an SD card in a CSV file format
void saveToSDCard()
{
  if(lastLogMinute == now.minute() && garageDoorStatus == 1)
  {
    return; // don't log anything if it's less than a minute since last event, unless the garage door is opened.
  }
  lastLogMinute = now.minute();

  File dataFile = SD.open("datalog.csv", FILE_WRITE);

  if(!dataFile)
  {
    SDCardLog = false;
    Serial.println("No SD Card");
    return;
  }


  //make a string for assembling the data to log
  String dataString = "";
  dataString += now.year();
    dataString += "-";
  if(now.month() < 10)
    dataString += "0";

  dataString += now.month();
    dataString += "-";
  if(now.day() < 10)
    dataString += "0";

  dataString += now.day();
  dataString += " ";
    if(now.hour() < 10)
    dataString += "0";
  dataString += now.hour();
    dataString += ":";
      if(now.minute() < 10)
    dataString += "0";
  dataString += now.minute();
    dataString += ":";
      if(now.second() < 10)
    dataString += "0";
  dataString += now.second();
  dataString += ",";
  //add data variables here
  dataString += String(garageDoorStatus);
  dataString += ",";
  dataString += String(tempMainFloor);
  dataString += ",";
  dataString += String(tempSetPoint);
  dataString += ",";
  dataString += String(maintainTemperature);
  dataString += ",";
  dataString += String(furnaceStatus);
  dataString += ",";
  dataString += String(ventFanForceOn);
  dataString += ",";
  dataString += String(ventFanAutoEnabled);
  dataString += ",";
  dataString += String(ventFanStatus);
  dataString += ",";
  dataString += String(backBedroomTemperature);
  dataString += ",";
  dataString += String(masterBedroomTemperature);
  dataString += ",";
  dataString += String(masterBedroomTemperatureSetPoint);
  dataString += ",";
  dataString += String(bedroomMaintainTemp);
  dataString += ",";
  dataString += String(bedroomHeaterStatus);
  dataString += ",";
  dataString += String(basementTempAmbient);
  dataString += ",";
  dataString += String(garageTempAmbient);
  dataString += ",";
  dataString += String(garageTempOutdoor);
  dataString += ",";
  dataString += String(blockHeaterEnabled);
  dataString += ",";
  dataString += String(blockHeaterStatus);
  dataString += ",";
  dataString += String(validPassword);



  //End of data variables

  dataFile.println(dataString);
  Serial.println(dataString);
  dataFile.close();

}

void writeHeaderToSDCard()
{
  File dataFile = SD.open("datalog.csv", FILE_WRITE);

  if(!dataFile)
  {
    SDCardLog = false;
    Serial.println("No SD Card");
    return;
  }


  //make a string for assembling the data to log
  String dataString = "";
  dataString += String("Date");
  dataString += " ";
  dataString += String("Time");
  dataString += ",";
  //add data variables here
  dataString += String("garageDoorStatus");
  dataString += ",";
  dataString += String("tempMainFloor");
  dataString += ",";
  dataString += String("tempSetPoint");
  dataString += ",";
  dataString += String("maintainTemperature");
  dataString += ",";
  dataString += String("furnaceStatus");
  dataString += ",";
  dataString += String("ventFanForceOn");
  dataString += ",";
  dataString += String("ventFanAutoEnabled");
  dataString += ",";
  dataString += String("ventFanStatus");
  dataString += ",";
  dataString += String("backBedroomTemperature");
  dataString += ",";
  dataString += String("masterBedroomTemperature");
  dataString += ",";
  dataString += String("masterBedroomTemperatureSetPoint");
  dataString += ",";
  dataString += String("bedroomMaintainTemp");
  dataString += ",";
  dataString += String("bedroomHeaterStatus");
  dataString += ",";
  dataString += String("basementTempAmbient");
  dataString += ",";
  dataString += String("garageTempAmbient");
  dataString += ",";
  dataString += String("garageTempOutdoor");
  dataString += ",";
  dataString += String("blockHeaterEnabled");
  dataString += ",";
  dataString += String("blockHeaterStatus");
  dataString += ",";
  dataString += String("validPassword");
  dataString += ",";
  dataString += String("FWversion:");
  dataString += String(FWversion);
  //End of data variables
  dataFile.println(dataString);
  Serial.println(dataString);
  dataFile.close();

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


/* floatFromTelnet()
	This function reads the telnet input and returns a floating point
	value determined as:
		The first ASCII character 0-9 on the serial port
		until the next delimiter character is received as a delimiter
	Other non-numeric / '.' / 'delimiter'  are ignored

	There is a timeout function of 1,000,000 loops - about 12 seconds
	This can be made shorter, but in my application, the nodes can sometimes reply very slowly
	so I keep this long to avoid missing data

        As input, this function takes an integer value indicating the delimiter
        to search for as the end of the floating point value

        If no serial input is available when this function is called, it will return -2222
*/
float floatFromTelnet(int delimiter)
{
	//local variables
	float inputFloat = 0; //setup a variable to store the float
	float isDecimal = 0;  //keep track of how many times to divide by 10
		              // when processing decimal inputs
	int inputByte;	      //store the byte read from the serial port
	double loopCounter = 0;

    bool isNegative = false;


	while (1)//Do this loop until it is broken out of by the delimiter
	{

     	if(loopCounter > 1000000)
     	{
     		return -1111; // Timeout error
     	}

        inputByte = client.read();
        if(inputByte == '-')
        {
        	isNegative = true;
        }
		if(inputByte == delimiter) //delimiter for end of float
		{
			break;//break out of the whole loop
		}

		if(inputByte == '.') //detect decimal portion
		{
			if(isDecimal == 0) // only do this the first time
				isDecimal = 10.0; //divide the next input by 10

		}

		if(inputByte > 47 && inputByte < 58) //filter numbers 0-9 only
		{
			//multiply by 10 and add the next digit (subtract 48 to convert
			//ASCII char to matching integer 0-9


			if(isDecimal == 0)
			{
				inputFloat = (inputFloat * 10) + (inputByte - 48);

			}

			else
			{
				inputFloat = (inputFloat) + ((inputByte -48) / isDecimal);
				isDecimal = isDecimal * (10); // divide next one by 10x more

			}
		}
		loopCounter++;
	}//End of while loop
	if(isNegative)// if it's negative, multiply by -1
	{
		inputFloat = inputFloat * -1.0;
	}
	return inputFloat;
}



int intFromTelnet(int delimiter)
{
	//local variables
	int inputInt = 0; //setup a variable to store the float
    int inputByte;	      //store the byte read from the serial port
    bool isNegative;
    double loopCounter = 0;


	while (1)//Do this loop until it is broken out of by the delimiter
	{

		if(loopCounter > 1000000)
     	{
     		return -1111; // Timeout error
     	}


        inputByte = client.read();
		if(inputByte == delimiter) //delimiter for end of float
		{
			break;//break out of the whole loop
		}

		if(inputByte > 47 && inputByte < 58) //filter numbers 0-9 only
		{
			//multiply by 10 and add the next digit (subtract 48 to convert
			//ASCII char to matching integer 0-9
			inputInt = (inputInt * 10) + (inputByte - 48);
		}
		loopCounter++;
	}//end of while loop

	if(isNegative)// if it's negative, multiply by -1
	{
		inputInt = inputInt * -1.0;
	}
	return inputInt;
}

void flushTelnet()
{
    while(client.available())
    {
      inputChar = client.read();// read in the data but don't do anything with it.
      timeOfLastInput = millis();//set the time of last Input to NOW
    }
}

int minutesSinceMidnight(int hours, int minutes)
{
 int msm;
 msm = minutes;
 msm += (hours * 60);
 return msm;
}

int hoursFrommsm(int msm)
{
 return msm / 60;
}

int minutesFrommsm(int msm)
{
 return msm % 60;
}

void telnetSerialRelay(int escapeChar)
{
  while(1)
  {
//read telnet and output to serial
  if(client.available())
  {
    inputChar = client.read();// read in the data
    if(inputChar == escapeChar) //see if it's the escape character
    {
      return; //leave the function if it's the escape character
    }
     timeOfLastInput = millis();//set the time of last Input to NOW
     
     Serial1.print(inputChar);//relay the telnet input to the serial port
  }
  
  if(Serial1.available()) // see if there is data waiting on the serial port
  {
    inputChar = Serial1.read(); //read in the serial data
    server.write(inputChar); //relay the serial data to the telnet client
  }
 
        if(didClientTimeout())
      {
        server.print(F("Session Timeout - Disconnecting"));
        server.write(newLine);
        server.write(carriageReturn);
        client.stop();
        password.reset();
        timeOfLastInput = millis();
        return;
      }
  }
}


//Read data from the serial port (XBEE) and put the received data into the correct variables
void readSerial()
{
	if(Serial1.available() > 0)
	{
		inputChar = Serial1.read();//read in 1 byte
		switch (inputChar)
                {
		case garage_doorStatus:
			garageDoorStatus = boolFromSerial1();
                        //to-do: detect change in garage door status from last value, if it changed, log this immediately to the telnet client and the SD card
		break;

		case garage_tempZone1:
			garageTempOutdoor = floatFromSerial1('!');
		break;

		case garage_tempZone2:
			garageTempAmbient = floatFromSerial1('!');
		break;

		case garage_autoCloseEnabled:
			garageAutoCloseStatus = boolFromSerial1();
		break;

                case garage_error:
                        garageDoorError = boolFromSerial1();
                break;
                
		case bsmt_tempZone1:
			basementTempAmbient = floatFromSerial1('!');
		break;

		case bsmt_tempZone2:
			backBedroomTemperature = floatFromSerial1('!');
		break;

		case bsmt_tempZone3:
			livingRoomTemperature = floatFromSerial1('!');
		break;

		case bdrm_temp:
			masterBedroomTemperature = floatFromSerial1('!');
		break;

		case bdrm_outlet1Status:
			bedroomHeaterStatus = boolFromSerial1();
		break;

		default: //if none of the case conditions is met
                    // do nothing
	        break;
                }

	}


}

