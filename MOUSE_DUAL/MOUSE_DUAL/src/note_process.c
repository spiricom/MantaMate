/*
 * note_process.c
 *
 * Created: 7/22/2015 4:47:37 PM
 *  Author: Elaine Chou & Jeff Snyder
 */ 

#include "note_process.h"

unsigned short calculateDACvalue(MantaMap whichmap, uint8_t noteVal);
extern unsigned char preset_num;



unsigned long scaledoctaveDACvalue = 54613; 
unsigned char tuning = 0;
signed char transpose = 0;
unsigned char octaveoffset = 0;
uint8_t amplitude = 0;
unsigned char lastButtVCA = 0; //0 if you want to turn this off


unsigned long maxkey = 0;




unsigned char voicefound = 0; // have we found a polyphony voice slot for the incoming note?
unsigned char voicecounter = 0;
unsigned char alreadythere = 0;
signed char checkstolen = -1;





unsigned short calculateDACvalue(MantaMap whichmap, uint8_t noteVal)
{
	signed long pitchclass;
	unsigned long templongnote = 0;
	unsigned int virtualnote;
	unsigned long templongoctave;
	unsigned short DAC1val;
	unsigned int note;
	

	switch(whichmap)
	{
		case WickiHaydenMap: note = whmap[noteVal]; break;    // wicki-hayden
		case HarmonicMap: note = harmonicmap[noteVal]; break;  // harmonic
		case PianoMap: note = pianomap[noteVal]; break;		// piano map
		default: note = noteVal; break;                     // no map
	}

	
	//templong = ((noteval + offset + transpose) * 54612);  // original simple equal temperament
	pitchclass = ((note + transpose + 24) % 12);  // add 24 to make it positive and centered on C
	virtualnote = (note + 13 + transpose - pitchclass);
	if (tuning == 0)
	templongnote = (twelvetet[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 1)
	templongnote = (overtonejust[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 2)
	templongnote = (kora1[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 3)
	templongnote = (meantone[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 4)
	templongnote = (werckmeister1[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 5)
	templongnote = (werckmeister3[pitchclass] * scaledoctaveDACvalue);
	
	templongnote = (templongnote / 10000);
	templongoctave = ((virtualnote + octaveoffset) * scaledoctaveDACvalue);
	templongoctave = (templongoctave / 100);
	DAC1val = templongnote + templongoctave;
	return DAC1val;
}

void joyVol(uint16_t slider_val) {
	//dacsend(0, 0, slider_val << 4);
	//dacsend(1, 0, slider_val << 4);
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

	if (numVoices < 3)
	{
		for (int i = 0; i < NUM_INST; i++)
		{
			manta[i].type = KeyboardInstrument;
			
			tKeyboard* keyboard = &manta[i].keyboard;
			
			tKeyboard_init(keyboard,numVoices);
		}
	}
	else 
	{
		
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
	for (int i = 0; i < NUM_INST; i++)
	{
		if (manta[i].type == KeyboardInstrument) tKeyboard_noteOn(&manta[i].keyboard, hex, weight);
	}
	
	dacSendKeyboard();
	
	manta_set_LED_hex(hex, Amber);	
}

void releaseLowerHexKey(int hex)
{
	for (int i = 0; i < NUM_INST; i++)
	{	
		if (manta[i].type == KeyboardInstrument) 
		{
			tKeyboard* keyboard = &manta[i].keyboard;
			
			tKeyboard_noteOff(keyboard, hex);
		}
	}
	
	dacSendKeyboard();
	
	manta_set_LED_hex(hex, Off);
}

void touchFunctionButtonKeys(MantaButton button)
{
	
	
}

void releaseFunctionButtonKeys(MantaButton button)
{
	
	
}

void processKeys(void)
{
	uint8_t i;

	for (i = 0; i < 48; i++)
	{
		//if the current sensor value of a key is positive and it was zero on last count
		if (!manta_data_lock) // manta_data_lock == 0
		{
			if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
			{
				touchLowerHexKey(i, butt_states[i]);
			}
			else if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
			{
				releaseLowerHexKey(i);
			}

			// update the past keymap array (stores the previous values of every key's sensor reading)
			pastbutt_states[i] = butt_states[i];
		}
		
	}
	
	for (i = 0; i < 4; i++)
	{
		if ((func_button_states[i] > 0) && (past_func_button_states[i] <= 0))
		{
			touchFunctionButtonKeys(i);
		}
		
		if ((func_button_states[i] <= 0) && (past_func_button_states[i] > 0))
		{
			touchFunctionButtonKeys(i);
		}
		
		past_func_button_states[i] = func_button_states[i];
	}

	
}


void processSliderKeys(uint8_t sliderNum, uint16_t val)
{
	// DO THIS
}

// reads the current state and sets output voltages, leds and 7 seg display
void dacSendKeyboard(void)
{
	if (type_of_device_connected == MantaConnected)
	{
		for (int inst = 0; inst < NUM_INST; inst++)
		{
			tKeyboard* keyboard = &manta[inst].keyboard;

			for (int i = 0; i < keyboard->numVoices; i++)
			{
				int inst = (int)(i/2);
				
				int note = keyboard->voices[i];
				if (note >= 0)
				{
					tRampSetDest(&out[inst][3*((inst == InstrumentOne)?(i):(i-2))+CVPITCH], calculateDACvalue(keyboard->map, note));
					
					dacsend		(i,0, 0xfff);
				}
				else
				{
					dacsend		(i,0, 0x000);
				}

			}
		}
	}
	else
	{
		
	}
}


void tuningTest(uint8_t oct)
{

	DAC16Send(0, calculateDACvalue(MantaMapNil, 12 * oct));
	DAC16Send(1, calculateDACvalue(MantaMapNil, 12 * oct));
	DAC16Send(2, calculateDACvalue(MantaMapNil, 12 * oct));
	DAC16Send(3, calculateDACvalue(MantaMapNil, 12 * oct));
	dacsend(0,1,0xfff);
	dacsend(1,1,0xfff);
	dacsend(2,1,0xfff);
	dacsend(3,1,0xfff);
	
}
