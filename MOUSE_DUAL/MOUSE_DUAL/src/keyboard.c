/*
 * keyboard.c
 *
 * Created: 6/19/2017 3:48:02 PM
 *  Author: Mike Mulshine
 */ 

#include "keyboard.h"
#include "tuning.h"

signed int whmap[48] = {0,2,4,6,8,10,12,14,7,9,11,13,15,17,19,21,12,14,16,18,20,\
22,24,26,19,21,23,25,27,29,31,33,24,26,28,30,32,34,36,38,31,33,35,37,39,41,43,45};

signed int harmonicmap[48] = {0,4,8,12,16,20,24,28,7,11,15,19,23,27,31,35,10,14,\
18,22,26,30,34,38,17,21,25,29,33,37,41,45,20,24,28,32,36,40,44,48,27,31,35,39,43,47,51,55};

signed int pianomap[48] = {0,2,4,5,7,9,11,12,1,3,-1,6,8,10,-1,-1,12,\
14,16,17,19,21,23,24,13,15,-1,18,20,22,-1,-1,24,26,28,29,31,33,35,36,25,27,-1,30,32,34,-1,-1};

void tKeyboard_setHexmap(tKeyboard* const keyboard, MantaMap map, signed int custom[48])
{
	keyboard->map = map;
	
	signed int* hexmap;
	
	if (map == HarmonicMap)			hexmap = harmonicmap;
	else if (map == PianoMap)		hexmap = pianomap;
	else if (map == WickiHaydenMap)	hexmap = whmap;
	else if (map == MantaMapCustom)	hexmap = custom;
	else if (map == MantaMapNil)
	{
		for (int i = 0; i < 48; i++)
		{
			keyboard->hexes[i].mapped = i;
		}
		return;
	}
	
	for (int i = 0; i < 48; i++)
	{
		keyboard->hexes[i].mapped = hexmap[i];
	}
	
}

/*
void tKeyboard_setTuning(tKeyboard* const keyboard, MantaTuning tuning)
{
	keyboard->tuning = tuning;
	
	unsigned long* scale;
	
	if (tuning == TwelveTetTuning)			scale = twelvetet;
	else if (tuning == OvertoneJustTuning)	scale = overtonejust;
	else if (tuning == Kora1Tuning)			scale = kora1;
	else if (tuning == MeantoneTuning)		scale = meantone;
	else if (tuning == Werckmeister1Tuning)	scale = werckmeister1;
	else if (tuning == Werckmeister3Tuning)	scale = werckmeister3;
	else if (tuning == MantaTuningNil)		scale = twelvetet;

	for (int i = 0; i < 48; i++)
	{
		int degree = keyboard->hexes[i].hexmap;
		
		if (degree >= 0)
		{
			degree = degree % 12;
			keyboard->hexes[i].pitch = scale[degree];
		}
	}
}
*/

void tKeyboard_init(tKeyboard* const keyboard, int numVoices)
{
	keyboard->numVoices = numVoices;
	keyboard->numVoicesActive = numVoices;
	keyboard->map = MantaMapNil;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	keyboard->trigCount = 0;
	
	for (int i = 0; i < 4; i ++)
	{
		keyboard->voices[i] = -1;
	}
	
	for (int i = 0; i < 48; i++)
	{
		tHex_init(&keyboard->hexes[i], i);
	}
	
	tNoteStack_init(&keyboard->stack, 48);
}

//ADDING A NOTE
void tKeyboard_noteOn(tKeyboard* const keyboard, int note, uint8_t vel)
{
	// if not in keymap or already on stack, dont do anything. else, add that note.
	if ((keyboard->hexes[note].mapped < 0) || (tNoteStack_contains(&keyboard->stack, note) >= 0)) return;
	else
	{
		tNoteStack_add(&keyboard->stack, note);

		BOOL found = FALSE;
		for (int i = 0; i < keyboard->numVoices; i++)
		{
			if (keyboard->voices[i] < 0)	// if inactive voice, give this hex to voice
			{
				found = TRUE;
				
				keyboard->voices[i] = note;
				keyboard->lastVoiceToChange = i;
				
				keyboard->hexes[note].active = TRUE;
				
				break;
			}
		}
		
		if (!found) //steal
		{
			int whichVoice = keyboard->lastVoiceToChange;
			int oldNote = keyboard->voices[whichVoice];
			keyboard->hexes[oldNote].active = FALSE;
			
			keyboard->voices[whichVoice] = note;
			keyboard->hexes[note].active = TRUE;
		}
	}
}

int otherNote = 0;

void tKeyboard_noteOff(tKeyboard* const keyboard, uint8_t note)
{
	// if not in keymap, return. else remove that note.
	if (keyboard->hexes[note].mapped < 0) return;
	else
	{
		tNoteStack_remove(&keyboard->stack, note);
		
		keyboard->hexes[note].active = FALSE;
		
		int deactivatedVoice = -1;
		for (int i = 0; i < keyboard->numVoices; i++)
		{
			if (keyboard->voices[i] == note)
			{
				keyboard->voices[i] = -1;
				keyboard->lastVoiceToChange = i;
				
				deactivatedVoice = i;
				break;
			}
		}
		
		if (keyboard->numVoices == 1 && tNoteStack_getSize(&keyboard->stack) > 0)
		{
			int oldNote = tNoteStack_first(&keyboard->stack);
			keyboard->voices[0] = oldNote;
			keyboard->lastVoiceToChange = 0;
			keyboard->hexes[oldNote].active = TRUE;
			
		}
		else if (deactivatedVoice != -1)
		{
			int i = 0;

			while (1)
			{
				otherNote = tNoteStack_get(&keyboard->stack, i++);
				if (otherNote < 0 ) break;
				
				if (keyboard->hexes[otherNote].active == FALSE)
				{
					keyboard->voices[deactivatedVoice] = otherNote;
					
					keyboard->lastVoiceToChange = deactivatedVoice;
					keyboard->hexes[otherNote].active = TRUE;
				}
			}
		}
	}
		
		

}
