/**
 * \file
 *
 * \brief Main functions for USB host mouse example
 *
 * Copyright (C) 2011-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include <asf.h>
#include "main.h"

#define TARGET_PBACLK_FREQ_HZ 32000000 // master clock divided by 2 (64MHZ/2 = 32MHz)
#define UNCONFIGUREDMODE 0
#define HOSTMODE 1
#define DEVICEMODE 2
#define TWELVEBIT 1
#define SIXTEENBIT 2


//------------------  C O N F I G U R A T I O N S  -------------------

#define EEPROM_ADDRESS        0x50        // EEPROM's TWI address
#define EEPROM_ADDR_LGT       3           // Address length of the EEPROM memory
#define VIRTUALMEM_ADDR_START 0x123456    // Address of the virtual memory in the EEPROM
#define TWI_SPEED             100000       // Speed of TWI  //was 100000 in my later example-JS

static void initSPIbus(void);
static void setDACSPI(spi_options_t spiOptions);
static void setMemorySPI(spi_options_t spiOptions);

int defaultTuningMap[8] = {0,1,2,3,4,5,6,7};

//SPI options for the 16 and 12 bit DACs
spi_options_t spiOptions16DAC =
{
	.reg          = 0,
	.baudrate     = 16000000,
	.bits         = 16,
	.spck_delay   = 1,
	.trans_delay  = 0,
	.stay_act     = 1,
	.spi_mode     = 1,
	.modfdis      = 1
};

spi_options_t spiOptions12DAC =
{
	.reg          = 0,
	.baudrate     = 2000000,
	.bits         = 8,
	.spck_delay   = 1,
	.trans_delay  = 1,
	.stay_act     = 1,
	.spi_mode     = 1,
	.modfdis      = 1
};

spi_options_t spiOptionsMemory =
{
	.reg          = 0,
	.baudrate     = 5000000, 
	.bits         = 8,
	.spck_delay   = 1,
	.trans_delay  = 1,
	.stay_act     = 1,
	.spi_mode     = 0,
	.modfdis      = 1
};

unsigned short dacouthigh = 0;
unsigned short dacoutlow = 0;
unsigned short DAC1outhigh = 0;
unsigned short DAC1outlow = 0;
unsigned char SPIbusy = 0;
unsigned char preset_num = 0;
unsigned char preset_to_save_num = 0;
unsigned char savingActive = 0;
uint16_t globalPitchGlide = 1;
uint16_t globalCVGlide = 7;
uint16_t globalPitchGlideDisplay = 1;
uint16_t globalCVGlideDisplay = 7;
unsigned char globalGlideMax = 80;
SuspendRetrieveType suspendRetrieve = RetrieveWhenever;
unsigned char number_for_7Seg = 0;
unsigned char blank7Seg = 0;
unsigned char tuningLoading = 0;
unsigned char transpose_indication_active = 0;
unsigned char normal_7seg_number = 0;
int prevSentPitch = -1;

const uint16_t glide_lookup[81] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,20,22,24,27,30,33,37,45,53,60,68,78,90,110,120, 130, 140, 150, 160, 170, 180, 195, 110, 125, 140, 155, 170, 185, 200, 220, 240, 260, 280, 300, 330, 360, 390, 420, 450, 480, 510, 550, 590, 630, 670, 710, 760, 810, 860, 910, 970, 1030, 1100, 1200, 1300, 1400, 1500, 1700, 1900, 2400, 2900, 3600};
																													   // 33																												56																				  72								 78				
BOOL no_device_mode_active = FALSE;

BOOL busyWithUSB = FALSE;

GlobalPreferences preference_num = PRESET_SELECT;
GlobalPreferences num_preferences = PREFERENCES_COUNT;
TuningOrLearnType tuningOrLearn = TUNING_SELECT;
GlidePreferences glide_pref = GLOBAL_PITCH_GLIDE;
ClockPreferences clock_pref = BPM;
ConnectedDeviceType type_of_device_connected = NoDeviceConnected;
ArpVsTouch arp_or_touch = ARP_MODE;

uint8_t freeze_LED_update = TRUE;

uint8_t currentNumberToMIDILearn = 0;
uint32_t dummycounter = 0;
uint8_t manta_mapper = 0;
uint8_t tuning_count = 0;
uint8_t spi_mode = 0;
uint8_t new_manta_attached = false;

uint32_t clock_speed = 0; // this is the speed of the internal sequencer clock 
uint32_t clock_speed_max = 99; 
uint32_t clock_speed_displayed = 0;
uint32_t tempoDivider = 4 ;
uint32_t tempoDividerMax = 9;

uint8_t SevSegArpMode = 0;
uint32_t upHeld = 0;
uint32_t downHeld = 0;
uint32_t holdTimeThresh = 8;
static uint32_t clockFrameCounter = 0;
static uint32_t buttonFrameCounter = 0;
static uint32_t buttonHoldSpeed = 120;
static uint32_t blink7SegCounter = 0;
static uint32_t blinkSpeed7Seg = 500;

int mantaCompositionSavePending = 0;
int mantaCompositionLoadPending = 0;

uint32_t myUSBMode = UNCONFIGUREDMODE;

#define EXT_INT_EXAMPLE_PIN_LINE1               AVR32_EIC_EXTINT_7_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE1          AVR32_EIC_EXTINT_7_FUNCTION
#define EXT_INT_EXAMPLE_LINE1                   EXT_INT7 // this should be right JS
#define EXT_INT_EXAMPLE_IRQ_LINE1               AVR32_EIC_IRQ_7 // not sure what this should be
#define EXT_INT_EXAMPLE_NB_LINES				1

//! Structure holding the configuration parameters of the EIC module.
eic_options_t eic_options[EXT_INT_EXAMPLE_NB_LINES];

twi_options_t my_opt;
twi_package_t I2Cpacket_sent, I2Cpacket_received;

const U8 test_pattern[] =  {
	0xAA,
	0x55,
	0xA5,
	0x5A,
	0x77,
0x99};


// Outputs 0 through 11 (counting left to right top down), expects 16 bit.

void sendDataToOutput(int which, int ramp, uint16_t data)
{
	
	//now send it out!
	if (which == 0)			
	{
		tIRampSetTime(&out[0][0], ramp);
		tIRampSetDest(&out[0][0], data);
	}
	else if (which == 1)	
	{
		tIRampSetTime(&out[0][1], ramp);
		tIRampSetDest(&out[0][1], (data >> 4));
	}
	else if (which == 2)	
	{
		tIRampSetTime(&out[0][2], ramp);
		tIRampSetDest(&out[0][2], (data >> 4));
	}
	else if (which == 3)
	{
		 tIRampSetTime(&out[0][3], ramp);
		tIRampSetDest(&out[0][3], data);
	}
	else if (which == 4)
	{
		tIRampSetTime(&out[0][4], ramp);
		tIRampSetDest(&out[0][4], (data >> 4));
	}
	else if (which == 5)
	{
		tIRampSetTime(&out[0][5], ramp);
		tIRampSetDest(&out[0][5], (data >> 4));
	}
	else if (which == 6)
	{
		tIRampSetTime(&out[1][0], ramp);
		tIRampSetDest(&out[1][0], data);
	}
	else if (which == 7)
	{
		tIRampSetTime(&out[1][1], ramp);
		tIRampSetDest(&out[1][1], (data >> 4));
	}
	else if (which == 8)
	{
		tIRampSetTime(&out[1][2], ramp);
		tIRampSetDest(&out[1][2], (data >> 4));
	}
	else if (which == 9)
	{
		tIRampSetTime(&out[1][3], ramp);
		tIRampSetDest(&out[1][3], data);
	}
	else if (which == 10)
	{
		tIRampSetTime(&out[1][4], ramp);
		tIRampSetDest(&out[1][4], (data >> 4));
	}
	else if (which == 11)
	{
		tIRampSetTime(&out[1][5], ramp);
		tIRampSetDest(&out[1][5], (data >> 4));
	}
}


/*! \brief Main function. Execution starts here.
 */
int main(void){
	
	irq_initialize_vectors();
	cpu_irq_enable();
	
	// Initialize the sleep manager
	sleepmgr_init();
	setupEIC();

	sysclk_init();
	flashc_set_wait_state(1); // necessary because the MCU is running at higher than 33MHz. -JS
	board_init();
	
	//comment this bootloader check in when the firmware is ready for release
	/*
	if (downSwitch() > 0) //enter MSC bootloader instead of main program if the down switch is held during startup
	{
		usb_msc_bl_start();
	}
	*/
	
	ui_init();

	//initialize the SPI bus for DAC
	initSPIbus();
	
	//send the messages to the DACs to make them update without software LDAC feature
	DACsetup();
	
	//make a randomized pattern for the NoDevice pattern presets
	noDeviceCreateNewRandomPatterns();
	noDevicePatterns.patternLength = 16;
	
	loadTuning(globalTuning);
	displayState = GlobalDisplayStateNil;
	currentTuningHex = -1;
	currentHexmapEditHex = -1;
	currentHexmapHex = -1;
	// figure out if we're supposed to be in host mode or device mode for the USB
	USB_Mode_Switch_Check();
	
	tKeyboard_init(&manta[InstrumentOne].keyboard, 1);
	tKeyboard_init(&manta[InstrumentTwo].keyboard, 1);
	tKeyboard_init(&fullKeyboard, 2);
	
	currentComp[InstrumentOne]=  -1;
	currentComp[InstrumentTwo]=  -1;
	
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			tIRampInit(&out[i][j], 2000, 0);
			tIRampSetDest(&out[i][j], 0);
		}
	}
	
	tIRampInit(&pitchBendRamp, 2000, 10);
	tIRampSetDest(&pitchBendRamp, 0);
	takeover = FALSE;
	hexmapEditMode = FALSE;
	directEditMode = FALSE;
	
	//start off on preset 0;
	
	
	
	
	preset_num = 0;
	//loadStartupStateFromExternalMemory();
	
	
	
	//delay_ms(600); // why is this necessary? taking it out makes the manta state not get initialized correctly, but it's weird that we need it
	initTimers();
	//updatePreset();
	Write7Seg(0);
	//I added this global variable - it's to allow minor functions to take over the 7-seg, then go back to whatever it is normally showing.
	normal_7seg_number = 0;
	// The USB management is entirely managed by interrupts.
	// As a consequence, the user application only has to play with the power modes.
	
	while (true) {	

		//currently putting low priority things like this in the main loop, as it should be the most background processes
		if (mantaSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringMantaPresetToExternalMemory();
			}
		}
		else if (mantaCompositionSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringMantaCompositionsToExternalMemory();
			}
		}
		else if (midiSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringMidiPresetToExternalMemory();
			}
		}
		else if (noDeviceSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringNoDevicePresetToExternalMemory();
			}
		}
		else if (tuningSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringTuningToExternalMemory();
			}
		}
		else if (hexmapSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringHexmapToExternalMemory();
			}
		}
		else if (startupStateSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringStartupStateToExternalMemory();
			}
		}
		else if (mantaLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingMantaPresetFromExternalMemory();
			}
		}
		else if (mantaCompositionLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingMantaCompositionsFromExternalMemory();
			}
		}
		else if (midiLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingMidiPresetFromExternalMemory();
			}
		}
		else if (noDeviceLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingNoDevicePresetFromExternalMemory();
			}
		}
		else if (tuningLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingTuningFromExternalMemory();
			}
		}
		else if (hexmapLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingHexmapFromExternalMemory();
			}
		}
		else if (startupStateLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingStartupStateFromExternalMemory();
			}
		}
		
		if (new_manta_attached)
		{
			delay_ms(5); //seems to help it get through the attachment process before it gets connected
			manta_LED_set_mode(HOST_CONTROL_FULL);
			manta_clear_all_LEDs();
			manta_send_LED();
			type_of_device_connected = MantaConnected;
			freeze_LED_update = FALSE;
			updatePreset();		//this will make it reset if the manta is unplugged and plugged back in. Might not be the desired behavior in case of accidental unplug, but will be cleaner if unplugged on purpose.
			new_manta_attached = false;
			busyWithUSB = FALSE;
		}
		
		sleepmgr_enter_sleep();
		
	}
	
}



// SequencerTwo timer interrupt.
__attribute__((__interrupt__))
static void tc2_irq(void)
{
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(TC2, TC2_CHANNEL);
	
	//TC2 is now also the internal metronome clock, the up/down button held sensing, and the 7seg blinker
	// as well as the timer for turning off triggers on the outputs
	

	if (!busyWithUSB) //this is so that interrupts and clocks don't screw up Manta USB enumeration
	{
		
		//step the internal clock
		if (clock_speed != 0)
		{
			if (clockFrameCounter >= clock_speed)
			{
				clockHappened();
				clockFrameCounter = 0;
			}
			clockFrameCounter++;
		}


		//watch the up and down buttons to catch the "hold down" action and speed up the preset scrolling
		if (upSwitch())
		{
			buttonFrameCounter++;
			if (buttonFrameCounter > buttonHoldSpeed)
			{
				upHeld++;
				if (upHeld > holdTimeThresh)
				{
					suspendRetrieve = DontRetrieve; //make it so it doesn't actually load the presets it's scrolling through until you release the button
					Preset_Switch_Check(1);
				}
				buttonFrameCounter = 0;
			}
		}
		else
		{
			if (upHeld > 0)
			{
				if (suspendRetrieve == DontRetrieve)
				{
					suspendRetrieve = RetrieveNow;
					Preset_Switch_Check(1);
				}
			}
			upHeld = 0;
		}
	
		if (downSwitch())
		{
			buttonFrameCounter++;
			if (buttonFrameCounter > buttonHoldSpeed)
			{
				downHeld++;
				if (downHeld > holdTimeThresh)
				{
					suspendRetrieve =DontRetrieve; //make it so it doesn't actually load the presets it's scrolling through until you release the button
					Preset_Switch_Check(0);
				}
				buttonFrameCounter = 0;
			}

		}
		else
		{
			if (downHeld > 0)
			{
				if (suspendRetrieve == DontRetrieve)
				{
					suspendRetrieve = RetrieveNow;
					Preset_Switch_Check(0);
				}
			}
			downHeld = 0;
		}
	
		blink7SegCounter++;
	
		if (blink7SegCounter >= blinkSpeed7Seg)
		{
			blink7SegCounter = 0;
		
			if (savingActive)
			{
				blank7Seg = !blank7Seg;
				Write7Seg(number_for_7Seg);
			}
			else
			{
				blank7Seg = 0;
				Write7Seg(number_for_7Seg);
			}
		}
	
		if (type_of_device_connected == JoystickConnected) return;
	
		else if (type_of_device_connected == NoDeviceConnected)
		{
			for (int i = 0; i < 12; i++)
			{
				if (noDevicePatterns.trigCount[i] > 0)
				{
					if (--(noDevicePatterns.trigCount[i]) == 0)
					{
						sendDataToOutput(i, 0, 0x0);
					}
				}
			}
		}
		else if ((type_of_device_connected == MIDIKeyboardConnected) || (type_of_device_connected == MIDIComputerConnected))
		{
			tMIDIKeyboard* keyboard =  &MIDIKeyboard;
		
			for (int i = 0; i < 12; i++)
			{
				if (keyboard->trigCount[i] > 0)
				{
					if (--(keyboard->trigCount[i]) == 0)
					{
						sendDataToOutput(i, 0, 0x0);
					}
				}
			}
		}
		else if (type_of_device_connected == MantaConnected)
		{
			if (!takeover) // Dual instrument, not takeover
			{
				for (int inst = 0; inst < 2; inst++)
				{
					tMantaInstrument* instrument = &manta[inst];
			
					if (instrument->type == DirectInstrument) // DirectInstrument
					{
						for (int i = 0; i < 48; i++)
						{
							DirectType type = instrument->direct.hexes[i].type;
							int output = instrument->direct.hexes[i].output;
							if (type == DirectTrigger)
							{
								if (instrument->direct.hexes[i].trigCount > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
								{
									if (--(instrument->direct.hexes[i].trigCount) == 0)
									{
										sendDataToOutput(6*inst+output, 0, 0x0);
									}
								}
							}
							else if (type == DirectCV)
							{
								sendDataToOutput(6*inst+output, globalCVGlide, butt_states[i] << 8);
							}
						}
					}
					else if (instrument->type == KeyboardInstrument)
					{
						if (instrument->keyboard.playMode == ArpMode)
						{
							if (instrument->keyboard.trigCount > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->keyboard.trigCount[fullKeyboard.currentVoice]) == 0)
								{
									tIRampSetDest(&out[inst][CVKTRIGGER-2+3*fullKeyboard.currentVoice], 0);
								}
							}
						}
						else if (instrument->keyboard.numVoices == 1)
						{
							if (instrument->keyboard.trigCount > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->keyboard.trigCount[0]) == 0)
								{
									tIRampSetDest(&out[inst][CVKTRIGGER], 0);
								}
							}
						}
					}
					else if (instrument->type == SequencerInstrument)
					{
						if (instrument->sequencer.pitchOrTrigger == PitchMode)
						{
							if (instrument->sequencer.trigCount[0] > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->sequencer.trigCount[0]) == 0)
								{
									tIRampSetDest(&out[inst][CVTRIGGER], 0);
								}
							}
						}
						else //otherwise we're in trigger mode
						{
							if (instrument->sequencer.trigCount[1] > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->sequencer.trigCount[1]) == 0)
								{
									tIRampSetDest(&out[inst][CVTRIG1], 0);
								}
							}
							if (instrument->sequencer.trigCount[2] > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->sequencer.trigCount[2]) == 0)
								{
									tIRampSetDest(&out[inst][CVTRIG2], 0);
								}
							}
							if (instrument->sequencer.trigCount[3] > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->sequencer.trigCount[3]) == 0)
								{
									tIRampSetDest(&out[inst][CVTRIG3], 0);
								}
							}
							if (instrument->sequencer.trigCount[4] > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->sequencer.trigCount[4]) == 0)
								{
									tIRampSetDest(&out[inst][CVTRIG4], 0);
								}
							}
						}
					}
			
				}
			}
			else if (takeoverType == KeyboardInstrument)
			{
				if (fullKeyboard.playMode == ArpMode)
				{
					if (fullKeyboard.trigCount[fullKeyboard.currentVoice] > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
					{
						if (--(fullKeyboard.trigCount[fullKeyboard.currentVoice]) == 0)
						{
							tIRampSetDest(&out[0][CVKTRIGGER-2+3*fullKeyboard.currentVoice], 0);
						}
					}
				}
			}
			else if (takeoverType == DirectInstrument)
			{
				for (int i = 0; i < 48; i++)
				{
					DirectType type = fullDirect.hexes[i].type;
					int output = fullDirect.hexes[i].output;
					if (type == DirectTrigger)
					{
						if (fullDirect.hexes[i].trigCount > 0)
						{
							if (--(fullDirect.hexes[i].trigCount) == 0)
							{
								sendDataToOutput(output, 0, 0x0);
							}
						}
					}
					else if (type == DirectCV)
					{
						sendDataToOutput(output, globalCVGlide, butt_states[i] << 8);
					}
				}
			}
		}
	}
}


// Glide timer.
__attribute__((__interrupt__))
static void tc3_irq(void)
{
	// Clear the interrupt flag. This is a side effect of reading the TC SR.

	tc_read_sr(TC3, TC3_CHANNEL);

	if (!busyWithUSB) //this is so that interrupts and clocks don't screw up Manta USB enumeration
	{
	
	
		if (type_of_device_connected == MantaConnected)
		{
			if (!takeover) // Dual instrument, not takeover
			{
				for (int inst = 0; inst < 2; inst++)
				{
					tMantaInstrument* instrument = &manta[inst];
	
					if (instrument->type == KeyboardInstrument) // KeyboardInstrument
					{
						if (instrument->keyboard.playMode == ArpMode)
						{
							tIRampSetTime    (&out[inst][CV1+3*fullKeyboard.currentVoice], globalCVGlide);
							tIRampSetDest    (&out[inst][CV1+3*fullKeyboard.currentVoice],  butt_states[instrument->keyboard.currentNote] << 4);
						}
						else
						{
							for (int i = 0; i < instrument->keyboard.numVoices; i++)
							{
								tIRampSetTime    (&out[inst][i*3 + CV1], globalCVGlide);
								tIRampSetDest    (&out[inst][i*3 + CV1],  butt_states[instrument->keyboard.voices[i]] << 4);
							}
						}
					
					}
		
				}
			}
			else if (takeoverType == KeyboardInstrument) // Takeover mode
			{
				if (fullKeyboard.playMode == ArpMode)
				{
					tIRampSetTime    (&out[0][CV1+3*fullKeyboard.currentVoice], globalCVGlide);
					tIRampSetDest    (&out[0][CV1+3*fullKeyboard.currentVoice],  butt_states[fullKeyboard.currentNote] << 4);
				}
				else
				{
					for (int i = 0; i < fullKeyboard.numVoices; i++)
					{
						int inst = (i / 2);
						tIRampSetTime (&out[inst][((i*3) % 6) + CV1], globalCVGlide);
						tIRampSetDest    (&out[inst][((i*3) % 6) + CV1],  butt_states[fullKeyboard.voices[i]] << 4);
					}
				}
			
			}
		}
		else if ((type_of_device_connected == MIDIComputerConnected) || (type_of_device_connected == MIDIKeyboardConnected))
		{
			for (int n = 0; n < 12; n++)
			{
				int cc = MIDIKeyboard.learnedCCsAndNotes[n][0];
				int note = MIDIKeyboard.learnedCCsAndNotes[n][1];
			
				if (cc < 255) //255 is the mark for "unused" == the mark of the BEAST : HAIL SATAN!
				{
					sendDataToOutput((n + MIDIKeyboard.firstFreeOutput), globalCVGlide, MIDIKeyboard.CCs[cc]); 
				}
				else if (note < 255)
				{
					BOOL tempOutputState = MIDIKeyboard.outputStates[n];
					
					if ((MIDIKeyboard.notes[note][0] > 0) && (tempOutputState == FALSE))
					{
						sendDataToOutput(n + MIDIKeyboard.firstFreeOutput, 0, 65535);
						MIDIKeyboard.outputStates[n] = TRUE;
						if (MIDIKeyboard.gatesOrTriggers == TRIGGERS)
						{
							MIDIKeyboard.trigCount[n + MIDIKeyboard.firstFreeOutput] = TRIGGER_TIMING;
						}
					}
					else if ((MIDIKeyboard.notes[note][0] == 0) && (tempOutputState == TRUE))
					{
						sendDataToOutput(n + MIDIKeyboard.firstFreeOutput, 0, 0);
						MIDIKeyboard.outputStates[n] = FALSE;
					}
				}
			
			}
		}
	
		//now tick those RAMPs and send the data to the DACs
		if ((type_of_device_connected == MIDIComputerConnected) || (type_of_device_connected == MIDIKeyboardConnected))
		{
			int32_t tempPitchBend = tIRampTick(&pitchBendRamp);
			if (MIDIKeyboard.numVoices == 1)
			{
				DAC16Send	(0, (uint16_t)(tIRampTick(&out[0][0]) + tempPitchBend));
				dacsend     (0, 0,  (uint16_t)tIRampTick(&out[0][1]));
				dacsend     (0, 1,  (uint16_t)tIRampTick(&out[0][2]));
				DAC16Send	(1, (uint16_t)tIRampTick(&out[0][3]));
				dacsend     (1, 0,  (uint16_t)tIRampTick(&out[0][4]));
				dacsend     (1, 1,  (uint16_t)tIRampTick(&out[0][5]));
				DAC16Send	(2, (uint16_t)tIRampTick(&out[1][0]));
				dacsend     (2, 0,  (uint16_t)tIRampTick(&out[1][1]));
				dacsend     (2, 1,  (uint16_t)tIRampTick(&out[1][2]));
				DAC16Send	(3, (uint16_t)tIRampTick(&out[1][3]));
				dacsend     (3, 0,  (uint16_t)tIRampTick(&out[1][4]));
				dacsend     (3, 1,  (uint16_t)tIRampTick(&out[1][5]));
			}
			else if (MIDIKeyboard.numVoices == 2)
			{
				DAC16Send	(0, (uint16_t)(tIRampTick(&out[0][0]) + tempPitchBend));
				dacsend     (0, 0,  (uint16_t)tIRampTick(&out[0][1]));
				dacsend     (0, 1,  (uint16_t)tIRampTick(&out[0][2]));
				DAC16Send	(1, (uint16_t)(tIRampTick(&out[0][3]) + tempPitchBend));
				dacsend     (1, 0,  (uint16_t)tIRampTick(&out[0][4]));
				dacsend     (1, 1,  (uint16_t)tIRampTick(&out[0][5]));
				DAC16Send	(2, (uint16_t)tIRampTick(&out[1][0]));
				dacsend     (2, 0,  (uint16_t)tIRampTick(&out[1][1]));
				dacsend     (2, 1,  (uint16_t)tIRampTick(&out[1][2]));
				DAC16Send	(3, (uint16_t)tIRampTick(&out[1][3]));
				dacsend     (3, 0,  (uint16_t)tIRampTick(&out[1][4]));
				dacsend     (3, 1,  (uint16_t)tIRampTick(&out[1][5]));
			}
			else if (MIDIKeyboard.numVoices == 3)
			{
				DAC16Send	(0, (uint16_t)(tIRampTick(&out[0][0]) + tempPitchBend));
				dacsend     (0, 0,  (uint16_t)tIRampTick(&out[0][1]));
				dacsend     (0, 1,  (uint16_t)tIRampTick(&out[0][2]));
				DAC16Send	(1, (uint16_t)(tIRampTick(&out[0][3]) + tempPitchBend));
				dacsend     (1, 0,  (uint16_t)tIRampTick(&out[0][4]));
				dacsend     (1, 1,  (uint16_t)tIRampTick(&out[0][5]));
				DAC16Send	(2, (uint16_t)(tIRampTick(&out[1][0]) + tempPitchBend));
				dacsend     (2, 0,  (uint16_t)tIRampTick(&out[1][1]));
				dacsend     (2, 1,  (uint16_t)tIRampTick(&out[1][2]));
				DAC16Send	(3, (uint16_t)tIRampTick(&out[1][3]));
				dacsend     (3, 0,  (uint16_t)tIRampTick(&out[1][4]));
				dacsend     (3, 1,  (uint16_t)tIRampTick(&out[1][5]));
			}
			else if (MIDIKeyboard.numVoices == 4)
			{
				DAC16Send	(0, (uint16_t)(tIRampTick(&out[0][0]) + tempPitchBend));
				dacsend     (0, 0,  (uint16_t)tIRampTick(&out[0][1]));
				dacsend     (0, 1,  (uint16_t)tIRampTick(&out[0][2]));
				DAC16Send	(1, (uint16_t)(tIRampTick(&out[0][3]) + tempPitchBend));
				dacsend     (1, 0,  (uint16_t)tIRampTick(&out[0][4]));
				dacsend     (1, 1,  (uint16_t)tIRampTick(&out[0][5]));
				DAC16Send	(2, (uint16_t)(tIRampTick(&out[1][0]) + tempPitchBend));
				dacsend     (2, 0,  (uint16_t)tIRampTick(&out[1][1]));
				dacsend     (2, 1,  (uint16_t)tIRampTick(&out[1][2]));
				DAC16Send	(3, (uint16_t)(tIRampTick(&out[1][3]) + tempPitchBend));
				dacsend     (3, 0,  (uint16_t)tIRampTick(&out[1][4]));
				dacsend     (3, 1,  (uint16_t)tIRampTick(&out[1][5]));
			}
		}
		else
		{
			DAC16Send	(0, (uint16_t)tIRampTick(&out[0][0]));
			dacsend     (0, 0,  (uint16_t)tIRampTick(&out[0][1]));
			dacsend     (0, 1,  (uint16_t)tIRampTick(&out[0][2]));
			DAC16Send	(1, (uint16_t)tIRampTick(&out[0][3]));
			dacsend     (1, 0,  (uint16_t)tIRampTick(&out[0][4]));
			dacsend     (1, 1,  (uint16_t)tIRampTick(&out[0][5]));
			DAC16Send	(2, (uint16_t)tIRampTick(&out[1][0]));
			dacsend     (2, 0,  (uint16_t)tIRampTick(&out[1][1]));
			dacsend     (2, 1,  (uint16_t)tIRampTick(&out[1][2]));
			DAC16Send	(3, (uint16_t)tIRampTick(&out[1][3]));
			dacsend     (3, 0,  (uint16_t)tIRampTick(&out[1][4]));
			dacsend     (3, 1,  (uint16_t)tIRampTick(&out[1][5]));
		}
	}
}

static void tc2_init(volatile avr32_tc_t *tc)
{

	static const tc_waveform_opt_t waveform_opt = {
		// Channel selection.
		.channel  = TC2_CHANNEL,
		// Software trigger effect on TIOB.
		.bswtrg   = TC_EVT_EFFECT_NOOP,
		// External event effect on TIOB.
		.beevt    = TC_EVT_EFFECT_NOOP,
		// RC compare effect on TIOB.
		.bcpc     = TC_EVT_EFFECT_NOOP,
		// RB compare effect on TIOB.
		.bcpb     = TC_EVT_EFFECT_NOOP,
		// Software trigger effect on TIOA.
		.aswtrg   = TC_EVT_EFFECT_NOOP,
		// External event effect on TIOA.
		.aeevt    = TC_EVT_EFFECT_NOOP,
		// RC compare effect on TIOA.
		.acpc     = TC_EVT_EFFECT_NOOP,
		/*
		 * RA compare effect on TIOA.
		 * (other possibilities are none, set and clear).
		 */
		.acpa     = TC_EVT_EFFECT_NOOP,
		/*
		 * Waveform selection: Up mode with automatic trigger(reset)
		 * on RC compare.
		 */
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
		// External event trigger enable.
		.enetrg   = false,
		// External event selection.
		.eevt     = 0,
		// External event edge selection.
		.eevtedg  = TC_SEL_NO_EDGE,
		// Counter disable when RC compare.
		.cpcdis   = false,
		// Counter clock stopped with RC compare.
		.cpcstop  = false,
		// Burst signal selection.
		.burst    = false,
		// Clock inversion.
		.clki     = false,
		// Internal source clock 3, connected to fPBA / 8.
		.tcclks   = TC_CLOCK_SOURCE_TC3
	};

	// Options for enabling TC interrupts
	static const tc_interrupt_t tc_interrupt = {
		.etrgs = 0,
		.ldrbs = 0,
		.ldras = 0,
		.cpcs  = 1, // Enable interrupt on RC compare alone
		.cpbs  = 0,
		.cpas  = 0,
		.lovrs = 0,
		.covfs = 0
	};
	
	// Initialize the timer/counter.
	tc_init_waveform(tc, &waveform_opt);
	/*
		* Set the compare triggers.
		* We configure it to count every 1 milliseconds.
		* We want: (1 / (fPBA / 8)) * RC = 1 ms, hence RC = (fPBA / 8) / 1000
		* to get an interrupt every 10 ms.
		*/
	tc_write_rc( tc, TC2_CHANNEL, 2000); // was 1000
	// configure the timer interrupt
	tc_configure_interrupts(tc, TC2_CHANNEL, &tc_interrupt);
	
}
uint64_t pba_hz = 0;
static void tc3_init(volatile avr32_tc_t *tc)
{

	static const tc_waveform_opt_t waveform_opt = {
		// Channel selection.
		.channel  = TC3_CHANNEL,
		// Software trigger effect on TIOB.
		.bswtrg   = TC_EVT_EFFECT_NOOP,
		// External event effect on TIOB.
		.beevt    = TC_EVT_EFFECT_NOOP,
		// RC compare effect on TIOB.
		.bcpc     = TC_EVT_EFFECT_NOOP,
		// RB compare effect on TIOB.
		.bcpb     = TC_EVT_EFFECT_NOOP,
		// Software trigger effect on TIOA.
		.aswtrg   = TC_EVT_EFFECT_NOOP,
		// External event effect on TIOA.
		.aeevt    = TC_EVT_EFFECT_NOOP,
		// RC compare effect on TIOA.
		.acpc     = TC_EVT_EFFECT_NOOP,
		/*
		 * RA compare effect on TIOA.
		 * (other possibilities are none, set and clear).
		 */
		.acpa     = TC_EVT_EFFECT_NOOP,
		/*
		 * Waveform selection: Up mode with automatic trigger(reset)
		 * on RC compare.
		 */
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
		// External event trigger enable.
		.enetrg   = false,
		// External event selection.
		.eevt     = 0,
		// External event edge selection.
		.eevtedg  = TC_SEL_NO_EDGE,
		// Counter disable when RC compare.
		.cpcdis   = false,
		// Counter clock stopped with RC compare.
		.cpcstop  = false,
		// Burst signal selection.
		.burst    = false,
		// Clock inversion.
		.clki     = false,
		// Internal source clock 3, connected to fPBA / 8.
		.tcclks   = TC_CLOCK_SOURCE_TC3
	};

	// Options for enabling TC interrupts
	static const tc_interrupt_t tc_interrupt = {
		.etrgs = 0,
		.ldrbs = 0,
		.ldras = 0,
		.cpcs  = 1, // Enable interrupt on RC compare alone
		.cpbs  = 0,
		.cpas  = 0,
		.lovrs = 0,
		.covfs = 0
	};
	
	// Initialize the timer/counter.
	tc_init_waveform(tc, &waveform_opt);
	/*
	* Set the compare triggers.
	* We configure it to count every 1 milliseconds.
	* We want: (1 / (fPBA / 8)) * RC = 1 ms,
	hence RC = (fPBA / 8) / 1000
	* to get an interrupt every 10 ms.
	*/
	tc_write_rc( tc, TC3_CHANNEL, 3000); // 2000 = approximately .5 ms   ///changed it to 3000 = .33ms to get MIDI to work better -- the fact that it was the same timing as the sof seemed to cause trouble.  4000 works with MIDI but breaks Manta
	
	// configure the timer interrupt
	tc_configure_interrupts(tc, TC3_CHANNEL, &tc_interrupt);
}

void initTimers (void)
{
	
	//tc1 = TC1;
	tc2 = TC2;
	tc3 = TC3;
	
	// Enable the clock to the selected example Timer/counter peripheral module.
	//sysclk_enable_peripheral_clock(TC1);
	sysclk_enable_peripheral_clock(TC2);
	sysclk_enable_peripheral_clock(TC3);
	
	//tc1_init(tc1);
	tc2_init(tc2);
	tc3_init(tc3);
	
	//tc_start(tc1, TC1_CHANNEL);
	tc_start(tc2, TC2_CHANNEL);
	tc_start(tc3, TC3_CHANNEL);

}

  
// interrupt handler for external gate signal input from synthesizer.
__attribute__((__interrupt__))
static void eic_int_handler1(void)
{
	eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE1);
	if (!busyWithUSB) //this is so that interrupts and clocks don't screw up Manta USB enumeration
	{
		clockHappened();
	}
}


// interrupt handler to find the state of the pushbutton switches on the panel and the USB vbus sensing pin
__attribute__((__interrupt__))
static void int_handler_switches (void)
{
	
	if( gpio_get_pin_interrupt_flag( GPIO_HOST_DEVICE_SWITCH) )
	{	// PA11 generated the interrupt.
		delay_us(5000); // to de-bounce
		
		USB_Mode_Switch_Check();
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_HOST_DEVICE_SWITCH);
	}
	
	else if( gpio_get_pin_interrupt_flag( GPIO_PRESET_DOWN_SWITCH) )
	{		
		//down switch
		delay_us(2000); // to de-bounce
		Preset_Switch_Check(0);
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_PRESET_DOWN_SWITCH);
	}
	
	else if( gpio_get_pin_interrupt_flag( GPIO_PRESET_UP_SWITCH) )
	{		
		//up switch
		delay_us(2000); // to de-bounce
		Preset_Switch_Check(1);
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_PRESET_UP_SWITCH);
	}
	
	else if( gpio_get_pin_interrupt_flag( GPIO_PREFERENCES_SWITCH) )
	{
		//up switch
		delay_us(2000); // to de-bounce
		Preferences_Switch_Check();
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_PREFERENCES_SWITCH);
	}
	
	else if( gpio_get_pin_interrupt_flag( GPIO_SAVE_SWITCH) )
	{
		//up switch
		delay_us(2000); // to de-bounce
		Save_Switch_Check();
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_SAVE_SWITCH);
	}
	
}

void USB_Mode_Switch_Check(void)
{	
		if (gpio_get_pin_value(GPIO_HOST_DEVICE_SWITCH))
		{

			if (myUSBMode == HOSTMODE)
			{
				uhc_stop(1);
			}
 			udc_start();
			myUSBMode = DEVICEMODE;
			
			udc_attach();

		}
		else
		{
			if (myUSBMode == DEVICEMODE)
			{
				udc_stop();
			}
			uhc_start();
			myUSBMode = HOSTMODE;
		}	
		/// TODO just took this out - should it check?
		//updatePreset();
}

void Preset_Switch_Check(uint8_t whichSwitch)
{
	if ((type_of_device_connected == MantaConnected) && displayState == UpDownSwitchBlock) return;
	
	if ((type_of_device_connected == NoDeviceConnected) && (no_device_mode_active == FALSE))
	{
		no_device_mode_active = TRUE;
		return;
	}

	if (preferencesSwitch()) //if you are holding down the preferences switch while pressing one of the up/down buttons, you are trying to switch between touch and arpeggiator modes for MIDI Keyboard
	{
		
		if (type_of_device_connected == NoDeviceConnected)
		{
			if(upSwitch())
			{
				noDeviceCreateNewRandomPatterns();
			}
			else if (downSwitch())
			{
				noDeviceSlightlyAlterRandomPatterns();
			}
		}
		else //otherwise you are trying to set an arpeggiator mode or touch mode change to Manta or MIDI.
		{
			SevSegArpMode = 1;

			if(upSwitch())
			{
				MIDIKeyboard.playMode = ArpMode;
				MIDIKeyboard.arpModeType++;
				if (MIDIKeyboard.arpModeType >= 8)
				{
					MIDIKeyboard.arpModeType = 7;
				}
				Write7Seg(MIDIKeyboard.arpModeType+1);
			}
			
			else if(downSwitch())
			{

				if (MIDIKeyboard.arpModeType > 0)
				{
					MIDIKeyboard.arpModeType--;
					MIDIKeyboard.playMode = ArpMode;
					Write7Seg(MIDIKeyboard.arpModeType+1);
				}
				else if (MIDIKeyboard.arpModeType == 0)
				{
					MIDIKeyboard.playMode = TouchMode;
					Write7Seg(0);
				}
			}
		}
		
		
	}
	
	else
	{
		if (displayState == DirectOutputSelect)
		{
			if (whichSwitch)
			{
				if (upSwitch())
				{
					if (++currentDirectEditOutput > (takeover ? 11 : 5)) currentDirectEditOutput = (takeover ? 11 : 5);
				}
			}
			else
			{
				if (downSwitch())
				{
					if (currentDirectEditOutput > 0) currentDirectEditOutput--;
				}
			}
			
			tDirect_setOutput(editDirect, currentDirectEditHex, currentDirectEditOutput);
			
			setDirectLEDs();
			
			Write7Seg((currentDirectEditOutput < 0) ? -1 : (currentDirectEditOutput+1));
			normal_7seg_number = (currentDirectEditOutput+1);
		}	
		else if (displayState == HexmapPitchSelect)
		{
			if (whichSwitch)
			{
				if (upSwitch())
				{
					if (++currentHexmapEditPitch > 127) currentHexmapEditPitch = 0;
				}
			}
			else
			{
				if (downSwitch())
				{
					if (currentHexmapEditPitch >= 0) currentHexmapEditPitch--;
				}
			}
		
			tKeyboard_assignNoteToHex(hexmapEditKeyboard, currentHexmapEditHex, currentHexmapEditPitch);
		
			dacSendKeyboard(hexmapEditInstrument);
		
			Write7Seg(currentHexmapEditPitch%100);
			normal_7seg_number = currentHexmapEditPitch;
		}
		else if (displayState == TuningHexSelect)
		{
			if (whichSwitch)
			{
				if (upSwitch())
				{
					currentMantaUITuning++;
					if (currentMantaUITuning >= 99)
					{
						currentMantaUITuning = 99;
					}
				}
			}
			else
			{
				if (downSwitch())
				{
					if (currentMantaUITuning <= 0)
					{
						currentMantaUITuning = 0;
					}
					else
					{
						currentMantaUITuning--;
					}
				}
			}
		
			mantaUITunings[currentTuningHex] = currentMantaUITuning;
		
			if (suspendRetrieve != DontRetrieve)
			{
				loadTuning(currentMantaUITuning);
			}
		
			Write7Seg(currentMantaUITuning);
			normal_7seg_number = currentMantaUITuning;
		}
		else if (preference_num == PRESET_SELECT)
		{	
			//if we are not in Save mode, then we are trying to instantaneously load a preset
			if (!savingActive)
			{
				if (whichSwitch)
				{
					if (upSwitch())
					{
						preset_num++;
					
						if (preset_num > 99)
						{
							preset_num = 99;
						}
						if (suspendRetrieve !=DontRetrieve)
						{
							initiateStoringStartupStateToExternalMemory();
							updatePreset();
						}
						Write7Seg(preset_num);
						normal_7seg_number = preset_num;
					}

				}
				else 
				{
					if (downSwitch())
					{
						if (preset_num <= 0)
						{
							preset_num = 0;
						}
						else
						{
							preset_num--;
						}
						if (suspendRetrieve != DontRetrieve)
						{
							initiateStoringStartupStateToExternalMemory();
							updatePreset();
						}
						Write7Seg(preset_num);
						normal_7seg_number = preset_num;
					}
				}
				if (suspendRetrieve == RetrieveNow) //this state happens if you have had a button held down and it's in "scan" mode, and then it has been released
				{
					initiateStoringStartupStateToExternalMemory();
					updatePreset();
					suspendRetrieve = RetrieveWhenever;
				}
			}
		
			//otherwise we are currently navigating to save a preset into a slot
			else
			{
				if (whichSwitch)
				{
					if (upSwitch())
					{
						preset_to_save_num++;
						if (preset_to_save_num > 99)
						{
							preset_to_save_num = 99;
						}
					}
				}
				else
				{
					if (downSwitch())
					{
						if (preset_to_save_num <= 10)
						{
							preset_to_save_num = 10;
						}
						else
						{
							preset_to_save_num--;
						}
					}
				}
				//should make it blink for extra clarity
				Write7Seg(preset_to_save_num);
				normal_7seg_number = preset_to_save_num;
			}
		}
	
		else if (preference_num == TUNING_AND_LEARN)
		{
			if (tuningOrLearn == TUNING_SELECT)
			{
				if (!tuningLoading)
				{
					if (whichSwitch)
					{
						if (upSwitch())
						{
							globalTuning++;
							if (globalTuning >= 99)
							{
								globalTuning = 99;
							}
							if (suspendRetrieve != DontRetrieve)
							{
								loadTuning(globalTuning);
							}
							Write7Seg(globalTuning);
							normal_7seg_number = globalTuning;
						}
					
					}
					else
					{
						if (downSwitch())
						{
							if (globalTuning <= 0)
							{
								globalTuning = 0;
							}
							else
							{
								globalTuning--;
							}
							if (suspendRetrieve != DontRetrieve)
							{
								loadTuning(globalTuning);
							}
							Write7Seg(globalTuning);
							normal_7seg_number = globalTuning;
						}
					}
					if (suspendRetrieve == RetrieveNow)
					{
						loadTuning(globalTuning);
						suspendRetrieve = RetrieveWhenever;
					}
				}
			}
			else //otherwise it's MIDI LEARN mode or PATTERN LENGTH mode.  Ignore buttons in MIDI LEARN, in no device mode change length
			{
				if (type_of_device_connected == NoDeviceConnected)
				{
					if (whichSwitch)
					{
						if (upSwitch())
						{
							if (noDevicePatterns.patternLength >= 32)
							{
								noDevicePatterns.patternLength = 32;
							}
							else
							{
								noDevicePatterns.patternLength++;
							}
							Write7Seg(noDevicePatterns.patternLength);
							normal_7seg_number = noDevicePatterns.patternLength;
						}
					}
					else
					{
						if (downSwitch())
						{
							if (noDevicePatterns.patternLength <= 1)
							{
								noDevicePatterns.patternLength = 1;
							}
							else
							{
								noDevicePatterns.patternLength--;
							}
							Write7Seg(noDevicePatterns.patternLength);
							normal_7seg_number = noDevicePatterns.patternLength;
						}
					}
				}
			}
		
		}
	
		else if (preference_num == GLOBAL_GLIDE_PREFERENCES)
		{
			if (whichSwitch)
			{
				if (upSwitch())
				{
					if (glide_pref == GLOBAL_PITCH_GLIDE)
					{
						globalPitchGlideDisplay++;
						if (globalPitchGlideDisplay >= globalGlideMax)
						{
							globalPitchGlideDisplay = globalGlideMax;
						}
						Write7Seg(globalPitchGlideDisplay);
						normal_7seg_number = globalPitchGlideDisplay;
						globalPitchGlide = glide_lookup[globalPitchGlideDisplay];
					}
					else  // otherwise it's global cv glide
					{
						globalCVGlideDisplay++;
						if (globalCVGlideDisplay >= globalGlideMax)
						{
							globalCVGlideDisplay = globalGlideMax;
						}
						if (globalCVGlideDisplay < 10)
						{
							Write7Seg(globalCVGlideDisplay+200);
							normal_7seg_number = (globalCVGlideDisplay + 200);
						}
						else
						{
							Write7Seg(globalCVGlideDisplay);
							normal_7seg_number = globalCVGlideDisplay;
						}
						globalCVGlide = glide_lookup[globalCVGlideDisplay];
					}

				}
			}
			else
			{
				if (downSwitch())
				{
					if (glide_pref == GLOBAL_PITCH_GLIDE)
					{
						if (globalPitchGlideDisplay <= 0)
						{
							globalPitchGlideDisplay = 0;
						}
						else
						{
							globalPitchGlideDisplay--;
						}
						Write7Seg(globalPitchGlideDisplay);
						normal_7seg_number = globalPitchGlideDisplay;
						globalPitchGlide = glide_lookup[globalPitchGlideDisplay];
					}
					else  // otherwise it's global cv glide
					{
						if (globalCVGlideDisplay <= 0)
						{
							globalCVGlideDisplay = 0;
						}
						else
						{
							globalCVGlideDisplay--;
						}
						if (globalCVGlideDisplay < 10)
						{
							Write7Seg(globalCVGlideDisplay+200);
							normal_7seg_number = (globalCVGlideDisplay + 200);
						}
						else
						{
							Write7Seg(globalCVGlideDisplay);
							normal_7seg_number = globalCVGlideDisplay;
						}
						globalCVGlide = glide_lookup[globalCVGlideDisplay];
					}

				}
			}

		}
	
		else if (preference_num == INTERNAL_CLOCK)
		{

			if (clock_pref == BPM)
			{
				if (whichSwitch)
				{
					if (upSwitch())
					{
						clock_speed_displayed++;
						if (clock_speed_displayed >= clock_speed_max)
						{
							clock_speed_displayed = clock_speed_max;
						}
					}
				}
				else
				{
					if (downSwitch())
					{
						if (clock_speed_displayed <= 0)
						{
							clock_speed_displayed = 0;
						}
						else
						{
							clock_speed_displayed--;
						}
					}
				}
				Write7Seg(clock_speed_displayed);
				normal_7seg_number = clock_speed_displayed;
			}
			else if (clock_pref == CLOCK_DIVIDER)
			{
				if (whichSwitch)
				{
					if (upSwitch())
					{
						tempoDivider++;
						if (tempoDivider >= tempoDividerMax)
						{
							tempoDivider = tempoDividerMax;
						}
					}
				}
				else
				{
					if (downSwitch())
					{
						if (tempoDivider <= 0)
						{
							tempoDivider = 0;
						}
						else
						{
							tempoDivider--;
						}
					}
				}
				Write7Seg(200 + tempoDivider); // writing values from 200-209 leaves the first digit blank, which helps visually distinguish this mode
				normal_7seg_number = 200 + tempoDivider;
			}
			if (clock_speed_displayed > 0)
			{
				clock_speed = (960000 / (clock_speed_displayed + 61)) / (1 << tempoDivider); //seems like this should be + 60, but for some reason it's off by one if so (based on measuring the timing)
			}
			else
			{
				clock_speed = 0;
			}

		}
	}
}

void Preferences_Switch_Check(void)
{
	if (preferencesSwitch())
	{
		
		tuningOrLearn = TUNING_SELECT; //leave MIDI learn mode if the preferences buttons is pressed
		LED_Off(PRESET_SAVE_LED); //turn this off, in case we were editing the nodevice length
		//if we aren't in a current save state, act normal and change preferences
		if (!savingActive)
		{
			preference_num++;
			if (preference_num >= num_preferences)
			{
				preference_num = 0;
			}
			updatePreferences();
		}
		//otherwise the P button acts as a "cancel" button for the save preset state
		else
		{
			//turn off save mode but don't store anything
			savingActive = 0;
			LED_Off(PRESET_SAVE_LED);
		}

	}
	
	else
	{
		if (SevSegArpMode == 1)
		{
			SevSegArpMode = 0;
			Write7Seg(normal_7seg_number);
		}
	}
}

void Save_Switch_Check(void)
{
	
	if (saveSwitch())
	{
		if (preference_num == PRESET_SELECT) //we're in normal preset mode, which allows saving
		{
				savingActive = !savingActive;
				updateSave();
		}
		
		else if (preference_num == INTERNAL_CLOCK)
		{
			if (clock_pref == CLOCK_DIVIDER)
			{
				//switch to BPM pref
				clock_pref = BPM;
				Write7Seg(clock_speed_displayed);
				normal_7seg_number = clock_speed_displayed;
			}
			else if (clock_pref == BPM)
			{
				//switch to Clock Divider pref
				clock_pref = CLOCK_DIVIDER;
				Write7Seg(200 + tempoDivider); // writing values from 200-209 leaves the first digit blank, which helps distiguish this mode
				normal_7seg_number = 200 + tempoDivider;
			}
		}
		
		
		else if (preference_num == GLOBAL_GLIDE_PREFERENCES)
		{
			if (glide_pref == GLOBAL_PITCH_GLIDE)
			{
				//switch to BPM pref
				glide_pref = GLOBAL_CV_GLIDE;
				if (globalCVGlide < 10)
				{
					Write7Seg(globalCVGlide+200);
				}
				else
				{
					Write7Seg(globalCVGlide);
				}
				normal_7seg_number = globalCVGlide;
			}
			else if (glide_pref == GLOBAL_CV_GLIDE)
			{
				//switch to Clock Divider pref
				glide_pref = GLOBAL_PITCH_GLIDE;
				Write7Seg(globalPitchGlide); // writing values from 200-209 leaves the first digit blank, which helps distiguish this mode
				normal_7seg_number = globalPitchGlide;
			}
		}
		
		else if (preference_num == TUNING_AND_LEARN)
		{
			if (tuningOrLearn == TUNING_SELECT)
			{
				if ((type_of_device_connected == MIDIComputerConnected) || (type_of_device_connected == MIDIKeyboardConnected))
				{
					//switch to midilearn
					tuningOrLearn = MIDILEARN_AND_LENGTH; //midilearn in this case
					currentNumberToMIDILearn = 0; //reset the MIDI learn counter number whenever you enter MIDI learn mode
					Write7Seg(currentNumberToMIDILearn+200);
					normal_7seg_number = currentNumberToMIDILearn+200;
				}
				else if (type_of_device_connected == NoDeviceConnected)
				{
					tuningOrLearn = MIDILEARN_AND_LENGTH; //length in this case
					Write7Seg(noDevicePatterns.patternLength);
					normal_7seg_number = noDevicePatterns.patternLength;
					LED_On(PRESET_SAVE_LED); //to give some indication
				}
			}
			else if (tuningOrLearn == MIDILEARN_AND_LENGTH)
			{
				//switch to Clock Divider pref
				tuningOrLearn = TUNING_SELECT;
				Write7Seg(globalTuning); // writing values from 200-209 leaves the first digit blank, which helps distinguish this mode
				normal_7seg_number = globalTuning;
				LED_Off(PRESET_SAVE_LED); //in case it was turned on due to a nodevice length adjustment
			}
		}
	}

}


void updatePreset(void)
{	
	if (type_of_device_connected == MantaConnected)
	{
		loadMantaPreset();
	}
	else if (type_of_device_connected == JoystickConnected)
	{
		loadJoystickPreset();
	}
	else if ((type_of_device_connected == MIDIComputerConnected) || (type_of_device_connected == MIDIKeyboardConnected))
	{
		loadMIDIPreset();
	}
	else if (type_of_device_connected == NoDeviceConnected)
	{
		loadNoDevicePreset();
	}
}



void updatePreferences(void)
{
	switch(preference_num)
	{
		case PRESET_SELECT:
		LED_Off(LEFT_POINT_LED);
		LED_Off(RIGHT_POINT_LED);
		LED_Off(PREFERENCES_LED);
		Write7Seg(preset_num);
		normal_7seg_number = preset_num;
		break;
		
		case TUNING_AND_LEARN:
		LED_Off(LEFT_POINT_LED);
		LED_On(RIGHT_POINT_LED);
		LED_On(PREFERENCES_LED);
		Write7Seg(globalTuning);
		normal_7seg_number = globalTuning;
		break;
		
		case GLOBAL_GLIDE_PREFERENCES:
		LED_On(LEFT_POINT_LED);
		LED_Off(RIGHT_POINT_LED);
		LED_On(PREFERENCES_LED);
		Write7Seg(globalPitchGlide);
		normal_7seg_number = globalPitchGlide;
		break;
		
		case INTERNAL_CLOCK:
		LED_On(LEFT_POINT_LED);
		LED_On(RIGHT_POINT_LED);
		LED_On(PREFERENCES_LED);
		Write7Seg(clock_speed_displayed);
		normal_7seg_number = clock_speed_displayed;
		break;
		
		default:
		break;
	}
}

void updateSave(void)
{
	if (savingActive)
	{
		LED_On(PRESET_SAVE_LED);
		//jump to the first available user preset slot if you are on the default factory presets
		if (preset_to_save_num <= 10)
		{
			preset_to_save_num = 10;
			Write7Seg(preset_to_save_num);
			normal_7seg_number = preset_to_save_num;
		}
	}
	else
	{
		if (type_of_device_connected == MantaConnected)
		{
			initiateStoringMantaPresetToExternalMemory();
		}
		else if ((type_of_device_connected == MIDIKeyboardConnected)||(type_of_device_connected == MIDIComputerConnected))
		{
			initiateStoringMidiPresetToExternalMemory();
		}
		else if (type_of_device_connected == NoDeviceConnected)
		{
			initiateStoringNoDevicePresetToExternalMemory();
		}		
		preset_num = preset_to_save_num;
		Write7Seg(preset_num);
		normal_7seg_number = preset_num;
	}
}

void clockHappened(void)
{
	if (type_of_device_connected == MantaConnected)
	{
		if (manta[InstrumentOne].type == SequencerInstrument) sequencerStep(InstrumentOne);
		if (manta[InstrumentTwo].type == SequencerInstrument) sequencerStep(InstrumentTwo);
		
		if (!takeover)
		{
			if (manta[InstrumentOne].type == KeyboardInstrument) keyboardStep(InstrumentOne);
			if (manta[InstrumentTwo].type == KeyboardInstrument) keyboardStep(InstrumentTwo);
		}
		else if (takeoverType == KeyboardInstrument)
		{
			keyboardStep(InstrumentNil);
		}
		
	}
	else if (type_of_device_connected == MIDIKeyboardConnected)
	{
		MIDIKeyboardStep();
	}
	else if (type_of_device_connected == NoDeviceConnected)
	{
		no_device_gate_in();
	}
	else if (type_of_device_connected == MIDIComputerConnected)
	{
		MIDIKeyboardStep();
		midi_ext_gate_in();
	}
}



void DACsetup(void)
{
	//let the portamento interrupt know the SPI is busy
	SPIbusy = 1;
	
	setDACSPI(spiOptions12DAC);
	spi_mode = TWELVEBIT;
	
	gpio_clr_gpio_pin(DAC2_CS);
	dacSetupwait1();
	spi_write(DAC_SPI,0x30);
	spi_write(DAC_SPI,0x00);
	spi_write(DAC_SPI,0x0F);
	dacSetupwait2();
	gpio_set_gpio_pin(DAC2_CS);
	dacSetupwait1();
	gpio_clr_gpio_pin(DAC3_CS);
	dacSetupwait1();
	spi_write(DAC_SPI,0x30);
	spi_write(DAC_SPI,0x00);
	spi_write(DAC_SPI,0x0F);
	dacSetupwait2();
	gpio_set_gpio_pin(DAC3_CS);
	dacSetupwait1();
}

void dacSetupwait1(void)
{
	cpu_delay_us(3,64000000);
}
void dacSetupwait2(void)
{
	cpu_delay_us(40,64000000);
}

void dacwait1(void)
{
	cpu_delay_us(9,64000000); //8 works but did 9 just to be safe. Interestingly, 5 works if optimizations are off, but breaks with optimization
}
void dacwait2(void)
{
	cpu_delay_us(1,64000000);
}

void memoryWait(void)
{
	cpu_delay_us(1,64000000); // what should this be? needs testing   was 20
}


void usb_msc_bl_start (void)
{
	Disable_global_interrupt();
	// Write at destination (AVR32_FLASHC_USER_PAGE + ISP_FORCE_OFFSET) the value
	// ISP_FORCE_VALUE. Size of ISP_FORCE_VALUE is 4 bytes.
	flashc_memset32(AVR32_FLASHC_USER_PAGE + ISP_FORCE_OFFSET, ISP_FORCE_VALUE, 4, TRUE);
	myWDT.us_timeout_period = 17777;
	wdt_enable(&myWDT);
	while (1);           // wait WDT time-out to reset and start the MSC bootloader
}


static void setDACSPI(spi_options_t spiOptions)
{
	SPIbusy = 1;
	spi_disable(DAC_SPI);
	spi_initMaster(DAC_SPI, &spiOptions);
	spi_selectionMode(DAC_SPI, 0, 0, 0);
	spi_selectChip(DAC_SPI, 0);
	spi_setupChipReg(DAC_SPI, &spiOptions, TARGET_PBACLK_FREQ_HZ);
	spi_enable(DAC_SPI);
}

static void setMemorySPI(spi_options_t spiOptions)
{
	spi_disable(MEMORY_SPI);
	spi_initMaster(MEMORY_SPI, &spiOptions);
	spi_selectionMode(MEMORY_SPI, 0, 0, 0);
	spi_selectChip(MEMORY_SPI, 0);
	spi_setupChipReg(MEMORY_SPI, &spiOptions, TARGET_PBACLK_FREQ_HZ);
	spi_enable(MEMORY_SPI);
}

void dacsend(unsigned char DACvoice, unsigned char DACnum, unsigned short DACval)
{
	//send a value to one of the DAC channels to be converted to analog voltage
	//DACnum is which type of output it goes to (0 = B, 1 = C)
	
	SPIbusy = 1;
	if (spi_mode != TWELVEBIT)
	{
		setDACSPI(spiOptions12DAC);
		spi_mode = TWELVEBIT;
	}

	dacouthigh = (DACval >> 4) & 0xFF;
	dacoutlow = ((DACval << 4) & 0xF0);

	if (DACnum == 0)
	{
		gpio_clr_gpio_pin(DAC2_CS);
		dacwait1();
		while((spi_write(DAC_SPI,DACvoice)) != 0);
		while((spi_write(DAC_SPI,dacouthigh)) !=0);
		while((spi_write(DAC_SPI,dacoutlow)) != 0);
		dacwait1();
		gpio_set_gpio_pin(DAC2_CS);
	}

	if (DACnum == 1)
	{
		gpio_clr_gpio_pin(DAC3_CS);
		dacwait1();
		while((spi_write(DAC_SPI,DACvoice)) != 0);
		while((spi_write(DAC_SPI,dacouthigh)) !=0);
		while((spi_write(DAC_SPI,dacoutlow)) != 0);
		dacwait1();
		gpio_set_gpio_pin(DAC3_CS);
	}
	SPIbusy = 0;
}

void DAC16Send(unsigned char DAC16voice, unsigned short DAC16val)
{
	unsigned char daccontrol = 0;

	SPIbusy = 1;
	//set up SPI to be 16 bit for the DAC
	if (spi_mode != SIXTEENBIT)
	{
		setDACSPI(spiOptions16DAC);
		spi_mode = SIXTEENBIT;
	}
	
	daccontrol = (16 | (DAC16voice << 1));
	DAC1outhigh = ((daccontrol << 8) + (DAC16val >> 8));
	DAC1outlow = ((DAC16val & 255) << 8);
	gpio_clr_gpio_pin(DAC1_CS);
	dacwait2();
	spi_write(DAC_SPI, DAC1outhigh);
	spi_write(DAC_SPI, DAC1outlow);
	dacwait2();
	gpio_set_gpio_pin(DAC1_CS);
	SPIbusy = 0;

}

static void initSPIbus(void)
{
	SPIbusy = 1;
	//prepare the pins the control the DAC and set them to default positions
	gpio_set_gpio_pin(MEMORY_CS);
	gpio_set_gpio_pin(DAC1_CS);
	gpio_set_gpio_pin(DAC2_CS);
	gpio_set_gpio_pin(DAC3_CS);
	//gpio_set_gpio_pin(DAC4_CS);
	gpio_clr_gpio_pin(REF1);
	//gpio_clr_gpio_pin(REF2);

	//allow pins to settle
	delay_ms(1);

	// Initialize as master
	setDACSPI(spiOptions12DAC);
	setMemorySPI(spiOptionsMemory);
	
	// Assign I/Os to SPI
	static const gpio_map_t MEMORY_SPI_GPIO_MAP =
	{
		{MEMORY_SPI_SCK_PIN,  MEMORY_SPI_SCK_FUNCTION },  // SPI Clock.
		{MEMORY_SPI_MISO_PIN, MEMORY_SPI_MISO_FUNCTION},  // MISO.
		{MEMORY_SPI_MOSI_PIN, MEMORY_SPI_MOSI_FUNCTION},  // MOSI.
		{MEMORY_SPI_NPCS_PIN, MEMORY_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};
	gpio_enable_module(MEMORY_SPI_GPIO_MAP,
	sizeof(MEMORY_SPI_GPIO_MAP) / sizeof(MEMORY_SPI_GPIO_MAP[0]));
	// Assign I/Os to SPI
	static const gpio_map_t DAC_SPI_GPIO_MAP =
	{
		{DAC_SPI_SCK_PIN,  DAC_SPI_SCK_FUNCTION },  // SPI Clock.
		{DAC_SPI_MISO_PIN, DAC_SPI_MISO_FUNCTION},  // MISO.
		{DAC_SPI_MOSI_PIN, DAC_SPI_MOSI_FUNCTION},  // MOSI.
		{DAC_SPI_NPCS_PIN, DAC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};
	gpio_enable_module(DAC_SPI_GPIO_MAP,
	sizeof(DAC_SPI_GPIO_MAP) / sizeof(DAC_SPI_GPIO_MAP[0]));
}

void setupEIC(void)
{
	// Enable edge-triggered interrupt.
	eic_options[0].eic_mode  = EIC_MODE_EDGE_TRIGGERED;
	// Interrupt will trigger on falling edge.
	eic_options[0].eic_edge  = EIC_EDGE_FALLING_EDGE;
	// Initialize in synchronous mode : interrupt is synchronized to the clock
	eic_options[0].eic_async  = EIC_SYNCH_MODE;
	eic_options[0].eic_filter = EIC_FILTER_ENABLED;
	// Set the interrupt line number.
	eic_options[0].eic_line   = EXT_INT_EXAMPLE_LINE1;
	gpio_enable_module_pin(EXT_INT_EXAMPLE_PIN_LINE1,EXT_INT_EXAMPLE_FUNCTION_LINE1);
	// Disable all interrupts.
	Disable_global_interrupt();
	INTC_init_interrupts();
	INTC_register_interrupt(&eic_int_handler1, EXT_INT_EXAMPLE_IRQ_LINE1, AVR32_INTC_INT0);
	eic_init(&AVR32_EIC, eic_options,EXT_INT_EXAMPLE_NB_LINES);
	eic_enable_line(&AVR32_EIC, eic_options[0].eic_line);
	eic_enable_interrupt_line(&AVR32_EIC, eic_options[0].eic_line);
	
	gpio_enable_pin_interrupt(GPIO_HOST_DEVICE_SWITCH , GPIO_PIN_CHANGE);	// PA11
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_HOST_DEVICE_SWITCH/8), AVR32_INTC_INT0);
	  
	gpio_enable_pin_interrupt(GPIO_PRESET_DOWN_SWITCH , GPIO_PIN_CHANGE);	// PB02
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_PRESET_DOWN_SWITCH/8), AVR32_INTC_INT0);
	
	gpio_enable_pin_interrupt(GPIO_PRESET_UP_SWITCH , GPIO_PIN_CHANGE);	// PA20
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_PRESET_UP_SWITCH/8), AVR32_INTC_INT0);
	
	gpio_enable_pin_interrupt(GPIO_SAVE_SWITCH , GPIO_PIN_CHANGE);	 //PX39
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_SAVE_SWITCH/8), AVR32_INTC_INT0);
	
	gpio_enable_pin_interrupt(GPIO_PREFERENCES_SWITCH , GPIO_PIN_CHANGE);	//PX02
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_PREFERENCES_SWITCH/8), AVR32_INTC_INT0);
	
	
	// Register the RTC interrupt handler to the interrupt controller.
	INTC_register_interrupt(&tc2_irq, TC2_IRQ, TC2_IRQ_PRIORITY);
	INTC_register_interrupt(&tc3_irq, TC3_IRQ, TC3_IRQ_PRIORITY);
	
	Enable_global_interrupt();
	
}

void main_suspend_action(void)
{
	//ui_powerdown();
}

void main_resume_action(void)
{
	//ui_wakeup();
}

void main_sof_action(void)
{
	USB_frame_action(udd_get_frame_number());
}

uint8_t upSwitch(void)
{
	return !(gpio_get_pin_value(GPIO_PRESET_UP_SWITCH));
}
uint8_t downSwitch(void)
{
	return !(gpio_get_pin_value(GPIO_PRESET_DOWN_SWITCH));
}
uint8_t saveSwitch(void)
{
	return !(gpio_get_pin_value(GPIO_SAVE_SWITCH));
}
uint8_t preferencesSwitch(void)
{
	return !(gpio_get_pin_value(GPIO_PREFERENCES_SWITCH));
}


void loadMantaPreset(void)
{
	if (preset_num == 0)
	{
		initMantaSequencer();
		initMantaLEDState();
	}
	else if ((preset_num >= 1) && (preset_num <= 4))
	{
		initMantaKeys(preset_num);
		initMantaLEDState();
	}
	else if (preset_num == 5)
	{
		initMantaAllCV();
		initMantaLEDState();
	}
	else if (preset_num == 6)
	{
		initMantaAllGates();
		initMantaLEDState();
	}
	else if (preset_num == 7)
	{
		initMantaAllTriggers();
		initMantaLEDState();
	}
	else if (preset_num == 8)
	{
		initMantaCVAndGates();
		initMantaLEDState();
	}
	else if (preset_num == 9)
	{
		initMantaCVAndTriggers();
		initMantaLEDState();
	}
	else
	{
		initiateLoadingMantaPresetFromExternalMemory();
	}
	//clearDACoutputs();
}

void initMantaLEDState(void)
{
	setKeyboardLEDs();
	setDirectLEDs();
	setSequencerLEDs();
	setCurrentInstrument(InstrumentOne);
	shiftOption1 = FALSE;
	shiftOption2 = FALSE;
	hexmapEditMode = FALSE;
	directEditMode = FALSE;
	
	displayState = GlobalDisplayStateNil;
}

void loadJoystickPreset(void)
{
	//joystick only has preset 0
	if (preset_num != 0)
	{
		preset_num = 0;
		Write7Seg(preset_num);
		normal_7seg_number = preset_num;
	}
	clearDACoutputs();
}

void loadMIDIPreset(void)
{
	if (preset_num == 0)
	{
		initMIDIArpeggiator();
	}
	else if ((preset_num >= 1) && (preset_num <= 4))
	{
		initMIDIKeys(preset_num, TRUE);
	}
	else if (preset_num == 5)
	{
		initMIDIAllCV();
	}
	else if (preset_num == 6)
	{
		initMIDIAllGates();
	}
	else if (preset_num == 7)
	{
		initMIDIAllTriggers();
	}
	else if (preset_num == 8)
	{
		initMIDICVAndGates();
	}
	else if (preset_num == 9)
	{
		initMIDICVAndTriggers();
	}
	else
	{
		initiateLoadingMidiPresetFromExternalMemory();
	}
	//clearDACoutputs();
}

void loadNoDevicePreset(void)
{
	//presets are handled internally by no_device_gate_in
	clearDACoutputs();
}

void clearDACoutputs(void)
{
	for (int i = 0; i < 12; i++)
	{
		sendDataToOutput(i,0,0);		
	}
}

void mantaPreset_encode(uint8_t* buffer)
{
	uint32_t indexCounter = 0;
	
	buffer[indexCounter++] = 1; // MantaMateVersion
	
	buffer[indexCounter++] = takeover;
	buffer[indexCounter++] = takeoverType;
	
	buffer[indexCounter++] = currentInstrument;
	buffer[indexCounter++] = manta[InstrumentOne].type;
	buffer[indexCounter++] = manta[InstrumentTwo].type;
	
	for (int inst = 0; inst < 2; inst++)
	{
		buffer[indexCounter++] = currentComp[inst];
		for (int i = 0; i < 16; i++)
		{
			buffer[indexCounter++] = compositionMap[inst][i];
		}
	}
	
	buffer[indexCounter++] = globalPitchGlide >> 8;
	buffer[indexCounter++] = globalPitchGlide & 0xff;
	
	buffer[indexCounter++] = globalCVGlide >> 8;
	buffer[indexCounter++] = globalCVGlide & 0xff;
	
	buffer[indexCounter++] = myGlobalTuningTable.cardinality;
	for (int i = 0; i < 128; i++)
	{
		buffer[indexCounter++] = (myGlobalTuningTable.tuningDACTable[i] >> 8);
		buffer[indexCounter++] = (myGlobalTuningTable.tuningDACTable[i] & 0xff);
	}
	
	buffer[indexCounter++] = globalTuning;
	buffer[indexCounter++] = tuningToUse; 
	buffer[indexCounter++] = currentTuningHex;
	buffer[indexCounter++] = currentMantaUITuning;
	for (int i = 0; i < 32; i++)
	{
		buffer[indexCounter++] = mantaUITunings[i];
	}
	
	tKeyboard_encode(&manta[InstrumentOne].keyboard, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_KEYBOARD;
	tKeyboard_encode(&manta[InstrumentTwo].keyboard, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_KEYBOARD;
	tKeyboard_encode(&fullKeyboard, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_KEYBOARD;
	
	tDirect_encode(&manta[InstrumentOne].direct, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_DIRECT;
	tDirect_encode(&manta[InstrumentTwo].direct, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_DIRECT;
	tDirect_encode(&fullDirect, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_DIRECT;
	
	tSequencer_encode(&manta[InstrumentOne].sequencer, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_SEQUENCER;
	tSequencer_encode(&manta[InstrumentTwo].sequencer, &buffer[indexCounter]);
	indexCounter += NUM_BYTES_PER_SEQUENCER;
}

void mantaPreset_decode(uint8_t* buffer)
{
	uint32_t indexCounter = 0;
	uint16_t lowByte, highByte;
	
	int version = buffer[indexCounter++];
	
	if (version == 1)
	{
		takeover = buffer[indexCounter++];
		takeoverType = buffer[indexCounter++];
		
		currentInstrument = buffer[indexCounter++];
		manta[InstrumentOne].type = buffer[indexCounter++];
		manta[InstrumentTwo].type = buffer[indexCounter++];
		
		for (int inst = 0; inst < 2; inst++)
		{
			currentComp[inst] = buffer[indexCounter++];
			for (int i = 0; i < 16; i++)
			{
				compositionMap[inst][i] = buffer[indexCounter++];
			}
		}
		
		highByte = (buffer[indexCounter++] << 8);
		lowByte = buffer[indexCounter++];
		globalPitchGlide = highByte + lowByte;
		
		highByte = (buffer[indexCounter++] << 8);
		lowByte = buffer[indexCounter++];
		globalCVGlide = highByte + lowByte;
		
		myGlobalTuningTable.cardinality = buffer[indexCounter++];
		for (int i = 0; i < 128; i++)
		{
			highByte = (buffer[indexCounter++] << 8);
			lowByte = (buffer[indexCounter++] & 0xff);
			myGlobalTuningTable.tuningDACTable[i] = (highByte + lowByte);
		}
		
		globalTuning = buffer[indexCounter++];
		tuningToUse = buffer[indexCounter++];
		currentTuningHex = buffer[indexCounter++];
		currentMantaUITuning = buffer[indexCounter++];
		for (int i = 0; i < 32; i++)
		{
			mantaUITunings[i] = buffer[indexCounter++];
		}
		
		tKeyboard_decode(&manta[InstrumentOne].keyboard, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_KEYBOARD;
		tKeyboard_decode(&manta[InstrumentTwo].keyboard, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_KEYBOARD;
		tKeyboard_decode(&fullKeyboard, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_KEYBOARD;
		
		tDirect_decode(&manta[InstrumentOne].direct, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_DIRECT;
		tDirect_decode(&manta[InstrumentTwo].direct, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_DIRECT;
		tDirect_decode(&fullDirect, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_DIRECT;
		tSequencer_decode(&manta[InstrumentOne].sequencer, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_SEQUENCER;
		tSequencer_decode(&manta[InstrumentTwo].sequencer, &buffer[indexCounter]);
		indexCounter += NUM_BYTES_PER_SEQUENCER;
	}
}


void midiPreset_encode(uint8_t* buffer)
{
	uint32_t indexCounter = 0;
	
	buffer[indexCounter++] = 1; // MantaMateVersion
	
	buffer[indexCounter++] = globalPitchGlide >> 8;
	buffer[indexCounter++] = globalPitchGlide & 0xff;
	
	buffer[indexCounter++] = globalCVGlide >> 8;
	buffer[indexCounter++] = globalCVGlide & 0xff;
	
	buffer[indexCounter++] = myGlobalTuningTable.cardinality;
	for (int i = 0; i < 128; i++)
	{
		buffer[indexCounter++] = (myGlobalTuningTable.tuningDACTable[i] >> 8);
		buffer[indexCounter++] = (myGlobalTuningTable.tuningDACTable[i] & 0xff);
	}
	buffer[indexCounter++] = globalTuning;
	
	tMIDIKeyboard_encode(&MIDIKeyboard, &buffer[indexCounter]);
}


void midiPreset_decode(uint8_t* buffer)
{
	uint32_t indexCounter = 0;
	uint16_t lowByte, highByte;
	
	int version = buffer[indexCounter++];
	
	if (version == 1)
	{
		highByte = (buffer[indexCounter++] << 8);
		lowByte = buffer[indexCounter++];
		globalPitchGlide = highByte + lowByte;
		
		highByte = (buffer[indexCounter++] << 8);
		lowByte = buffer[indexCounter++];
		globalCVGlide = highByte + lowByte;
		
		myGlobalTuningTable.cardinality = buffer[indexCounter++];
		for (int i = 0; i < 128; i++)
		{
			highByte = (buffer[indexCounter++] << 8);
			lowByte = (buffer[indexCounter++] & 0xff);
			myGlobalTuningTable.tuningDACTable[i] = (highByte + lowByte);
		}
		globalTuning = buffer[indexCounter++];
		
		tMIDIKeyboard_decode(&MIDIKeyboard, &buffer[indexCounter]);
	}

}


void noDevicePreset_encode(uint8_t* buffer)
{
	uint32_t indexCounter = 0;
	
	buffer[indexCounter++] = 1; // MantaMateVersion
	
	buffer[indexCounter++] = globalCVGlide >> 8;
	buffer[indexCounter++] = globalCVGlide & 0xff;
	
	tNoDevice_encode(&noDevicePatterns, &buffer[indexCounter]);
}


void noDevicePreset_decode(uint8_t* buffer)
{
	uint32_t indexCounter = 0;
	uint16_t lowByte, highByte;
	
	int version = buffer[indexCounter++];
	
	if (version == 1)
	{
		highByte = (buffer[indexCounter++] << 8);
		lowByte = buffer[indexCounter++];
		globalCVGlide = highByte + lowByte;
		
		tNoDevice_decode(&noDevicePatterns, &buffer[indexCounter]);	
	}

}