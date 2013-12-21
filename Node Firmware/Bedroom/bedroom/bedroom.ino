#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>

//constants
#define heaterControlRelay         2 //relay to control 300W heater
#define oneWireBus1               12 // temperature sensor
#define keypadAnalog              A0 // input from keyPad LCD keypad
#define loopsPerSecond            84 // used for estimating the timing for occasional updates
#define tempUpdateDelay           30 // how many seconds (approx) before updating temperature


//Variables
float setPoint = 18.0;
float ambientTemp = -1;
bool heaterEnabled = false;
bool heaterOn = false;
bool debounceHold = false;
int  debounceDelayms = 200;
int  input = 0;
unsigned int tempUpdateCountdown = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire1(oneWireBus1);
  // Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors1(&oneWire1);
  
  
void setup() {
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
 
  pinMode(heaterControlRelay, OUTPUT);//ACTIVE LOW pin connected to relay that turns on the furnace
 
  //Start with the heater off
  digitalWrite(heaterControlRelay, HIGH);
  sensors1.begin(); // Start up the library for one-wire bus 1  
}




void loop() {
  //update the display
  lcd.setCursor(0,0);// position in top left
  lcd.print("Set : ");
  lcd.print(setPoint);
  
  lcd.setCursor(0,1);//left side of buttom row
  lcd.print("Meas: ");
  lcd.print(ambientTemp);
  
  lcd.setCursor(12,0);//right most section
  if(heaterEnabled)
   lcd.print("HOLD"); 
  
  else
    lcd.print(" OFF");
    
  lcd.setCursor(13,1);
  if(heaterOn)
    lcd.print(" ON");
    
  else
    lcd.print("OFF");
    
    
  
 
  //if a key was just pressed, wait a little while before checking again (prevent jittery response)
  if(debounceHold)
  {
    delay(debounceDelayms);
    debounceHold = false;
  }
    

  //See if it's time to update the sensors that only update periodically
  if(tempUpdateCountdown == 0) //don't update the temperature sensors every loop because it take too long
  {
//Put a * beside the temperature on the screen while updating
    lcd.setCursor(11,1);
    lcd.print("*");
    sensors1.requestTemperatures();
    ambientTemp = sensors1.getTempCByIndex(0);
    tempUpdateCountdown = (tempUpdateDelay * loopsPerSecond) + 1;
    lcd.setCursor(11,1);
    lcd.print(" ");
  }    
  tempUpdateCountdown--; //decrement the countdown until the next update  
    
    
//read and respond to keypad input
  input = readKeypad();
  
  if(input == 2) // Up button pressed
  {
   if(setPoint >= 23.0)
   { 
     // don't increase it
   }
   else
   { 
     setPoint = setPoint + 0.25;
   }
    debounceHold = true;
  }
  
  if(input == 3) // Down button pressed
  {
    if(setPoint <= 15.0)
    {
     // don't decrease it
    }
    else
    {
    setPoint = setPoint - 0.25;
    }
    debounceHold = true;

  }
  
  if(input == 5)
  {
    heaterEnabled = !heaterEnabled;
    debounceHold = true;
   }
   
   
 //determine if the heater should be turned on or off
 if(ambientTemp < (setPoint - 0.2) && ambientTemp != -127.0)//- 127.0 is a sensor error
  {
   if(heaterEnabled)
   {
     heaterOn = true;
     digitalWrite(heaterControlRelay, LOW);//turn the heater on - active LOW
   }  
}
   
   if(ambientTemp > (setPoint + 0.2) || ambientTemp == -127.0 || !heaterEnabled)//-127.0 is a sensor error
   {
     heaterOn = false;
     digitalWrite(heaterControlRelay, HIGH);//turn the heater off - active LOW
   }
 
}

int readKeypad()
{
 
 //Returns 1 = Right  (approx 000 counts)
 //        2 = Up     (approx 132 counts)
 //        3 = Down   (approx 307 counts)
 //        4 = Left   (approx 480 counts)
 //        5 = Select (approx 722 counts)
 //        0 = None  (approx 1023 counts)
 
 int keypadInput;
 keypadInput = analogRead(keypadAnalog);
 
 if(keypadInput < 50)
   return 1;
   
 else if (keypadInput > 100 && keypadInput < 150)
    return 2;
   
 else if (keypadInput > 290 && keypadInput < 340)
   return 3;
   
 else if (keypadInput > 460 && keypadInput < 500)
   return 4;
  
 
 else if (keypadInput > 700 && keypadInput < 750)
    return 5;
   
 else
    return 0; 
 
 
}
