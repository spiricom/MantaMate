/*
 * keyboard.h
 *
 * Created: 6/19/2017 3:48:19 PM
 *  Author: Jeff Snyder
 */ 


#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "utilities.h"

#include "notestack.h"

signed int whmap[48] ;

signed int harmonicmap[48];

signed int pianomap[48];


typedef struct _tHex
{
	int hexmap;
	int pitch;
	BOOL active;
	uint16_t weight;
	
} tHex;

void tHex_init(tHex* const hex, int which);


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
	
	signed int transpose;
	
} tKeyboard;

void tKeyboard_init(tKeyboard* const keyboard, int numVoices);

//ADDING A NOTE
void tKeyboard_noteOn(tKeyboard* const keyboard, int note, uint8_t vel);

void tKeyboard_noteOff(tKeyboard* const keyboard, uint8_t note);

void tKeyboard_setKeymap(tKeyboard* const keyboard, MantaMap map);

void tKeyboard_setTuning(tKeyboard* const keyboard, MantaTuning tuning);


#endif /* KEYBOARD_H_ */