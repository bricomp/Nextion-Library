# Nextion-Library
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
