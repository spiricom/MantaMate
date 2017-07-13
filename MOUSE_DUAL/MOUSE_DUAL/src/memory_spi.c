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



#include "memory_spi.h"



//external memory variables
uint32_t sectors_left_to_erase = 0;
uint16_t currentSector = 0;
uint16_t currentPage = 0;
uint16_t startingSector = 0;
uint32_t pages_left_to_store = 0;
uint32_t pages_left_to_load = 0;
unsigned char savePending = 0;
unsigned char loadPending = 0;
unsigned char tuningSavePending = 0;
unsigned char tuningLoadPending = 0;
uint16_t busy;

int memorySPICheckIfBusy(void)
{
	memoryWait();gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	
	spi_write(MEMORY_SPI, 0x05); //0x05 (read status register 1)
	memoryWait();
	
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

// every preset will live in a block (block 0-89)
// user tunings start at block 100, and there is one tuning per sector (so sectors 1600-1690)


//in the presets:
// every "sequence" takes up 612 bytes.

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
#define NUM_PAGES_PER_TUNING 3
#define NUM_SECTORS_THAT_NEED_TO_BE_ERASED 5

uint8_t whichSequence = 0;
uint8_t sequencePageNumber = 0;

void initiateStoringPresetToExternalMemory(void)
{
	startingSector = preset_to_save_num * 16;  // * 16 to get the sector number we will store it in
	currentSector = preset_to_save_num * 16;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	tSequencer_encode(&manta[InstrumentOne].sequencer, encodeBuffer[0]); //fill a buffer with the local sequencers
	tSequencer_encode(&manta[InstrumentTwo].sequencer, encodeBuffer[1]); //one for each
	
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
}




void initiateStoringTuningToExternalMemory(uint8_t tuning_num_to_save)
{
	currentSector = tuning_num_to_save + 1600;  // user tuning 1-99 are sectors 1601-1699
	currentPage = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	LED_On(PRESET_SAVE_LED); //make the light momentarily turn red so that there's some physical feedback about the MantaMate receiving the tuning data
	pages_left_to_store = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	memorySPIEraseSector(currentSector); //we only need to erase one sector per tuning
	tuningSavePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringTuningToExternalMemory(void)
{
	//if there are bytes to store, write those bytes!
	if (pages_left_to_store > 0)
	{
		memorySPIWrite(currentSector, currentPage, &tuning8BitBuffer[currentPage*256], 256);
		
		//update variables for next round
		currentPage++;
		pages_left_to_store--;
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		tuningSavePending = 0;
		sendSysexSaveConfim(); //let the computer know that the save completed correctly
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingPresetFromExternalMemory(void)
{
	startingSector = preset_num * 16;  // * 16 to get the sector number we will store it in
	currentSector = preset_num * 16;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
		
	pages_left_to_load = NUM_PAGES_PER_PRESET; //set the variable for the total number of pages to count down from while we store them
    continueLoadingPresetFromExternalMemory();
	loadPending = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
	
}

void continueLoadingPresetFromExternalMemory(void)
{

	if (pages_left_to_load > 0)
	{
		// load a page
		if (currentPage == 0)
		{
			//load global prefs data structure
		}
		else if (currentPage == 1)
		{
			//load MIDI prefs data structure
		}
		else if (currentPage == 2)
		{
			//load joystick prefs data structure
		}
		else if (currentPage == 3)
		{
			//load Manta keyboard mode prefs data structure
		}
		
		//load Manta Sequencer data
		//first load the local memory of sequencers 1 and 2
		else if ((currentPage >= CURRENT_SEQUENCE_PAGE_START) && (currentPage < COMPOSITIONS_PAGE_START))
		{
			whichSequence = (uint16_t)((currentPage - CURRENT_SEQUENCE_PAGE_START) / NUM_PAGES_PER_SEQUENCE);
			sequencePageNumber = (currentPage - CURRENT_SEQUENCE_PAGE_START) % NUM_PAGES_PER_SEQUENCE;
			memorySPIRead(currentSector,  (currentPage % 16) ,  &decodeBuffer[whichSequence][sequencePageNumber], 256);
		}
		
		//then load the compositions for both sequencers
		else if ((currentPage >= COMPOSITIONS_PAGE_START) && (currentPage < NUM_PAGES_PER_PRESET))
		{
			whichSequence = (uint16_t)((currentPage - COMPOSITIONS_PAGE_START) / NUM_PAGES_PER_COMPOSITION);
			sequencePageNumber = (currentPage - COMPOSITIONS_PAGE_START) % NUM_PAGES_PER_COMPOSITION;
			memorySPIRead(currentSector,  (currentPage % 16) ,  &memoryInternalCompositionBuffer[whichSequence][sequencePageNumber], 256);
		}
		//update variables for next round
		currentPage++;
		pages_left_to_load--;
		currentSector = (startingSector + (uint16_t)(currentPage / 16)); //increment the current Sector whenever currentPage wraps over 16
	}
	else //otherwise load is done!
	{
		//mark the load procedure as finished
		loadPending = 0;
		initMantaSequencer(); //TODO: shouldn't actually do this here - need to be more smart about loading the necessary memory and initializing correctly
		//tSequencer_decode(&sequencer[0], decodeBuffer[0]); //fill a buffer with the local sequencers
		//tSequencer_decode(&sequencer[1], decodeBuffer[1]); //one for each
	}
}


void initiateLoadingTuningFromExternalMemory(uint8_t tuning_to_load)
{
	currentSector = tuning_to_load + 1600;  // set sector to the location of the tuning we want to load
	currentPage = 0; //start on the first page
		
	pages_left_to_load = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	continueLoadingTuningFromExternalMemory();
	tuningLoadPending = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}


void continueLoadingTuningFromExternalMemory(void)
{
	if (pages_left_to_load > 0)
	{
		memorySPIRead(currentSector, currentPage, &tuning8BitBuffer[currentPage*256], 256);
		
		//update variables for next round
		currentPage++;
		pages_left_to_load--;
	}
	else //otherwise load is done!
	{
		//mark the load procedure as finished
		tuningLoadPending = 0;
		
		uint16_t location_in_tuning_array = 0;	
		uint32_t tempPitch = 0;
		for (int i = 0; i < 516; i+=4) //could make this smarter, since it very likely loads a pile of zeros into that array
		{
			tempPitch = 0;
			tempPitch = (tuning8BitBuffer[i] << 21);
			tempPitch += (tuning8BitBuffer[i+1] << 14);
			tempPitch += (tuning8BitBuffer[i+2] << 7);
			tempPitch += tuning8BitBuffer[i+3];
			
			externalTuning[location_in_tuning_array] = tempPitch;
			location_in_tuning_array++;
		}
		
		if ((externalTuning[0] > 0) && (externalTuning[0] < 128)) //if there is a user stored tuning at that location (we can tell because the first element of the array is the cardinality, which is between 1-127 inclusive for valid tunings
		{
			computeTuningDACTable(&myGlobalTuningTable, External);
		}
		else
		{
			computeTuningDACTable(&myGlobalTuningTable, Local);
		}
	}
}
