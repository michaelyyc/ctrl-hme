/*
  LiquidCrystal Library - Hello World
 
 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the 
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.
 
 This sketch prints "Hello World!" to the LCD
 and shows the time.
 
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 
 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

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
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  keypadInput = analogRead(keypadInputPin);
  if(keypadInput > 720 && keypadInput < 725)
  {
    Serial.print(79);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Opening....");
    delay(2000);
  }
  
  lcd.setCursor(0, 0);
  
 
  while(Serial.available() > 0)
  {
    incomingByte = Serial.read();
    if(incomingByte == ',')
    {
      lcd.print("               ");//make sure rest of line is blank
      lcd.setCursor(0, 1);
      incomingByte = Serial.read();//ignore one byte (space)
    }
    else
      lcd.print(incomingByte);
    }
  delay(250);
}

