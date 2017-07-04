/*
 * MIDI.c
 * This is where functions that are MIDI-specific (but not specific to device vs. host USB communication) should be placed.
 * Both the device version of the midi implementation and the host version should call these functions if they need them.
 * Created: 6/20/2017 10:24:24 PM
 *  Author: Jeff Snyder
 */ 

#include "midi.h"
#include "main.h"
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
	}

	return i;
}


uint16_t sysexByteCounter = 0;
uint8_t inSysex = 0;
uint8_t sysexBuffer[1024];

void handleMIDIMessage(uint8_t ctrlByte, uint8_t msgByte1, uint8_t msgByte2)
{
	uint8_t control = ctrlByte & 0xf0;
	
	if (!inSysex)
	{
		switch(control)
		{

			case 144:
				if (msgByte2)
				{
					tMIDIKeyboard_noteOn(&MIDIKeyboard, msgByte1, msgByte2);
					dacSendMIDIKeyboard();
				}
				else //to deal with note-offs represented as a note-on with zero velocity
				{
					tMIDIKeyboard_noteOff(&MIDIKeyboard, msgByte1);
					dacSendMIDIKeyboard();
				}
				break;
			case 128:
				tMIDIKeyboard_noteOff(&MIDIKeyboard, msgByte1);
				dacSendMIDIKeyboard();
			break;
			
			// control change
			case 176:
			//controlChange(msgByte1,msgByte2);
			
			break;
			// program change
			case 192:
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

//For the MIDI keyboards knobs
void controlChange(uint8_t ctrlNum, uint8_t val)
{
	switch (ctrlNum)
	{
		
		case 0:
		dacsend(0, 0, val * 32);
		break;
		
		case 1:
		dacsend(1, 0, val * 32);
		break;
		
		case 2:
		dacsend(2, 0, val * 32);
		break;
		
		case 3:
		dacsend(3, 0, val * 32);
		break;
		
		case 4:
		dacsend(0, 1, val * 32);
		break;
		
		case 5:
		dacsend(1, 1, val * 32);
		break;
		
		case 6:
		dacsend(2, 1, val * 32);
		break;
		
		case 7:
		dacsend(3, 1, val * 32);
		break;
		
		
		case 31:
		
		break;
		case 32:
		
		break;

		
		default:
		break;
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
			tuning8BitBuffer[i -2] = sysexBuffer[i];
		}
		initiateStoringTuningToExternalMemory(tuningPresetToStore);
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

void sendSysexSaveConfim(void)
{
	LED_On(PRESET_SAVE_LED);
	mySendBuf[0] = 0x09;
	mySendBuf[1] = 0xF0;
	mySendBuf[2] = 0x49;
	mySendBuf[3] = 0xF7;
	ui_my_midi_send();
}

void initMIDIArpeggiator(void)
{
	
}

void initMIDIKeys(int numVoices)
{
		takeover = TRUE;
		takeoverType = KeyboardInstrument;
		tMIDIKeyboard_init(&MIDIKeyboard, numVoices);
		
		for(int i=0; i<12; i++)
		{
			sendDataToOutput(i, 5, 0);
		}
}

void tMIDIKeyboard_init(tMIDIKeyboard* const keyboard, int numVoices)
{

		keyboard->numVoices = numVoices;
		keyboard->numVoicesActive = numVoices;
		keyboard->lastVoiceToChange = 0;
		keyboard->transpose = 0;
		keyboard->trigCount = 0;
		
		for (int i = 0; i < 4; i ++)
		{
			keyboard->voices[i][0] = -1;
		}
		
		tNoteStack_init(&keyboard->stack, 128);
}


void tMIDIKeyboard_noteOn(tMIDIKeyboard* const keyboard, int note, uint8_t vel)
{
	// if not in keymap or already on stack, dont do anything. else, add that note.
	if (tNoteStack_contains(&keyboard->stack, note) >= 0) return;
	else
	{
		tNoteStack_add(&keyboard->stack, note);

		BOOL found = FALSE;
		for (int i = 0; i < keyboard->numVoices; i++)
		{
			if (keyboard->voices[i][0] < 0)	// if inactive voice, give this hex to voice
			{
				found = TRUE;
					
				keyboard->voices[i][0] = note;
				keyboard->voices[i][1] = vel;
				keyboard->lastVoiceToChange = i;
					
				break;
			}
		}
			
		if (!found) //steal
		{
			int whichVoice = keyboard->lastVoiceToChange;
			int oldNote = keyboard->voices[whichVoice][0];	
			keyboard->voices[whichVoice][0] = note;

		}
	}
}

void tMIDIKeyboard_noteOff(tMIDIKeyboard* const keyboard, uint8_t note)
{

	tNoteStack_remove(&keyboard->stack, note);
		
	int deactivatedVoice = -1;
	for (int i = 0; i < keyboard->numVoices; i++)
	{
		if (keyboard->voices[i][0] == note)
		{
			keyboard->voices[i][0] = -1;
			keyboard->lastVoiceToChange = i;
				
			deactivatedVoice = i;
			break;
		}
	}
		
	if (keyboard->numVoices == 1 && tNoteStack_getSize(&keyboard->stack) > 0)
	{
		int oldNote = tNoteStack_first(&keyboard->stack);
		keyboard->voices[0][0] = oldNote;
		keyboard->lastVoiceToChange = 0;
			
	}
	else if (deactivatedVoice != -1)
	{
		int i = 0;

		while (1)
		{
			int otherNote = tNoteStack_get(&keyboard->stack, i++);
			if (otherNote < 0 ) break;
				
			/*
			if (keyboard->hexes[otherNote].active == FALSE)
			{
				keyboard->voices[deactivatedVoice] = otherNote;
					
				keyboard->lastVoiceToChange = deactivatedVoice;
				keyboard->hexes[otherNote].active = TRUE;
			}
			*/
		}
	}
}