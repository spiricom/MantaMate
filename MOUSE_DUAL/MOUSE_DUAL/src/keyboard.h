/*
 * keyboard.h
 *
 * Created: 6/19/2017 3:48:19 PM
 *  Author: Mike Mulshine
 */ 


#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "utilities.h"

#include "hex.h"

#include "notestack.h"

signed int defaultHexmap[48] ;

signed int blankHexmap[48];

signed int whmap[48] ;

signed int harmonicmap[48];

signed int pianomap[48];
	
typedef struct _tKeyboard
{
	int numVoices;
	int numVoicesActive;
	
	int voices[MAX_VOICES];
	
	int lastVoiceToChange;
	
	tNoteStack stack;
	
	MantaMap map;

	tHex hexes[48];
	
	signed int transpose;
	
	SequencerPatternType pattern;
	
	int trigCount;
	
} tKeyboard;

void tKeyboard_init(tKeyboard* const keyboard, int numVoices);

//ADDING A NOTE
void tKeyboard_noteOn(tKeyboard* const keyboard, int note, uint8_t vel);

void tKeyboard_noteOff(tKeyboard* const keyboard, uint8_t note);


void tKeyboard_setHexmap(tKeyboard* const keyboard,signed int pitch[48], signed int color[48]);
void tKeyboard_blankHexmap(tKeyboard* const keyboard);
void tKeyboard_assignNoteToHex(tKeyboard* const keyboard, int whichHex, int whichNote);
signed int tKeyboard_getCurrentNoteForHex(tKeyboard* const keyboard, int whichHex);

#endif /* KEYBOARD_H_ */