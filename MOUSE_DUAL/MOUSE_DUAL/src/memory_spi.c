/*
 * memory_spi.c
 *
 * Created: 3/31/2017 5:03:56 PM
 *  Author: Jeff Snyder
 */ 

//S25FL164K0XMFA011 is the chip (or a newer chip in the same family)
// a page is 256 bytes. 
// a sector is 16 pages (4096 bytes)
// a block is 16 sectors (65535 bytes)



#include "memory_spi.h"
#include "main.h"

uint8_t whichSequence = 0;
uint8_t sequencePageNumber = 0;

//external memory variables
uint32_t sectors_left_to_erase = 0;
uint16_t currentSector = 0;
uint16_t currentPage = 0;
uint16_t startingSector = 0;

uint16_t numTuningBytesRemaining_toSend = 0;
uint16_t numTuningBytesRemaining_toGet = 0;

unsigned char mantaSavePending = 0;
unsigned char mantaLoadPending = 0;
unsigned char midiSavePending = 0;
unsigned char midiLoadPending = 0;
unsigned char noDeviceSavePending = 0;
unsigned char noDeviceLoadPending = 0;
unsigned char tuningSavePending = 0;
unsigned char tuningLoadPending = 0;
unsigned char startupStateSavePending = 0;
unsigned char startupStateLoadPending = 0;
unsigned char hexmapSavePending = 0;
unsigned char hexmapLoadPending = 0;
uint16_t numHexmapBytesRemaining_toSend = 0;
uint16_t numHexmapBytesRemaining_toGet = 0;

uint8_t tempPreset[8] = {0, 125, 126, 127, 128, 129, 130, 131};

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
void memorySPIWrite(int sector, int page, uint8_t* buffer, int numBytes)
{
	// Write Enable
	memorySPIWriteEnable();
	
	// WRITE Page Program
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0x02);  // Page Program code
	memorySPIWriteAddress(sector, page);
	
	for (int i = 0; i < numBytes; i++)
	{
		//spi_write(MEMORY_SPI, (buffer[i]&0xffff));
 		spi_write(MEMORY_SPI, buffer[i]);
	}
	
	memoryWait();gpio_set_gpio_pin(MEMORY_CS);memoryWait();
	
}

void memorySPIRead(int sector, int page, uint8_t* buffer, int numBytes)
{
	uint8_t *_ram = buffer;
	uint16_t data;
	
	// READ Read Data
	gpio_clr_gpio_pin(MEMORY_CS);memoryWait();
	spi_write(MEMORY_SPI, 0x03); // Read Data code
	memorySPIWriteAddress(sector, page);
	
	//do a dummy write to read in the value
	for (int i = 0; i < numBytes; i++)
	{
		spi_write(MEMORY_SPI, 0x00);
		spi_read(MEMORY_SPI, &data);
		*_ram++ = data;
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
// there are 256 blocks

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
// Pages 10-43 is the composition collection for sequencer 1 (34 pages per sequence if there are 14 savable sequences)
// Pages 44-77 is the composition collection for sequencer 2 (34 pages per sequence if there are 14 savable sequences)

// sector erase takes 50ms vs 500ms block erase, so it makes sense to only erase the sectors we are using (since we only use 4 sectors per preset = 200ms instead of 500ms save time)

//global settings format:
// byte 1 = seq1Pvt, seq2Pvt, sequencer_mode, clock_speed, compositions,  


uint16_t bytes_left_to_transfer;
uint16_t pages_left_to_transfer;
// manta preset saving and loading

void initiateStoringMantaPresetToExternalMemory(void)
{
	startingSector = (preset_to_save_num * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR;  // * 16 to get the sector number we will store it in
	currentSector = startingSector;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	mantaPreset_encode(mantamate_internal_preset_buffer);
	
	//start by erasing the memory in the location we want to store
	sectors_left_to_erase = NUM_SECTORS_PER_MANTA_PRESET; //erase 5 sectors because that will give us enough room for a whole preset
	pages_left_to_transfer = NUM_PAGES_PER_MANTA_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left_to_transfer = NUM_BYTES_PER_MANTA_PRESET;
	memorySPIEraseSector(currentSector); 
	sectors_left_to_erase--;
	
	mantaSavePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

		
void continueStoringMantaPresetToExternalMemory(void)
{
	//if there are still sectors to erase, do those
	if (sectors_left_to_erase > 0)
	{
		currentSector++;
		memorySPIEraseSector(currentSector); //erase 5 sectors because that will give us enough room for a whole preset
		sectors_left_to_erase--;
	}
	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	else if (pages_left_to_transfer > 0)
	{
		//otherwise we are ready to start saving data
		// store a page
		if (currentPage == 0)
		{
			currentSector = startingSector; // start back at the beginning of the memory location for this preset (it was incremented to erase)
			//store global prefs data structure
		}
		
		memorySPIWrite(currentSector,  (currentPage % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left_to_transfer -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		currentSector = (startingSector + (uint16_t)(currentPage / NUM_PAGES_PER_SECTOR)); //increment the current Sector whenever currentPage wraps over 16
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		mantaSavePending = 0;	
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingMantaPresetFromExternalMemory(void)
{
	startingSector = (preset_num * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR;  // * 16 to get the sector number we will load from
	currentSector = startingSector;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	pages_left_to_transfer = NUM_PAGES_PER_MANTA_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left_to_transfer = NUM_BYTES_PER_MANTA_PRESET;
	mantaLoadPending = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
	
}

void continueLoadingMantaPresetFromExternalMemory(void)
{
	if (pages_left_to_transfer > 0)
	{
		memorySPIRead(currentSector,  (currentPage % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
				
		bytes_left_to_transfer -= NUM_BYTES_PER_PAGE;
				
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		currentSector = (startingSector + (uint16_t)(currentPage / NUM_PAGES_PER_SECTOR)); //increment the current Sector whenever currentPage wraps over 16
	}

	else //otherwise load is done!
	{
		//mark the load procedure as finished
		mantaLoadPending = 0;
		mantaPreset_decode(mantamate_internal_preset_buffer);
		initMantaLEDState();
	}
}


/// MIDI preset saving and loading

void initiateStoringMidiPresetToExternalMemory(void)
{
	startingSector = preset_to_save_num + MIDI_PRESET_STARTING_SECTOR;  // * 16 to get the sector number we will store it in
	currentSector = startingSector;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	midiPreset_encode(mantamate_internal_preset_buffer);
	
	//start by erasing the memory in the location we want to store

	pages_left_to_transfer = NUM_PAGES_PER_MIDI_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left_to_transfer = NUM_BYTES_PER_MIDI_PRESET;
	memorySPIEraseSector(currentSector);
	
	midiSavePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}


void continueStoringMidiPresetToExternalMemory(void)
{

	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	if (pages_left_to_transfer > 0)
	{
		memorySPIWrite(currentSector,  (currentPage % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left_to_transfer -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		currentSector = (startingSector + (uint16_t)(currentPage / NUM_PAGES_PER_SECTOR)); //increment the current Sector whenever currentPage wraps over 16
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		midiSavePending = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingMidiPresetFromExternalMemory(void)
{
	startingSector = (preset_num + MIDI_PRESET_STARTING_SECTOR);  // * 16 to get the sector number we will load from
	currentSector = startingSector;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	pages_left_to_transfer = NUM_PAGES_PER_MIDI_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left_to_transfer = NUM_BYTES_PER_MIDI_PRESET;
	midiLoadPending = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
	
}

void continueLoadingMidiPresetFromExternalMemory(void)
{
	if (pages_left_to_transfer > 0)
	{
		memorySPIRead(currentSector,  (currentPage % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left_to_transfer -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		currentSector = (startingSector + (uint16_t)(currentPage / NUM_PAGES_PER_SECTOR)); //increment the current Sector whenever currentPage wraps over 16
	}

	else //otherwise load is done!
	{
		//mark the load procedure as finished
		midiLoadPending = 0;
		midiPreset_decode(mantamate_internal_preset_buffer);
	}
}


/// No Device preset saving and loading

void initiateStoringNoDevicePresetToExternalMemory(void)
{
	startingSector = preset_to_save_num + NODEVICE_PRESET_STARTING_SECTOR;  // * 16 to get the sector number we will store it in
	currentSector = startingSector;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	noDevicePreset_encode(mantamate_internal_preset_buffer);
	
	//start by erasing the memory in the location we want to store

	pages_left_to_transfer = NUM_PAGES_PER_NODEVICE_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left_to_transfer = NUM_BYTES_PER_NODEVICE_PRESET;
	memorySPIEraseSector(currentSector);
	
	noDeviceSavePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}


void continueStoringNoDevicePresetToExternalMemory(void)
{

	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	if (pages_left_to_transfer > 0)
	{
		memorySPIWrite(currentSector,  (currentPage % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left_to_transfer -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		currentSector = (startingSector + (uint16_t)(currentPage / NUM_PAGES_PER_SECTOR)); //increment the current Sector whenever currentPage wraps over 16
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		noDeviceSavePending = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingNoDevicePresetFromExternalMemory(void)
{
	startingSector = (preset_num + NODEVICE_PRESET_STARTING_SECTOR);  // * 16 to get the sector number we will load from
	currentSector = startingSector;  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage = 0; //start on the first page
	
	pages_left_to_transfer = NUM_PAGES_PER_NODEVICE_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left_to_transfer = NUM_BYTES_PER_NODEVICE_PRESET;
	noDeviceLoadPending = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
	
}

void continueLoadingNoDevicePresetFromExternalMemory(void)
{
	if (pages_left_to_transfer > 0)
	{
		memorySPIRead(currentSector,  (currentPage % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left_to_transfer -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		currentSector = (startingSector + (uint16_t)(currentPage / NUM_PAGES_PER_SECTOR)); //increment the current Sector whenever currentPage wraps over 16
	}

	else //otherwise load is done!
	{
		//mark the load procedure as finished
		noDeviceLoadPending = 0;
  		noDevicePreset_decode(mantamate_internal_preset_buffer);
	}
}



// tuning saving and loading

void initiateStoringTuningToExternalMemory(uint8_t tuning_num_to_save)
{
	currentSector = tuning_num_to_save + TUNING_STARTING_SECTOR;  // user tuning 1-99 are sectors 1601-1699
	currentPage = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	LED_On(PRESET_SAVE_LED); //make the light momentarily turn red so that there's some physical feedback about the MantaMate receiving the tuning data
	pages_left_to_transfer = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	numTuningBytesRemaining_toSend =  TUNING_8BIT_BUFFER_SIZE;
	memorySPIEraseSector(currentSector); //we only need to erase one sector per tuning
	tuningSavePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringTuningToExternalMemory(void)
{
	//if there are bytes to store, write those bytes!
	if (pages_left_to_transfer > 0)
	{
		uint16_t numTuningBytesToSend;
		
		if (numTuningBytesRemaining_toSend > 256)
		{
			numTuningBytesToSend = 256;
		}
		else
		{
			numTuningBytesToSend = numTuningBytesRemaining_toSend;
		}
		
		
		memorySPIWrite(currentSector, currentPage, &tuning8BitBuffer[currentPage*256], numTuningBytesToSend);
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		numTuningBytesRemaining_toSend -= numTuningBytesToSend;
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		tuningSavePending = 0;
		sendSysexSaveConfim(); //let the computer know that the save completed correctly
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingTuningFromExternalMemory(uint8_t tuning_to_load)
{
	currentSector = tuning_to_load + TUNING_STARTING_SECTOR;  // set sector to the location of the tuning we want to load
	currentPage = 0; //start on the first page
	numTuningBytesRemaining_toGet = TUNING_8BIT_BUFFER_SIZE;
	pages_left_to_transfer = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	continueLoadingTuningFromExternalMemory();
	tuningLoadPending = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}


void continueLoadingTuningFromExternalMemory(void)
{
	if (pages_left_to_transfer > 0)
	{
		
		uint16_t numTuningBytesToGet = 0;
		if (numTuningBytesRemaining_toGet > 256)
		{
			numTuningBytesToGet = 256;
		}
		else
		{
			numTuningBytesToGet = numTuningBytesRemaining_toGet;
		}
		
		memorySPIRead(currentSector, currentPage, &tuning8BitBuffer[currentPage*256], numTuningBytesToGet);
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		numTuningBytesRemaining_toGet -= numTuningBytesToGet;
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




// startup state saving and loading

void initiateStoringStartupStateToExternalMemory(void)
{
	//start by erasing the memory in the location we want to store
	pages_left_to_transfer = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	memorySPIEraseSector(STARTUP_STATE_SECTOR); //we only need to erase one sector per tuning
	startupStateSavePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
	tempPreset[0] = preset_num;
	tempPreset[1] = 125;
	tempPreset[2] = 200;
	tempPreset[3] = 201;
}

void continueStoringStartupStateToExternalMemory(void)
{
	memorySPIWrite(STARTUP_STATE_SECTOR, 0, tempPreset, 8);
	//mark the save procedure as finished
	startupStateSavePending = 0;
}	


void loadStartupStateFromExternalMemory(void)
{
	memorySPIRead(STARTUP_STATE_SECTOR, 0, tempPreset, 8);
	startupStateLoadPending = 1;
}

void continueLoadingStartupStateFromExternalMemory(void)
{
	startupStateLoadPending = 0;
	preset_num = tempPreset[0];
	updatePreset();
	Write7Seg(preset_num);
	normal_7seg_number = preset_num;
}



// hexmap saving and loading


void initiateStoringHexmapToExternalMemory(uint8_t hexmap_num_to_save)
{
	currentSector = hexmap_num_to_save + HEXMAP_STARTING_SECTOR;  // user hexmap 1-99 are sectors 1701-1799
	currentPage = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	LED_On(PRESET_SAVE_LED); //make the light momentarily turn red so that there's some physical feedback about the MantaMate receiving the hemxap data
	pages_left_to_transfer = NUM_PAGES_PER_HEXMAP; //set the variable for the total number of pages to count down from while we store them
	numHexmapBytesRemaining_toSend =  NUM_BYTES_PER_HEXMAP;
	memorySPIEraseSector(currentSector); //we only need to erase one sector per hexmap
	hexmapSavePending = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringHexmapToExternalMemory(void)
{
	//if there are bytes to store, write those bytes!
	if (pages_left_to_transfer > 0)
	{
		uint16_t numHexmapBytesToSend;
		
		if (numHexmapBytesRemaining_toSend > 256)
		{
			numHexmapBytesToSend = 256;
		}
		else
		{
			numHexmapBytesToSend = numHexmapBytesRemaining_toSend;
		}
		
		
		memorySPIWrite(currentSector, currentPage, &hexmapBuffer[currentPage*256], numHexmapBytesToSend);
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		numHexmapBytesRemaining_toSend -= numHexmapBytesToSend;
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		hexmapSavePending = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingHexmapFromExternalMemory(uint8_t hexmap_to_load)
{
	currentSector = hexmap_to_load + HEXMAP_STARTING_SECTOR;  // set sector to the location of the hexmap we want to load
	currentPage = 0; //start on the first page
	numHexmapBytesRemaining_toGet = NUM_BYTES_PER_HEXMAP;
	pages_left_to_transfer = NUM_PAGES_PER_HEXMAP; //set the variable for the total number of pages to count down from while we store them
	continueLoadingHexmapFromExternalMemory();
	hexmapLoadPending = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}


void continueLoadingHexmapFromExternalMemory(void)
{
	if (pages_left_to_transfer > 0)
	{
		
		uint16_t numHexmapBytesToGetNow = 0;
		if (numHexmapBytesRemaining_toGet > 256)
		{
			numHexmapBytesToGetNow = 256;
		}
		else
		{
			numHexmapBytesToGetNow = numHexmapBytesRemaining_toGet;
		}
		
		memorySPIRead(currentSector, currentPage, &tuning8BitBuffer[currentPage*256], numHexmapBytesToGetNow);
		
		//update variables for next round
		currentPage++;
		pages_left_to_transfer--;
		numHexmapBytesRemaining_toGet -= numHexmapBytesToGetNow;
	}
	else //otherwise load is done!
	{
		//mark the load procedure as finished
		hexmapLoadPending = 0;
		
		//copy the data into current keyboard hexmapppppp
		
		////!!!!!!!!!!!!!! whooooo
		
	}
}