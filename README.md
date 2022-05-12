# Nextion-Library

#### THIS DOCUMENT IS STILL UNDER DEVELOPMENT. PLEASE ALSO READ THE Nextion.h DOCUMENT IN Resources

This ReadMe is a little wordy but tries to explain all the nuances of using Nextion. For a more concise explanation see the Nextion.h file or the information in resources.

Use this code as a framework to produce your own Nextion Library.
See Resources for Printable Word/pdf documents.

  Code by Robert E Bridges.
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
	Nextion		display(&NextionDisplay);
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

This function simply gathers any Nextion returned characters and prints them to the SerialUsb (Serial) port. If there is any data returned it first prints the "nextionTime"  (which is in ms) followed by the returned bytes in HEX format followed by id. This is useful in debugging when a bad situation occurs.

There are more configuration routines/functions, but first let's get to a state where your code is usable.

If a relevant function does not already exist in the Library then use the command

`sendCommand( command );` as in, for example, `display.sendCommand( "tsw 255,0" );`

There is no need to worry about the \xFF\xFF\xFF characters, they will be sent automatically over the previously setup Serial stream.

Of course there should not be any reply from the Nextion unless the *bkcmd* level has been set to 1 or 3. This will be discussed later when describing the setBkCmdLevel function.

Of course the Nextion is a HMI (Human Machine Interface) and it would be expected that there will be some output from the Nextion.

This is handled by the `getReply(timeout)` and `respondToReply()` functions.

The *getReply()* function can be used in two forms `display.getReply()` and `display.getReply(timeout)`. In the first the serial port is simply checked for any returned characters. If there are non then *false* is returned. In the second form the function waits for timeout ms or a character to appear.

If there is a reply from Nextion then the Reply Char is received and the required  number of following char/bytes dependent upon the value of the Id. The first character returned is known as the id character.

A *nextionEventType* variable (called *nextionEvent*) is used for all communications from the Nextion.

The Id char is placed in *nextionEvent.id*. The number of remaining char/bytes to be received is dependant upon the value of the *id*. The remaining chars are placed in *nextionEvent.reply8* ready to be decoded.

A *true* is returned if there is an *Id* char and the required number of chars are returned from the Nextion. Otherwise *false* is returned. If the first char is received within timeout a further timeout of up to 1 second is allowed for remaining characters. This proc does NOT get any strings returned from Nextion. Use `display.respondToReply()` for that.																			*

##### respondToReply()

This is where it is going to get **heavy**.

After having used `display.getReply()` to determine that there is a response from the Nextion and to have got that reply `display.respondToReply()` is used to decode that reply and take the required action. As was mentioned earlier this is where it is likely that you will need to add code.

respondToReply() - returns true if something needs responding to.
This is where you need to put your code. Use getReply() to get any info from the  Nextion (see above) and this function to decode the reply and respond to it. It returns true if further response is needed.

I like to have requests from the Nextion Display embedded into numbers.

Within this code I want to turn valves on or off. The number returned by the Nextion contains  the valve to be moved and whether it should be opened or closed (0 or 1). 

If you have handled the Nextion response fully then set *needsResponse* to *false*.  

Below is the listing for *respondToReply*.

	bool Nextion::respondToReply() {   //returns true if something needs responding to
	
	  bool     needsResponse = true;
	  uint16_t zz;
	  uint32_t valve;
	  bool	 how;
	  
	  switch (nextionEvent.id) {
		case invalidInstruction:	// Returned when instruction sent by user has failed
		case instructionSuccess:	// (ONLY SENT WHEN bkcmd = 1 or 3 )
			comdExecOk = true;
		case invalidComponentId:	// Returned when invalid Component ID or name was used
		case invalidPageId:			// Returned when invalid Page ID or name was used
		case invalidPictureId:		// Returned when invalid Picture ID was used
		case invalidFontId:			// Returned when invalid Font ID was used
		case invalidFileOperation:	// Returned when File operation fails
		case invalidCrc:			// Returned when Instructions with CRC validation fails
	    							// their CRC check
		case invalidBaudRateSetting:// Returned when invalid Baud rate was used
		case invalidWaveformIdChan:	// Returned when invalid Waveform ID or 
									// Channel # was used
		case invalidVarNameAttrib:	// Returned when invalid Variable name or invalid
	    							// attribute was used
		case invalidVarOperation:	// Returned when Operation of Variable is invalid.ie: 
									// Text assignment t0.txt = abc or t0.txt = 23, 
		case assignmentFailed:		// Returned when attribute assignment failed to assign
		case EEPROMOperationFailed:	// Returned when an EEPROM Operation has failed
		case invalidQtyParams:		// Returned when the number of instruction parameters 
									// is invalid
		case ioOperationFailed:		// Returned when an IO operation has failed
		case invalidEscapeChar:		// Returned when an unsupported escape character is used
		case variableNameToLong:	// Returned when variable name is too long.Max length is
	    							// 29 characters: 14 for page + “.” + 14 for component.
		case serialBufferOverflow:	// Returned when a Serial Buffer overflow occurs
	    							// Buffer will continue to receive the current 
	    							// instruction, all previous instructions are lost.
			if (nextionEvent.id != instructionSuccess) {
				nextionError = true;
				errorCode	 = nextionEvent.id;
			}
			break;
		case touchEvent:
	    //		Serial.println("Touch Event");
			break;
		case currentPageNumber:
		//		Serial.println("Current Page Number");
			break;
		case touchCoordinateAwake:
		//		Serial.println("Touch Coordinate Awake");
			break;
		case touchCoordinateSleep:
		//		Serial.println("Touch Coordinate Sleep");
			break;
		case stringDataEnclosed:
		//		Serial.println("String Data Enclosed");
			if (!GetNextionString()) {
				nextionError = true;
				errorCode    = invalidNumCharsReturned;
			};
			break;
		case numericDataEnclosed:
			zz = nextionEvent.reply7.num[0];  // (uint16_t)nextionEvent.reply7.ans[0] * 256 +
	        								  // (uint16_t)nextionEvent.reply7.ans[1];
			switch (zz) {
				case 0x0000: //Switch/Valve 0 off
				case 0x0001: //Switch/Valve 0 on
				case 0x0100: //Switch/Valve 1 off"
				case 0x0101: //Switch/Valve 1 on
				case 0x0200: //Switch/Valve 2 off
				case 0x0201: //Switch/Valve 2 on
				case 0x0300: //Switch/Valve 3 off
				case 0x0301: //Switch/Valve 3 on
				case 0x0400: //Switch/Valve 4 off
				case 0x0401: //Switch/Valve 4 on
				case 0x0500: //Turn Boiler Off
				case 0x0501: //Turn Boiler On
				case 0x0600: //Turn Hot Water Off
				case 0x0601: //Turn Hot Water On
					valve = zz / 0x100;
					how = ((zz % 0x100) == 1);
					turnValveOnOrOff(valve, how);
					needsResponse = false;
					break;
				case 0xFA00: //Nextion Set baudrate back to 9600
					SetTeensyBaud(9600);
					needsResponse = false;
				case 0xFDFD: // Indicates Nextion Serial Buffer Clear
					serialBufferClear	= true;
					needsResponse		= false;
				default:
					Serial.print("Some other NumericDataEnclosed data|: ");
					Serial.print(nextionEvent.reply7.num[0], HEX); Serial.print(" ");
					Serial.print(nextionEvent.reply7.num[1], HEX); Serial.println();
					break;
			}
			break;
		case autoEnteredSleepMode:
		//		Serial.println("Auto Entered Sleep Mode");
			break;
		case autoAwakeFromSleepMode:
		//		Serial.println("Auto awake mode");
			break;
		case nextionReady:
		//		Serial.println("Nextion Ready");
			break;
		case powerOnMicroSDCardDet:
			break;
		case transparentDataFin:
		//		Serial.println("Transparent data finished");
			break;
		case transparentDataReady:
		//		Serial.println("Transparent data ready");
			break;
		default:
			Serial.print("How did I get here:"); Serial.println(nextionEvent.id, HEX);
			_s->flush();
			clearBuffer();
			break;
	   }
	   return needsResponse;
	}

The first 19 responses, except for *`instructionSuccess`* are errors. Their value is placed in the variable `display.errorCode` and the variable `display.nextionError` is set to *true*. Note that `display.nextionError` is set to false when using `display.getReply()` and valid data is returned.

When `display.respondToReply()` returns *true* (response needed) it is the programmers responsibility to determine if an error has occurred.

The next four categories, which are NOT currently handled, are `touchEvent, currentPageNumber, touchCoordinateAwake` and `touchCoordinateSleep`. If any of these are likely to be returned by your implementation then they will need code to handle them. Currently for these entries `display.respondToReply()`returns *true*, *response required*.

The next item is a Nextion string return. The (private) function `GetNextionString()` gathers the Nextion string data and sends it to the string setup using `display.setTextBuffer` (see later). If this has not been setup or there are more characters than will fit in the string they are sent to Serial (the Screen). If a *string* has been successfully received then `display.stringWaiting` is set to *true*.

Now we get to an interesting bit (at least for me), `numericDataEnclosed`. I have a number of dual-state buttons on my Nextion implementation. I use the following Nextion code in the *Touch Release Event*.

		swResult=0x0200+Sw2.val
		get swResult

This is picked up by the `case 0x0200: //Switch Valve 2 off` or `case 0x0201: //Switch Valve 2 on` and the following code to switch a valve on or off.

				valve = zz / 0x100;
				how   = ((zz % 0x100) == 1);
				turnValveOnOrOff(valve, how);
				needsResponse = false;

The default entry for the `numericDataEnclosed` category is to indicate that a condition exists that the numeric data is not handled correctly.

The next six categories are also not currently handled autoEnteredSleepMode, autoAwakeFromSleepMode, nextionReady, powerOnMicroSDCardDet, transparentDataFin and transparentDataReady.

The default setting for the main Case is to indicate that a situation has occurred where the Nextion has responded with an unrecognised response, perhaps due to an error condition.

It should be noted that the main loop should look something like below:-

			void loop()
				if (getReply()){
					if (respondToReply()){
						// data NOT handled by respondToReply()
						// must be handled here.
						// look at: display.nextionError .. has an error occurred 
						//          or stringWaiting has a string been received
						//          or the other rteturn categories not already handled.
					}
				}
				// do something else
			}
