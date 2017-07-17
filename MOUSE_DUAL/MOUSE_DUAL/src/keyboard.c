/*
 * keyboard.c
 *
 * Created: 6/19/2017 3:48:02 PM
 *  Author: Mike Mulshine
 */ 

#include "keyboard.h"
#include "tuning.h"
#include "stdlib.h"

signed int blankHexmap[48] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

signed int defaultHexmap[48] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};


signed int whmap[48] = {
	0,2,4,6,8,10,12,14,
	7,9,11,13,15,17,19,21,
	12,14,16,18,20,22,24,26,
	19,21,23,25,27,29,31,33,
	24,26,28,30,32,34,36,38,
	31,33,35,37,39,41,43,45};
	

signed int harmonicmap[48] = {0,4,8,12,16,20,24,28,7,11,15,19,23,27,31,35,10,14,18,22,26,30,34,38,17,21,25,29,33,37,41,45,20,24,28,32,36,40,44,48,27,31,35,39,43,47,51,55};

signed int pianomap[48] = {0,2,4,5,7,9,11,12,1,3,-1,6,8,10,-1,-1,12,14,16,17,19,21,23,24,13,15,-1,18,20,22,-1,-1,24,26,28,29,31,33,35,36,25,27,-1,30,32,34,-1,-1};
	
signed int freemap[48] = {
	6,8,10,12,14,16,18,19,
	13,15,17,19,21,23,25,27,
	18,20,22,24,26,28,30,32,
	25,27,29,31,33,35,37,39,
	30,32,34,36,38,40,42,44,
	37,39,41,43,45,47,49,51 };

signed int isomap[48] = {
	18,20,22,24,26,28,30,32,
	19,21,23,25,27,29,31,33,
	25,27,29,31,33,35,37,39,
	26,28,30,32,34,36,38,40,
	32,34,36,38,40,42,44,46,
	33,35,37,39,41,43,45,47};
	
	
//signed int hexmaps[100][48];

void tKeyboard_setToDefault(tKeyboard* const keyboard, MantaMap which)
{
	if (which == DefaultMap)
	{
		for (int i = 0; i < 48; i++)
		{
			keyboard->hexes[i].pitch = defaultHexmap[i];
			keyboard->hexes[i].color = Amber;
		}
	}
	else if (which == PianoMap)
	{
		for (int i = 0; i < 48; i++)
		{
			int pitch = pianomap[i];
			keyboard->hexes[i].pitch = pitch;
			
			MantaLEDColor color = Off;
			
			if (pitch >= 0)
			{
				switch (pitch%12)
				{
					case 1:
					color = Red;
					break;
					
					case 3:
					color = Red;
					break;
					
					case 6:
					color = Red;
					break;
					
					case 8:
					color = Red;
					break;
					
					case 10:
					color = Red;
					break;
					
					default:
					color = Amber;
					break;
				}
			}
			
			keyboard->hexes[i].color = color;
			
		}
	}
	else if (which == HarmonicMap || which == WickiHaydenMap)
	{
		signed int* pitches = (which == HarmonicMap) ? harmonicmap : whmap;
		for (int i = 0; i < 48; i++)
		{
			int pitch = keyboard->hexes[i].pitch = pitches[i];
			
			int pitchMod12 = pitch % 12;
			
			MantaLEDColor color = Off;
			
			if (pitchMod12 == 7)		color = Amber;
			else if (pitchMod12 == 0)	color = Red;
			
			keyboard->hexes[i].color = color;
		}
	}
	else if (which == IsomorphicMap || which == FreeMap)
	{
		signed int* pitches = (which == IsomorphicMap) ? isomap : freemap;
		for (int i = 0; i < 48; i++)
		{
			int pitch = keyboard->hexes[i].pitch = pitches[i];
			
			int pitchMod12 = pitch % 12;
			
			MantaLEDColor color = Off;
			
			if ((pitchMod12 == 9) || (pitchMod12 == 0))			color = Red;
			else if ((pitchMod12 == 4) || (pitchMod12 == 3))	color = Amber;
			
			keyboard->hexes[i].color = color;
		}
	}
}


void tKeyboard_setHexmap(tKeyboard* const keyboard, signed int pitch[48])
{
	for (int i = 0; i < 48; i++)
	{
		keyboard->hexes[i].pitch = pitch[i];
	}
}

void tKeyboard_blankHexmap(tKeyboard* const keyboard)
{
	for (int i = 0; i < 48; i++)
	{
		keyboard->hexes[i].pitch = -1;
		keyboard->hexes[i].color = Off;
	}
}

void tKeyboard_assignNoteToHex(tKeyboard* const keyboard, int whichHex, int whichNote)
{
	keyboard->hexes[whichHex].pitch = whichNote;
	
	if (whichNote < 0) keyboard->hexes[whichHex].color = Off;
}

signed int tKeyboard_getCurrentNoteForHex(tKeyboard* const keyboard, int whichHex)
{
	return keyboard->hexes[whichHex].pitch;
}

void tKeyboard_setArpModeType(tKeyboard* const keyboard, ArpModeType type)
{
	keyboard->arpModeType = type;
}


void tKeyboard_nextNote(tKeyboard* const keyboard)
{
	int hex = -1;
	int random;
	ArpModeType type = keyboard->arpModeType;
	
	tNoteStack* ns = ((type >= ArpModeUp && type <= ArpModeUpDown) || type == ArpModeRandomWalk) ? &keyboard->orderStack :  &keyboard->stack;
	
	if (type == ArpModeUp || type == ArpModeOrderTouchBackward)
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
		random = rand() % 0xffff;
		if (random > 0x8000)
		{
			keyboard->phasor = (keyboard->phasor + 1) % ns->size;
		}
		else
		{
			keyboard->phasor = (keyboard->phasor - 1) % ns->size;
		}
	}
	else if (type == ArpModeRandom)
	{
		keyboard->phasor = rand() % ns->size;
	}
		

	hex = tNoteStack_get(ns, keyboard->phasor);
		
	if (hex >= 0)
	{
		keyboard->currentNote = hex;
	}

}

void tKeyboard_init(tKeyboard* const keyboard, int numVoices)
{
	// Arp mode stuff
	keyboard->currentVoice = 0;
	keyboard->maxLength = 48;
	keyboard->phasor = 0;
	keyboard->arpModeType = ArpModeUp;
	keyboard->playMode = TouchMode;
	keyboard->currentNote = -1;
	
	// Normal stuff
	keyboard->numVoices = numVoices;
	keyboard->numVoicesActive = numVoices;
	keyboard->lastVoiceToChange = 0;
	keyboard->transpose = 0;
	
	
	for (int i = 0; i < 4; i ++)
	{
		keyboard->voices[i] = -1;
		keyboard->trigCount[i] = 0;
	}
	
	for (int i = 0; i < 48; i++)
	{
		tHex_init(&keyboard->hexes[i], i);
	}
	
	tNoteStack_init(&keyboard->stack, 48);
	tNoteStack_init(&keyboard->orderStack, 48);
}

void tKeyboard_orderedAddToStack(tKeyboard* thisKeyboard, uint8_t noteVal)
{
	uint8_t j;
	int myPitch, thisPitch, nextPitch;
	
	tNoteStack* ns = &thisKeyboard->orderStack;
	
	int whereToInsert = 0;

	for (j = 0; j < ns->size; j++)
	{
		myPitch = thisKeyboard->hexes[noteVal].pitch;
		thisPitch = thisKeyboard->hexes[ns->notestack[j]].pitch;
		nextPitch = thisKeyboard->hexes[ns->notestack[j+1]].pitch;
			
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

//ADDING A NOTE
void tKeyboard_noteOn(tKeyboard* const keyboard, int note, uint8_t vel)
{
	// if not in keymap or already on stack, dont do anything. else, add that note.
	if ((keyboard->hexes[note].pitch < 0) || (tNoteStack_contains(&keyboard->stack, note) >= 0)) return;
	else
	{
		tKeyboard_orderedAddToStack(keyboard, note);
		tNoteStack_add(&keyboard->stack, note);
			
		if (keyboard->playMode != ArpMode)
		{
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
				//if (keyboard->numVoices == 1)
				
				{
					int whichVoice = keyboard->lastVoiceToChange;
					
					int oldNote = keyboard->voices[whichVoice];
					keyboard->hexes[oldNote].active = FALSE;
					
					keyboard->voices[whichVoice] = note;
					keyboard->hexes[note].active = TRUE;
					
					keyboard->lastVoiceToChange = whichVoice;
				}
				
				/*
				else 
				{
					for (int i = 0; i < keyboard->numVoices; i++)
					{
						int thisVoiceNote = keyboard->voices[i];
						
						if (tNoteStack_contains(&keyboard->stack, thisVoiceNote) >= 0) continue;
						else
						{
							keyboard->hexes[thisVoiceNote].active = FALSE;
							keyboard->lastVoiceToChange = i;
							keyboard->voices[i] = note;
							keyboard->hexes[note].active = TRUE;
						}
					}
				}
				*/
				
			}
		}
		
		
	}
}

int otherNote = 0;

void tKeyboard_noteOff(tKeyboard* const keyboard, uint8_t note)
{
	// if not in keymap, return. else remove that note.
	if (keyboard->hexes[note].pitch < 0) return;
	else
	{
		tNoteStack_remove(&keyboard->stack, note);
		tNoteStack_remove(&keyboard->orderStack, note);
		
		if (keyboard->playMode != ArpMode)
		{
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
		
		

}


void tKeyboard_hexmapEncode(tKeyboard* const keyboard, uint8_t* buffer)
{
	for (int i = 0; i < 48; i++)
	{
		buffer[i*3] = keyboard->hexes[i].pitch >> 8;
		buffer[(i*3) + 1] = keyboard->hexes[i].pitch & 0xff;
		buffer[(i*3) + 2] = keyboard->hexes[i].color & 0xff;
	}
}

void tKeyboard_hexmapDecode(tKeyboard* const keyboard, uint8_t* buffer)
{
	for (int i = 0; i < 48; i++)
	{
		keyboard->hexes[i].pitch = (buffer[i*3] << 8) + buffer[(i*3)+1];
		keyboard->hexes[i].color = (MantaLEDColor)buffer[(i*3)+2];
	}
}

void tKeyboard_encode(tKeyboard* const keyboard, uint8_t* buffer)
{
	buffer[0] = keyboard->numVoices;
	buffer[1] = keyboard->transpose >> 8;
	buffer[2] = keyboard->transpose & 0xff;
	buffer[3] = keyboard->playMode;
	buffer[4] = keyboard->arpModeType;
	tKeyboard_hexmapEncode(keyboard, &buffer[5]);
}

//3*48 + 5 = 149
void tKeyboard_decode(tKeyboard* const keyboard, uint8_t* buffer)
{
	keyboard->numVoices = buffer[0];
	keyboard->transpose = (buffer[1] << 8) +  buffer[2];
	keyboard->playMode = buffer[3];
	keyboard->arpModeType = buffer[4];
	tKeyboard_hexmapDecode(keyboard, &buffer[5]);
}