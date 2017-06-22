/*
 * note_process.h
 *
 * Created: 7/22/2015 4:47:53 PM
 *  Author: Elaine
 */ 


#ifndef NOTE_PROCESS_H_
#define NOTE_PROCESS_H_


#include "keyboard.h"
#include <stdint.h>
#include "7Segment.h"
#include <math.h>
#include <asf.h>
#include "tuning.h"
#include "main.h"



unsigned short lookupDACvalue(MantaMap whichmap, uint8_t noteVal);

void initKeys(int numVoices);
void processKeys(void);
void dacSendKeyboard(MantaInstrument);
void processSliderKeys(uint8_t sliderNum, uint16_t val);

void addNote(uint8_t noteVal, uint8_t vel);
void removeNote(uint8_t noteVal);

void controlChange(uint8_t ctrlNum, uint8_t val);
void programChange(uint8_t programNum);

void noteOut(void);
void joyVol(uint16_t slider_val);

void tuningTest(uint8_t whichOct);

// Birl
#define FINGERMAX 11
#define FINGERMIN 10
#define FINGERMAX_SCALE (1.0/(float)(FINGERMAX-FINGERMIN))

void controlChangeBirl(uint8_t ctrlNum, uint8_t val);
void birlPitchOut(void);
float_t calculateBirlPitch(void);
void BirlBreathNegOut(void);
void BirlBreathPosOut(void);

uint16_t finger[11];
uint8_t finger_lowbytes[11];
uint8_t finger_highbytes[11];
uint16_t birlBreathPos;
uint8_t birlBreathPosHigh;
uint8_t birlBreathPosLow1;
uint8_t birlBreathPosLow2;
uint16_t birlBreathNeg;
uint8_t birlBreathNegHigh;
uint8_t birlBreathNegLow1;
uint8_t birlBreathNegLow2;

float_t fingerFloat[11];

float_t	birlOctave;
float_t	birlOffset;

#endif /* NOTE_PROCESS_H_ */
