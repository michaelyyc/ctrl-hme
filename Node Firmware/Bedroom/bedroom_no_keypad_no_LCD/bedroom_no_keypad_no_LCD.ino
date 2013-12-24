#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>

//constants
#define heaterControlRelay        5 //relay to control 300W heater
#define oneWireBus1                2 // temperature sensor
#define keypadAnalog              A0 // input from keyPad LCD keypad
#define loopsPerSecond            84 // used for estimating the timing for occasional updates
#define tempUpdateDelay           30 // how many seconds (approx) before updating temperature


//Variables
float setPoint = 17.75;
float setPointHysterisis = 0.75;
float ambientTemp = -1;
bool heaterEnabled = false;
bool heaterOn = false;
bool debounceHold = false;
int  debounceDelayms = 200;
int  input = 0;
char inputChar;
unsigned int tempUpdateCountdown = 0;

  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire1(oneWireBus1);
  // Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors1(&oneWire1);
  
  
void setup() {
  // set up the LCD's number of columns and rows: 
 
  pinMode(heaterControlRelay, OUTPUT);//ACTIVE LOW pin connected to relay that turns on the furnace
  //Start with the heater off
  digitalWrite(heaterControlRelay, HIGH);
  sensors1.begin(); // Start up the library for one-wire bus 1  
  Serial.begin(9600);
  
  delay(500);
  sensors1.requestTemperatures();
  ambientTemp = sensors1.getTempCByIndex(0);
  
}




void loop() {
  
    sensors1.requestTemperatures();
    ambientTemp = sensors1.getTempCByIndex(0);
    if(ambientTemp < setPoint - setPointHysterisis)
    {
      if(ambientTemp != 0.0 && ambientTemp != -127.0)//ignore bad values
        digitalWrite(heaterControlRelay, LOW);//Turn heater ON
    }
    
    if(ambientTemp > setPoint + setPointHysterisis)
    {
      if(ambientTemp != 0.0 && ambientTemp != -127.0)//ignore bad values
        digitalWrite(heaterControlRelay, HIGH);//Turn heater OFF
    }
    
    if(Serial.available());
    {
      inputChar = Serial.read();
      if(inputChar == '?')
      {
        Serial.print("Ambient Temp: ");
        Serial.println(ambientTemp);
      }
      
      else if(inputChar == 't')
      {
        Serial.println("Test mode - turning on for 5 seconds");
        digitalWrite(heaterControlRelay, LOW);
        delay(5000);
        Serial.println("Turning OFF for 5 seconds");
        digitalWrite(heaterControlRelay, HIGH);
        delay(5000);
        Serial.println("resuming normal operation");  
       }
    }
  
}

