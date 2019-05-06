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
		return (myTable->tuningDACTable[myPitchClass] + (6553 * myOctave)); // 65535 / 10 = 6553 (10 octaves in dac range)
	}
	else
	{
		int32_t pitch = (myTable->tuningDACTable[myNote]);
		return pitch;
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
		/*
		if (takeover && (fullKeyboard.playMode == ArpMode))
		{
			manta[currentInstrument].keyboard.playMode = ArpMode;
			manta[currentInstrument].keyboard.arpModeType = fullKeyboard.arpModeType;
		}
		*/
		
		takeover = FALSE;
		
		for (int i = 0; i < NUM_INST; i++)
		{
			manta[i].type = KeyboardInstrument;
			
			tKeyboard* keyboard = &manta[i].keyboard;
			
			tKeyboard_setToDefault(keyboard, PianoMap);
			
			currentTuningHex = -1;
			
			keyboard->numVoices = 1;
		}
	}
	else // Takeover mode
	{
		/*
		if (!takeover && (manta[currentInstrument].keyboard.playMode == ArpMode))
		{
			fullKeyboard.playMode = ArpMode;
			fullKeyboard.arpModeType = manta[currentInstrument].keyboard.arpModeType;
		}
		*/
		
		takeover = TRUE;
		
		tKeyboard_setToDefault(&fullKeyboard, PianoMap);
		
		takeoverType = KeyboardInstrument;
		
		fullKeyboard.numVoices = numVoices;
	}
	
	clearDACoutputs();
	
	resetMantaUI();
}


void touchDirectEdit(int hex)
{
	displayState = DirectOutputSelect;
	
	currentDirectEditHex = hex;
	
	currentDirectEditOutput =  tDirect_getOutput(editDirect, currentDirectEditHex);
	
	Write7Seg((currentDirectEditOutput < 0) ? -1 : (currentDirectEditOutput+1));
	
	DirectType type = tDirect_getType(editDirect, currentDirectEditHex);
	
	if (hex < 48)
	{
		if ((type == DirectTypeNil) || (currentDirectEditHex == lastDirectEditHex))
		{
			type =  (type == DirectTrigger) ? DirectGate :
			(type == DirectGate) ? DirectCV :
			(type == DirectCV) ? DirectTypeNil :
			(type == DirectTypeNil) ? DirectTrigger :
			DirectTypeNil;
			
			tDirect_setType(editDirect, currentDirectEditHex, type);
		}
		
	}
	else if (hex < 50)
	{
		type =  (type == DirectTypeNil) ? DirectCV :
		(type == DirectCV) ? DirectTypeNil :
		DirectTypeNil;
			
		tDirect_setType(editDirect, currentDirectEditHex, type);
	}
	
	setDirectLEDs();
}

void releaseDirectEdit(int hex)
{
	if (hex == currentDirectEditHex)
	{
		lastDirectEditHex = currentDirectEditHex;
		currentDirectEditHex = -1;
		Write7Seg(-1);
		displayState = PresetSwitchBlock;
	}
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
			
			if (!firstEdition)
			{
				if (color == Off)        color = Amber;
				else if (color == Amber) color = Red;
				else if (color == Red)   color = Off;
				else					 color = Amber;
			}
			else
			{
				if (color == Off)        color = Amber;
				else if (color == Amber) color = Off;
			}
			
			hexmapEditKeyboard->hexes[currentHexmapEditHex].color = color;
			
			manta_set_LED_hex(currentHexmapEditHex, color);
		}
		
		Write7Seg(currentHexmapEditPitch%100);
		
		manta_set_LED_slider(SliderOne, (hexmapEditKeyboard->hexes[currentHexmapEditHex].pitch >> 4) + 1);
		manta_set_LED_slider(SliderTwo, (hexmapEditKeyboard->hexes[currentHexmapEditHex].fine >> 9) + 1);
		
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
		
		displayState = PresetSwitchBlock;
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
			tKeyboard_noteOff(&manta[i].keyboard, hex);
			dacSendKeyboard(i);
			//manta_set_LED_hex(hex, manta[i].keyboard.hexes[hex].color);
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
	if (displayState == HexmapPitchSelect)
	{
		// Fine tune and octave for pitch
		if (sliderNum == SliderOne)
		{
			hexmapEditKeyboard->hexes[currentHexmapEditHex].pitch = (val >> 5);
			
			currentHexmapEditPitch = hexmapEditKeyboard->hexes[currentHexmapEditHex].pitch;

			dacSendKeyboard(hexmapEditInstrument);
			
			Write7Seg(currentHexmapEditPitch%100);
			normal_7seg_number = currentHexmapEditPitch;
		}
		else if (sliderNum == SliderTwo)
		{
			hexmapEditKeyboard->hexes[currentHexmapEditHex].fine = val;
			
			dacSendKeyboard(hexmapEditInstrument);
		}
		
		manta_set_LED_slider(SliderOne, (hexmapEditKeyboard->hexes[currentHexmapEditHex].pitch >> 4) + 1);
		manta_set_LED_slider(SliderTwo, (hexmapEditKeyboard->hexes[currentHexmapEditHex].fine >> 9) + 1);
	}
	else
	{
		if (!takeover)
		{
			if (manta[currentInstrument].type == KeyboardInstrument)
			{
				tKeyboard* keyboard = &manta[currentInstrument].keyboard;
				
				if (keyboard->numVoices < 4)
				{
					int whichGroup;
					if (currentInstrument == InstrumentTwo && keyboard->numVoices == 1)
						whichGroup = 1;
					else
						whichGroup = (keyboard->numVoices * 3) / 6;
					int sliderStartPos = ((keyboard->numVoices * 3) + CVKSLIDEROFFSET) % 6;
					
					tIRampSetTime(&out[whichGroup][sliderStartPos + sliderNum], globalCVGlide);
					tIRampSetDest(&out[whichGroup][sliderStartPos + sliderNum], val);
					
					manta_set_LED_slider(sliderNum, (val >> 9) + 1);
				}
			}
			else if (manta[currentInstrument].type == DirectInstrument)
			{
				tDirect* direct = &manta[currentInstrument].direct;
				
				int output = tDirect_getOutput(direct, (48+sliderNum));
				
				if (direct->sliders[sliderNum].type == DirectCV)
				{
					direct->sliders[sliderNum].value = val;
					manta_set_LED_slider(sliderNum, (val >> 9) + 1);
					
					sendDataToOutput(6*currentInstrument+output, globalCVGlide, (direct->sliders[sliderNum].value << 4));
				}
			}
		}
		else if (takeoverType == KeyboardInstrument)
		{
			tKeyboard* keyboard = &fullKeyboard;
			
			if (keyboard->numVoices < 4)
			{
				int whichGroup = (keyboard->numVoices * 3) / 6;
				int sliderStartPos = ((keyboard->numVoices * 3) + CVKSLIDEROFFSET) % 6;
				
				tIRampSetTime(&out[whichGroup][sliderStartPos + sliderNum], globalCVGlide);
				tIRampSetDest(&out[whichGroup][sliderStartPos + sliderNum], val);
				
				manta_set_LED_slider(sliderNum, (val >> 9) + 1);
			}
			
		}
		else if (takeoverType == DirectInstrument)
		{
			fullDirect.sliders[sliderNum].value = val;
			
			int output = tDirect_getOutput(&fullDirect, (48+sliderNum));
			
			if (fullDirect.sliders[sliderNum].type == DirectCV)
			{
				fullDirect.sliders[sliderNum].value = val;
				manta_set_LED_slider(sliderNum, (val >> 9) + 1);
				
				sendDataToOutput(output, globalCVGlide, (fullDirect.sliders[sliderNum].value << 4));
			}
		}
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
		
		
		if (keyboard->stack.size <= 0) 
		{
			tIRampSetDest(&out[which][CVKTRIGGER-2+3*keyboard->currentVoice], 0);
		}
		else if (newNote >= 0)
		{
			tIRampSetTime(&out[which][CVKPITCH+3*keyboard->currentVoice], globalPitchGlide);
			tIRampSetDest(&out[which][CVKPITCH+3*keyboard->currentVoice], lookupDACvalue(&myGlobalTuningTable, keyboard->hexes[newNote].pitch, keyboard->transpose) + ((keyboard->hexes[newNote].fine >> 2) - 512));
			tIRampSetTime(&out[which][CVKTRIGGER-2+3*keyboard->currentVoice], 0);
			tIRampSetDest(&out[which][CVKTRIGGER-2+3*keyboard->currentVoice], 65535);
			keyboard->trigCount[keyboard->currentVoice] = TRIGGER_TIMING;
		}
	}
	else
	{
		for (int i = 0; i < keyboard->numVoices; i++)
		{
			int note = keyboard->voices[i];
			int thing = takeover ? (int)(i/2) : which;
			
			if (note >= 0)
			{
				tIRampSetTime(&out[thing][((i*3) % 6)+CVKPITCH], globalPitchGlide);
				int32_t pitch = lookupDACvalue(&myGlobalTuningTable, keyboard->hexes[note].pitch, keyboard->transpose);
				pitch += ((keyboard->hexes[note].fine >> 2) - 512);
				tIRampSetDest(&out[thing][((i*3) % 6)+CVKPITCH], pitch);
				
				if (prevSentPitch[i] != (keyboard->hexes[note].pitch + keyboard->transpose))
				{
					//if we are in mono mode, then we have room for a trigger output, too
					if ((keyboard->numVoices == 1)) //if we are in mono mode, then we have room for a trigger output, too
					{
						tIRampSetTime(&out[thing][CVKTRIGGER], 0);
						tIRampSetDest(&out[thing][CVKTRIGGER], 65535);
					}
				
					// set gate cv low for gate retrigger
					tIRampSetTime(&out[thing][((i*3) % 6)+CVKGATE], 0);
					tIRampSetDest(&out[thing][((i*3) % 6)+CVKGATE], 0);
					keyboard->trigCount[i] = TRIGGER_TIMING;
					
					//this is to avoid retriggers on the same note when other notes are released in monophonic mode
					prevSentPitch[i] = keyboard->hexes[note].pitch + keyboard->transpose;
				}
			}
			else
			{
				tIRampSetDest(&out[thing][((i*3) % 6)+CVKGATE], 0);
				//let the monophonic trigger handling know there has been a note-off event
				prevSentPitch[i] = -1;
			}
		}
	}
	
}



