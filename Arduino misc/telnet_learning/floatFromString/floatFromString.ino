
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


/* floatFromSerial()
	This function reads the serial port and returns a floating point
	value determined as:
		The first ASCII character 0-9 on the serial port
		until the next delimiter character is received as a delimiter
	Other non-numeric / '.' / 'delimiter'  are ignored
	There is a 1 second watchdog timer, if this expires the function returns -1111

        As input, this function takes an integer value indicating the delimiter 
        to search for as the end of the floating point value
        
        If no serial input is available when this function is called, it will return -2222
*/
float floatFromSerial(int delimiter)
{
	//local variables
	float inputFloat = 0; //setup a variable to store the float
	float isDecimal = 0;  //keep track of how many times to divide by 10
		              // when processing decimal inputs
        int inputByte;	      //store the byte read from the serial port
        int startTime = millis();
        
        if(!Serial.available())
          return -2222;
        
	while (1)//Do this loop until it is broken out of by the delimiter
	{
		if(millis() > (startTime + 1000))
                {
                    return -1111;
                }
  
                inputByte = Serial.read();
		if(inputByte == delimiter) //delimiter for end of float
		{
			break;//break out of the whole loop
		}
		
		if(inputByte == '.') //detect decimal portion
		{
			if(isDecimal == 0) // only do this the first time
				isDecimal = 10.0; //divide the next input by 10
		}
		
		if(inputByte > 47 && inputByte < 58) //filter numbers 0-9 only
		{
			//multiply by 10 and add the next digit (subtract 48 to convert
			//ASCII char to matching integer 0-9
			
			if(isDecimal == 0)
			{
				inputFloat = (inputFloat * 10) + (inputByte - 48);
			}
			
			else
			{
				inputFloat = (inputFloat) + ((inputByte -48) / isDecimal);
				isDecimal = isDecimal * (10); // divide next one by 10x more				
			}
		}	
	}
	return inputFloat;
}
