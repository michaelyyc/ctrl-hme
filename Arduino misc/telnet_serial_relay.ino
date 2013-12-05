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
    
    for(i = 0; i < 25; i++)
    {
      inputChar = client.read();//Ignore 26 bytes of data sent by telnet clients upon connecting

    }

    
    while(client == true)//don't ask for the password as long as the same client is connected
      {
      // read bytes from the incoming client and write them back
      // to any clients connected to the server:
      if(client.available());
      {
        inputChar = client.read();
        Serial.print(inputChar);//sent out to serial whatever telnet client has sent;
      }
    }
        if(Serial.available())
      {
        inputChar = Serial.read();
        server.write(inputChar);//output everything from serial to the telnet client
      }
  }
}

