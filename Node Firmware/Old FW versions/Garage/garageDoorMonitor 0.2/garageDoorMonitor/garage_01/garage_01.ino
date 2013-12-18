/*
Garage door monitor
Check whether the door is open, close it after a specified time limit is reached
 
 */

//Commands from the controller
  #define requestFWVer            0
  #define requestDoorStatus       1
  #define requestTempZone1        2
  #define requestActivateDoor     3
  #define requestAutoCloseStatus  4
  #define requestDisableAutoClose 5
  #define requestEnableAutoClose  6
  #define requestActive120V1      7
  #define requestDeactivate120V1  8
  #define requestCPUtemp          9
  #define requestTempZone2       10
  #define requestClearErrorFlag  11
  #define requestPowerSupplyV    12
  
  //I/O pin definitions
  #define doorStatusPin           8
  #define statusLight            13
  #define doorControlRelay        2 
   
  //Constants
  #define activationTime         15
  #define timeLimit             300
  #define FWversion             0.1
  #define baud                 9600

  //Variables
  bool doorStatus = 0;// 1 = closed, 0 = open
  bool doorError = 0; //if repeated attempts to close door don't work, this bit is set and no more attempts are made
  bool autoCloseEnable = 1; //1 = enabled , 0 = disabled
  int i;
  int i2;
  int commandReq = 0;
  bool lightStatus;
  
  
  
  
  // the setup routine runs once when the node is powered on
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(baud);
  pinMode(doorControlRelay, OUTPUT);//pin connected to relay that signals door opener/closer
  pinMode(statusLight, OUTPUT);//status light on board
  pinMode(doorStatusPin, INPUT);//garage door sensor (1 = closed, 0 = open)
}


//Main loop  
void loop() {
  //Check for requests
  checkRequests();

  //Read all the sensors
  doorStatus = digitalRead(doorStatusPin);

 //Code for door auto-close function and countdown 
  if(doorStatus == 0 && autoCloseEnable)
  {
    for(i = timeLimit + 10, i >= 0; i--;) //start countdown, allow extra 10 seconds while door opens
    {
    //Check for requests
    checkRequests();
    
    lightStatus = !lightStatus;
    digitalWrite(statusLight, lightStatus);
     // Wait 1 second before blinking light
     for(i2 = 0, i2 < 100; i2++;)
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
     digitalWrite(doorControlRelay, HIGH);
     delay(200);   //leave relay closed for this amount of time (ms)
     digitalWrite(doorControlRelay, LOW);
     delay(200);
     //Don't confirm door status change for opening
     return;
  }
  
    if(doorStatus == 0)  // Door is open, close it and confirm it closes
  {
     //Serial.println("going from open to closed");
     digitalWrite(doorControlRelay, HIGH);
     delay(200);   //leave relay closed for this amount of time (ms)
     digitalWrite(doorControlRelay, LOW);
     
     // Wait 20 seconds to allow the door to close fully, but answer requests that come during this time
     for(i = 0, i < 2000; i++;)
      {
        //check for requests
        checkRequests();
        //delay for 10ms
        delay(10);
      }

     doorStatus = digitalRead(doorStatusPin); //check the door status pin
        
     if(doorStatus == 1) // door successfully closed, go back to monitoring
     {
       return;
     }
      
     if(doorStatus == 0) // Door didn't close
     {
       //Serial.println("ERROR: Door didn't close");
       //Serial.println("Try again...");
       digitalWrite(statusLight, HIGH);
       digitalWrite(doorControlRelay, HIGH);
       delay(200);   //leave relay closed for this amount of time (ms)
       digitalWrite(doorControlRelay, LOW);
     // Wait 20 seconds to allow the door to close fully, but answer requests that come during this time
     for(i = 0, i < 2000; i++;)
      {
        //check for requests
        checkRequests();
        //delay for 10ms
        delay(10);
      }

      doorStatus = digitalRead(doorStatusPin); // check the door status again
       
     if(doorStatus == 1) // door successfully closed, go back to monitoring
     {
       //Serial.println("Door close successful on second attempt");
       return;
     }
     
       
       if(doorStatus == 0)//door sensor still says open after two attemps, door status is unknown - possible sensor failure??
       {
        while(1)//infinite loop to halt program operation
         {
           doorError = 1;
         }
       }
  }
  }
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
  }
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
          Serial.print("0");
        }
        if(doorStatus == 0)
        {
          Serial.print("1");
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



  Serial.print("!");//reply for unimplemented commands
      return;
}




