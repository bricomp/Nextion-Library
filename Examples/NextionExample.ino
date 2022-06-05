
#define hardwareRTC
#define usingSoftwareSerialz

//#include "whichTeensy.h"
#include "Arduino.h"
#include <Stream.h>

#ifdef hardwareRTC
// Load your RTC Library here
#include <RV-3028-C7.h>     // https://github.com/constiko/RV-3028_C7-Arduino_Library
#else
#include <TimeLib.h>
#endif

#ifdef usingSoftwareSerial
#include <SoftwareSerial.h>
#endif
#include "Nextion.h"

#ifdef usingSoftwareSerial
SoftwareSerial NextionDisplay(2, 3);    // Choose your pins Rx,Tx
#endif
#if defined ARDUINO_TEENSY35 && !defined usingSoftwareSerial
#define NextionDisplay Serial5
#endif
#if (!defined (ARDUINO_TEENSY35) && !defined (usingSoftwareSerial) )
#define NextionDisplay Serial1
#endif

#ifdef hardwareRTC
// Create your Hardware RTC Here
RV3028  rtc;
#endif
Nextion display(&NextionDisplay);

elapsedMillis	nextionTime;
bool const		open   = true;
bool const		closed = false;
uint32_t		nextionBaudRate = 9600;

#define debugstz

void changeTeensyBaud(uint32_t baud) {
#ifdef debugst
	Serial.print("Setting Teensy baudrate to "); Serial.println(baud);
#endif
	NextionDisplay.end();
	delay(100);
	NextionDisplay.begin(baud);
}

/*****************************************************************************
*    Position Nextion "page0.CommentBoxDisplay" at x,y and display text      *
*    Delay for "before ms" before moving/displaying text.					 *
*    Delay for "after" ms before exiting routine.							 *
******************************************************************************/
void DisplayInBox(uint8_t x, uint8_t y, const char* text, uint32_t before, uint32_t after) {
	const uint8_t d = 20;

	if (before > 0) delay(before);

	display.setNumVarValue("page0.CommentBox.x", x);

	delay(d); display.printAnyReturnCharacters(nextionTime, 100);
	display.setNumVarValue("page0.CommentBox.y", y);

	delay(d); display.printAnyReturnCharacters(nextionTime, 101);
	display.sendCommand("page0.CommentBox.txt=",text,true);

	if (after > 0) delay(after); else delay(d); display.printAnyReturnCharacters(nextionTime, 102);
}

void DisplayPageZero() {
	DisplayInBox(92, 75, " This Video demonstrates Teensy Controlling                      a Nextion Display.                           It is part of a Boiler C/H Control System.", 0,1000);
}

/****************************************************************
*      Turns on/off displaying clock/time on Nextion			*
*      Does this by making the variables making up the time		*
*      visible on not.											*
*****************************************************************/

void DisplayClock(bool displayIt) {

	uint32_t viz;
	if (displayIt) viz = 1; else viz = 0;
	display.sendCommand("vis hour,", viz);
	display.sendCommand("vis minute,", viz);
	display.sendCommand("vis timeSep,", viz);
}

/**********************************************************
*      Call back fn for Nextion Valve change request      *
***********************************************************/
void SetValveOnOrOff_FromNextion(uint32_t which, bool how) {
	switch (which) {
	case 0 ... 4:   // valves
		if (how == open) {
			Serial.print("Opening valve "); Serial.println(which);
			// OpenControlValve((controlValveIdType)which, true);
		}
		else {
			Serial.print("Closing valve "); Serial.println(which);
			//CloseControlValve((controlValveIdType)which, true);
		}
		break;
	case 5:
		Serial.print("Turning Boiler ");
		if (how) Serial.println("On"); else Serial.println("Off");
		//if (!TurnBoiler(how, true)) {}
		break;
	case 6:
		Serial.print("Turning Hot Water ");
		if (how) Serial.println("On"); else Serial.println("Off");
		//if (!TurnHW(how, true)) {}
		break;
	default:
		Serial.println("In fn SetValveOnOrOff_FromNextion, don't know how I got here.");
		break;
	}
}

char strBuffer[100];

void setup() {
	Serial.begin(9600);
	while (!Serial && millis() < 5000) {}

	display.begin(nextionBaudRate, changeTeensyBaud);

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

	display.reset();
	display.setValveCallBack(SetValveOnOrOff_FromNextion);
	nextionTime = 0;
	DisplayInBox(92, 85, "OK, should now have control",0,3000);
	DisplayInBox(92, 85, "             Just setting up the Teensy RTC                       and Update time on Nextion",0, 3000);
	Serial.println("Just starting");
/*
	display.gotoPage(2);
	if (display.setStrVarValue("t0", "About to read and then set a Float to x0")) {
		if (!display.getStringVarValue("t0")) {
			Serial.println("Error Occurred getting string");
		};
		display.setTextBuffer(strBuffer, sizeof(strBuffer));
		if (display.getStringVarValue("t0")) {
			Serial.print("strBuffer="); Serial.println(strBuffer);
		}else {
			Serial.println("Error Occurred getting string");
		}
	}else{
		Serial.println("Error Occurred setting string");
		display.printAnyReturnCharacters(1000,555);
	};
	Serial.print("Float x0 = "); Serial.println( display.getNumVarFloat("x0"));
	display.setStrVarValue("t0", "Seting Float x0 to 1234.5678 to 2dp rounded");
	display.setNumVarFloat("x0", 1234.5678, 2, true);
	delay(5000);
	display.setStrVarValue("t0", "Seting Float x0 to 1234.5678 to 2dp NOT rounded");
	display.setNumVarFloat("x0", 1234.5678, 2, false);
	delay(10000);
	display.gotoPage(0);
*/

#ifdef hardwareRTC
	Wire.begin();
	/********************************************************************
	*           This is for the RV-3028-C7 RTC that I am using.			*
	*			Replace with code for your own RTC.						*
	*********************************************************************/
	if (rtc.begin() == false) {
		Serial.println("Something went wrong, check wiring");
		DisplayInBox(92, 85, "Something went wrong, check wiring", 0,0);
		while (1);
	}
	else if (rtc.updateTime() == false) {		//Updates the time variables from RTC
		DisplayInBox(92, 85, "Unable to get time from RTC", 0,3000);
		Serial.println("Unable to get time from RTC");
	}
#endif

	DisplayInBox(92, 85, "OK, that went fine", 0,2000);

#ifdef hardwareRTC
	if (rtc.updateTime()) //Updates the time variables from RTC
	{
		uint32_t time = (uint32_t)rtc.getHours() * 0x10000 + (uint32_t)rtc.getMinutes() * 0x100 + (uint32_t)rtc.getSeconds();
		display.setTime(time);

		String currentTime = rtc.stringTimeStamp();
		Serial.print("  RTC: ");  Serial.println(currentTime);
	}
	else {
		DisplayInBox(92, 85, "RTC failed to update", 0, 3000);
		Serial.print("RTC failed to update");
	}
#else
	uint32_t time = hour() * 0x10000 + (uint32_t)minute() * 0x100 + (uint32_t)second();
	display.setTime(time);
#endif
	DisplayInBox(92, 85, "Just set-up the Teensy RTC",0,3000);
	display.setNextionBaudRate(115200);

	display.setStrVarValue("debugTxt", "Here we are Debug Text");
	display.viewDebugText(true);
	display.setNumVarValue("debugTxt.xcen", 0);
	delay(1000);
	display.viewDebugText(false);

	DisplayInBox(5, 51, "    As you can see we have some simulated                                  LEDs.                                             Just about to turn them all off", 0,10000);
	//                     1234567890123456789012345678901234567890
	//								1 		  2         3         4
	display.clearLeds();
	
	DisplayInBox(5, 51, "      Now going to turn them on one by one.                The middle row will be flashing.              The flashing is carried out by the Nextion.", 3000,0);
	//                     1234567890123456789012345678901234567890
	//								1 		  2         3         4
	uint8_t how = 1;
	for (uint8_t p = 0; p < 3; p++) {
		for (uint8_t n = 0; n < 8; n++) {
			display.setLedState((topMidBottmType)p, n, (onOffFlashingType)how);
			display.setNextionLeds((topMidBottmType)p);
			if (how == 2) delay(1200); else delay(600);   // matches the frequency of update of Nextion "SecondTmr" used in example
		}
		how++;
		if (how == 3) how = 1;
	}
	display.turnScreenDimOn(false);// display.sendCommand("dimTmr=0");
	DisplayInBox(5, 51, "Now clear the LEDs", 0,0);
	display.clearLeds();
	DisplayInBox(5, 51, "...and turn them on with no delay", 0,3000);
	how = 1;
	for (uint8_t p = 0; p < 3; p++) {
		for (uint8_t n = 0; n < 8; n++) {
			display.setLedState((topMidBottmType)p, n, (onOffFlashingType)how);
		}
		display.setNextionLeds((topMidBottmType)p);
		how++;
		if (how == 3) how = 1;
	}

	DisplayInBox(95, 130, "               Above there are Switches                                  to Turn Valves On/Off.                            Turning them on/off one by one.", 3000,3000);

	display.turnNextionValve(0, open);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,2);
	display.turnNextionValve(0, closed);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,3);
	display.turnNextionValve(1, open);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,4);
	display.turnNextionValve(1, closed);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,5);
//	Serial.println("Changing Baud Rate to 115200");
//	display.SetBaudRate(115200);
	delay(200);
	display.turnNextionValve(2, open);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,6);
	display.turnNextionValve(2, closed);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,7);
	display.turnNextionValve(3, open);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,8);
	display.turnNextionValve(3, closed);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,9);
	display.turnNextionValve(4, open);
	delay(2000);
	display.turnNextionValve(4, closed);
	delay(2000);	display.printAnyReturnCharacters(nextionTime,10);

	DisplayClock(false);
	DisplayInBox(105, 192, "               Now telling Nextion to turn                           Hot Water on for 150 Minutes.                 Time Compressed in Simulation Mode.       At timeout Teensy Messaged to turn off H/W", 0, 5000);

	display.setHotWaterOnForMins(150);
	if (display.getReply(1000)) {
		if (display.respondToReply()) {
			Serial.println("Response to Nextion Required");
		};
	}
	delay(2000);	display.printAnyReturnCharacters(nextionTime,11);

	Serial.println();

	// We are still on page0 but about to write text to page1
	display.preserveTopTextLine();
	delay(20);	display.printAnyReturnCharacters(nextionTime, 12);
	display.writeToTopTextLine("This is a protected top line");
	delay(20);	display.printAnyReturnCharacters(nextionTime, 13);
	DisplayClock(true);
}

uint8_t counter = 0;
uint8_t n = 0;

void loop() {

	delay(1000);

	if (display.getReply()) {			// See if anything come from Nextion
		display.respondToReply();		// ...if it has action it.
	}

	Serial.println("Writing to page1 from page0");
	for (n = 0; n < 3; n++) {
		display.printCommandOrErrorTextMessage("C", "Hello from Teensy", true);
		nextionTime = 0;
		while (nextionTime < 2000) {
			display.printAnyReturnCharacters(nextionTime, 14);
		}
		Serial.println(nextionTime);
		n++;
	}
	delay(2000);
	Serial.println("Switching to display screen 1");

	display.turnNextionButton(6, false);
	DisplayClock(true);
	Serial.println("Goto Page 1");
	display.gotoPage(1);
	delay(100);  display.printAnyReturnCharacters(nextionTime, 100);
	Serial.println("OK So nothing came back");
//	Serial.println("Just sending text to Nextion without the library...Naughty");

//	Serial.println("Sending 1 bit of text to Nextion");

//	Serial.println("page0.msg.txt=\"Hello from Teensy\"");

	display.printCommandOrErrorTextMessage("C", "Hello from Teensy", false);
	display.printNumericText((uint32_t)n, true);
	delay(1000);  display.printAnyReturnCharacters(nextionTime, 101);

	Serial.println("Sending lot's of text to Nextion");

	delay(1000);
	nextionTime = 0;
	for (n = 0; n < 250; n++) {
		Serial.print(n);
		Serial.print(" C Hello from Teensy "); Serial.print(n); Serial.print(" - ");	
//		if (n == 1) {
			Serial.print(display.getNumVarValue("HWDwnCtr")); Serial.print(" - ");
//		}
		Serial.println(nextionTime);


		display.printCommandOrErrorTextMessage("C", "Hello from Teensy ", false);
		display.printNumericText((uint32_t)n, true);					// "true" finish Nextion command

		if ((n>0) && (n % 50) == 0) {
			if (display.askSerialBufferClear(10000)) {           // wait for up to 10 secconds for Nextion imput buffer to clear
				Serial.println("Serial buffer clear");					// every 50 times through loop
			} else
			{
				display.printAnyReturnCharacters(nextionTime, 102);
			};
		}
		display.printAnyReturnCharacters(nextionTime, 1103);
		if (n == 150) {

			display.sendCommand("vis CommentBox,0");					// at the 150th iteration turn off CommentBox
		}
	}
	Serial.print("250 lines printed on Nextion in "); Serial.print(nextionTime); Serial.println("ms");

	delay(10000);  //10 seconds

	Serial.println("page 0");

	display.gotoPage(0);
	nextionTime = 0;

	while (1) {

		DisplayInBox(92, 115, "     Using getReply() and respondToReply()                              for 1 minute.                                          Press some switch buttons                                  and see what happens", 0, 0);
		//		DisplayInBox(105, 192, "               Now telling Nextion to turn                           Hot Water on for 150 Minutes.                 Time Compressed in Simulation Mode.       At timeout Teensy Messaged to turn off H/W", 0, 5000);
		while ((nextionTime / 30000) %2 ==0 ){
			if (display.getReply()) {			// See if anything come from Nextion
				display.respondToReply();		// ...if it has action it.
			}
		}

		DisplayInBox(92, 115, "        Using printAnyReturnCharacters                                  for 1 minute.                                         Press some switch buttons                                   and see what happens", 0, 0);
		while ((nextionTime / 30000) % 2 == 1){
			display.printAnyReturnCharacters(nextionTime, 200);
		}

	}
}
