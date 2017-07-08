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
					if (!(tuningOrLearn == MIDILEARN))
					{
						tMIDIKeyboard_noteOn(&MIDIKeyboard, msgByte1, msgByte2);
						dacSendMIDIKeyboard();
					}
					else
					{
						learnMIDINote(msgByte1, msgByte2);
					}

				}
				else //to deal with note-offs represented as a note-on with zero velocity
				{
					if (!(tuningOrLearn == MIDILEARN))
					{
						tMIDIKeyboard_noteOff(&MIDIKeyboard, msgByte1);
						dacSendMIDIKeyboard();
					}
				}
				break;
			case 128:
				if (!(tuningOrLearn == MIDILEARN))
				{
					tMIDIKeyboard_noteOff(&MIDIKeyboard, msgByte1);
					dacSendMIDIKeyboard();
				}
			break;
			
			// control change
			case 176:
			if (!(tuningOrLearn == MIDILEARN))
			{
				controlChange(msgByte1,msgByte2);
			}
			else
			{
				learnMIDICC(msgByte1, msgByte2);
			}
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
	MIDIKeyboard.CCs[ctrlNum] = val;
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

void initMIDIKeys(int numVoices, BOOL pitchout)
{
		tMIDIKeyboard_init(&MIDIKeyboard, numVoices, pitchout);
		
		for(int i=0; i<12; i++)
		{
			sendDataToOutput(i, 5, 0);
		}
}

void tMIDIKeyboard_init(tMIDIKeyboard* keyboard, int numVoices, BOOL pitchOutput)
{
		keyboard->numVoices = numVoices;
		keyboard->numVoicesActive = numVoices;
		keyboard->lastVoiceToChange = 0;
		keyboard->transpose = 0;
		keyboard->trigCount = 0;
		keyboard->noPitchOutput = !pitchOutput;
		if (pitchOutput == FALSE)
		{
			keyboard->firstFreeOutput = 0;
		}
		else if (numVoices == 1)
		{
			keyboard->firstFreeOutput = 4; //in this case we add a trigger to the outputs
		}
		else
		{
			keyboard->firstFreeOutput = (3 * numVoices);
		}

		//default learned CCs and notes are just the CCs 1-128 - notes are skipped
		for (int i = 0; i < 128; i++)
		{
			keyboard->learnedCCsAndNotes[i][0] = i+1;
			keyboard->learnedCCsAndNotes[i][1] = -1;
		}

		for (int i = 0; i < 4; i ++)
		{
			keyboard->voices[i][0] = -1;
		}
		
		tNoteStack_init(&keyboard->stack, 128);
}


void tMIDIKeyboard_noteOn(tMIDIKeyboard* keyboard, int note, uint8_t vel)
{
	// if not in keymap or already on stack, dont do anything. else, add that note.
	if (tNoteStack_contains(&keyboard->stack, note) >= 0) return;
	else
	{
		tNoteStack_add(&keyboard->stack, note);

		
		BOOL found = FALSE;
		for (int i = 0; i < keyboard->numVoices; i++)
		{
			if (keyboard->voices[i][0] < 0)	// if inactive voice, give this note to voice
			{
				found = TRUE;
					
				keyboard->voices[i][0] = note;
				keyboard->voices[i][1] = vel;
				keyboard->lastVoiceToChange = i;
				keyboard->notes[note][0] = vel;
				keyboard->notes[note][1] = TRUE;
				break;
			}
		}
			
		if (!found) //steal
		{
			int whichVoice = keyboard->lastVoiceToChange;
			int oldNote = keyboard->voices[whichVoice][0];	
			keyboard->voices[whichVoice][0] = note;
			keyboard->voices[whichVoice][1] = vel;
			keyboard->notes[oldNote][1] = FALSE; //mark the stolen voice as inactive (in the second dimension of the notes array)
			
			keyboard->notes[note][0] = vel;
			keyboard->notes[note][1] = TRUE;

			
		}
	}
}

void tMIDIKeyboard_noteOff(tMIDIKeyboard* keyboard, uint8_t note)
{

	int remove_test = tNoteStack_remove(&keyboard->stack, note);

	int deactivatedVoice = -1;
	for (int i = 0; i < keyboard->numVoices; i++)
	{
		if (keyboard->voices[i][0] == note)
		{
			keyboard->voices[i][0] = -1;
			keyboard->voices[i][1] = 0;
			keyboard->lastVoiceToChange = i;
			deactivatedVoice = i;
			break;
		}
	}
		
	if ((keyboard->numVoices == 1) && (tNoteStack_getSize(&keyboard->stack) > 0))
	{
		int oldNote = tNoteStack_first(&keyboard->stack);
		keyboard->voices[0][0] = oldNote;
		keyboard->voices[0][1] = keyboard->notes[oldNote][0];
		keyboard->lastVoiceToChange = 0;
	}
	
	//grab old notes off the stack if there are notes waiting to replace the free voice
	else if (deactivatedVoice != -1)
	{
		int i = 0;

		while (1)
		{
			int otherNote = tNoteStack_get(&keyboard->stack, i++);
			if (otherNote < 0 ) break;
				
			if (keyboard->notes[otherNote][1] == FALSE) //if there is a stolen note waiting (marked inactive but on the stack)
			{
				keyboard->voices[deactivatedVoice][0] = otherNote;
				keyboard->voices[deactivatedVoice][1] = keyboard->notes[otherNote][0];
				keyboard->lastVoiceToChange = deactivatedVoice;
			}
		}
	}

	keyboard->notes[note][0] = 0;
	keyboard->notes[note][1] = FALSE;
}

void learnMIDINote(uint8_t note, uint8_t vel)
{
	BOOL alreadyFoundNote = FALSE;
	
	for (int i = 0; i < currentNumberToMIDILearn; i++)
	{
		if (MIDIKeyboard.learnedCCsAndNotes[i][1] == note)
		{
			alreadyFoundNote = TRUE;
		}
	}
	if (!alreadyFoundNote) //if it's new
	{
		MIDIKeyboard.learnedCCsAndNotes[currentNumberToMIDILearn][1] = note; //store the note number in the learned array
		MIDIKeyboard.learnedCCsAndNotes[currentNumberToMIDILearn][0] = -1; //mark the CC part of the array as not applicable
		currentNumberToMIDILearn++;
		if (currentNumberToMIDILearn <= 9)
		{
			Write7Seg(currentNumberToMIDILearn + 200);
			normal_7seg_number = currentNumberToMIDILearn + 200;
		}
		else
		{
			Write7Seg(currentNumberToMIDILearn);
			normal_7seg_number = currentNumberToMIDILearn;
		}
	}
	
}

void learnMIDICC(uint8_t ctrlnum, uint8_t val)
{
	BOOL alreadyFoundCC = FALSE;
		
	for (int i = 0; i < currentNumberToMIDILearn; i++)
	{
		if (MIDIKeyboard.learnedCCsAndNotes[i][0] == ctrlnum)
		{
			alreadyFoundCC = TRUE;
		}
	}
	if (!alreadyFoundCC) //if it's new
	{
		MIDIKeyboard.learnedCCsAndNotes[currentNumberToMIDILearn][0] = ctrlnum; //store the CC in the learned array
		MIDIKeyboard.learnedCCsAndNotes[currentNumberToMIDILearn][1] = -1; //mark the note part of the array as not applicable
		currentNumberToMIDILearn++;
		if (currentNumberToMIDILearn <= 9)
		{
			Write7Seg(currentNumberToMIDILearn + 200);
			normal_7seg_number = currentNumberToMIDILearn + 200;
		}
		else
		{
			Write7Seg(currentNumberToMIDILearn);
			normal_7seg_number = currentNumberToMIDILearn;
		}
	}
}
