/*
 * note_process.c
 *
 * Created: 7/22/2015 4:47:37 PM
 *  Author: Jeff Snyder / Mike Mulshine / Elaine Chou
 */ 

#include "manta_keys.h"


uint8_t applyNoteMap(MantaMap whichmap, uint8_t noteVal)
{

	uint8_t note = 0;
	switch(whichmap)
	{
		case WickiHaydenMap: note = whmap[noteVal]; break;    // wicki-hayden
		case HarmonicMap: note = harmonicmap[noteVal]; break;  // harmonic
		case PianoMap: note = pianomap[noteVal]; break;		// piano map
		default: note = noteVal; break;                     // no map
	}
	return note;
}	
	

unsigned short lookupDACvalue(tTuningTable* myTable, uint16_t noteVal, signed int transpose)
{
	uint16_t myNote = (noteVal + transpose);
		
	if (myNote >= 128)
	{
		uint16_t myOctave = (myNote / myTable->cardinality);
		uint16_t myPitchClass = (myNote % myTable->cardinality);
		return (myTable->tuningDACTable[myPitchClass] + (6553 * myOctave));
	}
	else
	{
		return myTable->tuningDACTable[myNote];
	}
}


void resetMantaUI(void)
{
	for (int i = 0; i < 48; i++) manta_set_LED_hex(i, Off);
	for (int i = 0; i < 2; i++)  manta_set_LED_slider(i, 0);
	for (int i = 0; i < 4; i++)  manta_set_LED_button(i, Off);
	
}

void initMantaKeys(int numVoices)
{
	if (numVoices < 2)
	{
		for (int i = 0; i < NUM_INST; i++)
		{
			manta[i].type = KeyboardInstrument;
			
			tKeyboard* keyboard = &manta[i].keyboard;
			
			keyboard->numVoices = 1;
		}
	}
	else // Takeover mode
	{
		if (!takeover && (manta[currentInstrument].keyboard.playMode == ArpMode))
		{
			fullKeyboard.playMode = ArpMode;
			fullKeyboard.arpModeType = manta[currentInstrument].keyboard.arpModeType;
		}
		
		takeover = TRUE;
		
		takeoverType = KeyboardInstrument;
		
		fullKeyboard.numVoices = numVoices;
	}
	
	for(int i=0; i<12; i++)
	{
		sendDataToOutput(i, 5, 0);
	}
	
	resetMantaUI();
}



void touchHexmapEdit(int hex, uint8_t weight)
{
	if (currentHexmapEditHex < 0)
	{
		currentHexmapEditHex = hex;
		
		tKeyboard_noteOn(hexmapEditKeyboard, hex, weight);
		dacSendKeyboard(hexmapEditInstrument);
		
		currentHexmapEditPitch = tKeyboard_getCurrentNoteForHex(hexmapEditKeyboard, currentHexmapEditHex);
		
		if (currentHexmapEditHex == lastHexmapEditHex)
		{
			MantaLEDColor color = hexmapEditKeyboard->hexes[currentHexmapEditHex].color;
			
			if (color == Off)        color = Amber;
			else if (color == Amber) color = Red;
			else if (color == Red)   color = Off;
			else					 color = Amber;
			
			hexmapEditKeyboard->hexes[currentHexmapEditHex].color = color;
			
			manta_set_LED_hex(currentHexmapEditHex, color);
		}
		
		Write7Seg(currentHexmapEditPitch%100);
		
		displayState = HexmapPitchSelect;
	}
}

void releaseHexmapEdit(int hex)
{
	if (currentHexmapEditHex == hex) 
	{
		tKeyboard_noteOff(hexmapEditKeyboard, hex);
		dacSendKeyboard(hexmapEditInstrument);
		
		lastHexmapEditHex = hex;
		currentHexmapEditHex = -1;
		
		Write7Seg(255);
		
		displayState = UpDownSwitchBlock;
	}
}

void touchKeyboardHex(int hex, uint8_t weight)
{
	if (!takeover)
	{ 
		if (manta[currentInstrument].type == KeyboardInstrument)
		{
			tKeyboard_noteOn(&manta[currentInstrument].keyboard, hex, weight);
			manta_set_LED_hex(hex, (manta[currentInstrument].keyboard.hexes[hex].pitch >= 0) ? ((manta[currentInstrument].keyboard.hexes[hex].color == Off) ? Amber : Off) : Off);
			
			if (manta[currentInstrument].keyboard.playMode == TouchMode)	dacSendKeyboard(currentInstrument);
		}
		
		
	}
	else if (takeoverType == KeyboardInstrument)
	{
		tKeyboard_noteOn(&fullKeyboard, hex, weight);
		manta_set_LED_hex(hex, (fullKeyboard.hexes[hex].pitch >= 0) ? ((fullKeyboard.hexes[hex].color == Off) ? Amber : Off) : Off);
		
		if (fullKeyboard.playMode == TouchMode) dacSendKeyboard(InstrumentNil);
	}
}

void releaseKeyboardHex(int hex)
{
	if (!takeover)
	{
		if (manta[currentInstrument].type == KeyboardInstrument)
		{
			tKeyboard_noteOff(&manta[currentInstrument].keyboard, hex);
			manta_set_LED_hex(hex, manta[currentInstrument].keyboard.hexes[hex].color);
			
			if (manta[currentInstrument].keyboard.playMode == TouchMode) dacSendKeyboard(currentInstrument);
		}
	}
	else if (takeoverType == KeyboardInstrument)
	{
		tKeyboard_noteOff(&fullKeyboard, hex);
		manta_set_LED_hex(hex, fullKeyboard.hexes[hex].color);

		if (fullKeyboard.playMode == TouchMode) dacSendKeyboard(InstrumentNil);
	}
}

void releaseLingeringKeyboardHex(int hex)
{
	if (!takeover)
	{
		for (int i = 0; i < NUM_INST; i++)
		{
			if (manta[i].type == KeyboardInstrument)
			{
				tKeyboard_noteOff(&manta[i].keyboard, hex);
				dacSendKeyboard(i);
				//manta_set_LED_hex(hex, manta[i].keyboard.hexes[hex].color);
			}
		}	
	}
	else if (takeoverType == KeyboardInstrument)
	{
		tKeyboard_noteOff(&fullKeyboard, hex);
		dacSendKeyboard(InstrumentNil);
		//manta_set_LED_hex(hex, fullKeyboard.hexes[hex].color);
	}
}

void touchFunctionButtonKeys(MantaButton button)
{
	if (button == ButtonTopLeft)
	{
		// step up
	}
	else if (button == ButtonTopRight)
	{
		// step up 
	}
	else if (button == ButtonBottomRight)
	{
		// toggle octave/semitone
	}
	else if (button == ButtonBottomLeft)
	{
		touchBottomLeftButton();
	}
	
	
}

void releaseFunctionButtonKeys(MantaButton button)
{
	if (button == ButtonTopLeft)
	{
		
	}
	else if (button == ButtonTopRight)
	{
		
	}
	else if (button == ButtonBottomRight)
	{
		// toggle octave/semitone
	}
	else if (button == ButtonBottomLeft)
	{
		releaseBottomLeftButton();
	}
	
}

void processSliderKeys(uint8_t sliderNum, uint16_t val)
{
	if (manta[currentInstrument].type != KeyboardInstrument) return;
	tKeyboard* keyboard;
	if (takeover || (currentInstrument == InstrumentNil))
	{
		keyboard = &fullKeyboard;
	}
	else
	{
		keyboard = &manta[currentInstrument].keyboard;
	}
	if (keyboard->numVoices < 4)
	{
		int whichGroup = (keyboard->numVoices * 3) / 6;
		int sliderStartPos = ((keyboard->numVoices * 3) + CVKSLIDEROFFSET) % 6;
			
		tIRampSetTime(&out[whichGroup][sliderStartPos + sliderNum], 10);
		tIRampSetDest(&out[whichGroup][sliderStartPos + sliderNum], val);
	}
}

void dacSendKeyboard(MantaInstrument which)
{
	tKeyboard* keyboard;
	
	if (takeover || (which == InstrumentNil))	
	{
		which = 0;
		keyboard = &fullKeyboard;
	}
	else			
	{
		keyboard = &manta[which].keyboard;
	}
	
	if (keyboard->playMode == ArpMode)
	{
		int newNote = keyboard->currentNote;
		if (newNote >= 0)
		{
			tIRampSetDest(&out[which][CVKPITCH+3*keyboard->currentVoice], lookupDACvalue(&myGlobalTuningTable, keyboard->hexes[newNote].pitch, keyboard->transpose));

			tIRampSetDest(&out[which][CVKTRIGGER-2+3*keyboard->currentVoice], 65535);
			keyboard->trigCount[keyboard->currentVoice] = TRIGGER_TIMING;
		}
		
		if (keyboard->stack.size <= 0) 
		{
			tIRampSetDest(&out[which][CVKTRIGGER-2+3*keyboard->currentVoice], 0);
		}
	}
	else
	{
		for (int i = 0; i < keyboard->numVoices; i++)
		{
			int note = keyboard->voices[i];
			if (note >= 0)
			{
				tIRampSetDest(&out[takeover ? (int)(i/2) : which][((i*3) % 6)+CVKPITCH], lookupDACvalue(&myGlobalTuningTable, keyboard->hexes[note].pitch, keyboard->transpose));

				tIRampSetDest(&out[takeover ? (int)(i/2) : which][((i*3) % 6)+CVKGATE], 4095);
				//if we are in mono mode, then we have room for a trigger output, too
				if ((keyboard->numVoices == 1) && (prevSentPitch != (keyboard->hexes[note].pitch + keyboard->transpose))) //if we are in mono mode, then we have room for a trigger output, too
				{
					tIRampSetDest(&out[which][CVKTRIGGER], 65535);
					keyboard->trigCount[0] = TRIGGER_TIMING;
				}
				//this is to avoid retriggers on the same note when other notes are released in monophonic mode
				prevSentPitch = keyboard->hexes[note].pitch + keyboard->transpose;
			}
			else
			{
				tIRampSetDest(&out[takeover ? (int)(i/2) : which][((i*3) % 6)+CVKGATE], 0);
				//let the monophonic trigger handling know there has been a note-off event
				prevSentPitch = -1;
			}
		}
	}
	
}



