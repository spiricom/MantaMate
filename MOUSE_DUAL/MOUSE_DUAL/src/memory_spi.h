/*
 * memory_spi.h
 *
 * Created: 3/31/2017 5:54:16 PM
 *  Author: Jeff Snyder
 */ 


#ifndef MEMORY_SPI_H_
#define MEMORY_SPI_H_


#include "sequencer_process.h"
#include "sequencer.h"
#include "main.h"
#include "midi.h"

#define NUM_TRANSFER_TYPES 19

//variables for the external memory operations -- maybe could be moved to the memory_spi.h
extern uint32_t sectors_left_to_erase[NUM_TRANSFER_TYPES];
extern uint16_t currentSector[NUM_TRANSFER_TYPES];
extern uint16_t currentPage[NUM_TRANSFER_TYPES];
extern uint16_t startingSector[NUM_TRANSFER_TYPES];
extern unsigned char pending[NUM_TRANSFER_TYPES];
// Memory SPI

int memorySPICheckIfBusy(void);
void memorySPIWrite(int sector, int page, uint8_t* buffer, int numBytes);
void memorySPIRead(int sector, int page, uint8_t* buffer, int numBytes);
void memorySPIEraseSector(uint16_t sector); // Sector can be 0-2047
void memorySPIEraseBlock(uint16_t block); // Block 0 - 127

void initiateStoringMantaPresetToExternalMemory(void);
void continueStoringMantaPresetToExternalMemory(void);
void initiateLoadingMantaPresetFromExternalMemory(void);
void continueLoadingMantaPresetFromExternalMemory(void);

void initiateStoringMantaCompositionsToExternalMemory(void);
void continueStoringMantaCompositionsToExternalMemory(void);
void initiateLoadingMantaCompositionsFromExternalMemory(void);
void continueLoadingMantaCompositionsFromExternalMemory(void);

void initiateStoringMidiPresetToExternalMemory(void);
void continueStoringMidiPresetToExternalMemory(void);
void initiateLoadingMidiPresetFromExternalMemory(void);
void continueLoadingMidiPresetFromExternalMemory(void);

void initiateStoringNoDevicePresetToExternalMemory(void);
void continueStoringNoDevicePresetToExternalMemory(void);
void initiateLoadingNoDevicePresetFromExternalMemory(void);
void continueLoadingNoDevicePresetFromExternalMemory(void);

void initiateStoringTuningToExternalMemory(void);
void continueStoringTuningToExternalMemory(void);
void initiateLoadingTuningFromExternalMemory(void);
void continueLoadingTuningFromExternalMemory(void);

void initiateStoringStartupStateToExternalMemory(void);
void continueStoringStartupStateToExternalMemory(void);
void loadStartupStateFromExternalMemory(void);
void continueLoadingStartupStateFromExternalMemory(void);

void initiateStoringHexmapToExternalMemory(uint8_t hexmap_num_to_save);
void continueStoringHexmapToExternalMemory(void);
void initiateLoadingHexmapFromExternalMemory(uint8_t hexmap_to_load);
void continueLoadingHexmapFromExternalMemory(void);

void initiateStoringDirectToExternalMemory(uint8_t direct_num_to_save);
void continueStoringDirectToExternalMemory(void);
void initiateLoadingDirectFromExternalMemory(uint8_t direct_to_load);
void continueLoadingDirectFromExternalMemory(void);

void initiateStoringSequencerToExternalMemory(uint8_t direct_num_to_save);
void continueStoringSequencerToExternalMemory(void);
void initiateLoadingSequencerFromExternalMemory(uint8_t direct_to_load);
void continueLoadingSequencerFromExternalMemory(void);

#endif /* MEMORY_SPI_H_ */
