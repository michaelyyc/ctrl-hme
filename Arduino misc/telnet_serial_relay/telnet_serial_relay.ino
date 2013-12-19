#include <SPI.h>
#include <Ethernet.h>
#include <DallasTemperature.h>
#include <OneWire.h>


//Constants
#define oneWireBus1     7 // Temperature Sensor
#define loopsPerSecond  40000 //used for calculating loops for periodic updates
#define baud            9600 // serial port baud rate
#define FWversion       0.11 // FW version

//Temporary commands for using local sensors instead of telnet relay
  #define requestFWVer              75 //K
  #define requestTemp               76 //L
  #define enableFurnace             77 //M
  #define disableFurnace            78 //N
  #define listCommands              63 // ?
  #define newLine                   12 // NEW LINE
  #define carriageReturn            13 


// network configuration.  gateway and subnet are optional.
 // the media access control (ethernet hardware) address for the shield:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
//the IP address for the shield:
byte ip[] = { 192, 168, 1, 230 };    
// the router's gateway address:
byte gateway[] = { 10, 0, 0, 1 };
// the subnet:
byte subnet[] = { 255, 255, 0, 0 };
char inputChar;
int i = 0;
int updateCounter = 0;
float tempAmbient;
OneWire oneWire1(oneWireBus1);  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors1(&oneWire1);  // Pass our oneWire reference to Dallas Temperature. 
int tempUpdateDelay = 30;
double tempUpdateCountdown;
char * convertedFloat;

// telnet defaults to port 23
EthernetServer server = EthernetServer(23);

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

  // if an incoming client connects, there will be bytes available to read:
 
  EthernetClient client = server.available();
  if (client == true) {
 //   Serial.println("Client connected...");
 //   Serial.println("ignoring 26 bytes..."); 
 
 
 //Found that the Apple telnet client sends 26 bytes of data once connection is established.
 // same thing happens with iTelnet client for iPhone, probably a standard Telnet protocol thing
 // Client trying to negotiate connecrtion? This sketch just ignores the first 26 bytes
   for(i = 0; i < 24; i++)
   {
      inputChar = client.read();// read in the data but don't do anything with it.
   }
 //  server.write("Hello Client");
   server.write("Welcome! Type '?' for a list of commands");
   server.write(newLine);
   server.write(carriageReturn);
   
   while(client.connected())
   {
     if(client.available())
     {
       inputChar = client.read();
       if(inputChar > 47 && inputChar < 123 && inputChar != 51)//ignore anything not alpha-numeric
       {
         Serial.print(inputChar);
         server.write(newLine);
         server.write(carriageReturn);
       }
          //disabled door opening and closing for security reasons until authentication features added
       if(inputChar == 51)
       {
         server.write("Door open/close locked");
       }
       
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
         server.write("furnace control not implemented yet");
         server.write(newLine);//new line
         server.write(carriageReturn);
       }
       
       if(inputChar == disableFurnace)
       {
         server.write("furnace control not implemented yet");
         server.write(newLine);//new line
         server.write(carriageReturn);
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
         server.write("C : Enable Furnace - maintain temperature");
         server.write(newLine);
         server.write(carriageReturn);
         server.write("D : Disable Furnace");   
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
         server.write("7 : Turn on Block Heater");  
         
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
         
       }       
     }

     if(Serial.available())
     {
       inputChar = Serial.read();//read the serial input
       server.write(inputChar);//output the serial input to the telnet client
     }
   tempUpdateCountdown--; //decrement the periodic update timer
   if(tempUpdateCountdown < 1)
   {
     tempAmbient = readAmbientTemp();
     tempUpdateCountdown = tempUpdateDelay * loopsPerSecond; //reset the countdown
     server.write("Upstairs Temperature : ");
     server.write(tempAmbient);
     server.write(" deg C ");
   }  
     
   }  
  }
   
   //See if it's time to update the periodic sensors
   tempUpdateCountdown--; //decrement the periodic update timer
   if(tempUpdateCountdown < 1)
   {
     tempAmbient = readAmbientTemp();
     tempUpdateCountdown = tempUpdateDelay * loopsPerSecond; //reset the countdown
   }  
}

float readAmbientTemp()
{
    float temp;
    sensors1.requestTemperatures();
    temp = sensors1.getTempCByIndex(0);
    return temp;
}

