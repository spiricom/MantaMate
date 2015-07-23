/*
 * note_process.c
 *
 * Created: 7/22/2015 4:47:37 PM
 *  Author: Elaine
 */ 

#include "main.h"
#include "note_process.h"
#include "dip204.h"

enum maps_t
{
	NO_MAP,
	WICKI_HAYDEN,
	HARMONIC
};

unsigned long twelvetet[12] = {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100};
unsigned long overtonejust[12] = {0, 111, 203, 316, 386, 498, 551, 702, 813, 884, 968, 1088};
unsigned long kora1[12] = {0, 185, 230, 325, 405, 498, 551, 702, 885, 930, 1025, 1105};
unsigned long meantone[12] = {0, 117, 193, 310, 386, 503, 579, 697, 773, 890, 966, 1083};
unsigned long werckmeister1[12] = {0, 90, 192, 294, 390, 498, 588, 696, 792, 888, 996, 1092};
unsigned long werckmeister3[12] = {0, 96, 204, 300, 396, 504, 600, 702, 792, 900, 1002, 1098};

unsigned int whmap[48] = {0,2,4,6,8,10,12,14,7,9,11,13,15,17,19,21,12,14,16,18,20,\
22,24,26,19,21,23,25,27,29,31,33,24,26,28,30,32,34,36,38,31,33,35,37,39,41,43,45};
unsigned int harmonicmap[48] = {0,4,8,12,16,20,24,28,7,11,15,19,23,27,31,35,10,14,\
18,22,26,30,34,38,17,21,25,29,33,37,41,45,20,24,28,32,36,40,44,48,27,31,35,39,43,47,51,55};

enum maps_t whichmap = NO_MAP;
unsigned long scaledoctaveDACvalue = 54612;
unsigned char tuning = 0;
signed char transpose = 0;
unsigned char octaveoffset = 0;
uint8_t amplitude = 0;
unsigned char lastButtVCA = 0; //0 if you want to turn this off

signed char notestack[48][2];
unsigned char numnotes = 0;
unsigned char currentnote = 0;
unsigned char polynum = 1;
unsigned char polyVoiceNote[4];
unsigned char polyVoiceBusy[4];
unsigned char changevoice[4];
unsigned char notehappened = 0;
unsigned char noteoffhappened = 0;
unsigned char voicefound = 0;
unsigned char voicecounter = 0;
unsigned char alreadythere = 0;
signed char checkstolen = -1;

void initNoteStack(void)
{
	uint8_t i;
	for(i=0; i<48; i++)
		notestack[i][0] = -1;
	numnotes = 0;
}

//ADDING A NOTE
//first figure out how many notes are currently in the stack
// next, take the last note in the stack and copy it into the position one index number past it
// now, do that for each note as you go down the list
// when you get the index number 0, after copying it, put the new note in it's place
void addNote(uint8_t noteVal, uint8_t vel)
{
	uint8_t j;
	checkstolen = -1;
	//it's a note-on -- add it to the monophonic stack
	if(numnotes == 0)
		DAC16Send(2,0xFFFF);

	//first move notes that are already in the stack one position to the right
	for (j = numnotes; j > 0; j--)
	{
		notestack[j][0] = notestack[(j - 1)][0];
		notestack[j][1] = notestack[(j - 1)][0];
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
	dip204_set_cursor_position(1,3);
	dip204_write_string("                   ");
	dip204_set_cursor_position(1,3);
	dip204_printf_string("%u notes",numnotes);
	dip204_hide_cursor();
}

//REMOVING A NOTE
//first, find the note in the stack
//then, remove it
//move everything to the right of it (if it's not negative 1) one index number less
//replace the last position with -1
void removeNote(uint8_t noteVal)
{
	uint8_t j,k;
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
		//now check if there are any polyphony voices waiting that got stolen.
		for (j = 0; j < numnotes; j++)
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
				}
			}
		}
	}
			
	if(numnotes == 0)
		DAC16Send(2,0);
		
	dip204_set_cursor_position(1,3);
	dip204_write_string("                   ");
	dip204_set_cursor_position(1,3);
	dip204_printf_string("%u notes",numnotes);
	dip204_hide_cursor();
}

unsigned short calculateDACvalue(void)
{
	signed long pitchclass;
	unsigned long templongnote = 0;
	unsigned int virtualnote;
	unsigned long templongoctave;
	unsigned short DAC1val;
	unsigned int note;
	
	switch(whichmap)
	{
		case WICKI_HAYDEN: note = whmap[currentnote]; break;    // wicki-hayden
		case HARMONIC: note = harmonicmap[currentnote]; break;  // harmonic
		default: note = currentnote; break;                     // no map
	}
	
	//templong = ((noteval + offset + transpose) * 54612);  // original simple equal temperament
	pitchclass = ((note + transpose + 21) % 12);  // add 21 to make it positive and centered on C
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
	return DAC1val*2;
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
	
	dacsend(3,2,amplitude<<4);/*
	if(amplitude != 0)
	{
		dip204_set_cursor_position(1,2);
		dip204_printf_string("                    ");
		dip204_set_cursor_position(1,2);
		dip204_printf_string("amplitude: %u", amplitude);
		dip204_hide_cursor();
	}*/
}

void midiVol()
{
	uint8_t vol = notestack[0][1];
	dacsend(3,2,vol<<4);
}

