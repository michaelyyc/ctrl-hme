#include <DallasTemperature.h>
#include <OneWire.h>

/*
Garage door monitor
Check whether the door is open, close it after a specified time limit is reached

Respond to requests from the controller which are received via serial input
It can be used with a wired or wireless serial connection, intended to be used with XBEE radio

The temperature sensors are DS18B20

*/

/*
//Commands from the controller
  #define requestFWVer            0
  #define requestDoorStatus       1
  #define requestTempZone1        2
  #define requestActivateDoor     3
  #define requestAutoCloseStatus  4
  #define requestDisableAutoClose 5
  #define requestEnableAutoClose  6
  #define requestActivate120V1      7
  #define requestDeactivate120V1  8
  #define requestCPUtemp          9
  #define requestTempZone2       10
  #define requestClearErrorFlag  11
  #define requestPowerSupplyV    12
  */
  
  
  
//Alternate commands for 0-9, a-z in ASCII for debugging with a keyboard
  #define requestFWVer            48 //0
  #define requestDoorStatus       49 //1
  #define requestTempZone1        50 //2
  #define requestActivateDoor     51 //3
  #define requestAutoCloseStatus  52 //4
  #define requestDisableAutoClose 53 //5
  #define requestEnableAutoClose  54 //6
  #define requestActivate120V1    55 //7
  #define requestDeactivate120V1  56 //8
  #define requestCPUtemp          57 //9
  #define requestTempZone2        97 //a
  #define requestClearErrorFlag   98 //b
  #define requestPowerSupplyV     99 //c
  

  //I/O pin definitions
  #define doorStatusPin           8 //door sensor connection
  #define statusLight            13 //status indicator light on board
  #define doorControlRelay        2 //door control relay
  #define oneWireBus1             3 //inside temperature sensor
  #define oneWireBus2             4 //outside temperature sensor
  #define relay120V1              5 //relay for 120V AC power (block heater)
   
  //Constants
  #define activationTime         15
  #define timeLimit             300
  #define FWversion             0.3
  #define baud                 9600
  #define loopsPerSecond      40000 //used in calculating loops for periodic updates

  //Variables
  bool doorStatus = 0;// 1 = closed, 0 = open
  bool doorError = 0; //if repeated attempts to close door don't work, this bit is set and no more attempts are made
  bool autoCloseEnable = 1; //1 = enabled , 0 = disabled
  int i;  //for loops
  int i2; //for loops
  int commandReq = 0; //variable to store the request from the controller
  float tempZone1 = -1; //temperature from Zone 1 in degrees C
  float tempZone2 = -1;
  float CPUTemp = -1;
  bool lightStatus;
  double tempUpdateDelay = 30; //how many seconds (approx) between updates of the temperature sensors
  double tempUpdateCountdown;

  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
 OneWire oneWire1(oneWireBus1);
 OneWire oneWire2(oneWireBus2);
  // Pass our oneWire reference to Dallas Temperature. 
  DallasTemperature sensors1(&oneWire1);
  DallasTemperature sensors2(&oneWire2);
  
  // the setup routine runs once when the node is powered on
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(baud);
  pinMode(doorControlRelay, OUTPUT);//pin connected to relay that signals door opener/closer
  digitalWrite(doorControlRelay, HIGH);// initialize pin to HIGH
  pinMode(statusLight, OUTPUT);//status light on board
  digitalWrite(statusLight, LOW);// initialize pin to low
  pinMode(doorStatusPin, INPUT);//garage door sensor (1 = closed, 0 = open)
  pinMode(relay120V1, OUTPUT);//active high relay for 120V output
  digitalWrite(relay120V1, HIGH);//turn off the 120V output (active LOW)

  sensors1.begin(); // Start up the library for one-wire bus 1
  sensors2.begin(); // Start up the library for one-wire bus 2 
  

}


//Main loop  
void loop() {
  
  
  //Check for requests
  checkRequests();

  //Read the sensors that get updated every loop
  doorStatus = digitalRead(doorStatusPin);
  
  //See if it's time to update the sensors that only update periodically
  if(tempUpdateCountdown == 0) //don't update the temperature sensors every loop because it take too long
  {
//set the status light on while checking
    digitalWrite(statusLight, HIGH);
    sensors1.requestTemperatures();
    sensors2.requestTemperatures();
    tempZone1 = sensors1.getTempCByIndex(0);
    tempZone2 = sensors2.getTempCByIndex(0);
    tempUpdateCountdown = (tempUpdateDelay * loopsPerSecond) + 1;
    CPUTemp = getCPUTemp();
// Turn off the status light
    digitalWrite(statusLight, LOW);

  }
  
  //decrement the periodic sensor update counter
  tempUpdateCountdown--;
  
 //Code for door auto-close function and countdown 
  if(doorStatus == 0 && autoCloseEnable)
  {
    for(i = timeLimit + 10; i >= 0; i--) //start countdown, allow extra 10 seconds while door opens
    {
    //Check for requests
    checkRequests();
    
    lightStatus = !lightStatus;
    digitalWrite(statusLight, lightStatus);
     // Wait 1 second before blinking light
     for(i2 = 0; i2 < 100; i2++)
      {
        //check for requests
        checkRequests();
        //delay for 10ms
        delay(10);
      }

    if(digitalRead(8))//true if door is closed
    {
      digitalWrite(statusLight, LOW);
      break; //jump out of this loop
    }
    if(i == 0)
    {
     activateDoor();
    }
    }
  }
}

  void activateDoor() //This function activates the door, and confirms that the position changes
// if the position hasn't changed, it will make one additional attempt. If it fails again, it will blink the red status light and 
// will not monitor or activate the door any longer until it is reset. It will set the DoorError flag.
// this is intended to prevent a sensor failure from activating the door continually. If the sensor fails and the door is closed
// the timer will start and the door will open when it expires, the status test will fail and will then close
// the status test will fail again and the door will remain closed.
{
//If the door is going from closed to open, the sensor input should change right away
  if(doorError == 1)
  {
   //do not attempt to open or close the door if the error flag is set
    return;
  }
  
  
  if(doorStatus == 1) // Door is closed, open it
  {
     //Serial.println("going from closed to open");
     digitalWrite(doorControlRelay, LOW);
     delay(200);   //leave relay closed for this amount of time (ms)
     digitalWrite(doorControlRelay, HIGH);
     delay(200);
     //Don't confirm door status change for opening
     return;
  }
  
    if(doorStatus == 0)  // Door is open, close it and confirm it closes
  {
    // Serial.println("going from open to closed");
     digitalWrite(doorControlRelay, LOW);
     delay(200);   //leave relay closed for this amount of time (ms)
     digitalWrite(doorControlRelay, HIGH);
     
     // Wait 20 seconds to allow the door to close fully, but answer requests that come during this time
   //   Serial.println("waiting for door to close");
     for(i = 0; i < 2000; i++)
      {
          //check for requests
        checkRequests();
        //delay for 10ms
        delay(10);
        
        // dont check any more if the auto-close function has been disabled
        if(autoCloseEnable == 0)
        {
          return;
        }
      }

      //Serial.println("checking door status");
     doorStatus = digitalRead(doorStatusPin); //check the door status pin
        
     if(doorStatus == 1) // door successfully closed, go back to monitoring
     {
         //Serial.println("door has closed");
       return;
     }
      
     if(doorStatus == 0) // Door didn't close
     {
       //Serial.println("ERROR: Door didn't close");
       //Serial.println("Try again...");
       digitalWrite(statusLight, HIGH);
       digitalWrite(doorControlRelay, LOW);
       delay(200);   //leave relay closed for this amount of time (ms)
       digitalWrite(doorControlRelay, HIGH);
     // Wait 20 seconds to allow the door to close fully, but answer requests that come during this time
       //Serial.println("waiting for door to close");

     for(i = 0; i < 2000; i++)
      {
        //check for requests
        checkRequests();
        //delay for 10ms
        delay(10);
        
        
         // dont check any more if the auto-close function has been disabled
        if(autoCloseEnable == 0)
        {
          return;
        }
      }

      doorStatus = digitalRead(doorStatusPin); // check the door status again
       
     if(doorStatus == 1) // door successfully closed, go back to monitoring
     {
       //Serial.println("Door close successful on second attempt");
       return;
     }
     
       
       if(doorStatus == 0)//door sensor still says open after two attemps, door status is unknown - possible sensor failure??
       {
                    //Serial.println("door error - program halted");
        while(1)//infinite loop to halt program operation
         {
           doorError = 1;

         }
       }
  }
  }
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
        Serial.print("Gv"); //send command reply prefix
        Serial.print(FWversion); //send command reply data
        return; 
      }

  if(commandReq == requestDoorStatus) //command is to return door status 0 = open, 1 = closed
      {
        doorStatus = digitalRead(doorStatusPin); //make sure to send a current value

        Serial.print("GS");//send command reply prefix  
        if(doorError)
        {
          Serial.print("E");//return E if there error flag is set.
          return;
        }  
        Serial.print(doorStatus); //return door status
        return;
      }



  if(commandReq ==  requestActivateDoor) //command is to activate door
      {
        Serial.print("GD"); //send command reply prefix
        //send the opposite of the current door status, converted to an ascii character 
        if(doorStatus == 1)
        {
          Serial.print("0"); //door is opening
        }
        if(doorStatus == 0)
        {
          Serial.print("1"); // door is closing
        }
        activateDoor(); // perform command
        return;
      }


  if(commandReq == requestAutoCloseStatus)
  {
    if(autoCloseEnable)
    {
      Serial.print("1");
    }
    else
    {
      Serial.print("0");
    }
    return;
  }
  if(commandReq == requestEnableAutoClose)
      {
        Serial.print("1");
        autoCloseEnable = 1;
        return;
      }


  
  if(commandReq == requestDisableAutoClose)
      {
        Serial.print("0");
        autoCloseEnable = 0;
        return;
      }


  
  if(commandReq == requestClearErrorFlag)
      {
       doorError = 0;
       Serial.print("1"); 
       return;
      }

  if(commandReq == requestTempZone1)
    {
    Serial.print("GT"); // send the prefix
    Serial.print(tempZone1);  //send the temperature as degrees C
    Serial.print("!"); // send the terminator
    return;
    }

  if(commandReq == requestTempZone2)
    {
    Serial.print("Gw"); // send the prefix
    Serial.print(tempZone2);  //send the temperature as degrees C
    Serial.print("!"); // send the terminator
    return;
    }

  if(commandReq == requestActivate120V1)
    {
    Serial.print("G11"); // send the reply
    digitalWrite(relay120V1, LOW);// turn on the 120V output (active LOW)
    return;
    }

  if(commandReq == requestDeactivate120V1)
    {
    Serial.print("G10"); // send the reply
    digitalWrite(relay120V1, HIGH);//turn off the 120V output (active LOW)
    return;
    }
    
  if (commandReq == requestCPUtemp)
  {
    Serial.print("Gt");
    Serial.print(CPUTemp);
    Serial.print("!");
    return;
  }
  
    
  Serial.print("!");//reply for unimplemented commands
      return;
  
}




