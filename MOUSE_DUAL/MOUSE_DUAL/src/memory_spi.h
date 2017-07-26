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

//variables for the external memory operations -- maybe could be moved to the memory_spi.h
extern uint32_t sectors_left_to_erase;
extern uint16_t currentSector;
extern uint16_t currentPage;
extern uint16_t startingSector;
extern unsigned char mantaSavePending;
extern unsigned char mantaLoadPending;
extern unsigned char midiSavePending;
extern unsigned char midiLoadPending;
extern unsigned char noDeviceSavePending;
extern unsigned char noDeviceLoadPending;
extern unsigned char tuningSavePending;
extern unsigned char tuningLoadPending;
extern unsigned char startupStateSavePending;
extern unsigned char startupStateLoadPending;
extern unsigned char hexmapSavePending;
extern unsigned char hexmapLoadPending;
extern unsigned char directSavePending;
extern unsigned char directLoadPending;
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

void initiateStoringTuningToExternalMemory(uint8_t tuning_num_to_save);
void continueStoringTuningToExternalMemory(void);
void initiateLoadingTuningFromExternalMemory(uint8_t tuning_to_load);
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

#endif /* MEMORY_SPI_H_ */
