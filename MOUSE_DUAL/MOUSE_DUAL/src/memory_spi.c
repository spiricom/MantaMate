/*
 * memory_spi.c
 *
 * Created: 3/31/2017 5:03:56 PM
 *  Author: Jeff Snyder
 */ 

//S25FL164K0XMFA011 is the chip (or a newer chip in the same family)

//a page is 256 bytes. 
// a sector is 16 pages (4096 bytes)
// a block is 16 sectors (65535 bytes)


#include "main.h"
#include "memory_spi.h"

uint16_t myReadBuffer1[1000];
int whichSector = 2;
uint16_t myOddNumbers[10] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
uint16_t myEvenNumbers[10] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18};
uint16_t myNumbers[10] = {1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9, 10};
uint16_t myCrazyNumbers[10] = {17,7,17,7,17,7,17,7,17,7};
	
uint16_t busy;

static void memorySPIWaitWhileBusy(void)
{
	
	memoryWait();gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	
	spi_write(MEMORY_SPI, 0x05); //0x05 (read status register 1)
	memoryWait();
	
	int count = 0;
	int total = 0;
	busy = 1;
	while (busy & 1)
	{
		spi_write(MEMORY_SPI, 0x00);
		spi_read(MEMORY_SPI, &busy);
		
		//if (++count == 50) { count = 0; Write7Seg(erasing & 1);}
		//total++;
	} 
	
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
	
}


static void memorySPIWriteAddress(uint16_t sector, uint16_t page)
{
	if (sector < 0 || sector > 2047) return;
	if (page < 0 || page > 15) return;
	
	uint32_t address = (sector << 12) + (page << 8);
	
	spi_write(MEMORY_SPI, (address & 0x00ff0000) >> 16);
	spi_write(MEMORY_SPI, (address & 0x0000ff00) >> 8 );
	spi_write(MEMORY_SPI, (address & 0x000000ff)      );
}

static void memorySPIWriteEnable(void)
{
	memoryWait();gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0x06); // Write Enable code
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
}

void memorySPIWrite(int sector, int page, uint16_t* buffer, int numBytes)
{
	// Write Enable
	memorySPIWriteEnable();
	
	// WRITE Page Program
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0x02);  // Page Program code
	memorySPIWriteAddress(sector, page);
	
	for (int i = 0; i < numBytes; i++)
	{
		spi_write(MEMORY_SPI, (buffer[i]&0xffff));
	}
	
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
	
	// Wait until Write done
	memorySPIWaitWhileBusy();
}

void memorySPIRead(int sector, int page, uint16_t* buffer, int numBytes)
{
	// READ Read Data
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0x03); // Read Data code
	memorySPIWriteAddress(sector, page);
	
	//do a dummy write to read in the value
	for (int i = 0; i < numBytes; i++)
	{
		spi_write(MEMORY_SPI, 0x00);
		spi_read(MEMORY_SPI, &buffer[i]);
	}
	
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
}

// Sector can be 0-2047
// Will block / synchronous / sequential / might never return / P = NP
void memorySPIEraseSector(uint16_t sector)
{
	// Write Enable
	memorySPIWriteEnable();
	
	// ERASE Sector Erase
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0x20);
	memorySPIWriteAddress(sector, 0);
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
	
	// Wait until Erase done
	memorySPIWaitWhileBusy();
}

// Block 0 - 127
// Will block / synchronous / sequential / might never return / P = NP
void memorySPIEraseBlock(uint16_t block)
{
	// Write Enable
	memorySPIWriteEnable();
	
	// ERASE Sector Erase
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0xd8);
	memorySPIWriteAddress((block << 4), 0);
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
	
	// Wait until Erase done
	memorySPIWaitWhileBusy();
}

void memorySPIWriteSequencer(int whichPreset, int whichSeq, uint16_t* buffer)
{
	int thisBlock = (whichPreset << 4); // 16 sectors in block
	int pageCount = whichSeq * 4; // 4 pages per sequence
	
	for (int i = 0; i < 4; i++) // write each page
	{
		int thisSector = (int)((pageCount + i) / 16);
		int thisPage = ((pageCount + i) % 16);
		memorySPIWrite(thisBlock + thisSector, thisPage, &buffer[i*128], 256);
	}
	
}

//a page is 256 bytes.
// a sector is 16 pages (4096 bytes)
// a block is 16 sectors (65535 bytes)

void memorySPIReadSequencer(int whichPreset, int whichSeq, uint16_t* buffer)
{
	int thisBlock = (whichPreset << 4);
	int pageCount = whichSeq * 4;
	
	for (int i = 0; i < 4; i++)
	{
		int thisSector = (int)((pageCount + i) / 16);
		int thisPage = ((pageCount + i) % 16);
		memorySPIRead(thisBlock + thisSector, thisPage, &buffer[i*128], 256);
	}
	
}



void sendDataToExternalMemory(void)
{
	memorySPIEraseBlock(0);

	memorySPIWrite(whichSector,  1,  myOddNumbers, 10);
	
	memorySPIWrite(whichSector, 5,  myEvenNumbers, 10);
	
	memorySPIWrite(whichSector, 3,  myNumbers, 10);
	
	memorySPIWrite(whichSector, 7,  myCrazyNumbers, 10);
	
	memorySPIRead(whichSector, 1, &myReadBuffer1[0], 10);
	
	memorySPIRead(whichSector, 5, &myReadBuffer1[10], 10);
	
	memorySPIRead(whichSector, 3, &myReadBuffer1[20], 10);
	
	memorySPIRead(whichSector, 7, &myReadBuffer1[30], 10);
	
	while(true)
	{
		for (int i = 0; i < 40; i++)
		{
			Write7Seg(myReadBuffer1[i]);
			
			delay_ms(500);
		}
	}
}