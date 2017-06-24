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


uint8_t applyNoteMap(MantaMap whichmap, uint8_t noteVal);

unsigned short lookupDACvalue(uint8_t noteVal, signed int transpose);

void initKeys(int numVoices);
void dacSendKeyboard(MantaInstrument);
void processSliderKeys(uint8_t sliderNum, uint16_t val);

void controlChange(uint8_t ctrlNum, uint8_t val);
void programChange(uint8_t programNum);

void noteOut(void);

void tuningTest(uint8_t whichOct);


#endif /* NOTE_PROCESS_H_ */
