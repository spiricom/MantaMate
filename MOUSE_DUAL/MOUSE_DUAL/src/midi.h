/*
 * MIDI.h
 *
 * Created: 6/20/2017 10:25:08 PM
 *  Author: Jeff Snyder
 */ 


#ifndef MIDI_H_
#define MIDI_H_

#include "utilities.h"
#include "notestack.h"

uint16_t sysexByteCounter;
uint8_t inSysex;
uint8_t sysexBuffer[1024];
extern uint8_t firstMIDIMessage;

uint16_t parseMIDI(uint16_t howManyNew);
void handleMIDIMessage(uint8_t ctrlByte, uint8_t msgByte1, uint8_t msgByte2);
void parseSysex(void);
void startSysexMessage(int msgByte1, int msgByte2);
void sendSysexSaveConfim(void);
void controlChange(uint8_t ctrlNum, uint8_t val);

void initMIDIArpeggiator(void);
void initMIDIKeys(int numVoices, BOOL pitchout);


typedef struct _tMIDIKeyboard
{
	int numVoices;
	int numVoicesActive;
	
	int voices[MAX_VOICES][2];
	
	int firstFreeOutput;
	
	int notes[128][2];
	
	int CCs[128];
	
	int learnedCCsAndNotes[128][2];
	
	int lastVoiceToChange;
	
	tNoteStack stack;
	
	signed int transpose;
	
	SequencerPatternType pattern;
	
	int trigCount;
	
	int32_t pitchBend;
	
	BOOL pitchOutput;
	
} tMIDIKeyboard;

void tMIDIKeyboard_pitchBend(tMIDIKeyboard* keyboard, uint8_t lowbyte, uint8_t highbyte);

void tMIDIKeyboard_noteOn(tMIDIKeyboard* keyboard, int note, uint8_t vel);

void tMIDIKeyboard_noteOff(tMIDIKeyboard* keyboard, uint8_t note);

void tMIDIKeyboard_init(tMIDIKeyboard* keyboard, int numVoices, int pitchout);

void learnMIDINote(uint8_t msgByte1, uint8_t msgByte2);

void learnMIDICC(uint8_t msgByte1, uint8_t msgByte2);

#endif /* MIDI_H_ */
