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
void initMIDIAllCV(void);
void initMIDIAllGates(void);
void initMIDIAllTriggers(void);
void initMIDICVAndGates(void);
void initMIDICVAndTriggers(void);

void MIDIKeyboardStep(void);

void dacSendMIDIKeyboard(void);


typedef struct _tMIDIKeyboard
{
	// Encode this in preset
	int numVoices;
	uint8_t learnedCCsAndNotes[128][2];
	MantaPlayMode playMode;
	ArpModeType arpModeType;
	signed int transpose;
	BOOL learned;
	int firstFreeOutput;
	GateVsTriggerType gatesOrTriggers;
	// - - - - - - - - - - -
	
	int numVoicesActive;
	
	int voices[MAX_VOICES][2];
	
	int notes[128][2];
	
	int CCs[128];
	
	uint8_t CCsRaw[128];
	
	int lastVoiceToChange;
	
	tNoteStack stack;
	tNoteStack orderStack;
	
	int32_t pitchBend;
	
	BOOL pitchOutput;
	
	int trigCount[4];
	int currentNote;
	int currentVoice;
	int maxLength;
	int phasor;
	BOOL up;
	
} tMIDIKeyboard;

void tMIDIKeyboard_pitchBend(tMIDIKeyboard* keyboard, uint8_t lowbyte, uint8_t highbyte);

void tMIDIKeyboard_noteOn(tMIDIKeyboard* keyboard, int note, uint8_t vel);

void tMIDIKeyboard_noteOff(tMIDIKeyboard* keyboard, uint8_t note);

void tMIDIKeyboard_init(tMIDIKeyboard* keyboard, int numVoices, int pitchout);

void learnMIDINote(uint8_t msgByte1, uint8_t msgByte2);

void learnMIDICC(uint8_t msgByte1, uint8_t msgByte2);

void tMIDIKeyboard_nextNote(tMIDIKeyboard* const keyboard);
void tMIDIKeyboard_orderedAddToStack(tMIDIKeyboard* thisKeyboard, uint8_t noteVal);

void tMIDIKeyboard_encode(tMIDIKeyboard* const keyboard, uint8_t* buffer);
void tMIDIKeyboard_decode(tMIDIKeyboard* const keyboard, uint8_t* buffer);

#endif /* MIDI_H_ */
