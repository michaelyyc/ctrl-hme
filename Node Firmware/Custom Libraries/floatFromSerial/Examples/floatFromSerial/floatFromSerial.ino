#include <floatFromSerial.h>


void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

}

void loop() {
  float processedInputFloat;
  
  if(Serial.available())
  {
    processedInputFloat = floatFromSerial(33); //read float form serial port delimited by '!'
    Serial.print("The input was processed as: ");
    Serial.println(processedInputFloat);
  }
  

}


