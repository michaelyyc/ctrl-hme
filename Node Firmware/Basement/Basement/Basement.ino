#include <DallasTemperature.h>
#include <OneWire.h>
#include <commandDefinitionsAlt.h>

/*
Basement temperature sensor and furnace control

Respond to requests from the controller which are received via serial input
It can be used with a wired or wireless serial connection, intended to be used with XBEE radio

The temperature sensor is DS18B20

Furnace and fan can both be controlled by remote command

*/




  //I/O pin definitions
  #define furnaceControlRelay         7 // controls heat on furnace
  #define fanControlRelay             4 // controls ventillation fan on furnace
  #define oneWireBus1                 2 // temperature sensor for zone 1 (basement)
  #define oneWireBus2                 5 // temperature sensor for zone 2 (back bedroom)
  #define oneWireBus3                 6 // temperature sensor for zone 3 (main floor)
  #define statusLight                13 // Status light on PCB
  
  //Constants
  #define FWversion            0.53
  #define baud                 9600
  #define loopsPerSecond      40000 //used in calculating loops for periodic updates
  #define tempUpdateDelay        30 //how many seconds (approx) between updates of the temperature sensors
  #define furnaceTimeout        300 //seconds to keep the furnace on if communication is lost while heating


  //Variables
  bool fanStatus = false; //if repeated attempts to close door don't work, this bit is set and no more attempts are made
  bool furnaceEnable = false; //furnace should be on when this is true
  int i;  //for loops
  int i2; //for loops
  int commandReq = 0; //variable to store the request from the controller
  float tempAmbient = -1; //temperature from Zone 1 in degrees C
  float tempZone2 = -1; // Temperature from zone 2 in degrees C
  float tempZone3 = -1; // Temperature from zone 3 in degrees C
  float CPUTemp = -1;
  bool lightStatus;
  long int tempUpdateCountdown;
  long int furnaceTimeoutCountdown;

  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
 OneWire oneWire1(oneWireBus1);
 OneWire oneWire2(oneWireBus2);
 OneWire oneWire3(oneWireBus3);
  // Pass our oneWire reference to Dallas Temperature. 
  DallasTemperature sensors1(&oneWire1);
  DallasTemperature sensors2(&oneWire2);
  DallasTemperature sensors3(&oneWire3);



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
  sensors1.begin(); // Start up the library for one-wire bus 1 - temperature zone 1
  sensors2.begin(); // Start up the library for one-wire bus 2 - temperature zone 2
  sensors3.begin(); // Start up the library for one-wire bus 3 - temperature zone 3
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
    
    //Get temperature from sensor 1, ask again if value returned is invalid
    //TO-DO : add logging for sensor read re-try and move retry into the requestTemperatures() function    

    sensors1.requestTemperatures();
    tempAmbient = sensors1.getTempCByIndex(0);
    if(tempAmbient == 85.0 || tempAmbient == -127.0)
    {
    delay(75);
    sensors1.requestTemperatures();
    tempAmbient = sensors1.getTempCByIndex(0);      
    }
    
    sensors2.requestTemperatures();
    tempZone2 = sensors2.getTempCByIndex(0);
    if(tempZone2 == 85.0 || tempZone2 == -127.0) //retry one time if bad value is read
    {
    delay(75);
    sensors2.requestTemperatures();
    tempZone2 = sensors2.getTempCByIndex(0);      
    }    
        
    sensors3.requestTemperatures();
    tempZone3 = sensors3.getTempCByIndex(0);
    if(tempZone3 == 85.0 || tempZone3 == -127.0) //retry one time if bad value is read
    {
    delay(75);
    sensors3.requestTemperatures();
    tempZone3 = sensors3.getTempCByIndex(0);      
    }    
    
    
    tempUpdateCountdown = (tempUpdateDelay * loopsPerSecond) + 1;
    CPUTemp = getCPUTemp();
// Turn off the status light
    digitalWrite(statusLight, LOW);
  }
  
  
  
//Decide whether to turn furnace ON or OFF

//Is the timeout counter expired OR has furnace been disabled?
  if(furnaceTimeoutCountdown == 0 || !furnaceEnable)
  {
    digitalWrite(furnaceControlRelay, HIGH); //active LOW - turn furnace OFF
  }

//Is timeoutcounter != 0 AND furnace is enabled?
  if(furnaceEnable && furnaceTimeoutCountdown != 0)
  {
    digitalWrite(furnaceControlRelay, LOW); //active LOW - turn furnace ON
    furnaceTimeoutCountdown--; //decrement the counter for furnace timeout countdown
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
  if(commandReq == bsmt_requestFWVer) //command is to request FW version
      {
        Serial.print("Bv"); //send command reply prefix
        Serial.print(FWversion); //send command reply data
        Serial.print("!");//send the terminator
        return; 
      }

  if(commandReq == bsmt_turnFurnaceOn)
  {
      furnaceEnable = true;
      Serial.print("BF1"); 
      furnaceTimeoutCountdown = (furnaceTimeout * loopsPerSecond) + 1;
    
  }
  
  if(commandReq == bsmt_turnFurnaceOff)
  {
    furnaceEnable = false;
    furnaceTimeoutCountdown = 0;
    Serial.print("Bf1");
  }
  
  if(commandReq == bsmt_turnFanOn)
  {
    Serial.print("BV1");
    digitalWrite(fanControlRelay, LOW);
    return;
  }
  
  if(commandReq == bsmt_turnFanOff)
  {
    Serial.print("Be1");
    digitalWrite(fanControlRelay, HIGH);
    return;
  }
  
  if(commandReq == bsmt_turnFurnaceAndFanOff)
  {
    Serial.print("Bv1");
    furnaceEnable = false;
    digitalWrite(furnaceControlRelay, HIGH);
    digitalWrite(fanControlRelay, HIGH);
    return;
  }
  
  if(commandReq == bsmt_requestTemp)
  {
    Serial.print("BT");
    Serial.print(tempAmbient);
    Serial.print("!");
    return;
  }
  
  if(commandReq == bsmt_requestCPUTemp)
  {
    Serial.print("Bt");
    Serial.print(CPUTemp);
    Serial.print("!");
    return;
  }

  if(commandReq == bsmt_requestFurnaceStatus)
  {
    Serial.print("BS");
    if(furnaceEnable)
      Serial.print("1");
    else
      Serial.print("0");
    Serial.print("!");
    return;
  }
  
  if(commandReq == bsmt_requestTempZone2)
  {
    Serial.print("Bo");
    Serial.print(tempZone2);
    Serial.print("!");
    return;
  }
  
  if(commandReq == bsmt_requestTempZone3)
  {
    Serial.print("Bu");
    Serial.print(tempZone3);
    Serial.print("!");
    return;
  }
    
//  Serial.print("!");//reply for unimplemented commands
      return;
}




