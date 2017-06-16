/*
 * note_process.c
 *
 * Created: 7/22/2015 4:47:37 PM
 *  Author: Elaine Chou & Jeff Snyder
 */ 

#include "note_process.h"


enum maps_t
{
	NO_MAP,
	WICKI_HAYDEN,
	HARMONIC,
	PIANO
};

static unsigned short calculateDACvalue(uint8_t noteVal);
extern unsigned char preset_num;

unsigned long twelvetet[12] = {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100};
unsigned long overtonejust[12] = {0, 111, 203, 316, 386, 498, 551, 702, 813, 884, 968, 1088};
unsigned long kora1[12] = {0, 185, 230, 325, 405, 498, 551, 702, 885, 930, 1025, 1105};
unsigned long meantone[12] = {0, 117, 193, 310, 386, 503, 579, 697, 773, 890, 966, 1083};
unsigned long werckmeister1[12] = {0, 90, 192, 294, 390, 498, 588, 696, 792, 888, 996, 1092};
unsigned long werckmeister3[12] = {0, 96, 204, 300, 396, 504, 600, 702, 792, 900, 1002, 1098};
	
unsigned long numTunings = 6; // we need to think about how to structure this more flexibly. Should maybe be a Tunings struct that includes structs that define the tunings, and then we won't have to manually edit this. Also important for users being able to upload tunings via computer.

signed int whmap[48] = {0,2,4,6,8,10,12,14,7,9,11,13,15,17,19,21,12,14,16,18,20,\
22,24,26,19,21,23,25,27,29,31,33,24,26,28,30,32,34,36,38,31,33,35,37,39,41,43,45};

signed int harmonicmap[48] = {0,4,8,12,16,20,24,28,7,11,15,19,23,27,31,35,10,14,\
18,22,26,30,34,38,17,21,25,29,33,37,41,45,20,24,28,32,36,40,44,48,27,31,35,39,43,47,51,55};

signed int pianomap[48] = {0,2,4,5,7,9,11,12,1,3,-1,6,8,10,-1,-1,12,\
14,16,17,19,21,23,24,13,15,-1,18,20,22,-1,-1,24,26,28,29,31,33,35,36,25,27,-1,30,32,34,-1,-1};

enum maps_t whichmap = NO_MAP;
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





unsigned short calculateDACvalue(uint8_t noteVal)
{
	signed long pitchclass;
	unsigned long templongnote = 0;
	unsigned int virtualnote;
	unsigned long templongoctave;
	unsigned short DAC1val;
	unsigned int note;
	
	if (manta_mapper == 1)
	{
		switch(whichmap)
		{
			case WICKI_HAYDEN: note = whmap[noteVal]; break;    // wicki-hayden
			case HARMONIC: note = harmonicmap[noteVal]; break;  // harmonic
			case PIANO: note = pianomap[noteVal]; break;		// piano map
			default: note = noteVal; break;                     // no map
		}
	}
	else
	{
		note = noteVal;
	}
	
	//templong = ((noteval + offset + transpose) * 54612);  // original simple equal temperament
	pitchclass = ((note + transpose + 24) % 12);  // add 24 to make it positive and centered on C
	virtualnote = (note + 13 + transpose - pitchclass);
	
	if (tuning == 0)		templongnote = (twelvetet[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 1)	templongnote = (overtonejust[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 2)	templongnote = (kora1[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 3)	templongnote = (meantone[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 4)	templongnote = (werckmeister1[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 5)	templongnote = (werckmeister3[pitchclass] * scaledoctaveDACvalue);
	
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

void tKeyboard_init(tKeyboard* const k, int numVoices)
{
	k->numVoices = numVoices;
	k->numPlaying = 0;
	
	k->currentNote = 0;
	k->noteOn = false;
	k->noteOff = false;
	
	tNoteStack_init(&k->notes,MAX_NUM_NOTES);
	tNoteStack_init(&k->vels,MAX_NUM_NOTES);
	
}

void initKeys(int numVoices)
{


	
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
		
			for(int i =0; i < keyboard->numVoices; i++)
			{
				if(!keyboard->polyVoiceBusy[i])
				dacsend(i,1,0);
			}
		}
	}
	
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

	dacSendKeyboard();
}


void processSliderKeys(uint8_t sliderNum, uint16_t val)
{
	// DO THIS
}


// reads the current state and sets output voltages, leds and 7 seg display
void dacSendKeyboard(void)
{
	if (currentDevice == DeviceManta)
	{
		for (int inst = 0; inst < NUM_INST; inst++)
		{
			tKeyboard* keyboard = &manta[inst].keyboard;

			if (keyboard->noteOn)
			{
				for (int i = 0; i < keyboard->numVoices; i++)
				{
					int inst = (int)(i/2);
					if (keyboard->changevoice[i])
					{
						//lcd_clear_line(i+1);
						if (keyboard->polyVoiceBusy[i])
						{
							tRampSetDest(&out[inst][CVPITCH], calculateDACvalue(keyboard->polyVoiceNote[i]));
							
							tRampSetDest(&out[inst][CVTRIGGER], 0xfff);
						}
						else
						{
							tRampSetDest(&out[inst][CVTRIGGER], 0x000);
						}
						
						keyboard->changevoice[i] = 0;
						keyboard->noteOn = false;
						keyboard->noteOff = false;
					}
				}
			}
		}
	}
	else if (currentDevice == DeviceMidi)
	{
		if (midiKeyboard.noteOn)
		{
			for (int i = 0; i < midiKeyboard.numVoices; i++)
			{
				int inst = (int)(i/2);
				if (midiKeyboard.changevoice[i])
				{
					//lcd_clear_line(i+1);
					if (midiKeyboard.polyVoiceBusy[i])
					{
						tRampSetDest(&out[inst][CVPITCH], calculateDACvalue(midiKeyboard.polyVoiceNote[i]));
						
						tRampSetDest(&out[inst][CVTRIGGER], 0xfff);
					}
					else
					{
						tRampSetDest(&out[inst][CVTRIGGER], 0x000);
					}
					
					midiKeyboard.changevoice[i] = 0;
					midiKeyboard.noteOn = false;
					midiKeyboard.noteOff = false;
				}
			}
		}
	}
	
	
}

void tuningTest(uint8_t whichOct)
{

	DAC16Send(0, calculateDACvalue(12 * whichOct));
	DAC16Send(1, calculateDACvalue(12 * whichOct));
	DAC16Send(2, calculateDACvalue(12 * whichOct));
	DAC16Send(3, calculateDACvalue(12 * whichOct));
	dacsend(0,1,0xfff);
	dacsend(1,1,0xfff);
	dacsend(2,1,0xfff);
	dacsend(3,1,0xfff);
	
}