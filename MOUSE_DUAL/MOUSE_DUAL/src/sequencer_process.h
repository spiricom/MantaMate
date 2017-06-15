/*
 * sequencer_process.h
 *
 * Created: 6/10/2016 1:44:56 PM
 *  Author: Jeff Snyder
 */ 


#ifndef SEQUENCER_PROCESS_H_
#define SEQUENCER_PROCESS_H_

#include "sequencer.h"

#define NUM_SEQ 2
#define sizeOfSerializedSequence  620 // increase this if the size of the serialized data gets larger (I set them to just slightly above the needed 611)
#define sizeOfBankOfSequences  sizeOfSerializedSequence*16

extern uint16_t memoryInternalCompositionBuffer[NUM_SEQ][sizeOfBankOfSequences];

void sequencerStep(void);

void processSequencer(void);

void processSliderSequencer(uint8_t sliderNum, uint16_t val);

void initSequencer(void);
void memoryInternalReadSequencer(int whichSeq, int whichhex, uint16_t* buffer);
void memoryInternalWriteSequencer(int whichSeq, int whichhex, uint16_t* buffer);
void memoryInternalCopySequencer(int sourceSeq, int sourceComp, int destSeq, int destComp);

void storePresetToExternalMemory(void);
void retrievePresetFromExternalMemory(void);


#endif /* SEQUENCER_PROCESS_H_ */
