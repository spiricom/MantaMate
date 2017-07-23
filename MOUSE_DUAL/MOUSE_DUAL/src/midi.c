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
int32_t wholeStepDACDivider = 7500; // the size of a DAC whole step is 1092.26  --- the size of a full pitch bend in one direction is 8192. 8192/1092.26 is 7.5  -- Multiply pitch bend value by 1000, then divide by 7500 to get in whole tone range.


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
					if (!(tuningOrLearn == MIDILEARN_AND_LENGTH))
					{
						tMIDIKeyboard_noteOn(&MIDIKeyboard, msgByte1, msgByte2);
						if (MIDIKeyboard.playMode != ArpMode)
						{
							dacSendMIDIKeyboard();
						}

					}
					else
					{
						learnMIDINote(msgByte1, msgByte2);
					}

				}
				else //to deal with note-offs represented as a note-on with zero velocity
				{
					if (!(tuningOrLearn == MIDILEARN_AND_LENGTH))
					{
						tMIDIKeyboard_noteOff(&MIDIKeyboard, msgByte1);
						if (MIDIKeyboard.playMode != ArpMode)
						{
							dacSendMIDIKeyboard();
						}
					}
				}
				break;
			case 128:
				if (!(tuningOrLearn == MIDILEARN_AND_LENGTH))
				{
					tMIDIKeyboard_noteOff(&MIDIKeyboard, msgByte1);
					if (MIDIKeyboard.playMode != ArpMode)
					{
						dacSendMIDIKeyboard();
					}
				}
			break;
			
			case 224:
			tMIDIKeyboard_pitchBend(&MIDIKeyboard, msgByte1, msgByte2);
			//dacSendMIDIKeyboard();
			break;
			
			// control change
			case 176:
			if (!(tuningOrLearn == MIDILEARN_AND_LENGTH))
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
	MIDIKeyboard.CCs[ctrlNum] = val << 9;
	MIDIKeyboard.CCsRaw[ctrlNum] = val;
	if (!MIDIKeyboard.learned)
	{
		if ((ctrlNum >=0) && (ctrlNum <=31))
		{
			MIDIKeyboard.CCs[ctrlNum] = ((val << 9) + (MIDIKeyboard.CCsRaw[ctrlNum + 32]<<2) +  (MIDIKeyboard.CCsRaw[ctrlNum + 64] >> 5));
		}
		
		if ((ctrlNum >=32) && (ctrlNum <=63))
		{
			MIDIKeyboard.CCs[ctrlNum-32] = ((MIDIKeyboard.CCsRaw[ctrlNum - 32] << 9) + (val << 2) + (MIDIKeyboard.CCsRaw[ctrlNum + 32] >> 5));
		}
		
		if ((ctrlNum >=64) && (ctrlNum <=96))
		{
			MIDIKeyboard.CCs[ctrlNum-64] = ((MIDIKeyboard.CCsRaw[ctrlNum - 64] << 9) + (MIDIKeyboard.CCsRaw[ctrlNum - 32] << 2 ) + (val >> 5));
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
			if (i < 522) //make sure it doesn't go out of the bounds of the array
			{
				tuning8BitBuffer[i -2] = sysexBuffer[i];
			}
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
	tMIDIKeyboard* keyboard = &MIDIKeyboard; //check that this is correct
	
	keyboard->numVoices = 1;
	keyboard->numVoicesActive = 1;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount[0] = 0;
	keyboard->trigCount[1] = 0;
	keyboard->trigCount[2] = 0;
	keyboard->trigCount[3] = 0;
	keyboard->learned = FALSE;
	keyboard->pitchOutput = TRUE;
	keyboard->gatesOrTriggers = GATES;

	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 128;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeOrderTouchForward; //let's default to order touched
	keyboard->playMode = ArpMode;
	keyboard->currentNote = -1;

	keyboard->firstFreeOutput = 4; //in this case we add a trigger to the outputs

	//default learned CCs and notes are just the CCs 1-128 - notes are skipped
	for (int i = 0; i < 128; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = i+1;
		keyboard->learnedCCsAndNotes[i][1] = 255;
		MIDIKeyboard.CCsRaw[i] = 0;
		MIDIKeyboard.CCs[i] = 0;
		keyboard->notes[i][0] = 0;
		keyboard->notes[i][1] = 0;
	}

	for (int i = 0; i < 4; i ++)
	{
		keyboard->voices[i][0] = -1;
	}

	tNoteStack_init(&keyboard->stack, 128);

	tNoteStack_init(&keyboard->orderStack, 128);	
}

void initMIDIKeys(int numVoices, BOOL pitchout)
{
	tMIDIKeyboard_init(&MIDIKeyboard, numVoices, pitchout);
	
	for(int i=0; i<12; i++)
	{
		sendDataToOutput(i, 5, 0);
	}
}

void initMIDIAllCV(void)
{
	tMIDIKeyboard* keyboard = &MIDIKeyboard; //check that this is correct
	keyboard->numVoices = 1;
	keyboard->numVoicesActive = 1;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount[0] = 0;
	keyboard->trigCount[1] = 0;
	keyboard->trigCount[2] = 0;
	keyboard->trigCount[3] = 0;
	keyboard->learned = FALSE;
	keyboard->pitchOutput = FALSE;
	keyboard->gatesOrTriggers = GATES;
	
	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 128;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeUp;
	keyboard->playMode = TouchMode;
	keyboard->currentNote = -1;
	
	keyboard->firstFreeOutput = 0;

	//default learned CCs and notes are just the CCs 1-128 - notes are skipped
	for (int i = 0; i < 128; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = i+1;
		keyboard->learnedCCsAndNotes[i][1] = 255;
		MIDIKeyboard.CCsRaw[i] = 0;
		MIDIKeyboard.CCs[i] = 0;
		keyboard->notes[i][0] = 0;
		keyboard->notes[i][1] = 0;
	}
}

void initMIDIAllGates(void)
{
	tMIDIKeyboard* keyboard = &MIDIKeyboard; //check that this is correct
	keyboard->numVoices = 1;
	keyboard->numVoicesActive = 1;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount[0] = 0;
	keyboard->trigCount[1] = 0;
	keyboard->trigCount[2] = 0;
	keyboard->trigCount[3] = 0;
	keyboard->learned = FALSE;
	keyboard->pitchOutput = FALSE;
	keyboard->gatesOrTriggers = GATES;
		
	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 128;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeUp;
	keyboard->playMode = TouchMode;
	keyboard->currentNote = -1;

	keyboard->firstFreeOutput = 0;

	//clear out default ccs and notes
	for (int i = 0; i < 128; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = 255;
		keyboard->learnedCCsAndNotes[i][1] = 255;
		MIDIKeyboard.CCsRaw[i] = 0;
		MIDIKeyboard.CCs[i] = 0;
		keyboard->notes[i][0] = 0;
		keyboard->notes[i][1] = 0;
	}
	//add in midi notes 60-71 for gates   //would be nice to set it up to do full chromatic instead by pitch class...
	for (int i = 0; i < 12; i++)
	{
		keyboard->learnedCCsAndNotes[i][1] = 60 + i;
	}
}

void initMIDIAllTriggers(void)
{
	tMIDIKeyboard* keyboard = &MIDIKeyboard; //check that this is correct
	keyboard->numVoices = 1;
	keyboard->numVoicesActive = 1;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount[0] = 0;
	keyboard->trigCount[1] = 0;
	keyboard->trigCount[2] = 0;
	keyboard->trigCount[3] = 0;
	keyboard->learned = FALSE;
	keyboard->pitchOutput = FALSE;
	keyboard->gatesOrTriggers = TRIGGERS;
		
	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 128;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeUp;
	keyboard->playMode = TouchMode;
	keyboard->currentNote = -1;

	keyboard->firstFreeOutput = 0;

	//clear out default ccs and notes
	for (int i = 0; i < 128; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = 255;
		keyboard->learnedCCsAndNotes[i][1] = 255;
		MIDIKeyboard.CCsRaw[i] = 0;
		MIDIKeyboard.CCs[i] = 0;
		keyboard->notes[i][0] = 0;
		keyboard->notes[i][1] = 0;
	}
	//add in midi notes 60-71 for gates   //would be nice to set it up to do full chromatic instead by pitch class...
	for (int i = 0; i < 12; i++)
	{
		keyboard->learnedCCsAndNotes[i][1] = 60 + i;
	}
}

void initMIDICVAndGates(void)
{
	tMIDIKeyboard* keyboard = &MIDIKeyboard; //check that this is correct
	keyboard->numVoices = 1;
	keyboard->numVoicesActive = 1;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount[0] = 0;
	keyboard->trigCount[1] = 0;
	keyboard->trigCount[2] = 0;
	keyboard->trigCount[3] = 0;
	keyboard->learned = FALSE;
	keyboard->pitchOutput = FALSE;
	keyboard->gatesOrTriggers = GATES;
		
	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 128;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeUp;
	keyboard->playMode = TouchMode;
	keyboard->currentNote = -1;

	keyboard->firstFreeOutput = 0;

	//clear out default ccs and notes
	for (int i = 0; i < 128; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = 255;
		keyboard->learnedCCsAndNotes[i][1] = 255;
		MIDIKeyboard.CCsRaw[i] = 0;
		MIDIKeyboard.CCs[i] = 0;
		keyboard->notes[i][0] = 0;
		keyboard->notes[i][1] = 0;
	}
	//add in CCs 1-6
	for (int i = 0; i < 6; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = i + 1;
	}
	//add in midi notes 60-66 for gates   //would be nice to set it up to do full chromatic instead by pitch class...
	for (int i = 0; i < 6; i++)
	{
		keyboard->learnedCCsAndNotes[i + 6][1] = 60 + i;
	}
}

void initMIDICVAndTriggers(void)
{
	tMIDIKeyboard* keyboard = &MIDIKeyboard; //check that this is correct
	keyboard->numVoices = 1;
	keyboard->numVoicesActive = 1;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount[0] = 0;
	keyboard->trigCount[1] = 0;
	keyboard->trigCount[2] = 0;
	keyboard->trigCount[3] = 0;
	keyboard->learned = FALSE;
	keyboard->pitchOutput = FALSE;
	keyboard->gatesOrTriggers = TRIGGERS;
	
	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 128;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeUp;
	keyboard->playMode = TouchMode;
	keyboard->currentNote = -1;

	keyboard->firstFreeOutput = 0;

	//clear out default ccs and notes
	for (int i = 0; i < 128; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = 255;
		keyboard->learnedCCsAndNotes[i][1] = 255;
		MIDIKeyboard.CCsRaw[i] = 0;
		MIDIKeyboard.CCs[i] = 0;
		keyboard->notes[i][0] = 0;
		keyboard->notes[i][1] = 0;
	}
	//add in CCs 1-6
	for (int i = 0; i < 6; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = i + 1;
	}
	//add in midi notes 60-66 for gates   //would be nice to set it up to do full chromatic instead by pitch class...
	for (int i = 0; i < 6; i++)
	{
		keyboard->learnedCCsAndNotes[i + 6][1] = 60 + i;
	}	
}

void tMIDIKeyboard_init(tMIDIKeyboard* keyboard, int numVoices, int pitchOutput)
{
	keyboard->numVoices = numVoices;
	keyboard->numVoicesActive = numVoices;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount[0] = 0;
	keyboard->trigCount[1] = 0;
	keyboard->trigCount[2] = 0;
	keyboard->trigCount[3] = 0;
	keyboard->learned = FALSE;
	keyboard->pitchOutput = pitchOutput;
	keyboard->gatesOrTriggers = GATES;
	
	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 128;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeUp;
	keyboard->playMode = TouchMode;
	keyboard->currentNote = -1;

	if (keyboard->pitchOutput == 0)
	{
		keyboard->firstFreeOutput = 0;
	}
	else
	{
		if (numVoices == 1)
		{
			keyboard->firstFreeOutput = 4; //in this case we add a trigger to the outputs
		}
		else
		{
			keyboard->firstFreeOutput = (3 * numVoices);
		}
	}

	//default learned CCs and notes are just the CCs 1-128 - notes are skipped
	for (int i = 0; i < 128; i++)
	{
		keyboard->learnedCCsAndNotes[i][0] = i+1;
		keyboard->learnedCCsAndNotes[i][1] = 255;
		MIDIKeyboard.CCsRaw[i] = 0;
		MIDIKeyboard.CCs[i] = 0;
		keyboard->notes[i][0] = 0;
		keyboard->notes[i][1] = 0;
	}

	for (int i = 0; i < 4; i ++)
	{
		keyboard->voices[i][0] = -1;
	}
		
	tNoteStack_init(&keyboard->stack, 128);
	
	tNoteStack_init(&keyboard->orderStack, 128);

}


//instead of inmcluding in dacsend, should have a separate pitch bend ramp, that is added when the ramps are ticked and sent to DAC
void tMIDIKeyboard_pitchBend(tMIDIKeyboard* keyboard, uint8_t lowbyte, uint8_t highbyte)
{
	int32_t tempPitch = (highbyte << 7) +  lowbyte;
	tempPitch = (tempPitch - 8192) * 1000;
	keyboard->pitchBend = (tempPitch / wholeStepDACDivider);
	tIRampSetDest(&pitchBendRamp, keyboard->pitchBend);
}




void tMIDIKeyboard_noteOn(tMIDIKeyboard* keyboard, int note, uint8_t vel)
{
	// if not in keymap or already on stack, dont do anything. else, add that note.
	if (tNoteStack_contains(&keyboard->stack, note) >= 0) return;
	else
	{
		tMIDIKeyboard_orderedAddToStack(keyboard, note);
		tNoteStack_add(&keyboard->stack, note);
		
		if (keyboard->playMode != ArpMode)
		{
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
					keyboard->notes[note][1] = 1;
					break;
				}
			}
			
			if (!found) //steal
			{
				int whichVoice = keyboard->lastVoiceToChange;
				int oldNote = keyboard->voices[whichVoice][0];	
				keyboard->voices[whichVoice][0] = note;
				keyboard->voices[whichVoice][1] = vel;
				keyboard->notes[oldNote][1] = 0; //mark the stolen voice as inactive (in the second dimension of the notes array)
			
				keyboard->notes[note][0] = vel;
				keyboard->notes[note][1] = TRUE;
			}
		}
	}
}

void tMIDIKeyboard_noteOff(tMIDIKeyboard* keyboard, uint8_t note)
{

	tNoteStack_remove(&keyboard->stack, note);
	tNoteStack_remove(&keyboard->orderStack, note);
	keyboard->notes[note][0] = 0;
	keyboard->notes[note][1] = 0;
	
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
		//monophonic handling
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
				
			if (keyboard->notes[otherNote][1] == 0) //if there is a stolen note waiting (marked inactive but on the stack)
			{
				keyboard->voices[deactivatedVoice][0] = otherNote; //set the newly free voice to use the old stolen note
				keyboard->voices[deactivatedVoice][1] = keyboard->notes[otherNote][0]; // set the velocity of the voice to be the velocity of that note
				keyboard->notes[otherNote][1] = 1; //mark that it is no longer stolen and is now active
				keyboard->lastVoiceToChange = deactivatedVoice; // mark the voice that was just changed as the last voice to change
			}
		}
	}


}

void learnMIDINote(uint8_t note, uint8_t vel)
{
	BOOL alreadyFoundNote = FALSE;
	MIDIKeyboard.learned = TRUE;
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
		MIDIKeyboard.learnedCCsAndNotes[currentNumberToMIDILearn][0] = 255; //mark the CC part of the array as not applicable
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
	MIDIKeyboard.learned = TRUE;
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
		MIDIKeyboard.learnedCCsAndNotes[currentNumberToMIDILearn][1] = 255; //mark the note part of the array as not applicable
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


//arp stuff

void tMIDIKeyboard_nextNote(tMIDIKeyboard* const keyboard)
{
	int random;
	ArpModeType type = keyboard->arpModeType;
	
	tNoteStack* ns = ((type >= ArpModeUp && type <= ArpModeUpDown) || type == ArpModeRandomWalk) ? &keyboard->orderStack :  &keyboard->stack;

	if (ns->size == 1)
	{
		keyboard->phasor = 0;
	}
	else if (type == ArpModeUp || type == ArpModeOrderTouchBackward)
	{
		if (++keyboard->phasor >= ns->size) keyboard->phasor = 0;
	}
	else if (type == ArpModeDown || type == ArpModeOrderTouchForward)
	{
		if (--keyboard->phasor < 0) keyboard->phasor = (ns->size-1);
	}
	else if (type == ArpModeUpDown || type == ArpModeOrderTouchForwardBackward)
	{
		if (keyboard->up)
		{
			if (++keyboard->phasor >= ns->size)
			{
				keyboard->phasor = ns->size-2;
				keyboard->up = FALSE;
			}
		}
		else // down
		{
			if (--keyboard->phasor < 0)
			{
				keyboard->phasor = 1;
				keyboard->up = TRUE;
			}
		}
	}
	else if (type == ArpModeRandomWalk)
	{
		random = (rand() >> 15) & 1;
		if (random > 0)
		{
			keyboard->phasor++;
		}
		else
		{
			keyboard->phasor--;
		}
		
		if (keyboard->phasor >= ns->size)
		{
			keyboard->phasor = ns->size - 2;
		}
		else if (keyboard->phasor < 0)
		{
			keyboard->phasor = 1;
		}
	}
	else if (type == ArpModeRandom)
	{
		keyboard->phasor = ((rand() >> 15) % (ns->size ? ns->size : 1));
	}
		
	keyboard->currentNote =  tNoteStack_get(ns, keyboard->phasor);
}

void tMIDIKeyboard_orderedAddToStack(tMIDIKeyboard* thisKeyboard, uint8_t noteVal)
{
	
	uint8_t j;
	int myPitch, thisPitch, nextPitch;
	
	tNoteStack* ns = &thisKeyboard->orderStack;
	
	int whereToInsert = 0;

	for (j = 0; j < ns->size; j++)
	{
		myPitch = noteVal;
		thisPitch = ns->notestack[j];
		nextPitch = ns->notestack[j+1];
		
		if (myPitch > thisPitch)
		{
			if ((myPitch < nextPitch) || (nextPitch == -1))
			{
				whereToInsert = j+1;
				break;
			}
		}
	}
	
	//first move notes that are already in the stack one position to the right
	for (j = ns->size; j > whereToInsert; j--)
	{
		ns->notestack[j] = ns->notestack[(j - 1)];
	}

	//then, insert the new note into the front of the stack
	ns->notestack[whereToInsert] =  noteVal;

	ns->size++;
	
}

void MIDIKeyboardStep(void)
{
	tMIDIKeyboard* keyboard = &MIDIKeyboard;
	if (keyboard->playMode != ArpMode) return;
	
	tMIDIKeyboard_nextNote(keyboard);
	
	if (++keyboard->currentVoice == keyboard->numVoices) keyboard->currentVoice = 0;
	
	dacSendMIDIKeyboard();
}

void dacSendMIDIKeyboard(void)
{
	tMIDIKeyboard* keyboard;
	keyboard = &MIDIKeyboard;
	
	if (keyboard->pitchOutput > 0)
	{
		if (keyboard->playMode == ArpMode)
		{
			int newNote = keyboard->currentNote;
			if (newNote >= 0)
			{
				tIRampSetDest(&out[0][CVKPITCH+3*keyboard->currentVoice], lookupDACvalue(&myGlobalTuningTable, newNote, keyboard->transpose));

				tIRampSetDest(&out[0][CVKTRIGGER-2+(3*keyboard->currentVoice)], 65535);
				keyboard->trigCount[keyboard->currentVoice] = 3;
			}
			
			if (keyboard->stack.size <= 0)
			{
				tIRampSetDest(&out[0][CVKTRIGGER-2+3*keyboard->currentVoice], 0);
			}
		}
		else
		{
			
			
			for (int i = 0; i < keyboard->numVoices; i++)
			{
				int note = keyboard->voices[i][0];
				int velocity = keyboard->voices[i][1];
				if (note >= 0)
				{
					int32_t tempDACPitch = (int32_t)(lookupDACvalue(&myGlobalTuningTable, note, keyboard->transpose));
					tIRampSetTime(&out[(int)(i/2)][((i*3) % 6)+CVKPITCH], globalPitchGlide);
					tIRampSetDest(&out[(int)(i/2)][((i*3) % 6)+CVKPITCH], tempDACPitch);
					tIRampSetTime(&out[(int)(i/2)][((i*3) % 6)+CVKGATE], 0);
					tIRampSetDest(&out[(int)(i/2)][((i*3) % 6)+CVKGATE], 4095 );
					tIRampSetTime(&out[(int)(i/2)][((i*3) % 6)+CVKVEL], 3);
					tIRampSetDest(&out[(int)(i/2)][((i*3) % 6)+CVKVEL],velocity << 5);
					//if we are in mono mode, then we have room for a trigger output, too
					if ((keyboard->numVoices == 1) && (prevSentPitch != (note + keyboard->transpose))) //if we are in mono mode, then we have room for a trigger output, too
					{
						tIRampSetDest(&out[0][CVKTRIGGER], 65535);
						keyboard->trigCount[0] = 3;
					}
					//this is to avoid retriggers on the same note when other notes are released in monophonic mode
					prevSentPitch = note + keyboard->transpose;
				}
				else
				{
					tIRampSetDest(&out[(int)(i/2)][((i*3) % 6)+CVKGATE], 0);
					tIRampSetDest(&out[(int)(i/2)][((i*3) % 6)+CVKVEL], 0);
					//let the monophonic trigger handling know there has been a note-off event
					prevSentPitch = -1;
				}
			}
		}
	}
}

void tMIDIKeyboard_encode(tMIDIKeyboard* const keyboard, uint8_t* buffer)
{
		buffer[0] = keyboard->numVoices;
		buffer[1] = keyboard->transpose >> 8;
		buffer[2] = keyboard->transpose & 0xff;
		buffer[3] = keyboard->playMode;
		buffer[4] = keyboard->arpModeType;
		buffer[5] = keyboard->learned;
		buffer[6] = keyboard->firstFreeOutput;
		buffer[7] = keyboard->gatesOrTriggers;
		int index_count = 8;
		
		for (int i = 0; i < 128; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				buffer[index_count] = keyboard->learnedCCsAndNotes[i][j];
				index_count++;
			}
		}
}

// 128*2 + 7 = 263
void tMIDIKeyboard_decode(tMIDIKeyboard* const keyboard, uint8_t* buffer)
{
	keyboard->numVoices = buffer[0];
	keyboard->transpose = (buffer[1] << 8) +  buffer[2];
	keyboard->playMode = buffer[3];
	keyboard->arpModeType = buffer[4];
	keyboard->learned = buffer[5];
	keyboard->firstFreeOutput = buffer[6];
	keyboard->gatesOrTriggers = buffer[7];
	int index_count = 8;
		
	for (int i = 0; i < 128; i++)
	{
		for (int j = 0; j < 2; j++)
		{
			keyboard->learnedCCsAndNotes[i][j] = buffer[index_count];
			index_count++;
		}
	}
}