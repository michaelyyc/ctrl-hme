#include <DallasTemperature.h>
#include <OneWire.h>

/*
Basement temperature sensor and furnace control

Respond to requests from the controller which are received via serial input
It can be used with a wired or wireless serial connection, intended to be used with XBEE radio

The temperature sensor is DS18B20

Furnace and fan can both be controlled by remote command

*/

/*
//Commands from the controller
  #define requestFWVer            32
  #define requestFurnaceStatus    33
  #define turnFurnaceOn           34
  #define turnFurnaceOff          35
  #define turnFanOn               36
  #define turnFurnaceAndFanOff    37
  #define requestTemp             38
  #define requestHumidity         39
  #define requestMoistureStatus   40
  #define requestCPUTemp          43
  */
  
  
  
//Alternate commands for 0-9, a-z in ASCII for debugging with a keyboard
  #define requestFWVer            65 //A
  #define requestFurnaceStatus    66 //B
  #define turnFurnaceOn           67 //C
  #define turnFurnaceOff          68 //D
  #define turnFanOn               69 //E
  #define turnFurnaceAndFanOff    70 //F
  #define requestTemp             71 //G
  #define requestHumidity         72 //H
  #define requestMoistureStatus   73 //I
  #define requestCPUTemp                 74 //J
  

  //I/O pin definitions
  #define furnaceControlRelay         3 //door sensor connection
  #define fanControlRelay             4 //status indicator light on board
  #define oneWireBus1                 2 // temperature sensor
  #define statusLight                13 // Status light on PCB
  
  //Constants
  #define FWversion             0.1
  #define baud                 9600
  #define loopsPerSecond      40000 //used in calculating loops for periodic updates

  //Variables
  bool furnaceStatus = 0;// 1 = closed, 0 = open
  bool fanStatus = 0; //if repeated attempts to close door don't work, this bit is set and no more attempts are made
  int i;  //for loops
  int i2; //for loops
  int commandReq = 0; //variable to store the request from the controller
  float tempAmbient = -1; //temperature from Zone 1 in degrees C
  float CPUTemp = -1;
  bool lightStatus;
  double tempUpdateDelay = 30; //how many seconds (approx) between updates of the temperature sensors
  double tempUpdateCountdown;

  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
 OneWire oneWire1(oneWireBus1);
  // Pass our oneWire reference to Dallas Temperature. 
  DallasTemperature sensors1(&oneWire1);
  
  // the setup routine runs once when the node is powered on
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(baud);
  pinMode(furnaceControlRelay, OUTPUT);//ACTIVE LOW pin connected to relay that turns on the furnace
  pinMode(fanControlRelay, OUTPUT); //ACTIVE LOW pin connected to relay that turns on the fan
  pinMode(statusLight, OUTPUT);//status light on board
 
  //Start with the furnace and fan off (active low)
  digitalWrite(fanControlRelay, HIGH);
  digitalWrite(furnaceControlRelay, HIGH);
  sensors1.begin(); // Start up the library for one-wire bus 1
}


//Main loop  
void loop() {
  
  
  //Check for requests
  checkRequests();

  //See if it's time to update the sensors that only update periodically
  if(tempUpdateCountdown == 0) //don't update the temperature sensors every loop because it take too long
  {
//set the status light on while checking
    digitalWrite(statusLight, HIGH);
    sensors1.requestTemperatures();
    tempAmbient = sensors1.getTempCByIndex(0);
    tempUpdateCountdown = (tempUpdateDelay * loopsPerSecond) + 1;
    CPUTemp = getCPUTemp();
// Turn off the status light
    digitalWrite(statusLight, LOW);

  }
  
  //decrement the periodic sensor update counter
  tempUpdateCountdown--;
  
}

 


double getCPUTemp(void)
{
//taken from: http://playground.arduino.cc/Main/InternalTemperatureSensor

  unsigned int wADC;
  double t;

  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC

  delay(20);            // wait for voltages to become stable.

  ADCSRA |= _BV(ADSC);  // Start the ADC

  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));

  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;

  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 288 ) / 1.22;

  // The returned temperature is in degrees Celcius.
  return (t);
}

     
void checkRequests()
{
    //Check for waiting requests
  if (Serial.available() > 0)
  {
    commandReq = Serial.read();
    //See if the command is for this node
    if(commandReq < 32)
    {
     commandReply(); 
    }
    //Leave this line for debugging, call the commandReply() function for any incomming serial command
     commandReply();
  }  
  return;
}

     
void commandReply()
{
  if(commandReq == requestFWVer) //command is to request FW version
      {
        Serial.print("Bv"); //send command reply prefix
        Serial.print(FWversion); //send command reply data
        return; 
      }

  if(commandReq == turnFurnaceOn)
  {
      Serial.print("BF1");
      digitalWrite(furnaceControlRelay, LOW);
      return;
      
  }
  
  if(commandReq == turnFurnaceOff)
  {
    Serial.print("Bf1");
    digitalWrite(furnaceControlRelay, HIGH);
    return;
  }
  
  if(commandReq == turnFanOn)
  {
    Serial.print("BV1");
    digitalWrite(fanControlRelay, LOW);
    return;
  }
  
  if(commandReq == turnFurnaceAndFanOff)
  {
    Serial.print("Bv1");
    digitalWrite(furnaceControlRelay, HIGH);
    digitalWrite(fanControlRelay, HIGH);
    return;
  }
  
  if(commandReq == requestTemp)
  {
    Serial.print("BT");
    Serial.print(tempAmbient);
    Serial.print("!");
    return;
  }
  
  if(commandReq == requestCPUTemp)
  {
    Serial.print("Bt");
    Serial.print(CPUTemp);
    Serial.print("!");
    return;
  }

  
  Serial.print("!");//reply for unimplemented commands
      return;
  
}




