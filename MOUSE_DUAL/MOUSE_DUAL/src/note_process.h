/*
 * note_process.h
 *
 * Created: 7/22/2015 4:47:53 PM
 *  Author: Elaine
 */ 


#ifndef NOTE_PROCESS_H_
#define NOTE_PROCESS_H_

#include <stdint.h>

void initNoteStack(void);
void addNote(uint8_t noteVal, uint8_t vel);
void removeNote(uint8_t noteVal);
void mantaVol(uint8_t *butts);
void midiVol(void);

void controlChange(uint8_t ctrlNum, uint8_t val);
void programChange(uint8_t programNum);

void noteOut(void);
void joyVol(uint16_t slider_val);

void tuningTest(uint8_t whichOct);

#endif /* NOTE_PROCESS_H_ */
