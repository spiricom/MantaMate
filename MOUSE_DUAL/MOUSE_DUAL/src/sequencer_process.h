/*
 * sequencer_process.h
 *
 * Created: 6/10/2016 1:44:56 PM
 *  Author: Jeff Snyder
 */ 


#ifndef SEQUENCER_PROCESS_H_
#define SEQUENCER_PROCESS_H_

#include "sequencer.h"

#define NUM_INST 2
#define sizeOfSerializedSequence  620 // increase this if the size of the serialized data gets larger (I set them to just slightly above the needed 611)
#define sizeOfBankOfSequences  sizeOfSerializedSequence*14 //there are now 14 possible sequence slots in composition mode

extern uint16_t encodeBuffer[NUM_INST][sizeOfSerializedSequence];
extern uint16_t decodeBuffer[NUM_INST][sizeOfSerializedSequence];
extern uint16_t memoryInternalCompositionBuffer[NUM_INST][sizeOfBankOfSequences];


void sequencerStep(MantaInstrument inst);
void uiStep					(MantaInstrument);

void processHexTouch(void);

void processSliderSequencer(uint8_t sliderNum, uint16_t val);

void initSequencer(void);
void memoryInternalReadSequencer(int whichSeq, int whichhex, uint16_t* buffer);
void memoryInternalWriteSequencer(int whichSeq, int whichhex, uint16_t* buffer);
void memoryInternalCopySequencer(int sourceSeq, int sourceComp, int destSeq, int destComp);

void initializeStoringPresetToExternalMemory(void);
void continueStoringPresetToExternalMemory(void);
void retrievePresetFromExternalMemory(void);


#endif /* SEQUENCER_PROCESS_H_ */
