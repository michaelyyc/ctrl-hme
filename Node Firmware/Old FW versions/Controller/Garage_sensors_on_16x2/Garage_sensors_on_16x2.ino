
// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int keypadInputPin = A0;
float keypadInput = 0;
char incomingByte;

void setup() {
  // set up the LCD's number of columns and rows: 
  Serial.begin(9600);
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("waiting ...");
  pinMode(keypadInputPin, INPUT);
}

void loop() {
  Serial.print("a");//request outdoor temp
  delay(1000);
  Serial.print("2");//request indoor temp
  delay(1000);
  Serial.print("1");
  delay(3000);
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  keypadInput = analogRead(keypadInputPin);
  if(keypadInput > 720 && keypadInput < 725)
  {
    Serial.print("3");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Opening....");
    delay(2000);
  }
 
  lcd.clear();
  lcd.setCursor(0, 0);
  
 
  while(Serial.available() > 0)
  {
    incomingByte = Serial.read();
    if(incomingByte == 'G')
    {
     //ignore it, dont print anything
    }
    else if(incomingByte == 'w')
    {
      lcd.print("Outside: ");
    }
    else if(incomingByte == 'T')
    {
      lcd.print("Inside:  ");
    }
    else if(incomingByte == '!')
    {
      lcd.setCursor(0, 1);// Move to second row
    }
    else if(incomingByte == 'S')
    {
      lcd.setCursor(14, 0); //move to char 15 in row 1
      lcd.print("DR");
      lcd.setCursor(15, 1); //position the cursor below the DR note:
    }
      else
      lcd.print(incomingByte);
    }
  delay(250);
}

