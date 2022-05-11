# Nextion-Library

#### THIS DOCUMENT IS STILL UNDER DEVELOPMENT. PLEASE ALSO READ THE Nextion.h DOCUMENT IN Resources

Use this code as a framework to produce your own Nextion Library.
See Resources for Printable Word/pdf documents.

  Code by Robert E Bridges bob@bricomp-uk.com
  This library is intended to be used to create your own Nextion Library. Most of it is done for you. 

  The function that you will mostly alter is the "respondToReply()" function.
  I developed this library to control the valves in my Home Heating system, so there are functions
  that pertain to the opening/closing of valves. This can be used as an example as to how to use/develop
  the Library.

  I mostly communicate with the nextion through the passing of data into/from numeric variables.

  I have a TimerEvent which runs at 600mS intervals, slow I know but fast enough for my current needs.

  When, for example, this timer notices that the numeric variable "SetTime" is not zero it takes the value
  from this variable and sets the Nextion time. 

  The format of the data in this variable is (in HEX) "HHMMSS".
  After having set the time the variable is set back to 0 again.

  Other variables are interrogated and responded to in a similar way by the code for this Timer Event.
  An example is to give an impression of a flashing led, turning on or off a radio button, with a different
  colour for on and off.

  Below is the Nextion code snippet to set the RTC time.

				//=================================
				//Set RTC time if SetTime > 0  NOTE: Variables declared in Nextion Programs.s
				//=================================
				if(SetTime!=0)
				{
				  xx=SetTime
				  xx=xx>>16
				  rtc3=xx				// Set the hour
				  xx=SetTime
				  xx=xx&0xFF00
				  xx=xx>>8				// Set the minutes
				  rtc4=xx
				  xx=SetTime&0xFF
				  rtc5=xx				// Set the seconds
				  SetTime=0
				}

To include the Nextion Library simply `#include "Nextion.h"`at the head of your program.

To create an instance of the code use  `Nextion display(&NextionDisplay);`

where NextionDisplay is the serial stream used, i.e. Serial1, Serial2, etc.

In my latest code I use:-

`#define NextionDisplay Serial5`

To setup the Nextion code in "`setup()`" use the command:-

​		`display.begin(nextionBaudRate, setTeensyBaud);`

where `nextionBaudRate` is the baud rate to be used to communicate with the Nextion and `setTeensyBaud` is a teensy callback function to set the baudrate on theTeensy.

A suitable function is shown below:-


		void setTeensyBaud(uint32_t baud) {
		#ifdef debugst
			Serial.print("Setting Teensy baudrate to "); Serial.println(baud);
		#endif
			NextionDisplay.end();
			delay(100);
			NextionDisplay.begin(baud);
		}

​		...now we must set the baud rate on the Teensy port:-


				NextionDisplay.begin(display.baudRate);

The variable `display.baudRate` returns the baudrate expected by the Nextion.

Now we are able to check that we can communicate with the Nextion. Use the function `display.commsOk();`

This will return **true** if all is ok.

It may be that the Nextion had been setup to communicate at, for instance, 115200 baud and we are trying to communicate at the initial 9600 baud. In this case communications will not be possible.

Never fear, all is not lost, simply use the function `display.recoverNextionComms()`. This will cycle through all the valid baud rates until communications are established. If no communications can be established 0 will be returned.

If comms are not established you could try re-setting the Nextion with the code `display.reset()`. This is a last resort and likely not successful as it requires comms to the Nextion in order to send the "rest" command.

A suitable startup for your program could be as below:-

	#define NextionDisplay Serial5
	
	elapsedMillis	nextionTime;
	Nextion			display(&NextionDisplay);
	nextionBaudRate = 9600;
	
	void setTeensyBaud(uint32_t baud) {
	#ifdef debugst
		Serial.print("Setting Teensy baudrate to "); Serial.println(baud);
	#endif
		NextionDisplay.end();
		delay(100);
		NextionDisplay.begin(baud);
	}
	
	void setup() {
		Serial.begin(9600);
		while (!Serial && millis() < 5000) {}
	
		display.begin(nextionBaudRate, setTeensyBaud);
	
		Serial.println(display.baudRate);
		NextionDisplay.begin(display.baudRate);
	
		Serial.println("Starting up");
		display.printAnyReturnCharacters(nextionTime, 1);
		if (!display.commsOk()) {
			Serial.print("No Comms. Attempting to Recover......");
			display.printAnyReturnCharacters(nextionTime, 1-1);
			if (display.recoverNextionComms() == 0) {
				display.printAnyReturnCharacters(nextionTime, 1-2);
				Serial.print("Unable to recover, trying Reset.....");
				if (!display.reset()) {
					display.printAnyReturnCharacters(nextionTime, 1-3);
					Serial.println("CRITICAL ERROR: Unable to communicate with Nextion");
					while (1){}
				} else 
				{
					Serial.println("Ok Just Reset, Should br Ok now");
				}
			} else
			{ 
				Serial.println("Now got Comms");
			}
		}
	}

In the above code you will see the function `display.printAnyReturnCharacters(nextionTime, 1);` that we have not discussed yet.

This function simply gathers any Nextion returned characters and prints them to the SerialUsb (Serial) port. If there is any data returned it first prints the "nextionTime"  (which is in ms) followed by the returned bytes in HEX format followed by the id. This is useful in debugging when a bad situation occurs.
