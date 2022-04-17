#include "Arduino.h"
#include <Nextion.h>
#include <Stream.h>

#define debugz
Nextion::Nextion(Stream* s) 
{
	_s = s;
}

Nextion::setNextionBaudCallbackFunc setNextionBaud;
bool	 nextionAutoBaud = false;
Nextion::nextionTurnValveOnOffCallbackFunc turnValveOnOrOff;

void Nextion::begin(uint32_t br, Nextion::setNextionBaudCallbackFunc func = nullptr) {
	baudRate = br;
	if (func) {
		setNextionBaud = func;
		nextionAutoBaud = true;
	}
};
void Nextion::setValveCallBack(Nextion::nextionTurnValveOnOffCallbackFunc func) {
	turnValveOnOrOff = func;
};

bool Nextion::getReply() {

	elapsedMillis	timeout;
	uint8_t			len = 255;
	uint8_t			n;

	if (_s->available()) {

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

		if (nextionEvent.id == stringDataEnclosed) { // read string data
		}
		else
		{
			timeout = 0;
			n = 0;
			while (n < len && timeout < 10000) {
				n = _s->available();
			};
			if (n == len) {
				_s->readBytes((char*)&nextionEvent.reply8, len);
			}
			else
			{

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

	ledState = state;
	mask = 0b011;

	if (which > 0) {
		which <<= 1;					// multiply which by 2 since led sate occupies 2 bits
		ledState <<= which;				// move state - put it over the leds that need switching
		mask <<= which;				// ditto mask - as above
	}

	mask = ~mask;				// flip the bits
	ledState &= mask;				// AND ledState with mask to turn the led on/off/flash
	nextionLeds[whichLed] |= ledState;	// OR the leds with ledState - put the led value into the leds number
}

void Nextion::setNextionLeds(topMidBottmType which) {   // on the display

	switch (which) {
	case top:
		_s->print("page0.TopLed.val=");
		break;
	case mid:
		_s->print("page0.MidLed.val=");
		break;
	case bottom:
		_s->print("page0.BotmLed.val=");
		break;
	}
	_s->print(nextionLeds[which]);
	_s->print("\xFF\xFF\xFF");
}

void Nextion::clearLeds() {
	for (uint8_t n = top; n < bottom; n++) {
		nextionLeds[n] = 0;
		setNextionLeds((topMidBottmType)n);
	}
}

void Nextion::finishNextionTextTransmittion() {
	//	Serial.print("\"");
	_s->print("\"");
	_s->print("\xFF\xFF\xFF");
	_s->print("click m0,1"); _s->print("\xFF\xFF\xFF");
}

void Nextion::printTextToNextion(const char* p, bool transmit) {
	//Serial.print("page1.va0.txt=\"");
	//Serial.print(p);
	_s->print("page0.msg.txt=\"");     // was:- _s->print("page1.va0.txt=\"");
	_s->print(p);
	if (transmit) {
		finishNextionTextTransmittion();
		_s->flush();
	}
}

void Nextion::printNumericText(uint32_t num, bool transmit) {
	//	Serial.print(num);
	_s->print(num);
	if (transmit) {
		finishNextionTextTransmittion();
		_s->flush();
	}
}

void Nextion::printCommandOrErrorTextMessage(const char* commandOrError, const char* textMessage, bool transmit) {
	printTextToNextion(commandOrError, false);
	_s->print(textMessage);
	if (transmit) {
		finishNextionTextTransmittion();
		_s->flush();
	}
}
//=====================================================================================================
void Nextion::clearBuffer() {

	unsigned long amt = millis();

	while (_s->available()) {
		_s->read();
		if ((millis() - amt) > 5000) {
			Serial.println(F("runaway"));
			break;
		}
	}
}

bool Nextion::commsOk() {
	bool ok = false;

	clearBuffer();
	nextionEvent.id = 0;
	_s->print("sendme\xFF\xFF\xFF");
	_s->flush();
	delay(10);
#ifdef debugw
	delay(10);
//	Serial.print("in commsOk ");
#endif
	ok = getReply();
#ifdef debug
//	if (ok) Serial.println("OK"); else Serial.println("Doh");
#endif
	return ok;
/*	if (getReply()) {
		Serial.print("Id: "); Serial.print(nextionEvent.id,HEX);
		if (nextionEvent.id == invalidVarNameAttrib) {
			delay(10);
			_s->print("sendme\xFF\xFF\xFF");
			delay(100);
			nextionEvent.id = 0;
			getReply();
			Serial.print(" Id: "); Serial.print(nextionEvent.id,HEX);
		}
		gotit = (nextionEvent.id == currentPageNumber);
	} else {
		Serial.println("Unable to get reply");
	}

	return gotit;
*/
}

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
	_s->flush();

	if (baudRate != 9600) setNextionBaud(resetNextionBaud);

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
#ifdef debug
		Serial.println("Using Reset Baud");
#endif
		break;
	case 1:
		baud = savedBaud;
#ifdef debug
		Serial.println("Using Previous Baud");
#endif
	default:
#ifdef debug
		Serial.println("Changing Baud Rate");
#endif
		baud = br;
		break;
	}
	if (baud != 9600) {

		setNextionBaudRate(baud);

	} else {

		recoveryBaudRate = resetNextionBaud;
		baudRate		 = resetNextionBaud;
	}

	delay(500);
	return commsOk();
}

uint32_t recoveryBaudRates[] = { 2400, 4800, 9600, 19200, 31250, 38400, 57600, 115200, 230400, 250000, 256000, 512000, 921600 };
uint8_t  numBaudRates		 = sizeof(recoveryBaudRates) / sizeof(uint32_t);

uint32_t Nextion::recoverNextionComms() {
	uint8_t baudCount = 0; // numBaudRates;
	bool gotit		  = false;

	setNextionBaud(recoveryBaudRate);
	gotit = commsOk();
	if (gotit) {
		return recoveryBaudRate;
	}
	while ( !gotit && ( baudCount < numBaudRates )) {
		baudCount++;
		if (recoveryBaudRates[baudCount] != recoveryBaudRate) {  // No point in trying what we have already done.
			setNextionBaud(recoveryBaudRates[baudCount]);
			delay(10);
			gotit = commsOk();
#ifdef debug
			//		Serial.print("Id: ");
			//		Serial.println(nextionEvent.id, HEX);
#endif
		}
	}
	if (gotit) {
		recoveryBaudRate = recoveryBaudRates[baudCount];
		return recoveryBaudRate;
	} else
	{
		return 0;
	}
};

bool Nextion::respondToReply() {   //returns true if something needs responding to
	bool     needsResponse = true;
	uint16_t zz;
	uint32_t valve;
	bool	 how;

	switch (nextionEvent.id) {

	case invalidInstruction:						// = 0x00;	// bkcmd 2,3	0x00 0xFF 0xFF 0xFF		Returned when instruction sent by user has failed
		break;
	case instructionSuccess:						// = 0x01;	// bkcmd 1,3	0x01 0xFF 0xFF 0xFF		(ONLY SENT WHEN bkcmd = 1 or 3 )
		Serial.println("Instruction Success");
		break;
	case invalidComponentId:						// = 0x02;	// bkcmd 2,3	0x02 0xFF 0xFF 0xFF		Returned when invalid Component ID or name was used
		break;
	case invalidPageId:								// = 0x03;	// bkcmd 2,3	0x03 0xFF 0xFF 0xFF		Returned when invalid Page ID or name was used
		break;
	case invalidPictureId:							// = 0x04;	// bkcmd 2,3	0x04 0xFF 0xFF 0xFF		Returned when invalid Picture ID was used
		break;
	case invalidFontId:								// = 0x05;	// bkcmd 2,3	0x05 0xFF 0xFF 0xFF		Returned when invalid Font ID was used
		break;
	case invalidFileOperation:						// = 0x06;	// bkcmd 2,3	0x06 0xFF 0xFF 0xFF		Returned when File operation fails
		break;
	case invalidCrc:								// = 0x09;	// bkcmd 2,3	0x09 0xFF 0xFF 0xFF		Returned when Instructions with CRC validation fails their CRC check
		break;
	case invalidBaudRateSetting:					// = 0x11;	// bkcmd 2,3	0x11 0xFF 0xFF 0xFF		Returned when invalid Baud rate was used
		break;
	case invalidWaveformIdChan:						// = 0x12;	// bkcmd 2,3	0x12 0xFF 0xFF 0xFF		Returned when invalid Waveform ID or Channel # was used
		break;
	case invalidVarNameAttrib:						// = 0x1A;	// bkcmd 2,3	0x1A 0xFF 0xFF 0xFF		Returned when invalid Variable name or invalid attribute was used
		break;
	case invalidVarOperation:						// = 0x1B;	// bkcmd 2,3	0x1B 0xFF 0xFF 0xFF		Returned when Operation of Variable is invalid.ie: Text assignment t0.txt = abc or t0.txt = 23, 
		break;										//													Numeric assignment j0.val = ”50? or j0.val = abc
	case assignmentFailed:							// = 0x1C;	// bkcmd 2,3	0x1C 0xFF 0xFF 0xFF		Returned when attribute assignment failed to assign
		break;
	case EEPROMOperationFailed:						// = 0x1D;	// bkcmd 2,3	0x1D 0xFF 0xFF 0xFF		Returned when an EEPROM Operation has failed
		break;
	case invalidQtyParams:							// = 0x1E;	// bkcmd 2,3	0x1E 0xFF 0xFF 0xFF		Returned when the number of instruction parameters is invalid
		break;
	case ioOperationFailed:							// = 0x1F;	// bkcmd 2,3	0x1F 0xFF 0xFF 0xFF		Returned when an IO operation has failed
		break;
	case invalidEscapeChar:							// = 0x20;	// bkcmd 2,3	0x20 0xFF 0xFF 0xFF		Returned when an unsupported escape uint8_tacter is used
		break;
	case variableNameToLong:						// = 0x23;	// bkcmd 2,3	0x23 0xFF 0xFF 0xFF		Returned when variable name is too long.Max length is 29 characters: 14 for page + “.” + 14 for component.
		break;
	case serialBufferOverflow:						// = 0x24; //	3			0x24 0xFF 0xFF 0xFF		Returned when a Serial Buffer overflow occurs	Buffer will continue to receive the current instruction, all previous instructions are lost.
//		Serial.println("Serial Buffer Overflow");
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
				setNextionBaud(9600);
				needsResponse = false;
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

void Nextion::printAnyReturnCharacters(uint32_t nextionTime, uint8_t id) {
	bool	gotFF = false;
	uint8_t fFCount = 0;
	char	c;

	if (_s->available()) {
		Serial.println(nextionTime);
		Serial.print(" id: "); Serial.print(id); Serial.print(" ");
		while (_s->available()) {
			c = _s->read();
			gotFF = (c == 0xFF);
			if (gotFF) {
				fFCount++;
			}
			else
			{
				fFCount = 0;
			}
			Serial.print(c, HEX); Serial.print(" ");
			if (fFCount == 3) {
				gotFF = false;
				Serial.println();
			}
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
}

void Nextion::setHotWaterOnForMins(uint8_t howLong) {
	_s->print("page0.HWDwnCtr.val="); _s->print(howLong); _s->print("\xFF\xFF\xFF");
}

void Nextion::setTime(uint32_t time) {
	_s->print("page0.SetTime.val="); _s->print(time); _s->print("\xFF\xFF\xFF");
}

void Nextion::setNextionBaudRate(uint32_t br) {
	recoveryBaudRate = baudRate;
	baudRate		 = br;
	_s->print("baud=");
	_s->print(br);
	_s->print("\xFF\xFF\xFF");
	_s->flush();
	if (nextionAutoBaud) setNextionBaud(br);
};





