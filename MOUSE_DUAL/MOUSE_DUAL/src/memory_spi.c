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


#include "midi.h"
#include "memory_spi.h"
#include "main.h"

#define NUM_TRANSFER_TYPES 19

uint8_t whichSequence = 0;
uint8_t sequencePageNumber = 0;

//external memory variables
uint32_t sectors_left_to_erase[NUM_TRANSFER_TYPES];
uint16_t currentSector[NUM_TRANSFER_TYPES];
uint16_t currentPage[NUM_TRANSFER_TYPES];
uint16_t startingSector[NUM_TRANSFER_TYPES];
uint16_t bytes_left[NUM_TRANSFER_TYPES];
uint16_t pages_left[NUM_TRANSFER_TYPES];
//uint16_t numTuningBytesRemaining_toSend = 0;
//uint16_t numTuningBytesRemaining_toGet = 0;
uint16_t whichInstCompositions = 0;

unsigned char pending[NUM_TRANSFER_TYPES];
uint8_t presetToTransfer[NUM_TRANSFER_TYPES];

uint16_t numHexmapBytesRemaining_toSend = 0;
uint16_t numHexmapBytesRemaining_toGet = 0;

uint16_t numDirectBytesRemaining_toSend = 0;
uint16_t numDirectBytesRemaining_toGet = 0;

uint16_t numSequencerBytesRemaining_toSend = 0;
uint16_t numSequencerBytesRemaining_toGet = 0;

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
// a block has 65535 bytes


// sector erase takes 50ms vs 500ms block erase, so it makes sense to only erase the sectors we are using

// manta preset saving and loading

void initiateStoringMantaPresetToExternalMemory(void)
{
	if(!inSysex)
	{
		mantaPreset_encode(mantamate_internal_preset_buffer);
		presetToTransfer[MantaPresetStore] = preset_to_save_num;
	}
	startingSector[MantaPresetStore] = (presetToTransfer[MantaPresetStore] * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR;  // * 16 to get the sector number we will store it in
	currentSector[MantaPresetStore] = startingSector[MantaPresetStore];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[MantaPresetStore] = 0; //start on the first page
	
	
	// start by erasing the memory in the location we want to store
	sectors_left_to_erase[MantaPresetStore] = NUM_SECTORS_PER_MANTA_PRESET; // erase 5 sectors because that will give us enough room for a whole preset
	pages_left[MantaPresetStore] = NUM_PAGES_PER_MANTA_PRESET; // set the variable for the total number of pages to count down from while we store them
	bytes_left[MantaPresetStore] = NUM_BYTES_PER_MANTA_PRESET;
	memorySPIEraseSector(currentSector[MantaPresetStore]); 
	sectors_left_to_erase[MantaPresetStore]--;
	
	pending[MantaPresetStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

		
void continueStoringMantaPresetToExternalMemory(void)
{
	//if there are still sectors to erase, do those
	if (sectors_left_to_erase[MantaPresetStore] > 0)
	{
		currentSector[MantaPresetStore]++;
		memorySPIEraseSector(currentSector[MantaPresetStore]); 
		sectors_left_to_erase[MantaPresetStore]--;
	}
	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	else if (pages_left[MantaPresetStore] > 0)
	{
		//otherwise we are ready to start saving data
		// store a page
		if (currentPage[MantaPresetStore] == 0)
		{
			currentSector[MantaPresetStore] = startingSector[MantaPresetStore]; // start back at the beginning of the memory location for this preset (it was incremented to erase)
			//store global prefs data structure
		}
		
		memorySPIWrite(currentSector[MantaPresetStore],  (currentPage[MantaPresetStore] % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage[MantaPresetStore]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left[MantaPresetStore] -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage[MantaPresetStore]++;
		pages_left[MantaPresetStore]--;
		currentSector[MantaPresetStore] = ((uint16_t)(startingSector[MantaPresetStore]) + (uint16_t)(currentPage[MantaPresetStore] / NUM_PAGES_PER_SECTOR)); //increment the current Sector whenever currentPage wraps over 16
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished, but start up the composition save procedure on the first of the two composition sets
		whichInstCompositions = 0;
		initiateStoringMantaCompositionsToExternalMemory();
		pending[MantaPresetStore] = 0;	
	}
}

uint8_t busy_loading_full_manta_preset = 0;
uint8_t resetLoader = 0;

void initiateLoadingMantaPresetFromExternalMemory(void)
{
	if (busy_loading_full_manta_preset)
	{
		resetLoader = 1;
	}
	busy_loading_full_manta_preset = 1;
	pending[MantaCompositionLoad] = 0;
	if (!sysexSend) presetToTransfer[MantaPresetLoad] = preset_num;
	startingSector[MantaPresetLoad] = (presetToTransfer[MantaPresetLoad] * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR;  // * 16 to get the sector number we will load from
	currentSector[MantaPresetLoad] = startingSector[MantaPresetLoad];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[MantaPresetLoad] = 0; //start on the first page
	
	pages_left[MantaPresetLoad] = NUM_PAGES_PER_MANTA_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left[MantaPresetLoad] = NUM_BYTES_PER_MANTA_PRESET;
	pending[MantaPresetLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.

}

void continueLoadingMantaPresetFromExternalMemory(void)
{
	if (pages_left[MantaPresetLoad] > 0)
	{
		memorySPIRead(currentSector[MantaPresetLoad],  (currentPage[MantaPresetLoad] % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage[MantaPresetLoad]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		if (!resetLoader)	
		{
			bytes_left[MantaPresetLoad] -= NUM_BYTES_PER_PAGE;
			
			//update variables for next round
			currentPage[MantaPresetLoad]++;
			pages_left[MantaPresetLoad]--;
			currentSector[MantaPresetLoad] = startingSector[MantaPresetLoad] + (uint16_t)(currentPage[MantaPresetLoad] / NUM_PAGES_PER_SECTOR); //increment the current Sector whenever currentPage wraps over 16
		}	
		else
		{
			resetLoader = 0;
		}
	}

	else //otherwise load is done!
	{
		//mark the load procedure as finished
		pending[MantaPresetLoad] = 0;
		whichInstCompositions = 0;
		initiateLoadingMantaCompositionsFromExternalMemory();
		if(!sysexSend)
		{
			MantaInstrumentType lastInst1Type = manta[InstrumentOne].type; MantaInstrumentType lastInst2Type = manta[InstrumentTwo].type;
			uint8_t last_clock_speed_displayed = clock_speed_displayed;
			mantaPreset_decode(mantamate_internal_preset_buffer);

		
			MantaInstrumentType inst1Type = manta[InstrumentOne].type; MantaInstrumentType inst2Type = manta[InstrumentTwo].type;
		
			if (takeover) clearDACoutputs();
			if (last_clock_speed_displayed > 0 && clock_speed_displayed == 0) clearDACoutputs();
			else 
			{
				if (lastInst1Type == SequencerInstrument && inst1Type != SequencerInstrument) clearInstrumentDACoutputs(InstrumentOne);
			
				if (inst1Type == SequencerInstrument && lastInst1Type != SequencerInstrument) clearInstrumentDACoutputs(InstrumentOne);
			
				if (lastInst2Type == SequencerInstrument && inst2Type != SequencerInstrument) clearInstrumentDACoutputs(InstrumentTwo);
			
				if (inst2Type == SequencerInstrument && lastInst2Type != SequencerInstrument) clearInstrumentDACoutputs(InstrumentTwo);
			}
		
			initMantaLEDState();	
		}
	}
}


/// store and load the composition sets for the Manta

void initiateStoringMantaCompositionsToExternalMemory(void)
{
	presetToTransfer[MantaCompositionStore] = presetToTransfer[MantaPresetStore];
	if (whichInstCompositions == 0)
	{
		startingSector[MantaCompositionStore] = (uint16_t)((presetToTransfer[MantaCompositionStore] * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR + NUM_SECTORS_PER_MANTA_PRESET); //after the manta preset info, pop in the composition array
	}
	else
	{
		startingSector[MantaCompositionStore] = (uint16_t)((presetToTransfer[MantaCompositionStore] * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR + NUM_SECTORS_PER_MANTA_PRESET + NUM_SECTORS_PER_COMPOSITION_BANK); //after the manta preset info, pop in the composition array
	}
	
	
	currentSector[MantaCompositionStore] = startingSector[MantaCompositionStore];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[MantaCompositionStore] = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	sectors_left_to_erase[MantaCompositionStore] = NUM_SECTORS_PER_COMPOSITION_BANK; //erase 5 sectors because that will give us enough room for a whole preset
	pages_left[MantaCompositionStore] = NUM_PAGES_PER_COMPOSITION_BANK; //set the variable for the total number of pages to count down from while we store them
	bytes_left[MantaCompositionStore] = NUM_BYTES_PER_COMPOSITION_BANK_ROUNDED_UP;
	memorySPIEraseSector(currentSector[MantaCompositionStore]);
	sectors_left_to_erase[MantaCompositionStore]--;
	
	pending[MantaCompositionStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringMantaCompositionsToExternalMemory(void)
{
	//if there are still sectors to erase, do those
	if (sectors_left_to_erase[MantaCompositionStore] > 0)
	{
		currentSector[MantaCompositionStore]++;
		memorySPIEraseSector(currentSector[MantaCompositionStore]); //erase 5 sectors because that will give us enough room for a whole preset
		sectors_left_to_erase[MantaCompositionStore]--;
	}
	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	else if (pages_left[MantaCompositionStore] > 0)
	{
		//otherwise we are ready to start saving data
		// store a page
		if (currentPage[MantaCompositionStore] == 0)
		{
			currentSector[MantaCompositionStore] = startingSector[MantaCompositionStore]; // start back at the beginning of the memory location for this preset (it was incremented to erase)
			//store global prefs data structure
		}
		
		memorySPIWrite(currentSector[MantaCompositionStore],  (currentPage[MantaCompositionStore] % NUM_PAGES_PER_SECTOR) ,  &memoryInternalCompositionBuffer[whichInstCompositions][currentPage[MantaCompositionStore]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left[MantaCompositionStore] -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage[MantaCompositionStore]++;
		pages_left[MantaCompositionStore]--;
		currentSector[MantaCompositionStore] = startingSector[MantaCompositionStore] + (uint16_t)(currentPage[MantaCompositionStore] / NUM_PAGES_PER_SECTOR); //increment the current Sector whenever currentPage wraps over 16
	}
	else //otherwise save is done!
	{
		//if you just finished the first set, start the second set
		if (whichInstCompositions == 0)
		{
			whichInstCompositions = 1;
			initiateStoringMantaCompositionsToExternalMemory();
		}
		//otherwise, you're all done!
		else
		{
			//mark the save procedure as finished
			if (sysexSend) sysexSend = FALSE;
			pending[MantaCompositionStore] = 0;
			LED_Off(PRESET_SAVE_LED);
		}

	}
}


void initiateLoadingMantaCompositionsFromExternalMemory(void)
{
	presetToTransfer[MantaCompositionLoad] = presetToTransfer[MantaPresetLoad];
	if (whichInstCompositions == 0)
	{
		startingSector[MantaCompositionLoad] = (presetToTransfer[MantaCompositionLoad] * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR + NUM_SECTORS_PER_MANTA_PRESET; //after the manta preset info, pop in the composition array
	}
	else
	{
		startingSector[MantaCompositionLoad] = (presetToTransfer[MantaCompositionLoad] * NUM_SECTORS_BETWEEN_MANTA_PRESETS) + MANTA_PRESET_STARTING_SECTOR + NUM_SECTORS_PER_MANTA_PRESET + NUM_SECTORS_PER_COMPOSITION_BANK; //after the manta preset info, pop in the composition array
	}
	currentSector[MantaCompositionLoad] = startingSector[MantaCompositionLoad];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[MantaCompositionLoad] = 0; //start on the first page
	
	pages_left[MantaCompositionLoad] = NUM_PAGES_PER_COMPOSITION_BANK; //set the variable for the total number of pages to count down from while we store them
	bytes_left[MantaCompositionLoad] = NUM_BYTES_PER_COMPOSITION_BANK_ROUNDED_UP;
	pending[MantaCompositionLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}

void continueLoadingMantaCompositionsFromExternalMemory(void)
{
	if (pages_left[MantaCompositionLoad] > 0)
	{
		memorySPIRead(currentSector[MantaCompositionLoad],  (currentPage[MantaCompositionLoad] % NUM_PAGES_PER_SECTOR) , &memoryInternalCompositionBuffer[whichInstCompositions][currentPage[MantaCompositionLoad]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		if (!resetLoader)
		{
			bytes_left[MantaCompositionLoad] -= NUM_BYTES_PER_PAGE;
		
			//update variables for next round
			currentPage[MantaCompositionLoad]++;
			pages_left[MantaCompositionLoad]--;
			currentSector[MantaCompositionLoad] = startingSector[MantaCompositionLoad] + (currentPage[MantaCompositionLoad] / NUM_PAGES_PER_SECTOR); //increment the current Sector whenever currentPage wraps over 16
		}
		else
		{
			resetLoader = 0;
		}
	}

	else //otherwise load is done!
	{
		if (whichInstCompositions == 0)
		{
			whichInstCompositions = 1;
			initiateLoadingMantaCompositionsFromExternalMemory();
		}
		else
		{
			//mark the load procedure as finished
			if (sysexSend) 
			{
				sendSysexMessage(MantaPreset, presetToTransfer[MantaPresetLoad]);
				sysexSend = FALSE;
			}
			pending[MantaCompositionLoad] = 0;
			busy_loading_full_manta_preset = 0;
		}
	}
}





/// MIDI preset saving and loading



void initiateStoringMidiPresetToExternalMemory(void)
{
	if(!inSysex)
	{
		midiPreset_encode(mantamate_internal_preset_buffer);
		presetToTransfer[MidiPresetStore] = preset_to_save_num;
	}
	startingSector[MidiPresetStore] = (uint16_t)(presetToTransfer[MidiPresetStore] + MIDI_PRESET_STARTING_SECTOR);  // * 16 to get the sector number we will store it in
	currentSector[MidiPresetStore] = startingSector[MidiPresetStore];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[MidiPresetStore] = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store

	pages_left[MidiPresetStore] = NUM_PAGES_PER_MIDI_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left[MidiPresetStore] = NUM_BYTES_PER_MIDI_PRESET;
	memorySPIEraseSector(currentSector[MidiPresetStore]);
	
	pending[MidiPresetStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}


void continueStoringMidiPresetToExternalMemory(void)
{
	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	if (pages_left[MidiPresetStore] > 0)
	{
		memorySPIWrite(currentSector[MidiPresetStore],  (currentPage[MidiPresetStore] % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage[MidiPresetStore]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left[MidiPresetStore] -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage[MidiPresetStore]++;
		pages_left[MidiPresetStore]--;
		currentSector[MidiPresetStore] = (uint16_t)((startingSector[MidiPresetStore] + (uint16_t)(currentPage[MidiPresetStore] / NUM_PAGES_PER_SECTOR))); //increment the current Sector whenever currentPage[MidiPresetStore] wraps over 16
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		if (sysexSend) sysexSend = FALSE;
		pending[MidiPresetStore] = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingMidiPresetFromExternalMemory(void)
{
	if (!sysexSend) presetToTransfer[MidiPresetLoad] = preset_num;
	startingSector[MidiPresetLoad] = (uint16_t)(presetToTransfer[MidiPresetLoad] + MIDI_PRESET_STARTING_SECTOR);  // * 16 to get the sector number we will load from
	currentSector[MidiPresetLoad] = startingSector[MidiPresetLoad];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[MidiPresetLoad] = 0; //start on the first page
	
	pages_left[MidiPresetLoad] = NUM_PAGES_PER_MIDI_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left[MidiPresetLoad] = NUM_BYTES_PER_MIDI_PRESET;
	pending[MidiPresetLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
	
}

void continueLoadingMidiPresetFromExternalMemory(void)
{
	if (pages_left[MidiPresetLoad] > 0)
	{
		memorySPIRead(currentSector[MidiPresetLoad],  (currentPage[MidiPresetLoad] % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage[MidiPresetLoad]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left[MidiPresetLoad] -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage[MidiPresetLoad]++;
		pages_left[MidiPresetLoad]--;
		currentSector[MidiPresetLoad] = (uint16_t)((startingSector[MidiPresetLoad] + (uint16_t)(currentPage[MidiPresetLoad] / NUM_PAGES_PER_SECTOR))); //increment the current Sector whenever currentPage[MidiPresetLoad] wraps over 16
	}

	else //otherwise load is done!
	{
		//mark the load procedure as finished
		pending[MidiPresetLoad] = 0;
		if (sysexSend)
		{
			sendSysexMessage(MidiPreset, presetToTransfer[MidiPresetLoad]);
			sysexSend = FALSE;
		}
		else 
		{
			midiPreset_decode(mantamate_internal_preset_buffer);
		}
	}
}


/// No Device preset saving and loading

void initiateStoringNoDevicePresetToExternalMemory(void)
{
	if(!inSysex)
	{
		noDevicePreset_encode(mantamate_internal_preset_buffer);
		presetToTransfer[NoDevicePresetStore] = preset_to_save_num;
	}
	startingSector[NoDevicePresetStore] = (uint16_t)(presetToTransfer[NoDevicePresetStore] + NODEVICE_PRESET_STARTING_SECTOR);  // * 16 to get the sector number we will store it in
	currentSector[NoDevicePresetStore] = startingSector[NoDevicePresetStore];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[NoDevicePresetStore] = 0; //start on the first page	
	
	//start by erasing the memory in the location we want to store

	pages_left[NoDevicePresetStore] = NUM_PAGES_PER_NODEVICE_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left[NoDevicePresetStore] = NUM_BYTES_PER_NODEVICE_PRESET;
	memorySPIEraseSector(currentSector[NoDevicePresetStore]);
	
	pending[NoDevicePresetStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}


void continueStoringNoDevicePresetToExternalMemory(void)
{

	//if there aren't sectors left to erase, but there are bytes to store, write those bytes!
	if (pages_left[NoDevicePresetStore] > 0)
	{
		memorySPIWrite(currentSector[NoDevicePresetStore],  (currentPage[NoDevicePresetStore] % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage[NoDevicePresetStore]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left[NoDevicePresetStore] -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage[NoDevicePresetStore]++;
		pages_left[NoDevicePresetStore]--;
		currentSector[NoDevicePresetStore] = (uint16_t)((startingSector[NoDevicePresetStore] + (uint16_t)(currentPage[NoDevicePresetStore] / NUM_PAGES_PER_SECTOR))); //increment the current Sector whenever currentPage[NoDevicePresetStore] wraps over 16
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		if (sysexSend) sysexSend = FALSE;
		pending[NoDevicePresetStore] = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingNoDevicePresetFromExternalMemory(void)
{
	if (!sysexSend) presetToTransfer[NoDevicePresetLoad] = preset_num;
	startingSector[NoDevicePresetLoad] = (uint16_t)(presetToTransfer[NoDevicePresetLoad] + NODEVICE_PRESET_STARTING_SECTOR);  // * 16 to get the sector number we will load from
	currentSector[NoDevicePresetLoad] = startingSector[NoDevicePresetLoad];  // this is the same, but we'll increment it in the erase loop while we want to keep the memory of the original value
	currentPage[NoDevicePresetLoad] = 0; //start on the first page
	
	pages_left[NoDevicePresetLoad] = NUM_PAGES_PER_NODEVICE_PRESET; //set the variable for the total number of pages to count down from while we store them
	bytes_left[NoDevicePresetLoad] = NUM_BYTES_PER_NODEVICE_PRESET;
	pending[NoDevicePresetLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
	
}

void continueLoadingNoDevicePresetFromExternalMemory(void)
{
	if (pages_left[NoDevicePresetLoad] > 0)
	{
		memorySPIRead(currentSector[NoDevicePresetLoad],  (currentPage[NoDevicePresetLoad] % NUM_PAGES_PER_SECTOR) ,  &mantamate_internal_preset_buffer[currentPage[NoDevicePresetLoad]*NUM_BYTES_PER_PAGE], NUM_BYTES_PER_PAGE);
		
		bytes_left[NoDevicePresetLoad] -= NUM_BYTES_PER_PAGE;
		
		//update variables for next round
		currentPage[NoDevicePresetLoad]++;
		pages_left[NoDevicePresetLoad]--;
		currentSector[NoDevicePresetLoad] = (uint16_t)((startingSector[NoDevicePresetLoad] + (uint16_t)(currentPage[NoDevicePresetLoad] / NUM_PAGES_PER_SECTOR))); //increment the current Sector whenever currentPage[NoDevicePresetLoad] wraps over 16
	}

	else //otherwise load is done!
	{
		//mark the load procedure as finished
		pending[NoDevicePresetLoad] = 0;
		if (sysexSend)
		{
			sendSysexMessage(NoDevicePreset, presetToTransfer[NoDevicePresetLoad]);
			sysexSend = FALSE;
		}
		else
		{
			noDevicePreset_decode(mantamate_internal_preset_buffer);
		}
	}
}



// tuning saving and loading

void initiateStoringTuningToExternalMemory(void)
{
	currentSector[TuningStore] = presetToTransfer[TuningStore] + TUNING_STARTING_SECTOR;  // user tuning 1-99 are sectors 1601-1699
	currentPage[TuningStore] = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	LED_On(PRESET_SAVE_LED); //make the light momentarily turn red so that there's some physical feedback about the MantaMate receiving the tuning data
	pages_left[TuningStore] = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	bytes_left[TuningStore] =  TUNING_8BIT_BUFFER_SIZE;
	memorySPIEraseSector(currentSector[TuningStore]); //we only need to erase one sector per tuning
	pending[TuningStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringTuningToExternalMemory(void)
{
	//if there are bytes to store, write those bytes!
	if (pages_left[TuningStore] > 0)
	{
		uint16_t numTuningBytesToSend;
		
		if (bytes_left[TuningStore] > 256)
		{
			numTuningBytesToSend = 256;
		}
		else
		{
			numTuningBytesToSend = bytes_left[TuningStore];
		}
		
		
		memorySPIWrite(currentSector[TuningStore], currentPage[TuningStore], &tuning8BitBuffer[currentPage[TuningStore]*256], numTuningBytesToSend);
		
		//update variables for next round
		currentPage[TuningStore]++;
		pages_left[TuningStore]--;
		bytes_left[TuningStore] -= numTuningBytesToSend;
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		pending[TuningStore] = 0;
		sendSysexSaveConfim(); //let the computer know that the save completed correctly
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingTuningFromExternalMemory(void)
{
	currentSector[TuningLoad] = presetToTransfer[TuningLoad] + TUNING_STARTING_SECTOR;  // set sector to the location of the tuning we want to load
	currentPage[TuningLoad] = 0; //start on the first page
	bytes_left[TuningLoad] = TUNING_8BIT_BUFFER_SIZE;
	pages_left[TuningLoad] = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	continueLoadingTuningFromExternalMemory();
	pending[TuningLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}


void continueLoadingTuningFromExternalMemory(void)
{
	if (pages_left[TuningLoad] > 0)
	{
		
		uint16_t numTuningBytesToGet = 0;
		if (bytes_left[TuningLoad] > 256)
		{
			numTuningBytesToGet = 256;
		}
		else
		{
			numTuningBytesToGet = bytes_left[TuningLoad];
		}
		
		memorySPIRead(currentSector[TuningLoad], currentPage[TuningLoad], &tuning8BitBuffer[currentPage[TuningLoad]*256], numTuningBytesToGet);
		
		//update variables for next round
		currentPage[TuningLoad]++;
		pages_left[TuningLoad]--;
		bytes_left[TuningLoad] -= numTuningBytesToGet;
	}
	else //otherwise load is done!
	{
		//mark the load procedure as finished
		pending[TuningLoad] = 0;
		
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
	presetToTransfer[StartupStateStore] = preset_num;
	//start by erasing the memory in the location we want to store
	pages_left[StartupStateStore] = NUM_PAGES_PER_TUNING; //set the variable for the total number of pages to count down from while we store them
	memorySPIEraseSector(STARTUP_STATE_SECTOR); //we only need to erase one sector per tuning
	pending[StartupStateStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
	tempPreset[0] = presetToTransfer[StartupStateStore];
	tempPreset[1] = 125;
	tempPreset[2] = 200;
	tempPreset[3] = 201;
}

void continueStoringStartupStateToExternalMemory(void)
{
	memorySPIWrite(STARTUP_STATE_SECTOR, 0, tempPreset, 8);
	//mark the save procedure as finished
	pending[StartupStateStore] = 0;
}	


void loadStartupStateFromExternalMemory(void)
{
	memorySPIRead(STARTUP_STATE_SECTOR, 0, tempPreset, 8);
	pending[StartupStateLoad] = 1;
}

void continueLoadingStartupStateFromExternalMemory(void)
{
	pending[StartupStateLoad] = 0;
	preset_num = tempPreset[0];
	updatePreset();
	Write7Seg(preset_num);
	normal_7seg_number = preset_num;
}



// hexmap saving and loading


void initiateStoringHexmapToExternalMemory(uint8_t hexmap_num_to_save)
{
	currentSector[HexmapStore] = hexmap_num_to_save + HEXMAP_STARTING_SECTOR;  // user hexmap 1-99 are sectors 1701-1799
	currentPage[HexmapStore] = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	LED_On(PRESET_SAVE_LED); //make the light momentarily turn red so that there's some physical feedback about the MantaMate receiving the hemxap data
	pages_left[HexmapStore] = NUM_PAGES_PER_HEXMAP; //set the variable for the total number of pages to count down from while we store them
	bytes_left[HexmapStore] =  NUM_BYTES_PER_HEXMAP;
	memorySPIEraseSector(currentSector[HexmapStore]); //we only need to erase one sector per hexmap
	pending[HexmapStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringHexmapToExternalMemory(void)
{
	//if there are bytes to store, write those bytes!
	if (pages_left[HexmapStore] > 0)
	{
		uint16_t numHexmapBytesToSend;
		
		if (bytes_left[HexmapStore] > 256)
		{
			numHexmapBytesToSend = 256;
		}
		else
		{
			numHexmapBytesToSend = bytes_left[HexmapStore];
		}
		
		
		memorySPIWrite(currentSector[HexmapStore], currentPage[HexmapStore], &hexmapBuffer[currentPage[HexmapStore]*256], numHexmapBytesToSend);
		
		//update variables for next round
		currentPage[HexmapStore]++;
		pages_left[HexmapStore]--;
		bytes_left[HexmapStore] -= numHexmapBytesToSend;
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		pending[HexmapStore] = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingHexmapFromExternalMemory(uint8_t hexmap_to_load)
{
	currentSector[HexmapLoad] = hexmap_to_load + HEXMAP_STARTING_SECTOR;  // set sector to the location of the hexmap we want to load
	currentPage[HexmapLoad] = 0; //start on the first page
	bytes_left[HexmapLoad] = NUM_BYTES_PER_HEXMAP;
	pages_left[HexmapLoad] = NUM_PAGES_PER_HEXMAP; //set the variable for the total number of pages to count down from while we store them
	continueLoadingHexmapFromExternalMemory();
	pending[HexmapLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}


void continueLoadingHexmapFromExternalMemory(void)
{
	if (pages_left[HexmapLoad] > 0)
	{
		
		uint16_t numHexmapBytesToGetNow = 0;
		if (bytes_left[HexmapLoad] > 256)
		{
			numHexmapBytesToGetNow = 256;
		}
		else
		{
			numHexmapBytesToGetNow = bytes_left[HexmapLoad];
		}
		
		memorySPIRead(currentSector[HexmapLoad], currentPage[HexmapLoad], &hexmapBuffer[currentPage[HexmapLoad]*256], numHexmapBytesToGetNow);
		
		//update variables for next round
		currentPage[HexmapLoad]++;
		pages_left[HexmapLoad]--;
		bytes_left[HexmapLoad] -= numHexmapBytesToGetNow;
	}
	else //otherwise load is done!
	{
		//mark the load procedure as finished
		pending[HexmapLoad] = 0;
		
		tKeyboard_hexmapDecode(hexmapEditKeyboard, hexmapBuffer);
	}
}

// Direct saving and loading
void initiateStoringDirectToExternalMemory(uint8_t direct_num_to_save)
{
	currentSector[DirectStore] = direct_num_to_save + DIRECT_STARTING_SECTOR;  // user direct 1-99 are sectors 1701-1799
	currentPage[DirectStore] = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	LED_On(PRESET_SAVE_LED); //make the light momentarily turn red so that there's some physical feedback about the MantaMate receiving the hemxap data
	pages_left[DirectStore] = NUM_PAGES_PER_DIRECT; //set the variable for the total number of pages to count down from while we store them
	bytes_left[DirectStore] =  NUM_BYTES_PER_DIRECT;
	memorySPIEraseSector(currentSector[DirectStore]); //we only need to erase one sector per direct
	pending[DirectStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringDirectToExternalMemory(void)
{
	//if there are bytes to store, write those bytes!
	if (pages_left[DirectStore] > 0)
	{
		uint16_t numDirectBytesToSend;
		
		if (bytes_left[DirectStore] > 256)
		{
			numDirectBytesToSend = 256;
		}
		else
		{
			numDirectBytesToSend = bytes_left[DirectStore];
		}
		
		
		memorySPIWrite(currentSector[DirectStore], currentPage[DirectStore], &directBuffer[currentPage[DirectStore]*256], numDirectBytesToSend);
		
		//update variables for next round
		currentPage[DirectStore]++;
		pages_left[DirectStore]--;
		bytes_left[DirectStore] -= numDirectBytesToSend;
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		pending[DirectStore] = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingDirectFromExternalMemory(uint8_t direct_to_load)
{
	currentSector[DirectLoad] = direct_to_load + DIRECT_STARTING_SECTOR;  // set sector to the location of the direct we want to load
	currentPage[DirectLoad] = 0; //start on the first page
	bytes_left[DirectLoad] = NUM_BYTES_PER_DIRECT;
	pages_left[DirectLoad] = NUM_PAGES_PER_DIRECT; //set the variable for the total number of pages to count down from while we store them
	continueLoadingDirectFromExternalMemory();
	pending[DirectLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}


void continueLoadingDirectFromExternalMemory(void)
{
	if (pages_left[DirectLoad] > 0)
	{
		
		uint16_t numDirectBytesToGetNow = 0;
		if (bytes_left[DirectLoad] > 256)
		{
			numDirectBytesToGetNow = 256;
		}
		else
		{
			numDirectBytesToGetNow = bytes_left[DirectLoad];
		}
		
		memorySPIRead(currentSector[DirectLoad], currentPage[DirectLoad], &directBuffer[currentPage[DirectLoad]*256], numDirectBytesToGetNow);
		
		//update variables for next round
		currentPage[DirectLoad]++;
		pages_left[DirectLoad]--;
		bytes_left[DirectLoad] -= numDirectBytesToGetNow;
	}
	else //otherwise load is done!
	{
		//mark the load procedure as finished
		pending[DirectLoad] = 0;
		
		tDirect_decode(editDirect, directBuffer);
		
		takeover = (editDirect->numOuts == 12) ? TRUE : FALSE;
		if (takeover) 
		{
			takeoverType = DirectInstrument;
			
			setUpperOptionHexLEDs();
			setLowerOptionHexLEDs();
		}
		
	}
}

// Sequencer saving and loading
void initiateStoringSequencerToExternalMemory(uint8_t sequencer_num_to_save)
{
	currentSector[SequencerStore] = sequencer_num_to_save + SEQUENCER_STARTING_SECTOR;  // user sequencer 1-99 are sectors 1701-1799
	currentPage[SequencerStore] = 0; //start on the first page
	
	//start by erasing the memory in the location we want to store
	LED_On(PRESET_SAVE_LED); //make the light momentarily turn red so that there's some physical feedback about the MantaMate receiving the hemxap data
	pages_left[SequencerStore] = NUM_PAGES_PER_SEQUENCER*2; //set the variable for the total number of pages to count down from while we store them
	bytes_left[SequencerStore] =  NUM_BYTES_PER_SEQUENCER*2;
	memorySPIEraseSector(currentSector[SequencerStore]); //we only need to erase one sector per sequencer
	pending[SequencerStore] = 1; //tell the rest of the system that we are in the middle of a save, need to keep checking until it's finished.
}

void continueStoringSequencerToExternalMemory(void)
{
	//if there are bytes to store, write those bytes!
	if (pages_left[SequencerStore] > 0)
	{
		uint16_t numSequencerBytesToSend;
		
		if (bytes_left[SequencerStore] > 256)
		{
			numSequencerBytesToSend = 256;
		}
		else
		{
			numSequencerBytesToSend = bytes_left[SequencerStore];
		}
		
		
		memorySPIWrite(currentSector[SequencerStore], currentPage[SequencerStore], &sequencerBuffer[currentPage[SequencerStore]*256], numSequencerBytesToSend);
		
		//update variables for next round
		currentPage[SequencerStore]++;
		pages_left[SequencerStore]--;
		bytes_left[SequencerStore] -= numSequencerBytesToSend;
	}
	else //otherwise save is done!
	{
		//mark the save procedure as finished
		pending[SequencerStore] = 0;
		LED_Off(PRESET_SAVE_LED);
	}
}

void initiateLoadingSequencerFromExternalMemory(uint8_t sequencer_to_load)
{
	currentSector[SequencerLoad] = sequencer_to_load + SEQUENCER_STARTING_SECTOR;  // set sector to the location of the sequencer we want to load
	currentPage[SequencerLoad] = 0; //start on the first page
	bytes_left[SequencerLoad] = NUM_BYTES_PER_SEQUENCER*2;
	pages_left[SequencerLoad] = NUM_PAGES_PER_SEQUENCER*2; //set the variable for the total number of pages to count down from while we store them
	LED_On(PRESET_SAVE_LED); 
	continueLoadingSequencerFromExternalMemory();
	pending[SequencerLoad] = 1; //tell the rest of the system that we are in the middle of a load, need to keep checking until it's finished.
}


void continueLoadingSequencerFromExternalMemory(void)
{
	if (pages_left[SequencerLoad] > 0)
	{
		
		uint16_t numSequencerBytesToGetNow = 0;
		if (bytes_left[SequencerLoad] > 256)
		{
			numSequencerBytesToGetNow = 256;
		}
		else
		{
			numSequencerBytesToGetNow = bytes_left[SequencerLoad];
		}
		
		memorySPIRead(currentSector[SequencerLoad], currentPage[SequencerLoad], &sequencerBuffer[currentPage[SequencerLoad]*256], numSequencerBytesToGetNow);
		
		//update variables for next round
		currentPage[SequencerLoad]++;
		pages_left[SequencerLoad]--;
		bytes_left[SequencerLoad] -= numSequencerBytesToGetNow;
	}
	else //otherwise load is done!
	{
		//mark the load procedure as finished
		pending[SequencerLoad] = 0;
		LED_Off(PRESET_SAVE_LED); 
		
		tSequencer_decode(&manta[whichCompositionInstrument].sequencer, &sequencerBuffer[whichCompositionInstrument*NUM_BYTES_PER_SEQUENCER]);
		
		memoryInternalWriteSequencer(whichCompositionInstrument, whichCompositionHex, &sequencerBuffer[whichCompositionInstrument*NUM_BYTES_PER_SEQUENCER]);
		
		compositionMap[whichCompositionInstrument][whichCompositionHex] = TRUE;
		
		currentComp[whichCompositionInstrument] = whichCompositionHex;
		
		setCurrentInstrument(whichCompositionInstrument);
		
		setUpperOptionHexLEDs();
		setLowerOptionHexLEDs();
	}
}
