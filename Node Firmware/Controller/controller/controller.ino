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


//Constants
	#define oneWireBus1                   7 // Temperature Sensor
	#define baud                       9600 // serial port baud rate
	#define FWversion                  0.17 // FW version
	#define tempMaximumAllowed         23.0// maximum temperature
	#define tempMinimumAllowed         17.0 //minimum temperature

//Commands for using local sensors instead of telnet relay
  	#define requestFWVer              75 //K
  	#define requestTemp               76 //L
 	#define enableFurnace             77 //M
  	#define disableFurnace            78 //N
  	#define increaseTempSetPoint      79 //O
  	#define decreaseTempSetPoint      80 //P
  	#define listCommands              63 // ?
  	#define logOff                   120 // x
  
  	#define newLine                   10 // ASCII NEW LINE
  	#define carriageReturn            13 // ASCII Carriage return


/* NETWORK CONFIGURATION
These variables are required to configure the Ethernet shield
The shield must be assigned a local static IP address that doesn't conflict
on the network. It DOES NOT have DHCP capabilities for Dynamic IP 

To reach this server from outside the local network, port forwarding
or VPN must be setup on the router
*/

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  //MAC Address
byte ip[] = { 192, 168, 1, 230 };   // IP Address
byte gateway[] = { 10, 0, 0, 1 }; //Gateway (optional, not used)
byte subnet[] = { 255, 255, 0, 0 }; //Subnet

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
float tempAmbient; //value used to store ambient temperature as measured by 1-wire sensor
double tempUpdateCountdown; //A countdown is used to update the temperature value periodically, not every loop
double tempUpdateDelay = 30; //Roughly the number of seconds between temp updates
double loopsPerSecond = 15000; //Roughly the number of times the main loop is processed each second
float tempSetPoint = 21.5; //The setPoint temperature
float tempHysterisis = 0.25; //The values applied as +/- to the set point to avoid chattering the furnace control
bool maintainTemperature = false; //Furnace ON/Off commands are only sent if this is true
bool furnaceStatus = false; //This is true if Furnace ON commands are being sent
bool validPassword = false; //This is true any time a valid user is logged in
Password password = Password( "1234" ); //The password is hardcoded. Change this value before compiling


/* SETUP - RUN ONCE
	Setup function initializes some variables and sets up the serial and ethernet ports
	this takes about 1 second to run
*/
void setup()
{
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
  validPassword = false;
  
  // if an incoming client connects, there will be bytes available to read:
 
  EthernetClient client = server.available();
  if (client == true) 
  {
 
 //Found that the some telnet clients send 24-26 bytes of data once connection is established.
 // Client trying to negotiate connecrtion? This sketch just ignores the first 250ms of data
  	delay(250); //wait 250ms for incoming data from the client
   while(client.available())
   	{
        inputChar = client.read();// read in the data but don't do anything with it.
	}
  

 // Prompt for password and compare to the actual value
 // The user must terminate password with '?' 
 // Typing errors can be cleared by sending '!'
 // If the password is wrong, the client will be disconnected and server will wait
 // for 30 seconds
   server.write("Enter password: ");
   while(!validPassword && client.connected())
   {
     
     if(client.available())
     {
       inputChar = client.read();
       switch (inputChar){
      case '!': //reset password
        password.reset();
       //  server.write("\tPassword is reset!");
      break;
      case '?': //evaluate password
        if (password.evaluate())
        {
          server.write("Logged in successfully");
          server.write(newLine);
          server.write(carriageReturn);
          password.reset();
          validPassword = true;

        }
        else
        {
          server.write("Login Failed - Disconnecting");
          client.stop();
          password.reset();
          delay(30000); //Wait a minute before allowing any more connections
      }
      break;
      default: //append any ascii input characters that are not a '!' nor a '?' to the currently entered password.
        if(inputChar > 47 && inputChar < 123)
          {
          password << inputChar;
          }
    }
	}
     
   }
 
 
 	//WELCOME MESSAGE - Displayed only at start of session
   if(validPassword)
   {
     server.write("Welcome! Type '?' for a list of commands");
     server.write(newLine);
     server.write(carriageReturn);
   } 
   
   //Connected Loop - this is repeated as long as the client is connected
   //The loop relays data between the telnet client and the serial port (XBEE network)
   //Certain commands are handed by this node directly to the telnet client without any serial communication
   //Only ascii data is relayed from the telnet client to the serial port
      while(client.connected())
   {
     if(client.available()) //This is true if the telnet client has sent any data
     {
       inputChar = client.read(); //read in the client data
     
  
  // COMMANDS HANDLED BY THIS NODE  
       if(inputChar == requestFWVer)
       {
         server.write("Controller FW version: ");
         server.print(FWversion);
         server.write(newLine);//new line
         server.write(carriageReturn);
     }
       
       if(inputChar == requestTemp)
       {
         server.write("Main Floor Temperature: ");
         server.print(tempAmbient);
         server.write(" deg C");
         server.write(newLine);//new line
         server.write(carriageReturn);

       }
       
       if(inputChar == enableFurnace)
       {
         maintainTemperature = true;
         furnaceStatus = false; //set this false to require a new temperature comparison before sending first ON command to furnace
         server.write("Furnace enabled, maintain ");
         server.print(tempSetPoint);
         server.write(" deg C at upstairs sensor");
         server.write(newLine);//new line
         server.write(carriageReturn);
       }
       
       if(inputChar == disableFurnace)
       {
         maintainTemperature = false;
         furnaceStatus = false;
         Serial.print("D");     
         server.write("furnace disabled - not maintaining temperature");
         server.write(newLine);//new line
         server.write(carriageReturn);
       }
       
       if(inputChar == increaseTempSetPoint)
       {
         if(tempSetPoint < tempMaximumAllowed)
         {
           tempSetPoint = tempSetPoint + 0.25;
           server.write("Increased temp set point to: ");
           server.print(tempSetPoint);
           server.write(" deg C");
         }
         else
           server.write("Maximum Temperature already Set");
       }
       
       if(inputChar == decreaseTempSetPoint)
       {
         if(tempSetPoint > tempMinimumAllowed)
         {
           tempSetPoint = tempSetPoint - 0.25;
           server.write("Decreased temp set point to: ");
           server.print(tempSetPoint);
           server.write(" deg C");
         }
         else
           server.write("Minimum Temperature Already Reached");
       }
       
       if(inputChar == logOff)
       {
         server.write("Goodbye!   ");
         client.stop();
       }
       
       if(inputChar == listCommands)
       {
         server.write("You can use these commands....");
         server.write(newLine);
         server.write(carriageReturn);
         server.write(newLine);
         server.write("GARAGE:");
         server.write(newLine);
         server.write(carriageReturn);
         server.write("0 : Request FW Version");
         server.write(newLine);
         server.write(carriageReturn);
         server.write("1 : Request Door Status");
         server.write(newLine);
         server.write(carriageReturn);
         server.write("2 : Request Inside Temperature");
         server.write(newLine);
         server.write(carriageReturn);
         server.write("3 : Activate Door");   
         server.write(newLine);
         server.write(carriageReturn);
         server.write("4 : Request Auto Close Status");   
         server.write(newLine);
         server.write(carriageReturn);
         server.write("5 : Disable Auto Close");  
         server.write(newLine);
         server.write(carriageReturn);
         server.write("6 : Enable Auto Close");  
         server.write(newLine);
         server.write(carriageReturn);
         server.write("7 : Turn on Block Heater");  
         server.write(newLine);
         server.write(carriageReturn);
         server.write("8 : Turn off Block Heater");  
         server.write(newLine);
         server.write(carriageReturn);
         server.write("a : Request outside temperature");  
             
     
         server.write(newLine);
         server.write(carriageReturn);
         server.write(newLine);
         server.write(carriageReturn);
         server.write(newLine);
         server.write("BASEMENT:");   
         server.write(carriageReturn);
         server.write(newLine);
         server.write("A : Request FW Version");
         server.write(carriageReturn);
         server.write(newLine);
         server.write("B : Request Furnace Status");
         server.write(carriageReturn);
         server.write(newLine);
         server.write("C : Turn Furnace On");
         server.write(newLine);
         server.write(carriageReturn);
         server.write("D : Turn Furnace Off");   
         server.write(newLine);
         server.write(carriageReturn);
         server.write("E : Turn Fan On");   
         server.write(newLine);
         server.write(carriageReturn);
         server.write("F : Turn Fan and Furnace Off");  
         server.write(newLine);
         server.write(carriageReturn);
         server.write("G : Request Temperature");  
         server.write(newLine);
         server.write(carriageReturn);
         
         server.write(newLine);
         server.write(carriageReturn);
         server.write(newLine);
         server.write("MAIN FLOOR:");   
         server.write(carriageReturn);
         server.write(newLine);
         server.write("K : Request FW Version");
         server.write(carriageReturn);
         server.write(newLine);
         server.write("L : Request Temperature");
         server.write(carriageReturn);
         server.write(newLine);
         server.write("M : Maintain Temperature");
         server.write(carriageReturn);
         server.write(newLine);
         server.write("N : Do Not Maintain Temperature");
         server.write(carriageReturn);
         server.write(newLine);
         server.write("O : Increase Set Temperature"); 
         server.write(carriageReturn);
         server.write(newLine);
         server.write("P : Decrease Set Temperature");         
         server.write(carriageReturn);
         server.write(newLine);    
         server.write(carriageReturn);
         server.write(newLine);   
         server.write("x : Log off");
         server.write(carriageReturn);
         server.write(newLine); 
       }  
  
  //COMMANDS NOT HANDLED BY THIS NODE
  //RELAY THEM TO THE SERIAL PORT (XBEE NETWORK)
    if(inputChar > 47 && inputChar < 123)//ignore anything not alpha-numeric
       {
         Serial.print(inputChar);
         server.write(newLine);
         server.write(carriageReturn);
       }
  
       
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
    temp = sensors1.getTempCByIndex(0);   
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

      if(tempAmbient == -127.0 || tempAmbient == 0.0) //sensor error, disable furnace
      {
       Serial.print("D");     
       server.write("Sensor ERROR - Sent OFF command to basement node");  
       server.write(carriageReturn);
       server.write(newLine);             
      }
    
     if(maintainTemperature && tempAmbient != - 127.0 && tempAmbient != 0.0)//only act when good sensor data is present and maintainTemperature is enabled
     {       
         //turn the furnace off if necessary 
       if(tempAmbient > (tempSetPoint + tempHysterisis))
        {
           furnaceStatus = false;//prevent furnace from turning on in the next loop
           Serial.print("D");     
           server.write("sent OFF command to basement node ");  
           server.print(tempAmbient);
           server.write(" deg C");
           server.write(carriageReturn);
           server.write(newLine);  
         }  
          // turn the furnace on if necessary
        if(furnaceStatus || (tempAmbient < (tempSetPoint - tempHysterisis)))
          {
             furnaceStatus = true;//used for hysterisis when the second part of the IF condition is no longer true
             Serial.print("C");//this command must be repeated at least every 5 minutes or the furnace will automatically turn off (see firmware for basement node)
             server.write("sent ON command to basement node "); 
             server.print(tempAmbient);
             server.write(" deg C"); 
             server.write(carriageReturn);
             server.write(newLine);            
          }
                 
     }
     return;
}


