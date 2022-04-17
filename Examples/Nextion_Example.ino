
#include "Arduino.h"
#include <RV-3028-C7.h>     // Reat Time Clockl Module - https://github.com/constiko/RV-3028_C7-Arduino_Library
#include <Nextion.h>

#define debugz

#define NextionDisplay Serial5

RV3028  rtc;
Nextion display(&NextionDisplay);

elapsedMillis	nextionTime;				// just used for various timing routines.
bool const		open   = true;				// valve states
bool const		closed = false;
uint32_t		nextionBaudRate = 9600;

void setNextionBaud(uint32_t baud) {
#ifdef debug
	Serial.print("Setting baudrate to "); Serial.println(baud);
#endif
	NextionDisplay.end();
	NextionDisplay.begin(baud);
}

void setup() {
	Serial.begin(9600);
	while (!Serial && millis() < 5000) {}

	display.begin( nextionBaudRate, setNextionBaud );
	NextionDisplay.begin(display.baudRate);

	if (!display.commsOk()) {

#ifdef debug
		Serial.println("No Nextion Comms...recovering Nextion Baudrate");
		Serial.print("Recovered Nextion Baud Rate: ");
#endif

		nextionBaudRate = display.recoverNextionComms();
		if ( nextionBaudRate == 0 ) {
			Serial.println("CRITICAL ERROR: Unable to Recover Nextion baud rate");
		}
#ifdef debug
		Serial.println(nextionBaudRate);
#endif

	}

	Wire.begin();

#ifdef debug
	Serial.println("Just starting..Setting RTC");
#endif

	if (rtc.begin() == false) {
		Serial.println("Something went wrong, check wiring");
		while (1);
	}
	else if (rtc.updateTime() == false) {		//Updates the time variables from RTC
		Serial.println("Unable to get time from RTC");
	}

	//	NextionDisplay.addMemoryForRead(serialReadBuffer, sizeof(serialReadBuffer));

#ifdef debug
	Serial.println("About to reset Nextion");
#endif

	if (!display.reset(115200)) {

#ifdef debug
		Serial.println("Error Resetting Nextion");
		Serial.print("Recovered Baud Rate: ");
#endif
		nextionBaudRate = display.recoverNextionComms();

		if (nextionBaudRate == 0) {					// Unable to detect Baud Rate..try to see what went wrong

			display.respondToReply();				// look to see what was returned by commsOk() used by recoverNextionComms()
			setNextionBaud(9600);					// Set the Teensy Nextion baud rate to 9600...the default
			display.setNextionBaudRate(115200);		// Now try to set to faster baud rate again.
			if (!display.commsOk()) {				// Just check tyhat it worked OK.
				Serial.println("CRITICAL ERROR: Unable to Communicate with Nextion display at 115200 baud");
			}
		}
	}

	while (nextionTime < 2000) {
		display.printAnyReturnCharacters(nextionTime,1);
	}
	uint32_t time;

	if (rtc.updateTime() == false) //Updates the time variables from RTC
	{
		Serial.print("RTC failed to update");
	}
	else {
		time = (uint32_t)rtc.getHours() * 0x10000 + (uint32_t)rtc.getMinutes() * 0x100 + (uint32_t)rtc.getSeconds();
		display.setTime(time);

		String currentTime = rtc.stringTimeStamp();
		Serial.print("  RTC: ");  Serial.println(currentTime);
	}

	/* I have a boiler system with valves controlled by a Teensy. The valve status/control buttons
	   are on the Nextion. I have a routine where I turn the valve display on or off. 
	   As a debug feature I can use the display.printAnyReturnCharacters(nextionTime,1) to print
	   the "time", in nextionTime units, followed by an Id followed by any data returned by the Nextion.
	   This is useful in determine if an error has occured and what it might be.
	   Obviously this would only be used during the development/debug phase.
   */
	display.turnNextionValve(0, open);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,1);
	display.turnNextionValve(0, closed);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,2);
	display.turnNextionValve(1, open);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,3);
	display.turnNextionValve(1, closed);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,4);

	Serial.println();

}

uint8_t counter = 0;
uint8_t n = 0;

void loop() {

	delay(1000);

	if (display.getReply()) {
		display.respondToReply();
	}

	switch (counter) {

		case 0 ... 2:

		// It takes Nextion some time to reply to a bad command at 9600 baud
			rtc.updateTime();
			display.printCommandOrErrorTextMessage("C", "Hello from Teensy", true);
			nextionTime = 0;
			while (nextionTime < 2000) {
				display.printAnyReturnCharacters(nextionTime,1); // see if any error response from Nextion
			}
			Serial.println(nextionTime);
			n++;
			counter++;
			break;
		case 3:
#ifdef debug
			Serial.println("page page1");
#endif
			NextionDisplay.print("page page1"); NextionDisplay.print("\xFF\xFF\xFF"); // Switch to page1-- - Outside library
			counter++;
			break;
		case 4:
			counter++;
			break;
		case 5:
#ifdef debug
			Serial.println("page0.msg.txt=\"Hello from Teensy\"");
#endif
			rtc.updateTime();
			display.printCommandOrErrorTextMessage("C", "Hello from Teensy", false);
			display.printNumericText((uint32_t)n, true);
			n++;
			counter++;
			break;
		case 6:
			for (n = 0; n < 120; n++) {
				rtc.updateTime();
				display.printCommandOrErrorTextMessage("C", "Hello from Teensy", false);
				display.printNumericText((uint32_t)n, true);
				delay(10);
			}
			counter++;
			break;
		case 7:
			delay(10000);  //10 seconds
			counter++;
			break;
		case 8:
#ifdef debug
			Serial.println("page page0");
#endif
			NextionDisplay.print("page page0"); NextionDisplay.print("\xFF\xFF\xFF"); // Switch back to page0-- - Outside library
			counter++;
			break;
		default:
			break;
	}
}
