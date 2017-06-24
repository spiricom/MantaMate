/*
 * note_process.c
 *
 * Created: 7/22/2015 4:47:37 PM
 *  Author: Elaine Chou & Jeff Snyder
 */ 

#include "note_process.h"

	

uint8_t amplitude = 0;
unsigned char lastButtVCA = 0; //0 if you want to turn this off


unsigned long maxkey = 0;




unsigned char voicefound = 0; // have we found a polyphony voice slot for the incoming note?
unsigned char voicecounter = 0;
unsigned char alreadythere = 0;
signed char checkstolen = -1;




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
	

	
unsigned short lookupDACvalue(uint8_t noteVal, signed int transpose)
{
	return tuningDACTable[(noteVal + transpose)];
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


uint8_t programNum;

void programChange(uint8_t num)
{
	programNum = num;
}

void resetMantaUI(void)
{
	for (int i = 0; i < 48; i++) manta_set_LED_hex(i, Off);
	for (int i = 0; i < 2; i++)  manta_set_LED_slider(i, 0);
	for (int i = 0; i < 4; i++)  manta_set_LED_button(i, Off);
	
}



void initKeys(int numVoices)
{
	if (numVoices < 2)
	{
		for (int i = 0; i < NUM_INST; i++)
		{
			manta[i].type = KeyboardInstrument;
			
			tKeyboard* keyboard = &manta[i].keyboard;
			
			tKeyboard_init(keyboard,numVoices);
		}
	}
	else // Takeover mode
	{
		takeover = TRUE;
		tKeyboard_init(&fullKeyboard, numVoices);
	}

	initTimers(); // Still configuring all three from sequencer, but only using t3.
	tc_start(tc3, TC3_CHANNEL);
	
	for(int i=0; i<4; i++)
	{
		dacsend(i,0,0);
		dacsend(i,0,0);
		DAC16Send(i,0);
	}
	
	resetMantaUI();
}

void touchLowerHexKey(int hex, uint8_t weight)
{
	tKeyboard* keyboard;
	
	
	if (!takeover)
	{ 
		if (manta[currentInstrument].type == KeyboardInstrument)
		{
			tKeyboard_noteOn(&manta[currentInstrument].keyboard, hex, weight);
			dacSendKeyboard(currentInstrument);
			manta_set_LED_hex(hex, Amber);
		}
	}
	else 
	{
		tKeyboard_noteOn(&fullKeyboard, hex, weight);
		dacSendKeyboard(InstrumentNil);
		manta_set_LED_hex(hex, Amber);
	}

	

	
}

void releaseLowerHexKey(int hex)
{
	if (!takeover)
	{
		if (manta[currentInstrument].type == KeyboardInstrument)
		{
			tKeyboard_noteOff(&manta[currentInstrument].keyboard, hex);
			dacSendKeyboard(currentInstrument);
			manta_set_LED_hex(hex, Off);
		}
	}
	else
	{
		tKeyboard_noteOff(&fullKeyboard, hex);
		dacSendKeyboard(InstrumentNil);
		manta_set_LED_hex(hex, Off);
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
	// DO THIS
}

// reads the current state and sets output voltages, leds and 7 seg display
void dacSendKeyboard(MantaInstrument which)
{
	tKeyboard* keyboard;
	
	if (takeover || (which == InstrumentNil))	
	{
		keyboard = &fullKeyboard;
	}
	else			
	{
		keyboard = &manta[which].keyboard;
	}
	

	for (int i = 0; i < keyboard->numVoices; i++)
	{
		int note = keyboard->voices[i];
		if (note >= 0)
		{
			uint8_t mappedNote = applyNoteMap(keyboard->map, note);
			tRampSetDest(&out[takeover ? (int)(i/2) : which][3*i+CVPITCH], lookupDACvalue(mappedNote, keyboard->transpose));
				
			dacsend		(takeover ? i : (which*2), 0, 0xfff);
		}
		else
		{
			dacsend		(takeover ? i : (which*2), 0, 0x000);
		}

	}

}


void tuningTest(uint8_t oct)
{
	while(1)
	{
		
		DAC16Send(0, lookupDACvalue(oct, 0));
		DAC16Send(1, lookupDACvalue(oct, 0));
		DAC16Send(2, lookupDACvalue(oct, 0));
		DAC16Send(3, lookupDACvalue(oct, 0));
		dacsend(0,1,0xfff);
		dacsend(1,1,0xfff);
		dacsend(2,1,0xfff);
		dacsend(3,1,0xfff);
		oct++;
		if (oct >= 127)
		{
			oct = 0;
		}
		delay_ms(100);
		
	}

	
}
