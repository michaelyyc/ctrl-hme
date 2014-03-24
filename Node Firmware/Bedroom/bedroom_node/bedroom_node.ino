#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <commandDefinitionsAlt.h>


//constants
#define heaterControlRelay        5 //relay to control heater
#define oneWireBus1               2 // temperature sensor
#define keypadAnalog              A0 // input from keyPad LCD keypad
#define loopsPerSecond            84 // used for estimating the timing for occasional updates
#define tempUpdateDelay           30 // how many seconds (approx) before updating temperature
#define FWVersion                 0.03
#define setPointMinimum           15.0
#define setPointMaximum           21.0
#define tempUpdateLoopInitial     500000
#define heaterTimeout             300 //seconds to wait before turning off heater if another command hasn't been received


//Variables
float ambientTemp = -1;
double tempUpdateLoop = tempUpdateLoopInitial;
char inputChar;
unsigned int tempUpdateCountdown = 0;
long int heaterTimeoutCountdown;
bool heaterOn = false;

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
    //Periodically update the reading from the temperature sensor
    if(tempUpdateLoop < 1)
    {
      digitalWrite(13, HIGH);
      sensors1.requestTemperatures(); 
      ambientTemp = sensors1.getTempCByIndex(0);
      tempUpdateLoop = tempUpdateLoopInitial;
      digitalWrite(13, LOW);
    }
    tempUpdateLoop--;
    if(heaterOn && heaterTimeoutCountdown < 1)
    {
      digitalWrite(heaterControlRelay, HIGH); // active low output
      heaterOn = false;
    }
    if(heaterOn)
    {
      heaterTimeoutCountdown--;
    }
    
    
    //Check for and respond to serial input commands from the controller
    if(Serial.available());
    {
      inputChar = Serial.read();
      if(inputChar == bdrm_requestFWVer)
      {
        Serial.print("Mv");
        Serial.print(FWVersion);
        Serial.print("!");
  
      }
      
      else if(inputChar == bdrm_requestTemp)
      {
        Serial.print("MT");
        Serial.print(ambientTemp);
        Serial.print("!");
      }
      
      else if(inputChar == bdrm_requestActivate120V1)
      {
        heaterTimeoutCountdown = heaterTimeout * loopsPerSecond;
        digitalWrite(heaterControlRelay, LOW);//Turn on heater (active low)
        heaterOn = true;
        Serial.print("MS1");
      }
      
      else if(inputChar == bdrm_requestDeactivate120V1)
      {
        digitalWrite(heaterControlRelay, HIGH);//Turn off heater (active low)
        heaterOn = false;
        Serial.print("Ms1");
      }
    }
}

