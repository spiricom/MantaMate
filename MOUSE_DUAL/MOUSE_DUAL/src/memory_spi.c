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
#include "sequencer_process.h"
#include "sequencer.h"


//external memory variables
uint32_t sectors_left_to_erase = 0;
uint16_t currentSector = 0;
uint16_t currentPage = 0;
uint16_t startingSector = 0;
uint32_t pages_left_to_store = 0;
uint32_t pages_left_to_retrieve = 0;
uint16_t busy;

int memorySPICheckIfBusy(void)
{
	memoryWait();gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	
	spi_write(MEMORY_SPI, 0x05); //0x05 (read status register 1)
	memoryWait();
	
	int count = 0;
	int total = 0;
	busy = 1;
	spi_write(MEMORY_SPI, 0x00);
	spi_read(MEMORY_SPI, &busy);
	
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
	return (busy & 1);
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

//no longer blocking!  Must check status before doing anything else with memory chip (it will not be ready otherwise)
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
// No longer blocking - need to check the status before doing anything else with memory chip!   P is still = NP 4eva
void memorySPIEraseSector(uint16_t sector)
{
	// Write Enable
	memorySPIWriteEnable();
	
	// ERASE Sector Erase
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0x20);
	memorySPIWriteAddress(sector, 0);
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
}

// Block 0 - 127
// No longer blocking - need to check the status before doing anything else with memory chip!  P is still = NP  4eva
void memorySPIEraseBlock(uint16_t block)
{
	// Write Enable
	memorySPIWriteEnable();
	
	// ERASE Sector Erase
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0xd8);
	memorySPIWriteAddress((block << 4), 0);
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
}

// EXTERNAL MEMORY PLAN
// a page is 256 bytes.
// a sector is 16 pages (4096 bytes)
// a block is 16 sectors (65535 bytes)

// every preset will live in a block (there are plenty - we have 128 blocks and can only really use 90 presets
// the remaining blocks can store user tunings, perhaps
// every "sequence" takes up 620 bytes.

// a block has 65535 bytes, and they will be set up like this:
// Page 0 is the global settings and manta sequencer settings
// Page 1 is the Manta keyboard-mode preset settings
// Page 2 is the MIDI host preset settings
// Page 3 is the MIDI device preset settings (when connected to a computer)
// Pages 4, 5, 6 is the current sequencer 1 (3 pages per sequence)
// Pages 7, 8, 9 is the current sequencer 2 (3 pages per sequence)
// Pages 10-43 is the composition collection for sequencer 1 (34 pages per sequence if there are 14 storable sequences)
// Pages 44-77 is the composition collection for sequencer 2 (34 pages per sequence if there are 14 storable sequences)

// sector erase takes 50ms vs 500ms block erase, so it makes sense to only erase the sectors we are using (since we only use 4 sectors per preset = 200ms instead of 500ms save time)

//global settings format:
// byte 1 = seq1Pvt, seq2Pvt, sequencer_mode, clock_speed, compositions,  
#define NUM_COMPOSITIONS 2
#define NUM_PAGES_PER_SEQUENCE 3
#define NUM_PAGES_PER_COMPOSITION 34
#define CURRENT_SEQUENCE_PAGE_START 4 // which page starts the current sequencer data  -- this is non zero so that we can have a few pages to store global prefs and non-Manta things before the sequencer stuff
#define COMPOSITIONS_PAGE_START 10 // which page starts the compositions data
#define NUM_PAGES_PER_PRESET (CURRENT_SEQUENCE_PAGE_START + (NUM_PAGES_PER_SEQUENCE*2) + (NUM_PAGES_PER_COMPOSITION*2))
#define NUM_SECTORS_THAT_NEED_TO_BE_ERASED 5

uint8_t whichSequence = 0;
uint8_t sequencePageNumber = 0;

void initiateStoringPresetToExternalMemory(void)
{
	startingSector = preset_to_save_num * 16;  // * 16 to get the sector number we will store it in
	currentSector = preset_to_save_num * 16;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	tSequencer_encode(&sequencer[0], encodeBuffer[0]); //fill a buffer with the local sequencers
	tSequencer_encode(&sequencer[1], encodeBuffer[1]); //one for each
	
	//start by erasing the memory in the location we want to store
	sectors_left_to_erase = NUM_SECTORS_THAT_NEED_TO_BE_ERASED; //erase 5 sectors because that will give us enough room for a whole preset
	pages_left_to_store = NUM_PAGES_PER_PRESET; //set the variable for the total number of pages to count down from while we store them
	memorySPIEraseSector(currentSector); 
	sectors_left_to_erase--;
	savePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringPresetToExternalMemory(void)
{
	//there are still sectors to erase, do those
	if (sectors_left_to_erase > 0)
	{
		currentSector++;
		memorySPIEraseSector(currentSector); //erase 5 sectors because that will give us enough room for a whole preset
		sectors_left_to_erase--;
	}
	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	else if (pages_left_to_store > 0)
	{
		//otherwise we are ready to start saving data
		
		// store a page
		if (currentPage == 0)
		{
			currentSector = startingSector; // start back at the beginning of the memory location for this preset (it was incremented to erase)
			//store global prefs data structure
		}
		else if (currentPage == 1)
		{
			//store MIDI prefs data structure
		}
		else if (currentPage == 2)
		{
			//store joystick prefs data structure
		}
		else if (currentPage == 3)
		{
			//store Manta keyboard mode prefs data structure
		}
		
		//store Manta Sequencer data
		//first store the local memory of sequencers 1 and 2
		else if ((currentPage >= CURRENT_SEQUENCE_PAGE_START) && (currentPage < COMPOSITIONS_PAGE_START))
		{
			whichSequence = (uint16_t)((currentPage - CURRENT_SEQUENCE_PAGE_START) / NUM_PAGES_PER_SEQUENCE);
			sequencePageNumber = (currentPage - CURRENT_SEQUENCE_PAGE_START) % NUM_PAGES_PER_SEQUENCE;
			memorySPIWrite(currentSector,  (currentPage % 16) ,  &encodeBuffer[whichSequence][sequencePageNumber], 256);
		}
			
		//then store the compositions for both sequencers
		else if ((currentPage >= COMPOSITIONS_PAGE_START) && (currentPage < NUM_PAGES_PER_PRESET))
		{
			whichSequence = (uint16_t)((currentPage - COMPOSITIONS_PAGE_START) / NUM_PAGES_PER_COMPOSITION);
			sequencePageNumber = (currentPage - COMPOSITIONS_PAGE_START) % NUM_PAGES_PER_COMPOSITION;
			memorySPIWrite(currentSector,  (currentPage % 16) ,  &memoryInternalCompositionBuffer[whichSequence][sequencePageNumber], 256);
		}
		//update variables for next round
		currentPage++;
		pages_left_to_store--;
		currentSector = (startingSector + (uint16_t)(currentPage / 16)); //increment the current Sector whenever currentPage wraps over 16
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		savePending = 0;	
		LED_Off(PRESET_SAVE_LED);
	}
	//Write7Seg(pages_left_to_store);
}
/*
void storePresetToExternalMemory(void)
{
	uint16_t currentSector = 	preset_to_save_num * 16;  //*16 to get the sector number we will store it in
	//start by erasing the memory in the location we want to store
	for (int i = 0; i < 5; i++)
	{
		memorySPIEraseSector(currentSector + i); //erase 5 sectors because that will give us enough room for a whole preset
	} 
	
	// TODO: save the global data and keyboard settings and whatnot here in the first few pages
	//
	
	// now ready for the sequencer data
	// create buffers and send over data of the local sequencer memory that is not saved as compositions yet
	// both sequencer one and sequencer two
	for (int i = 0; i < NUM_SEQ; i++)
	{
		tSequencer_encode(&sequencer[i], encodeBuffer);
		for (int j = 0; j < NUM_PAGES_PER_SEQUENCE; j++)
		{
			int currentPageNumber = CURRENT_SEQUENCE_PAGE_START + (i * NUM_PAGES_PER_SEQUENCE) + j;
			memorySPIWrite(currentSector,  currentPageNumber ,  &encodeBuffer[currentPageNumber], 256);
		}
	}
	//now save the composition mode sequences stored for each sequencer
	for (int i = 0; i < NUM_SEQ; i++)
	{
		for (int j = 0; j < NUM_PAGES_PER_COMPOSITION; j++) // write each page
		{
			int currentPageNumber = (COMPOSITIONS_PAGE_START + (i * NUM_PAGES_PER_COMPOSITION) + j);
			if (currentPageNumber >= 16)
			{
				currentSector++;
				currentPageNumber = 0;
			}
			memorySPIWrite(currentSector, currentPageNumber, &memoryInternalCompositionBuffer[i][currentPageNumber], 256);
		}
	}
}
*/

void retrievePresetFromExternalMemory(void)
{
	
	/*
	
	uint16_t currentSector = preset_num * 16;  // * 16 to get the sector number we are starting to grab from
	// TODO: retrieve the global data and keyboard settings and whatnot here in the first few pages
	//
	
	// now ready for the sequencer data
	// create buffers and grab the data to fill the local sequencer memory that is not saved as compositions yet
	// both sequencer one and sequencer two
	for (int i = 0; i < NUM_SEQ; i++)
	{	
		for (int j = 0; j < NUM_PAGES_PER_SEQUENCE; j++)
		{
			int currentPageNumber = CURRENT_SEQUENCE_PAGE_START + (i * NUM_PAGES_PER_SEQUENCE) + j;
			memorySPIRead(currentSector,  currentPageNumber ,  &decodeBuffer[currentPageNumber*256], 256);
		}

	}
	//now get the composition mode sequences stored for each sequencer
	for (int i = 0; i < NUM_SEQ; i++)
	{
		for (int j = 0; j < NUM_PAGES_PER_COMPOSITION; j++) // write each page
		{
			int currentPageNumber = (COMPOSITIONS_PAGE_START + (i * NUM_PAGES_PER_COMPOSITION) + j);
			if (currentPageNumber >= 16)
			{
				currentSector++;
				currentPageNumber = 0;
			}
			memorySPIRead(currentSector, currentPageNumber, &memoryInternalCompositionBuffer[i][currentPageNumber*256], 256);
			initSequencer();
		}
	}
	*/
}
