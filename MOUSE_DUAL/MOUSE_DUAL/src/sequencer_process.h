/*
 * sequencer_process.h
 *
 * Created: 6/10/2016 1:44:56 PM
 *  Author: Jeff Snyder
 */ 


#ifndef SEQUENCER_PROCESS_H_
#define SEQUENCER_PROCESS_H_

#include "sequencer.h"


void sequencerStep(MantaInstrument inst);
void uiStep					(MantaInstrument);

void keyboardStep(MantaInstrument inst);
void MIDIKeyboardStep(void);

void processHexTouch(void);

void touchHexmapEdit(int hex, uint8_t weight);
void releaseHexmapEdit(int hex);

void processSliderSequencer(uint8_t sliderNum, uint16_t val);
void setTuningLEDs(void);
void initMantaSequencer(void);
void memoryInternalReadSequencer(int whichSeq, int whichhex, uint8_t* buffer);
void memoryInternalWriteSequencer(int whichSeq, int whichhex, uint8_t* buffer);
void memoryInternalCopySequencer(int sourceSeq, int sourceComp, int destSeq, int destComp);
void clearSequencer(MantaInstrument inst);
void touchDirectHex(int hex);
void releaseDirectHex(int hex);
void jumpToStep(MantaInstrument inst, int step);

void switchToMode(MantaEditPlayMode mode);

void initializeStoringPresetToExternalMemory(void);
void continueStoringPresetToExternalMemory(void);
void retrievePresetFromExternalMemory(void);

void indicateTransposition(int number);


#endif /* SEQUENCER_PROCESS_H_ */
