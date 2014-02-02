
int buzzer = 12;
int LED = 13;
int doorStatusPin = 9;
bool doorStatus = false;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(buzzer, OUTPUT);   
  pinMode(LED, OUTPUT);
  pinMode(doorStatus, INPUT);  
}

// the loop routine runs over and over again forever:
void loop() {
  //read input
  doorStatus = digitalRead(doorStatusPin);
  
  if(!doorStatus)
  {
    digitalWrite(LED, HIGH);
    for(int i2 = 0; i2 < 4; i2++)
    {
      for(int i = 0; i < 100; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delay(1);
      }
      for(int i = 0; i < 250; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(2);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delay(2);
      }
      delay(250);
      doorStatus = digitalRead(doorStatusPin);
      if(doorStatus)
        break;
    }
    while(!doorStatus)//wait for door to close
    {
      doorStatus = digitalRead(doorStatusPin);
    }
      digitalWrite(LED, LOW);
      for(int i = 0; i < 100; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(2);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delay(2);
      }
      for(int i = 0; i < 50; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(1);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delay(1);
      }
      delay(50);
      for(int i = 0; i < 100; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delayMicroseconds(500);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delayMicroseconds(500);
      }
      delay(50);
      for(int i = 0; i < 100; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delayMicroseconds(500);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delayMicroseconds(500);               // wait for a second
      }
      delay(50);
      for(int i = 0; i < 100; i++)
      {
      digitalWrite(buzzer, HIGH);   // turn the LED on (HIGH is the voltage level)
      delayMicroseconds(500);               // wait for a second
      digitalWrite(buzzer, LOW);    // turn the LED off by making the voltage LOW
      delayMicroseconds(500);               // wait for a second
      }      
  }
}
