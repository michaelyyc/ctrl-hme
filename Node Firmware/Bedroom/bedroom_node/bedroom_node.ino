#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <commandDefinitionsAlt.h>


//constants
#define heaterControlRelay        5 //relay to control 300W heater
#define oneWireBus1                2 // temperature sensor
#define keypadAnalog              A0 // input from keyPad LCD keypad
#define loopsPerSecond            84 // used for estimating the timing for occasional updates
#define tempUpdateDelay           30 // how many seconds (approx) before updating temperature
#define FWVersion                 0.01
#define setPointMinimum           15.0
#define setPointMaximum           21.0
#define tempUpdateLoopInitial     500000


//Variables
float setPoint = 18.75;
float setPointHysterisis = 0.50;
float ambientTemp = -1;
bool maintainSetPoint = false;
bool heaterOn = false;
int  input = 0;
double tempUpdateLoop = tempUpdateLoopInitial;
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
    
    
    //Control the heater
    if(maintainSetPoint && (ambientTemp < (setPoint - setPointHysterisis)))
    {
      if(ambientTemp != 0.0 && ambientTemp != -127.0)//ignore bad values
        digitalWrite(heaterControlRelay, LOW);//Turn heater ON
    }
    
    if(!maintainSetPoint || (ambientTemp > (setPoint + setPointHysterisis)))
    {
       digitalWrite(heaterControlRelay, HIGH);//Turn heater OFF
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
      else if(inputChar == bdrm_increaseSetPoint)
      {
        if(setPoint < setPointMaximum)
           setPoint += 0.25;
   
        Serial.print("Ms");
        Serial.print(setPoint);
        Serial.print("!");
      }
      else if(inputChar == bdrm_decreaseSetPoint)
      {
        if(setPoint > setPointMinimum)
          setPoint -= 0.25;
  
        Serial.print("Ms");
        Serial.print(setPoint);
        Serial.print("!");        
      }
      else if(inputChar == bdrm_requestSetPoint)
      {
        Serial.print("Ms");
        Serial.print(setPoint);
        Serial.print("!");    
      }
      else if(inputChar == bdrm_maintainSetPoint)
      {
        maintainSetPoint = true;
        Serial.print("MH1");
      }
      else if(inputChar == bdrm_dontMaintainSetPoint)
      {
        maintainSetPoint = false;
        Serial.print("Mh1");
      }  
    }
}

