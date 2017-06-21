/*
 * MIDI.c
 * This is where functions that are MIDI-specific (but not specific to device vs. host USB communication) should be placed.
 * Both the device version of the midi implementation and the host version should call these functions if they need them.
 * Created: 6/20/2017 10:24:24 PM
 *  Author: Jeff Snyder
 */ 

#include "main.h"
#include "midi.h"
#include <asf.h>


uint8_t firstMIDIMessage = 0;

uint16_t parseMIDI(uint16_t howManyNew)
{
	uint8_t ctrlByte;
	uint8_t msgByte1;
	uint8_t msgByte2;
	uint16_t i = 0;
	uint8_t endOfData = 0;
	
	if (firstMIDIMessage == 1)
	{
		firstMIDIMessage = 0;
		return 0;
	}
	while(endOfData == 0)
	{
		if (my_buf[i] > 0)
		{
			ctrlByte = my_buf[i+1];
			msgByte1 = my_buf[i+2];
			msgByte2 = my_buf[i+3];
			handleMIDIMessage(ctrlByte,msgByte1,msgByte2);
			i = i + 4;
			if (i >= howManyNew)
			{
				endOfData = 1;
			}
		}
		else
		{
			endOfData = 1;
		}
		//Write7Seg(i);
	}

	//Write7Seg(i);
	return i;
}


uint16_t sysexByteCounter = 0;
uint8_t inSysex = 0;
static uint8_t sysexBuffer[512];

void handleMIDIMessage(uint8_t ctrlByte, uint8_t msgByte1, uint8_t msgByte2)
{
	uint8_t control = ctrlByte & 0xf0;

	//uint8_t channel = ctrlByte & 0x0f;
	
	
	//lcd_clear_line(1);
	//MEMORY_printf_string("control: %u",ctrlByte);
	//lcd_clear_line(2);
	//MEMORY_printf_string("note: %u", msgByte1);
	if (!inSysex)
	{
		switch(control)
		{

			case 144:
				if (msgByte2)
				{
					tKeyboard_noteOn(&midiKeyboard, msgByte1, msgByte2);
					dacSendKeyboard(InstrumentFull);
				}
				else //to deal with note-offs represented as a note-on with zero velocity
				{
					tKeyboard_noteOff(&midiKeyboard, msgByte1);
					dacSendKeyboard(InstrumentFull);
				}
				break;
			case 128:
				tKeyboard_noteOff(&midiKeyboard, msgByte1);
				dacSendKeyboard(InstrumentFull);
			break;
			// control change
			case 176:
			//controlChange(msgByte1,msgByte2);
			//LED_On(LED2);
			//midiVol();
			break;
			// program change
			case 192:
			//programChange(msgByte1);
			break;
			case 240:
			startSysexMessage(msgByte1, msgByte2);
			break;
			default:
			break;
		}
	}
	else //otherwise, we are in the middle of a sysex message, let's keep pluggin them bytes into the message to parse once it's finished
	{
		if (ctrlByte == 0xf7)
		{
			parseSysex();
		}
		else
		{
			sysexBuffer[sysexByteCounter] = ctrlByte;
			sysexByteCounter++;
		}
		if (msgByte1 == 0xf7)
		{
			parseSysex();
		}
		else
		{
			sysexBuffer[sysexByteCounter] = msgByte1;
			sysexByteCounter++;
		}
		if (msgByte2 == 0xf7)
		{
			parseSysex();
		}
		else
		{
			sysexBuffer[sysexByteCounter] = msgByte2;
			sysexByteCounter++;
		}
	}
}

void parseSysex(void)
{
	int tuningPresetToStore = 0;
	int fullPresetToStore = 0;
	
	//check which kind of sysex message it is (tuning or preset storage)
	if (sysexBuffer[0] == 1) //it's tuning
	{
		tuningPresetToStore = sysexBuffer[1];
		
		//iterate through the sysex array and do what we need to do with the message
		for (int i = 2; i < sysexByteCounter; i++) //only go up to the element right before the "end" message happened
		{
			
		}
		
	}
	else // it's full preset storage
	{
		fullPresetToStore = sysexBuffer[1];
		
		//iterate through the sysex array and do what we need to do with the message
		for (int i = 2; i < sysexByteCounter; i++) //only go up to the element right before the "end" message happened
		{
			
		}
	}

	//reset the sysex variables
	inSysex = 0;
	sysexByteCounter = 0;
}

void startSysexMessage(int msgByte1, int msgByte2)
{
	if (msgByte1 == 0x7e) // it's a non realtime sysex
	{
		if (msgByte2 == 0x49) // it's for the MantaMate
		{
			//let's start watching the incoming bytes until we get a message that signals the end of the midi sysex message
			sysexByteCounter = 0;
			inSysex = 1;
		}
	}
}