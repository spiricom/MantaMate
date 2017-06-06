/*
 * memory_spi.h
 *
 * Created: 3/31/2017 5:54:16 PM
 *  Author: Jeff Snyder
 */ 


#ifndef MEMORY_SPI_H_
#define MEMORY_SPI_H_


// Memory SPI
void memorySPIWrite(int sector, int page, uint16_t* buffer, int numBytes);
void memorySPIRead(int sector, int page, uint16_t* buffer, int numBytes);
void memorySPIEraseSector(uint16_t sector); // Sector can be 0-2047. Will block / synchronous / sequential / might never return / P = NP
void memorySPIEraseBlock(uint16_t block); // Block 0 - 127Will block / synchronous / sequential / might never return / P = NP

void memorySPIWriteSequencer(int whichPreset, int whichSeq, uint16_t* buffer);
void memorySPIReadSequencer(int whichPreset, int whichSeq, uint16_t* buffer);

void sendDataToExternalMemory(void);


#endif /* MEMORY_SPI_H_ */