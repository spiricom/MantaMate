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


unsigned char numnotes = 0;
unsigned char currentnote = 0;
unsigned long maxkey = 0;
unsigned char polymode = 0;


unsigned char changevoice[4]; // flags to indicate a voice has a new note value
unsigned char notehappened = 0;
unsigned char noteoffhappened = 0;
unsigned char voicefound = 0; // have we found a polyphony voice slot for the incoming note?
unsigned char voicecounter = 0;
unsigned char alreadythere = 0;
signed char checkstolen = -1;




void initNoteStack(void)
{
	uint8_t i;
	for(i=0; i<48; i++)
	{
		if (notestack[i][0] != -1)
		{
			removeNote(notestack[i][0]);
			noteOut();
		}
	}
	numnotes = 0;
	for(i=0; i<4; i++)
	{
		dacsend(i,0,0);
		dacsend(i,0,0);
		DAC16Send(i,0);
	}
}

//ADDING A NOTE
// move all the notes into the next higher index and put the new note in index 0
// first figure out how many notes are currently in the stack
// next, copy the last note in the stack from its index (k) into the next index (k+1)
// now, do that for each note, moving from the last to the first index
// when you get to index 0, after copying it, put the new note in index 0
void addNote(uint8_t noteVal, uint8_t vel)
{
	uint8_t j;
	checkstolen = -1;
	signed int mappedNote = 0;
		
	// this function needs to check the incoming note value against the currently used manta map if it's for the manta, in order to know not to sensors that don't represent notes (a -1 in the noteMap)
	// it seems inefficient to calculate this twice, so we should store this and pass it to the calculateDACvalue instead of calculating it there, too. Not sure where to do that yet, as currently sendNote is called separately, and they don't know about each other.
	if (manta_mapper == 1)
	{
		switch(whichmap)
		{
			case WICKI_HAYDEN: mappedNote = whmap[noteVal]; break;    // wicki-hayden
			case HARMONIC: mappedNote = harmonicmap[noteVal]; break;  // harmonic
			case PIANO: mappedNote = pianomap[noteVal]; break;		// piano map
			default: mappedNote = noteVal; break;                     // no map
		}
	}
	else
	{
		mappedNote = noteVal;
	}

	if (mappedNote >= 0)
	{

		//first move notes that are already in the stack one position to the right
		for (j = numnotes; j > 0; j--)
		{
			notestack[j][0] = notestack[(j - 1)][0];
			notestack[j][1] = notestack[(j - 1)][1];
		}

		//then, insert the new note into the front of the stack
		notestack[0][0] = noteVal;
		notestack[0][1] = vel;

		//also, assign a new polyphony voice to the note on for the polyphony handling
		voicefound = 0;
		voicecounter = 0;
		for (j = 0; j < polynum; j++)
		{
			if ((polyVoiceBusy[j] == 0) && (voicefound == 0))
			{
				polyVoiceNote[j] = noteVal;  // store the new note in a voice if a voice is free - store it without the offset and transpose (just 0-31).
				polyVoiceBusy[j] = 1;
				changevoice[j] = 1;
				voicefound = 1;
			}	
			voicecounter++;
				
			if ((voicecounter == polynum) && (voicefound == 0))
			{
				polyVoiceNote[(polynum - 1)] = noteVal;  // store the new note in a voice if a voice is free - store it without the offset and transpose (just 0-31).
				polyVoiceBusy[(polynum - 1)] = 1;
				changevoice[(polynum - 1)] = 1;
				voicefound = 1;
			}
		}
		numnotes++;
		notehappened = 1;
		currentnote = notestack[0][0];

	}
}

//REMOVING A NOTE
//first, find the note in the stack
//then, remove it
//move everything to the right of it (if it's not negative 1) one index number less
//replace the last position with -1
void removeNote(uint8_t noteVal)
{
	uint8_t j,k;
	signed int mappedNote = 0;
	

	// this function needs to check the incoming note value against the currently used manta map if it's for the manta, in order to know not to sensors that don't represent notes (a -1 in the noteMap)
	// it seems inefficient to calculate this twice, so we should store this and pass it to the calculateDACvalue instead of calculating it there, too.
	if (manta_mapper == 1)
	{
		switch(whichmap)
		{
			case WICKI_HAYDEN: mappedNote = whmap[noteVal]; break;    // wicki-hayden
			case HARMONIC: mappedNote = harmonicmap[noteVal]; break;  // harmonic
			case PIANO: mappedNote = pianomap[noteVal]; break;		// piano map
			default: mappedNote = noteVal; break;                     // no map
		}
	}
	else
	{
		mappedNote = noteVal;
	}

	if (mappedNote >= 0)
	{
		//it's a note-off, remove it from the stack
		//go through the notes that are currently held down to find the one that released
		for (j = 0; j < numnotes; j++)
		{
			//if it's the note that just got released
			if (notestack[j][0] == noteVal)
			{
				for (k = 0; k < (numnotes - j); k++)
				{
					notestack[k + j][0] = notestack[k + j + 1][0];
					//if it's the last one, write negative 1 beyond it (it's already been copied to the position to the left of it)
					if (k == ((numnotes - j) - 1))
						notestack[k + j + 1][0] = -1;
				}
				// in case it got put on the stack multiple times
				j--;
				numnotes--;
				notehappened = 1;
				noteoffhappened = 1;
			}
		}

		//also, remove that note from the polyphony array if it's there.
		for (j = 0; j < polynum; j++)
		{
			if (polyVoiceNote[j] == noteVal)
			{
				polyVoiceBusy[j] = 0;
				changevoice[j] = 1;
				checkstolen = j;
			}
		}
			
		//remove it from the notestack and decrement numnotes
		if (notestack[0][0] != -1)
			currentnote = notestack[0][0];

		// if we removed a note from the polyphony array
		if (checkstolen != -1)
		{
			j = 0;
			//now check if there are any polyphony voices waiting that got stolen.
			while(checkstolen != -1 && j < numnotes)
			{
				//if you find a held note in the notestack
				if (notestack[j][0] != -1)
				{
					//check if it has no voice associated with it
					alreadythere = 0;
					for (k = 0; k < polynum; k++)
					{
						if ((polyVoiceNote[k] == notestack[j][0]) && (polyVoiceBusy[k] == 1))
							alreadythere = 1;
					}
					// if you didn't find it, use the voice that was just released to sound it.
					if (alreadythere == 0)
					{
						polyVoiceNote[checkstolen] = notestack[j][0];
						polyVoiceBusy[checkstolen] = 1;
						changevoice[checkstolen] = 1;
						notehappened = 1;
						checkstolen = -1;
					}
				}
				j++;
			}
		}
		
		for(k=0; k<polynum; k++)
		{
			if(!polyVoiceBusy[k])
				dacsend(k,1,0);
		}
	}
}

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

void mantaVol(uint8_t *butts)
{
	uint8_t j;
	// volume control
	amplitude = 0;		
	if (lastButtVCA == 1)
	{
		if(numnotes > 0)
			amplitude = butts[notestack[0][0]];
	}
	else
	{
		for(j=0; j<numnotes; j++)
		{
			uint8_t val = butts[notestack[j][0]];
			if(val > amplitude)
			amplitude = val;
		}
	}
	
	//dacsend(3,2,amplitude<<4);
	/*if(amplitude != 0)
	{
		MEMORY_clear_line(2);
		MEMORY_printf_string("amplitude: %u", amplitude);
		MEMORY_hide_cursor();
	}*/
}

//Some issues with smoothness, random vol glitches  TODO: note sure if this is useful anymore, now that we are generalizing the CC# to CV output and trying to do midi learn
void midiVol(void)
{
	uint8_t i;
	unsigned short vol;
	
	vol = (notestack[0][1] * sysVol) >> 2;  // scale volume
	
	for(i=0; i<polynum; i++)
	{
		if(polyVoiceBusy[i])
			;//dacsend(i+2,0,(vol)&0xFFF);
		else
			;//dacsend(i+2,0,0);	
	}
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
	polynum = 4;
	sysVol = 0x7F; 
	float_t	birlOctave = 12;
	float_t	birlOffset = 23;
	
	sequencer_mode = 0;
	if (numVoices > 1)  polymode = 1;
	else				polymode = 0;
	
	polynum = numVoices;
	
	for (int i = 0; i < 12; i++)
	{
		tRampInit(&keyRamp[i], 2000, 1, 1);
	}
	
	initTimers(); // Still configuring all three from sequencer, but only using t3.
	tc_start(tc3, TC3_CHANNEL);
	
	resetMantaUI();
	
	initNoteStack();
	
	noteOut();
}

void touchLowerHexKey(int hex, uint8_t weight)
{
	addNote(hex, weight);
	
	manta_set_LED_hex(hex, Amber);	
}

void releaseLowerHexKey(int hex)
{
	removeNote(hex);
	
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

	noteOut();
}


void processSliderKeys(uint8_t sliderNum, uint16_t val)
{
	// DO THIS
}


// reads the current state and sets output voltages, leds and 7 seg display
void noteOut()
{
	int i;

	
	if (polymode == 0)
	{
		if (notehappened || noteoffhappened)
		{
			if (numnotes != 0)
			{
				tRampSetDest(&keyRamp[0],calculateDACvalue(notestack[0][0]));
				
				tRampSetDest(&keyRamp[1], 0xfff);
			}
			else
			{
				tRampSetDest(&keyRamp[1], 0x000);
			}
			notehappened = 0;
			noteoffhappened = 0;
		}

	}
	else
	{
		if (notehappened)
		{
			for (i = 0; i < polynum; i++)
			{
				if (changevoice[i])
				{
					//lcd_clear_line(i+1);
					if (polyVoiceBusy[i])
					{
						tRampSetDest(&keyRamp[3*i], calculateDACvalue(polyVoiceNote[i]));
						
						tRampSetDest(&keyRamp[3*i+1], 0xfff);
					}
					else
					{
						tRampSetDest(&keyRamp[3*i+1], 0x000);
					}
					
					changevoice[i] = 0;
					notehappened = 0;
					noteoffhappened = 0;
				}
			}
			
			/*
			for(j=0; j<polynum; j++)
			{
				if(polyVoiceBusy[j])
				dacsend(j,1,0xFFF);
			}
			*/
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