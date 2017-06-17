/*
 * memory_spi.h
 *
 * Created: 3/31/2017 5:54:16 PM
 *  Author: Jeff Snyder
 */ 


#ifndef MEMORY_SPI_H_
#define MEMORY_SPI_H_

//variables for the external memory operations -- maybe could be moved to the memory_spi.h
extern uint32_t sectors_left_to_erase;
extern uint16_t currentSector;
extern uint16_t currentPage;
extern uint16_t startingSector;
extern uint32_t pages_left_to_store;
extern uint32_t pages_left_to_retrieve;

// Memory SPI

int memorySPICheckIfBusy(void);
void memorySPIWrite(int sector, int page, uint16_t* buffer, int numBytes);
void memorySPIRead(int sector, int page, uint16_t* buffer, int numBytes);
void memorySPIEraseSector(uint16_t sector); // Sector can be 0-2047
void memorySPIEraseBlock(uint16_t block); // Block 0 - 127

void initiateStoringPresetToExternalMemory(void);
void continueStoringPresetToExternalMemory(void);

#endif /* MEMORY_SPI_H_ */