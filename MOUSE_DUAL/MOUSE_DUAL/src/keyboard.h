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

signed int freemap[48];

signed int isomap[48];

#define NUM_BYTES_PER_HEXMAP 240 // was 1152 = 144 * 8 which was always wrong... now 

uint8_t hexmapBuffer[NUM_BYTES_PER_HEXMAP];
	
typedef struct _tKeyboard
{
	// Encode this in preset
	int numVoices;
	tHex hexes[48]; // 288 bytes
	signed int transpose;
	MantaPlayMode playMode;
	ArpModeType arpModeType;
	// - - - - - - - - - - -
	
	MantaLEDColor blackKeyColor;
	int numVoicesActive;
	int voices[MAX_VOICES];
	int lastVoiceToChange;
	tNoteStack stack;
	tNoteStack orderStack;
	MantaMap map;
	int trigCount[4];
	int currentNote;
	int currentVoice;
	int maxLength;
	int phasor;
	BOOL up;
	
	
} tKeyboard;

void tKeyboard_init(tKeyboard* const keyboard, int numVoices, MantaLEDColor blackKeyColor);

//ADDING A NOTE
void tKeyboard_noteOn(tKeyboard* const keyboard, int note, uint8_t vel);

void tKeyboard_noteOff(tKeyboard* const keyboard, uint8_t note);

void tKeyboard_setArpModeType(tKeyboard* const keyboard, ArpModeType type);

void tKeyboard_orderedAddToStack(tKeyboard* thisKeyboard, uint8_t noteVal);

void tKeyboard_nextNote(tKeyboard* const keyboard);

void tKeyboard_setToDefault(tKeyboard* const keyboard, MantaMap which);
void tKeyboard_setHexmap(tKeyboard* const keyboard,signed int pitch[48]);
void tKeyboard_blankHexmap(tKeyboard* const keyboard);
void tKeyboard_assignNoteToHex(tKeyboard* const keyboard, int whichHex, int whichNote);
signed int tKeyboard_getCurrentNoteForHex(tKeyboard* const keyboard, int whichHex);

void tKeyboard_hexmapEncode(tKeyboard* const keyboard, uint8_t* buffer);
void tKeyboard_hexmapDecode(tKeyboard* const keyboard, uint8_t* buffer);

void tKeyboard_encode(tKeyboard* const keyboard, uint8_t* buffer);
void tKeyboard_decode(tKeyboard* const keyboard, uint8_t* buffer);

#endif /* KEYBOARD_H_ */