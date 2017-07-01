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

signed int whmap[48] ;

signed int harmonicmap[48];

signed int pianomap[48];


#define NUM_HEXES 48
#define MAX_VOICES 4

typedef struct _tKeyboard
{
	int numVoices;
	int numVoicesActive;
	
	tHex hexes[48];
	
	int voices[MAX_VOICES];
	
	int lastVoiceToChange;
	
	tNoteStack stack;
	
	MantaMap map;
	
	MantaTuning tuning;
	
	int currentHexmap;

	int hexmaps[8];
	
	signed int transpose;
	
	SequencerPatternType pattern;
	
	int trigCount;
	
} tKeyboard;

void tKeyboard_init(tKeyboard* const keyboard, int numVoices);

//ADDING A NOTE
void tKeyboard_noteOn(tKeyboard* const keyboard, int note, uint8_t vel);

void tKeyboard_noteOff(tKeyboard* const keyboard, uint8_t note);

void tKeyboard_setHexmap(tKeyboard* const keyboard, MantaMap map, signed int custom[48]);

void tKeyboard_setTuning(tKeyboard* const keyboard, MantaTuning tuning);


#endif /* KEYBOARD_H_ */