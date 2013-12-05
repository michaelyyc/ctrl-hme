#include <SPI.h>

#include <Ethernet.h>


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

// telnet defaults to port 23
EthernetServer server = EthernetServer(23);

void setup()
{
  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);
  Serial.begin(9600);
  // start listening for clients
  server.begin();
}

void loop()
{
  // if an incoming client connects, there will be bytes available to read:
  EthernetClient client = server.available();
  if (client == true) {
 //   Serial.println("Client connected...");
 //   Serial.println("ignoring 26 bytes..."); //Found that the Apple telnet client sends 26 bytes of data once connection is established.
                                            // same thing happens with iTelnet client for iPhone, probably a standard Telnet protocol thing
                                            // Client trying to negotiate connecrtion? This sketch just ignores the first 26 bytes
 
   for(i = 0; i < 26; i++)
   {
      inputChar = client.read();// read in the data but don't do anything with it.
   }
 //  server.write("Hello Client");
   
   
   while(client == true)
   {
     if(client.available())
     {
       inputChar = client.read();
       if(inputChar > 47 && inputChar < 123)//ignore anything not alpha-numeric
       {
         Serial.print(inputChar);
       }
     }
     
     if(Serial.available())
     {
       inputChar = Serial.read();//read the serial input
       server.write(inputChar);//output the serial input to the telnet client
     }
   }  
    
  }
}

