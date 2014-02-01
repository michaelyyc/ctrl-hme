
int buzzer = 13;
int doorStatusPin = 9;
bool doorStatus = false;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(buzzer, OUTPUT);   
  pinMode(doorStatus, INPUT);  
}

// the loop routine runs over and over again forever:
void loop() {
  //read input
  doorStatus = digitalRead(doorStatusPin);
  
  while(!doorStatus)
  {
    for(int i2 = 0; i2 < 5; i2++)
    {
      for(int i = 0; i < 250; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delay(1);
      }
      delay(250);
    }
    delay(3500);
    doorStatus = digitalRead(doorStatusPin);
    
  }
}
