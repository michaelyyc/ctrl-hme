/*
Garage door monitor
Check whether the door is open, close it after timeLimit (Seconds) is reached
 */

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(2, OUTPUT);//pin connected to relay that signals door opener/closer
  pinMode(13, OUTPUT);//status light on board
  pinMode(8, INPUT);//garage door sensor (1 = closed, 0 = open)
}

//The pin identifiers and time limits should be changed to constants
  int doorStatus = 0;// 1 = closed, 0 = open
  int doorStatusPin = 8;
  int statusLight = 13;
  int doorControlRelay = 2;
  int timeLimit = 300;// maximum time before closing door [300 sec = 5 minutes]
  int i;
  int incomingByte = 0;
  bool lightStatus;
  
// the loop routine runs over and over:
void loop() {
  doorStatus = digitalRead(doorStatusPin); //check the door status
  if(doorStatus == 0) //if the door is open
  {
    Serial.println("door is open, start counting");
    for(i = timeLimit + 10, i >= 0; i--;) //start countdown, allow extra 10 seconds while door opens
    {
    Serial.println(i); //print the countdown to the serial port
    checkSerial(); //check for inputs
    lightStatus = !lightStatus;
    digitalWrite(statusLight, lightStatus); //blink the light
    delay(1000); // wait 1 second
    if(digitalRead(doorStatusPin))//check if the door is closed
    {
      Serial.println("Door closed, abort countdown");
      digitalWrite(statusLight, LOW); //turn off the status light
      break; //jump out of this loop
    }
    if(i == 0)
    {
     Serial.println("Timer expired, close door");  
     activateDoor();
    }
    }
  }
  if(doorStatus == 1) //door is closed
  {
    Serial.println("Door is closed, continue monitoring");
    checkSerial();
    delay(1000);
  }
}

void activateDoor() //This function activates the door, and confirms that the position changes
// if the position hasn't changed, it will make one additional attempt. If it fails again, it will blink the red status light and 
// will not monitor or activate the door any longer until it is reset 
// this is intended to prevent a sensor failure from activating the door continually. If the sensor fails and the door is closed
// the timer will start and the door will open when it expires, the status test will fail and will then close, the status test will fail again and the door will remain closed.
{
//If the door is going from closed to open, the sensor input should change right away
  if(doorStatus == 1) // Door is closed, open it
  {
     Serial.println("going from closed to open");
     digitalWrite(doorControlRelay, HIGH);
     delay(200);   //leave relay closed for this amount of time (ms)
     digitalWrite(doorControlRelay, LOW);
     delay(3000);
     //Don't confirm door status change for opening
     return;
  }
  
    if(doorStatus == 0)  // Door is open, close it and confirm it closes
  {
     Serial.println("going from open to closed");
     digitalWrite(doorControlRelay, HIGH);
     delay(200);   //leave relay closed for this amount of time (ms)
     digitalWrite(doorControlRelay, LOW);
     delay(20000);// Wait 20 seconds to allow the door to close fully
     //The board will not respond to serial inputs during this 20 seconds
     
     doorStatus = digitalRead(doorStatusPin); //check the door status pin
    
    
     if(doorStatus == 1) // door successfully closed, go back to monitoring
     {
       return;
     }
     
     
     if(doorStatus == 0) // Door didn't close
     {
       Serial.println("ERROR: Door didn't close");
       Serial.println("Try again...");
       digitalWrite(statusLight, HIGH);
       digitalWrite(doorControlRelay, HIGH);
       delay(200);   //leave relay closed for this amount of time (ms)
       digitalWrite(doorControlRelay, LOW);
       delay(20000);// Wait 20 seconds to allow the door to close fully
       doorStatus = digitalRead(doorStatusPin); // check the door status again
       
     if(doorStatus == 1) // door successfully closed, go back to monitoring
     {
       Serial.println("Door close successful on second attempt");
       return;
     }
     
       
       if(doorStatus == 0)//door sensor still says open after two attemps, door status is unknown - possible sensor failure??
       {
        while(1)//infinite loop to halt program operation, blink status light every 1/2 second
         {
           digitalWrite(statusLight, LOW);
           delay(250);
           digitalWrite(statusLight, HIGH);
           delay(250);
           digitalWrite(statusLight, LOW);
           delay(250);
           digitalWrite(statusLight, HIGH);
           delay(250);
           Serial.println("Possible sensor or relay fault, program halted");
         }
       }
  }
  }
}
     

void checkSerial() //function to handle incoming data on the serial port
//ignores everything except the character 'O' which will trigger the door opener
{
  if (Serial.available() > 0)
  {
    incomingByte = Serial.read();
    Serial.println(incomingByte); //echo the received command (just for debugging)
    if (incomingByte == 79)// 79 = 'O', door open command
    {
      activateDoor();
    }
    return;
  }
}
  



