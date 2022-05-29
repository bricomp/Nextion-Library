#include "Arduino.h"
#include <Nextion.h>
#include <Stream.h>

#define bkcmd1or3allowedz

#define debugvz
Nextion::Nextion(Stream* s) 
{
	_s = s;
}

Nextion::setNextionBaudCallbackFunc SetTeensyBaud;
bool	 nextionAutoBaud = false;
Nextion::nextionTurnValveOnOffCallbackFunc turnValveOnOrOff;

void Nextion::begin(uint32_t br, Nextion::setNextionBaudCallbackFunc func = nullptr) {
	baudRate = br;
	if (func) {
		SetTeensyBaud = func;
		nextionAutoBaud = true;
	}
};
void Nextion::setValveCallBack(Nextion::nextionTurnValveOnOffCallbackFunc func) {
	turnValveOnOrOff = func;
};

bool Nextion::getReply(uint32_t timeout = 0) {

	elapsedMillis	timeOut;
	uint8_t			len = 255;
	uint8_t			n;

	while ((timeOut < timeout) && !_s->available()) {}
	comdExecOk = false;

	if (_s->available()) {
		comdExecOk		= false;
		nextionError	= false;
		nextionEvent.id = _s->read();

		switch (nextionEvent.id) {
		case invalidInstruction ... invalidFileOperation:		// 0x00 ... 0x06: //
		case invalidCrc:										// 0x09
		case invalidBaudRateSetting ... invalidWaveformIdChan:	// 0x11..0x12
		case invalidVarNameAttrib ... invalidEscapeChar:		// 0x1A..0x20
		case variableNameToLong:								// 0x23
		case serialBufferOverflow:								// 0x24
			len = 3;
			break;
		case touchEvent:										// 0x65
			len = 6;
			break;
		case currentPageNumber:									// 0x66
			len = 4;
			break;
		case touchCoordinateAwake ... touchCoordinateSleep:		// 0x67..0x68
			len = 8;
			break;
		case stringDataEnclosed:								// 0x70
			len = 0;
			break;
		case numericDataEnclosed:								// 0x71
			len = 7;
			break;
		case autoEnteredSleepMode ... powerOnMicroSDCardDet:	// 0x86..0x89
		case transparentDataFin ... transparentDataReady:		// 0xFD..0xFE
			len = 3;
			break;
		default:
			break;
		}

		if (nextionEvent.id == stringDataEnclosed) { // read string data done in respondToReply()
		}
		else
		{
			timeOut = 0;
			n = 0;
			while (n < len && timeOut < 10000) {
				n = _s->available();
			};
			if (n == len) {
				_s->readBytes((char*)&nextionEvent.reply8, len);
			}
			else
			{
				nextionError = true;
				errorCode	 = invalidNumCharsReturned;
			}
			if ((nextionEvent.id >= invalidInstruction) && (nextionEvent.id <= serialBufferOverflow)) {
				nextionError = true;
				errorCode	 = nextionEvent.id;
			}
		}
		if ((nextionEvent.id == 0x67) || (nextionEvent.id == 0x68)) {
			nextionEvent.reply8.xPos = (uint16_t)nextionEvent.reply8.x[0] * 256 + (uint16_t)nextionEvent.reply8.x[1];
			nextionEvent.reply8.yPos = (uint16_t)nextionEvent.reply8.y[0] * 256 + (uint16_t)nextionEvent.reply8.y[1];
		}
		return true;
	}
	return false;
};

void Nextion::setLedState(topMidBottmType whichLed, uint8_t which/*0..7*/, onOffFlashingType state) {

	uint16_t ledState;
	uint16_t mask;
	uint16_t leds;

	ledState = state;
	leds	 = nextionLeds[whichLed];
	mask     = 0b011;

	if (which > 0) {
		which    <<= 1;					// multiply which by 2 since led sate occupies 2 bits
		ledState <<= which;				// move state - put it over the leds that need switching
		mask     <<= which;				// ditto mask - as above
	}

	mask  = ~mask;						// flip the bits
	leds &= mask;						// AND leds with mask to clear a space for the ledState
	leds |= ledState;					// OR leds with led state to insert the ledstate;
	nextionLeds[whichLed] = leds;		// put the leds back into the led array
}

void Nextion::setNextionLeds(topMidBottmType which) {   // on the display

	switch (which) {
		case top:
			_s->print("TopLed=");
			break;
		case mid:
			_s->print("MidLed=");
			break;
		case bottom:
			_s->print("BotmLed=");
			break;
		default:
			break;
	}
	_s->print(nextionLeds[which]);
	_s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
#ifdef debugv
	Serial.print(which);  Serial.print("-");  Serial.println(nextionLeds[which], BIN);
#endif
}

void Nextion::clearLeds() {

	for (uint8_t n = top; n <= bottom; n++) {
		nextionLeds[n] = 0;
		setNextionLeds((topMidBottmType)n);
	}
}

#define debugtz

void Nextion::finishNextionTextTransmittion() {
#ifdef debugt
	Serial.print("\"\xFF\xFF\xFF");
	if (mbedTimeInText) {
		Serial.print("click m0,1");
	}else
	{
		Serial.print("click m0,0");
	}
	Serial.print("\xFF\xFF\xFF");
	Serial.println(" -- finishNextionTextTransmittion");
#endif

	_s->print("\"\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
	if (!lastComdCompletedOk(1000)) {
		Serial.println("finishNextionTextTransmittion: Nextion command did NOT complete ok");
	};
#endif
	if (mbedTimeInText) {
		_s->print("click m0,1"); 
	}
	else
	{
		_s->print("click m0,0");
	}
	_s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
}

void Nextion::pntTextToNextion(bool embedTimeInTxt, const char* p, bool transmit) {
#ifdef debugt
	Serial.print("printTextToNextion: ");
	if (embedTimeInTxt) {
		Serial.print("page0.msg.txt=\"");
	}
	else
	{
		Serial.print("page1.va0.txt=\"");     // was:- _s->print("page1.va0.txt=\"");
	}
	Serial.print(p);
#endif
	mbedTimeInText = embedTimeInTxt;
	if (embedTimeInTxt) {
		_s->print("page0.msg.txt=\"");     // was:- _s->print("page1.va0.txt=\"");
	}
	else
	{
		_s->print("page1.va0.txt=\"");     // was:- _s->print("page1.va0.txt=\"");
	}
	_s->print(p);
	if (transmit) {
		finishNextionTextTransmittion();
		_s->flush();
	}
}

void Nextion::printTextToNextion(const char* p, bool transmit) {
	pntTextToNextion(false, p, transmit);
}

void Nextion::printTimeEmbeddedTextToNextion(const char* p, bool transmit) {
	pntTextToNextion(true, p, transmit);
}

void Nextion::printMoreTextToNextion(const char* p, bool transmit) {
#ifdef debugt
	Serial.print(p);
#endif
	_s->print(p);
	if (transmit) {
		finishNextionTextTransmittion();
		_s->flush();
	}
}

void Nextion::printNumericText(int32_t num, bool transmit) {
#ifdef debugt
	Serial.print(num);
#endif
	_s->print(num);
	if (transmit) {
		finishNextionTextTransmittion();
		_s->flush();
	}
}

void Nextion::printCommandOrErrorTextMessage(const char* commandOrError, const char* textMessage, bool transmit) {
#ifdef debugt
	Serial.print("printCommandOrErrorTextMessage: ");
#endif
	pntTextToNextion(true,commandOrError, false);
#ifdef debugt
	Serial.print(textMessage);
#endif
	_s->print(textMessage);
	if (transmit) {
		finishNextionTextTransmittion();
		_s->flush();
	}
}

/********************************************************************************************
*		preserveTopTextLine() - Top text line writing inhibited.							*
*-------------------------------------------------------------------------------------------*
*		All general text commands do not use top line if this command actuated.           	*
*********************************************************************************************/
void Nextion::preserveTopTextLine() {
	_s->print("topScrlTxtLn=18\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
}

/********************************************************************************************
*		writeToTopTextLine(const char* textMessage)											*
*********************************************************************************************/
#define debugthisz
void Nextion::writeToTopTextLine(const char* textMessage) {
#ifdef debugthis
	Serial.print("page1.TopTextLn.txt=\"");
	Serial.print(textMessage);
	Serial.print("\"\xFF\xFF\xFF");
#endif
	_s->print("page1.TopTextLn.txt=\"");
	_s->print(textMessage);
	_s->print("\"\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
};

/********************************************************************************************
*		releaseTopTextLine() - Allows writing to the Top Text Line							*
*-------------------------------------------------------------------------------------------*
*		All general text commands can use top line again (Default Setting).			    	*
*********************************************************************************************/
void Nextion::releaseTopTextLine() {
	_s->print("topScrlTxtLn=19\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
}

/********************************************************************************************
*		clearTextScreen() - Clears the Nextion Text Screen (page1)							*
*-------------------------------------------------------------------------------------------*
*		If the Top Line is preserved that is not cleared, use clearTopTextLine instead.    	*
*********************************************************************************************/
void Nextion::clearTextScreen() {
	_s->print("click ClrScr,1\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
};

/********************************************************************************************
*		clearTopTextLine() - Clears the Nextion Text Screen Top Text Line					*
*-------------------------------------------------------------------------------------------*
*		If the Top Line is preserved that is not cleared, use clearTopTextLine instead.    	*
*********************************************************************************************/
void Nextion::clearTopTextLine() {
	_s->print("page1.TopTextLn.txt=\"\"\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
};

void Nextion::clearBuffer() {

	unsigned long amt = millis();

	while (_s->available()) {
		_s->read();
		delay(1);
		if ((millis() - amt) > 5000) {
			Serial.println(F("runaway"));
			break;
		}
	}
}
#define debugcz

bool Nextion::commsOk() {
	bool ok = false;

	clearBuffer();
	nextionEvent.id = 0;
	_s->print("sendme\xFF\xFF\xFF");
	_s->flush();
	delay(10);
#ifdef debugc
	delay(10);
//	Serial.print("in commsOk ");
#endif
	ok = getReply();
#ifdef debugc
	if (ok) Serial.println("OK"); else Serial.println("Doh");
#endif
	return ok;
}

#define debugrz

bool Nextion::reset(uint32_t br){
/*
		|----- Start up message ----| |- Ready Message -|
		0x00 0x00 0x00 0xFF 0xFF 0xFF  0x88 0xFF 0xFF 0xFF
		 |   |--- uint32_t ----| |--|  |--- uint32_t ----|
		 |                         |
		 |                          \------- byte - last 0xFF
		  \---- nextionEvent.id


		struct resetReplyType {           first 00 in nextionEvent.id
			uint32_t	startup4Bytes;	  00 00 FF FF swap little endian to big endian = 0x0FFFF0000
			uint8_t		startupByte;	  FF
			uint32_t	readyReply;		  88 FF FF FF swap little endian to big endian = 0x0FFFFFF88
		};
	*/
	elapsedMicros  NextionTime;
	uint8_t  const resetReplyCharCount = 10;
	uint32_t const waitFor1stCharTime  = 1000; //ms = 1 second
	uint32_t startupReplyTime;
	uint8_t	 len;
	uint32_t savedBaud;
	uint32_t baud;
	
	savedBaud = baudRate;  // preserve current, before reset, baud rate

	startupReplyTime = 13 * 10000 / baudRate + 60;   // 13 chars to allow some leeway + 60 to allow diference between startup message and ready message
	clearBuffer();
	_s->flush();
	_s->print("rest\xFF\xFF\xFF");
//	_s->flush();

	SetTeensyBaud(resetNextionBaud);
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk	= true;		// Only used when bkcmd = 1 or 3
	checkComdComplete		= false;	// Only used when bkcmd = 1 or 3
	bkcmd					= onFailure;
#endif
	baudRate				= 9600;

	nextionTime = 0;
	while ((nextionTime < waitFor1stCharTime) && (_s->available() < 1)) {  // wait for first character of reply to appear
		delay(1);
	}
	nextionTime = 0;
	while ((nextionTime < startupReplyTime) && (_s->available() < resetReplyCharCount)) {
		delay(1);
	}

	delay(100);
	len = _s->available();
	/*
	* DO NOT BE TEMPTED TO USE getReply() instead of this code.
	* Nextion Reset returns 0x00 as the first character. 
	* That is a Nextion error code decoded by getReply()
	*/
	_s->readBytes((char*)&nextionEvent, len);
	if ( (len != resetReplyCharCount) || !( (nextionEvent.id == 0) && (nextionEvent.resetReply.startup4Bytes == 0x0FFFF0000) && (nextionEvent.resetReply.startupByte == 0x0FF) && (nextionEvent.resetReply.readyReply == 0x0FFFFFF88 ))) {
		Serial.print("len= "); Serial.println(len);

		Serial.print(nextionEvent.id, HEX); Serial.print(" ");

		if (len != resetReplyCharCount) {
			for (uint8_t n = 0; n < len; n++) {
				Serial.print(nextionEvent.data[n], HEX); Serial.print(" ");
			}
		}
		else {
			Serial.print(nextionEvent.resetReply.startup4Bytes, HEX); Serial.print(" ");
			Serial.print(nextionEvent.resetReply.startupByte, HEX); Serial.print(" ");
			Serial.println(nextionEvent.resetReply.readyReply, HEX);
		}
		return false;      // should really decode the error - data contained in "nextionEvent"
	}

	delay(20);

	switch (br) {
	case 0:				// don't bother chainging Baud Rate
		baud = resetNextionBaud;
#ifdef debugr
		Serial.println("Using Reset Baud");
#endif
		break;
	case 1:
		baud = savedBaud;
#ifdef debugr
		Serial.println("Using Previous Baud");
#endif
	default:
#ifdef debugr
		Serial.println("Changing Baud Rate");
#endif
		baud = br;
		break;
	}
	if (baud != 9600) {
#ifdef debugt
		Serial.print("About to set Nextion and Teensy baud rate to ");
		Serial.println(baud);
#endif

		setNextionBaudRate(baud);

	} else {

		recoveryBaudRate = resetNextionBaud;
		baudRate		 = resetNextionBaud;
	}

	delay(500);
#ifdef debugr
	Serial.println("About to check commsOk()");
#endif
	return commsOk();
}
#define debugrcz

uint32_t recoveryBaudRates[] = { 2400, 4800, 9600, 19200, 31250, 38400, 57600, 115200, 230400, 250000, 256000, 512000, 921600 };
uint8_t  numBaudRates		 = sizeof(recoveryBaudRates) / sizeof(uint32_t);

uint32_t Nextion::recoverNextionComms() {
	uint8_t baudCount = 0; // numBaudRates;
	bool gotit		  = false;

	SetTeensyBaud(recoveryBaudRate);
	gotit = commsOk();
	if (gotit) {
		return recoveryBaudRate;
	}
	while ( !gotit && ( baudCount < numBaudRates )) {
		baudCount++;
		if (recoveryBaudRates[baudCount] != recoveryBaudRate) {  // No point in trying what we have already done.
			SetTeensyBaud(recoveryBaudRates[baudCount]);
			delay(10);
			gotit = commsOk();
#ifdef debugrc
					Serial.print("Id: ");
					Serial.println(nextionEvent.id, HEX);
#endif
		}
	}
	if (gotit) {
		recoveryBaudRate = recoveryBaudRates[baudCount];
		baudRate		 = recoveryBaudRate;
		return recoveryBaudRate;
	} else
	{
		return 0;
	}
};
#define debugGNSz
bool Nextion::GetNextionString() {
	elapsedMillis t;
	bool		  gotFF = false;
	uint8_t		  fFCount = 0;
	char		  c;

	while (!_s->available() && t < 10) {}
	if (_s->available()) {
		stringWaiting = false;
		txtBufCharPtr = 0;
		while (_s->available() && (fFCount < 3)) {
#ifdef debugGNS
			Serial.print(txtBufCharPtr); Serial.print(" - ");
#endif
			c = _s->read();
			gotFF = (c == 0xFF);
			if (gotFF) {
				fFCount++;
			}
			else
			{
				fFCount = 0;
			}
			if (txtBufCharPtr >= txtBufSize) {
				Serial.print(c);
			}
			else
			{
				txtBufPtr[ txtBufCharPtr ] = c;
				txtBufCharPtr++;
			}
			t = 0;
			while (!_s->available() && t < 10) {}  // 10 for potential low baud rate

		}
		if (txtBufCharPtr >= txtBufSize) {
			Serial.println();
		}else
		{
			txtBufPtr[txtBufCharPtr] = '\0';
		}
	}
	stringWaiting = (fFCount == 3);
	return stringWaiting;
}

bool Nextion::respondToReply() {   //returns true if something needs responding to
	bool     needsResponse = true;
	uint16_t zz;
	uint32_t valve;
	bool	 how;

	switch (nextionEvent.id) {

		case invalidInstruction:						// Returned when instruction sent by user has failed

		case instructionSuccess:						// (ONLY SENT WHEN bkcmd = 1 or 3 )
			comdExecOk = true;

		case invalidComponentId:						// Returned when invalid Component ID or name was used

		case invalidPageId:								// Returned when invalid Page ID or name was used

		case invalidPictureId:							// Returned when invalid Picture ID was used

		case invalidFontId:								// Returned when invalid Font ID was used

		case invalidFileOperation:						// Returned when File operation fails

		case invalidCrc:								// Returned when Instructions with CRC validation fails their CRC check

		case invalidBaudRateSetting:					// Returned when invalid Baud rate was used

		case invalidWaveformIdChan:						// Returned when invalid Waveform ID or Channel # was used

		case invalidVarNameAttrib:						// Returned when invalid Variable name or invalid attribute was used

		case invalidVarOperation:						// Returned when Operation of Variable is invalid.ie: Text assignment t0.txt = abc or t0.txt = 23, 

		case assignmentFailed:							// Returned when attribute assignment failed to assign

		case EEPROMOperationFailed:						// Returned when an EEPROM Operation has failed

		case invalidQtyParams:							// Returned when the number of instruction parameters is invalid

		case ioOperationFailed:							// Returned when an IO operation has failed

		case invalidEscapeChar:							// Returned when an unsupported escape character is used

		case variableNameToLong:						// Returned when variable name is too long.Max length is 29 characters: 14 for page + “.” + 14 for component.

		case serialBufferOverflow:						// Returned when a Serial Buffer overflow occurs	Buffer will continue to receive the current instruction, all previous instructions are lost.
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
/*			Serial.println("Touch Coordinate Awake");
			Serial.print(nextionEvent.reply8.xPos);
			Serial.print(" - ");
			Serial.print(nextionEvent.reply8.yPos);
			Serial.print(" - Press(1)/Release(0) : ");
			Serial.println(nextionEvent.reply8.pressed);
*/			break;
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

			zz = nextionEvent.reply7.num[0];  // (uint16_t)nextionEvent.reply7.ans[0] * 256 + (uint16_t)nextionEvent.reply7.ans[1];
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
					if (nextionAutoBaud) {
						needsResponse = false;
					}
					break;
				case 0xFDFD: // Indicates Nextion Serial Buffer Clear
					serialBufferClear	= true;
					needsResponse		= false;
					break;
				default:
					Serial.print("Some other NumericDataEnclosed data: ");
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

void Nextion::printAnyReturnCharacters(uint32_t nextionTime, uint8_t id) {
	elapsedMillis t;
	bool		  gotFF = false;
	uint8_t		  fFCount = 0;
	char		  c;

	while (!_s->available() && t < 10) {}

	if (_s->available()) {
		Serial.println();
		Serial.print("NTime: ");
		Serial.print(nextionTime);
		Serial.print(" id:("); Serial.print(id); Serial.print(") ");
		while (_s->available()) {
			c = _s->read();
			gotFF = (c == 0xFF);
			if (gotFF) {
				fFCount++;
			}else
			{
				fFCount = 0;
			}
			Serial.print(c, HEX); Serial.print(" ");
			if (fFCount == 3) {
				gotFF = false;
				Serial.print(" -- ");
			}
			t = 0;
			while (!_s->available() && t < 10) {}

		}
		Serial.println();
	}
}
char valveId[] = { '0','1','2','3','4','5','6' };

void Nextion::turnNextionButton(uint8_t which, bool on) {
	char onChar  = '1';
	char offChar = '0';

	_s->print("page0.Sw");
	_s->print(valveId[which]);
	_s->print(".val=");
	if (on) {
		_s->print(onChar);
	}
	else
	{
		_s->print(offChar);
	}
	_s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
}

void Nextion::setHotWaterOnForMins(uint8_t howLong) {
	_s->print("HWDwnCtr="); _s->print(howLong); _s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
}

void Nextion::setTime(uint32_t time) {
	_s->print("SetTime="); _s->print(time); _s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
}

void Nextion::setNextionBaudRate(uint32_t br) {
	recoveryBaudRate = baudRate;
	baudRate		 = br;
	_s->print("baud=");
	_s->print(br);
	_s->print("\xFF\xFF\xFF");
	_s->flush();
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
	if (nextionAutoBaud) SetTeensyBaud(br);
};

void Nextion::setBackLight(uint32_t backLight) {
	if (backLight > 100) {
		backLight = 100;
	}
	_s->print("dim="); _s->print(backLight); _s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
}

#define debug2z
int32_t Nextion::getNumVarValue(const char* varName) {
	elapsedMillis n;
	bool		  ok = false;
	rep7IntType	  val;

#ifdef debug2
	Serial.print("get "); Serial.print(varName); Serial.println("\xFF\xFF\xFF");
#endif
	val.number32bitInt = -1; //=No Answer

	_s->print("get "); _s->print(varName); _s->print("\xFF\xFF\xFF");
	while ((n < 100) && (!ok)) {
		ok = getReply();
	}
	if (ok) {
		val		  = nextionEvent.reply7int;
//		tempStore = val.ans[0];	val.ans[0] = val.ans[3]; val.ans[3] = tempStore;
//		tempStore = val.ans[1];	val.ans[1] = val.ans[2]; val.ans[2] = tempStore;
	}
	return val.number32bitInt;
}

#define debugt4z
bool Nextion::getStringVarValue(const char* varName) {

	elapsedMillis n;
	bool		  ok = false;

#ifdef debugt4
	Serial.print("get "); Serial.print(varName); Serial.println("\xFF\xFF\xFF");
#endif
	_s->print("get "); _s->print(varName); _s->print("\xFF\xFF\xFF");
	while ((n < 100) && (!ok)) {
		ok = getReply();
	}
	if (ok) {
#ifdef debugt4
		Serial.print(nextionEvent.id, HEX); Serial.println("  Getting Nextion Return String");
#endif
		ok = GetNextionString();
	}
	return ok;
}

#define debugt3z
void Nextion::sendCommand(const char* command) {
#ifdef debugt3
	Serial.print(command); Serial.println("\xFF\xFF\xFF");
#endif
	_s->print(command); _s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
};

void Nextion::sendCommand(const char* command, uint32_t num) {
#ifdef debugt3
	Serial.print(command); Serial.print(num); Serial.println("\xFF\xFF\xFF");
#endif
	_s->print(command); _s->print(num); _s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
};

void Nextion::Nextion::sendCommand(const char* command, const char* txt, bool encloseText) {
#ifdef debugt3
	Serial.print(command); Serial.print(txt); Serial.println("\xFF\xFF\xFF");
#endif
	_s->print(command); 
	if (encloseText) _s->print("\""); 
	_s->print(txt);
	if (encloseText) _s->print("\"");
	_s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
};

#define debug1z
bool Nextion::setNumVarValue(const char* varName, int32_t var) {

	elapsedMillis n;
	bool		  ok = false;

#ifdef debug1
	Serial.print(varName); Serial.print("="); Serial.print(var); Serial.println("\xFF\xFF\xFF");
#endif
	_s->print(varName); _s->print("="); _s->print(var); _s->print("\xFF\xFF\xFF");
	while ((n < 100) && (!ok)) {
		ok = !getReply();
	}
#ifdef bkcmd1or3allowed
	if (ok) checkedComdCompleteOk = !checkComdComplete;
#endif
	return ok;
};

bool Nextion::turnDebugOn(bool on) {
	return setNumVarValue("debug", on);
}

bool Nextion::turnScreenDimOn(bool on) {

	return setNumVarValue("dimAllowed", on);
};

bool Nextion::setScreenDimTime(uint32_t dimTime) {
	int32_t timer;
	timer = getNumVarValue("SecondTmr.tim");
	if (timer != -1) {
		timer = dimTime * 1000 / timer;
		return setNumVarValue("dimTimer", timer);
	}else{
		return false;
	}
};

bool Nextion::setDaylightSavingOn(bool on) {
	return setNumVarValue("bst", on);
};

/********************************************************************************************
*		Set the TextBuffer to be used for Text Returned From Nextion					    *
*********************************************************************************************/
void Nextion::setTextBuffer(const char* textMessage, uint8_t textBufSize) {
	txtBufPtr  = textMessage;
	txtBufSize = textBufSize;
};

void Nextion::askSerialBufferClear() {
	serialBufferClear = false;
	_s->print("get clrBufr\xFF\xFF\xFF");
};

bool Nextion::askSerialBufferClear(uint32_t timeout) {
	elapsedMillis n;

	askSerialBufferClear();
	while (!serialBufferClear && n <= timeout) {
		isSerialBufferClear();
	}
	return serialBufferClear;
};

/********************************************************************************************
*		isSerialBufferClear() - Query answer from askSerialBufferClear() above				*
*********************************************************************************************/
bool Nextion::isSerialBufferClear() {

	if (getReply()) {
		if (respondToReply()){}
	}
	return serialBufferClear;
};

void Nextion::gotoPage(uint32_t which) {
	_s->print("page "); _s->print(which); _s->print("\xFF\xFF\xFF");
#ifdef bkcmd1or3allowed
	checkedComdCompleteOk = !checkComdComplete;
#endif
};

/********************************************************************************************
*		setBkCmdLevel(bkcmdStateType level) - Sets Nextion bkcmd value						*
*-------------------------------------------------------------------------------------------*
*		The default value is onFailure (2)													*
*		When set to onSuccess (1) and always (3) the variable comdExecOk will be set to		*
*		true on succesful completion of command. it is ignored for all other values.		*
*********************************************************************************************/
void Nextion::setBkCmdLevel(bkcmdStateType level) {
	_s->print("bkcmd="); _s->print((uint8_t)level); _s->print("\xFF\xFF\xFF"); 
#ifdef bkcmd1or3allowed
	if (level == 1 || level == 3) {
		checkedComdCompleteOk = false;
		checkComdComplete	  = true;  // indicates need to check command(s) completed
	}else
	{
		checkedComdCompleteOk = true;
		checkComdComplete	  = false;  // indicates need to check command(s) completed
	}
#else
	Serial.println("CRITICAL ERROR: bkcmd level 1 or 3 NOT allowed unless compiled with #define bkcmd1or3allowed");
#endif
	bkcmd				    = level;
};

bool Nextion::lastComdCompletedOk(uint32_t timeout) {
#ifdef bkcmd1or3allowed
	bool isOk = false;

	if (!checkComdComplete || checkedComdCompleteOk) return true;
	isOk = (getReply(timeout) && ( nextionEvent.id == instructionSuccess ));
	checkedComdCompleteOk = true;
	return isOk;
#else
	return true;
#endif
};
