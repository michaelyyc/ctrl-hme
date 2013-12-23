#include "valueFromSerial.h"
#include <Arduino.h>


/* floatFromSerial()
	This function reads the serial port and returns a floating point
	value determined as:
		The first ASCII character 0-9 on the serial port
		until the next delimiter character is received as a delimiter
	Other non-numeric / '.' / 'delimiter'  are ignored
	
	There is a timeout function of 500,000 loops - about 6 seconds
	
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
	double loopCounter = 0;
    
    bool isNegative = false;    
        
        
	while (1)//Do this loop until it is broken out of by the delimiter
	{

     	if(loopCounter > 500000)
     	{
     		return -1111; // Timeout error
     	}
          
        inputByte = Serial.read();
        if(inputByte == '-')
        {
        	isNegative = true;
        }
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
		loopCounter++;
	}//End of while loop
	if(isNegative)// if it's negative, multiply by -1
	{
		inputFloat = inputFloat * -1.0;
	}
	return inputFloat;
}


bool boolFromSerial()
{
	//local variables
    int inputByte;	      //store the byte read from the serial port
    double loopCounter = 0;
    

                     
	while (1)//Do this loop until it is broken out of by detecting a 1 or 0
	{
		if(loopCounter > 500000)//longer timeout counter because timeout
							//cannot be discerned from a real false value
     	{
     		return false; // Timeout error
     	}
  
        inputByte = Serial.read();//read a byte from the serial port
		
		if(inputByte == '1') //ASCII  = true
		{
			return true;
		}
		
		if(inputByte =='0') 
		{
			return false;
		}
	loopCounter++;	
	}
}



int intFromSerial(int delimiter)
{
	//local variables
	int inputInt = 0; //setup a variable to store the float
    int inputByte;	      //store the byte read from the serial port
    bool isNegative;  
    double loopCounter = 0;
     

	while (1)//Do this loop until it is broken out of by the delimiter
	{
		
		if(loopCounter > 500000)
     	{
     		return -1111; // Timeout error
     	}
          
          
        inputByte = Serial.read();
		if(inputByte == delimiter) //delimiter for end of float
		{
			break;//break out of the whole loop
		}
		
		if(inputByte > 47 && inputByte < 58) //filter numbers 0-9 only
		{
			//multiply by 10 and add the next digit (subtract 48 to convert
			//ASCII char to matching integer 0-9
			inputInt = (inputInt * 10) + (inputByte - 48);
		}	
		loopCounter++;
	}//end of while loop
	
	if(isNegative)// if it's negative, multiply by -1
	{
		inputInt = inputInt * -1.0;
	}	
	return inputInt;
}