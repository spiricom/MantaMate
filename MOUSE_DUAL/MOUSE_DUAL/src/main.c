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
unsigned char globalGlide = 1;
unsigned char globalGlideMax = 198;
unsigned char suspendRetrieve = 0;
unsigned char number_for_7Seg = 0;
unsigned char blank7Seg = 0;
unsigned char tuningLoading = 0;
unsigned char transpose_indication_active = 0;
unsigned char normal_7seg_number = 0;


GlobalPreferences preference_num = PRESET_SELECT;
GlobalPreferences num_preferences = PREFERENCES_COUNT;
TuningOrLearnType tuningOrLearn = TUNING_SELECT;
ClockPreferences clock_pref = BPM;
ConnectedDeviceType type_of_device_connected = NoDeviceConnected;


uint8_t freeze_LED_update = 0;

uint8_t currentNumberToMIDILearn = 0;
uint32_t dummycounter = 0;
uint8_t manta_mapper = 0;
uint8_t tuning_count = 0;
uint8_t manta_data_lock = 0; // probably not necessary, added this when I was worried about read and write happening simultaneously, but it wasn't the case.
uint8_t spi_mode = 0;
uint8_t new_manta_attached = false;

uint32_t clock_speed = 0; // this is the speed of the internal sequencer clock 
uint32_t clock_speed_max = 99; 
uint32_t clock_speed_displayed = 0;
uint32_t tempoDivider = 3;
uint32_t tempoDividerMax = 9;




uint32_t USB_frame_counter = 0; // used by the internal sequencer clock to count USB frames (which are the source of the internal sequencer metronome)

uint8_t joystick_mode = 0; 

uint32_t myUSBMode = UNCONFIGUREDMODE;

// here's some interrupt stuff I added to try to get the external gate working
#define EXT_INT_EXAMPLE_PIN_LINE1               AVR32_EIC_EXTINT_7_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE1          AVR32_EIC_EXTINT_7_FUNCTION
#define EXT_INT_EXAMPLE_LINE1                   EXT_INT7 // this should be right JS
#define EXT_INT_EXAMPLE_IRQ_LINE1               AVR32_EIC_IRQ_7 // not sure what this should be
#define EXT_INT_EXAMPLE_NB_LINES				1

#define  PATTERN_TEST_LENGTH        (sizeof(test_pattern)/sizeof(U8))

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


// Outputs 0 through 11 (counting left to right top down), expects 12 bit.
// I abstracted this one step away from the DAC to put ramps in there -JS
//now you add a final argument to tell it how many bits -- assuming 16 or 12.
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

int testVal = 0;
int loopCounter = 0;

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

	ui_init();

	//initialize the SPI bus for DAC
	initSPIbus();
	
	//send the messages to the DACs to make them update without software LDAC feature
	DACsetup();
	
	loadTuning(globalTuning);
	displayState = GlobalDisplayStateNil;
	currentTuningHex = -1;
	currentHexmapEditHex = -1;
	// figure out if we're supposed to be in host mode or device mode for the USB
	USB_Mode_Switch_Check();
	
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			tIRampInit(&out[i][j], 2000, 0);
			tIRampSetDest(&out[i][j], 0);
		}
	}
	
	takeover = FALSE;
	hexmapEditMode = FALSE;
	
	//start off on preset 0;
	preset_num = 0;
	delay_ms(600);
	initTimers();
	updatePreset();
	Write7Seg(0);
	//I added this global variable - it's to allow minor functions to take over the 7-seg, then go back to whatever it is normally showing.
	normal_7seg_number = 0;
	// The USB management is entirely managed by interrupts.
	// As a consequence, the user application only has to play with the power modes.
	
	while (true) {	

		//currently putting low priority things like this in the main loop, as it should be the most background processes
		if (savePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringPresetToExternalMemory();
			}
		}
		else if (tuningSavePending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueStoringTuningToExternalMemory();
			}
		}

		else if (loadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingPresetFromExternalMemory();
			}
		}
		
		else if (tuningLoadPending)
		{
			if (!memorySPICheckIfBusy()) //if the memory is not busy - ready for new data or a new write routine
			{
				continueLoadingTuningFromExternalMemory();
			}
		}

		if (new_manta_attached)
		{
			manta_LED_set_mode(HOST_CONTROL_FULL);
			updatePreset();		//this will make it reset if the manta is unplugged and plugged back in. Might not be the desired behavior in case of accidental unplug, but will be cleaner if unplugged on purpose.
			new_manta_attached = false;
		}
		
		sleepmgr_enter_sleep();
		
	}
}

uint32_t upHeld = 0;
uint32_t downHeld = 0;
uint32_t holdTimeThresh = 8;
static uint32_t clockFrameCounter = 0;
static uint32_t buttonFrameCounter = 0;
static uint32_t buttonHoldSpeed = 120;
static uint32_t blink7SegCounter = 0;
static uint32_t blinkSpeed7Seg = 500;



// SequencerTwo timer interrupt.
__attribute__((__interrupt__))
static void tc2_irq(void)
{
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(TC2, TC2_CHANNEL);
	
	//TC2 is now also the internal metronome clock, the up/down button held sensing, and the 7seg blinker
	// as well as the timer for turning off triggers on the outputs
	
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
				suspendRetrieve = 1; //make it so it doesn't actually load the presets it's scrolling through until you release the button
				Preset_Switch_Check(1);
			}
			buttonFrameCounter = 0;
		}
	}
	else
	{
		if (upHeld > 0)
		{
			suspendRetrieve = 0;
			Preset_Switch_Check(1);
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
				suspendRetrieve = 1; //make it so it doesn't actually load the presets it's scrolling through until you release the button
				Preset_Switch_Check(0);
			}
			buttonFrameCounter = 0;
		}

	}
	else
	{
		if (downHeld > 0)
		{
			suspendRetrieve = 0;
			Preset_Switch_Check(0);
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
	
	if (type_of_device_connected == JoystickConnected)
	{
		return;
	}
	
	else if (type_of_device_connected == NoDeviceConnected)
	{
		for (int i = 0; i < 12; i++)
		{
			if (noDeviceTrigCount[i] > 0)
			{
				if (--(noDeviceTrigCount[i]) == 0)
				{
					sendDataToOutput(i, 0, 0x0);
				}
			}
		}
	}
	else if ((type_of_device_connected == MIDIKeyboardConnected) || (type_of_device_connected == MIDIComputerConnected))
	{
		tMIDIKeyboard* keyboard =  &MIDIKeyboard;

		if (keyboard->trigCount > 0)
		{
			if (--(keyboard->trigCount) == 0)
			{
				sendDataToOutput(3, 0, 0x0);
			}
		}
	}
	else //otherwise it's a Manta
	{
		if (!takeover) // Dual instrument, not takeover
		{
			for (int inst = 0; inst < 2; inst++)
			{
				tMantaInstrument* instrument = &manta[inst];
			
				if (instrument->type == DirectInstrument) // DirectInstrument
				{
					for (int i = 0; i < instrument->direct.numOuts; i++)
					{
						DirectType type = instrument->direct.outs[i].type;
						if (type == DirectTrigger)
						{
							if (instrument->direct.outs[i].trigCount > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
							{
								if (--(instrument->direct.outs[i].trigCount) == 0)
								{
									sendDataToOutput(6*inst+i, 0, 0x0);
								}
							}
						}
						else if (type == DirectCV)
						{
							sendDataToOutput(6*inst+i, 10, butt_states[instrument->direct.outs[i].hex] << 8);
						}
					}
				}
				else if (instrument->type == KeyboardInstrument) // DirectInstrument
				{
					if (instrument->keyboard.numVoices == 1)
					{
						if (instrument->keyboard.trigCount > 0) //added to avoid rollover issues if the counter keeps decrementing past 0 -JS
						{
							if (--(instrument->keyboard.trigCount) == 0)
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
		else if (takeoverType == DirectInstrument)
		{
			for (int i = 0; i < fullDirect.numOuts; i++)
			{
				DirectType type = fullDirect.outs[i].type;
				if (type == DirectTrigger)
				{
					if (fullDirect.outs[i].trigCount > 0)
					{
						if (--(fullDirect.outs[i].trigCount) == 0)
						{
							sendDataToOutput(i, 0, 0x0);
						}
					}
				}
				else if (type == DirectCV)
				{
					sendDataToOutput(i, 0, butt_states[fullDirect.outs[i].hex] << 8);
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

	if (type_of_device_connected == MantaConnected)
	{
		if (!takeover) // Dual instrument, not takeover
		{
			for (int inst = 0; inst < 2; inst++)
			{
				tMantaInstrument* instrument = &manta[inst];
	
				if (instrument->type == KeyboardInstrument) // KeyboardInstrument
				{
					for (int i = 0; i < instrument->keyboard.numVoices; i++)
					{
						tIRampSetTime (&out[inst][i*3 + CV1], 10);
						tIRampSetDest    (&out[inst][i*3 + CV1],  butt_states[instrument->keyboard.voices[i]] << 4); 
					}
				}
		
			}
		}
		else if (takeoverType == KeyboardInstrument) // Takeover mode
		{
			for (int i = 0; i < fullKeyboard.numVoices; i++)
			{
				int inst = (i / 2);
				tIRampSetTime (&out[inst][((i*3) % 6) + CV1], 10);
				tIRampSetDest    (&out[inst][((i*3) % 6) + CV1],  butt_states[fullKeyboard.voices[i]] << 4);
			}
		}
	}
	
	else if ((type_of_device_connected == MIDIComputerConnected) || (type_of_device_connected == MIDIKeyboardConnected))
	{
		for (int n = 0; n < 12; n++)
		{
			int cc = MIDIKeyboard.learnedCCsAndNotes[n][0];
			int note = MIDIKeyboard.learnedCCsAndNotes[n][1];
			
			if (cc >= 0)
			{
				sendDataToOutput(n + MIDIKeyboard.firstFreeOutput, 14, MIDIKeyboard.CCs[cc]); //TODO: ramp messes up the ability to really slowly approach tiny increments - should improve that possibility in the ramp code -JS
			}
			else if (note >= 0)
			{
				if (MIDIKeyboard.notes[note][0] > 0)
				{
					sendDataToOutput(n + MIDIKeyboard.firstFreeOutput, 0, 65535);
				}
				else
				{
					sendDataToOutput(n + MIDIKeyboard.firstFreeOutput, 0, 0);
				}
			}
			
		}
	}
	
	DAC16Send	(0, tIRampTick(&out[0][0]));
	dacsend     (0, 0,  tIRampTick(&out[0][1])); 
	dacsend     (0, 1,  tIRampTick(&out[0][2]));  
	DAC16Send	(1, tIRampTick(&out[0][3]));
	dacsend     (1, 0,  tIRampTick(&out[0][4]));
	dacsend     (1, 1,  tIRampTick(&out[0][5]));
	DAC16Send	(2, tIRampTick(&out[1][0]));
	dacsend     (2, 0,  tIRampTick(&out[1][1]));
	dacsend     (2, 1,  tIRampTick(&out[1][2]));
	DAC16Send	(3, tIRampTick(&out[1][3]));
	dacsend     (3, 0,  tIRampTick(&out[1][4]));
	dacsend     (3, 1,  tIRampTick(&out[1][5]));
	
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
	clockHappened();
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

			//LED_On(LED1);
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
			//LED_Off(LED1);
			if (myUSBMode == DEVICEMODE)
			{
				udc_stop();
			}
			//UHC_MODE_CHANGE(true);
			uhc_start();
			myUSBMode = HOSTMODE;
		}	
		updatePreset();
}

void Preset_Switch_Check(uint8_t whichSwitch)
{
	if (displayState == UpDownSwitchBlock) return;
	
	if (displayState == HexmapPitchSelect)
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
		
		if (!suspendRetrieve)
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
				}
			}
			updatePreset();
			Write7Seg(preset_num);
			normal_7seg_number = preset_num;
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
					}
				}
				if (!suspendRetrieve)
				{
					loadTuning(globalTuning);
				}
				Write7Seg(globalTuning);
				normal_7seg_number = globalTuning;
			}
		}
		else //otherwise it's MIDI LEARN mode
		{
			
		}
		
	}
	
	else if (preference_num == PORTAMENTO_TIME)
	{

		if (whichSwitch)
		{
			if (upSwitch())
			{
				globalGlide = globalGlide + 2;
				if (globalGlide >= globalGlideMax)
				{
					globalGlide = globalGlideMax;
				}
			}
		}
		else
		{
			if (downSwitch())
			{
				if (globalGlide <= 0)
				{
					globalGlide = 0;
				}
				else
				{
					globalGlide = globalGlide - 2;
				}
			}
		}
		Write7Seg(globalGlide / 2);
		normal_7seg_number = globalGlide / 2;
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
			clock_speed = (960000 / (clock_speed_displayed + 61)) / (1 << tempoDivider); //seems like this should be + 60, but for some reason it's off by one if so.
		}
		else
		{
			clock_speed = 0;
		}

	}
}

void Preferences_Switch_Check(void)
{
	if (preferencesSwitch())
	{
		
		tuningOrLearn = TUNING_SELECT; //leave MIDI learn mode if the preferences buttons is pressed
		
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
		
		else if (preference_num == TUNING_AND_LEARN)
		{
			if (tuningOrLearn == TUNING_SELECT)
			{
				//switch to midilearn 
				tuningOrLearn = MIDILEARN;
				currentNumberToMIDILearn = 0; //reset the MIDI learn counter number whenever you enter MIDI learn mode
				Write7Seg(currentNumberToMIDILearn+200);
				normal_7seg_number = currentNumberToMIDILearn+200;
			}
			else if (tuningOrLearn == MIDILEARN)
			{
				//switch to Clock Divider pref
				tuningOrLearn = TUNING_SELECT;
				Write7Seg(globalTuning); // writing values from 200-209 leaves the first digit blank, which helps distiguish this mode
				normal_7seg_number = globalTuning;
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
	else if (type_of_device_connected == MIDIComputerConnected)
	{
		loadMIDIComputerPreset();
	}
	else if (type_of_device_connected == MIDIKeyboardConnected)
	{
		loadMIDIKeyboardPreset();
	}
	else if (type_of_device_connected == NoDeviceConnected)
	{
		loadNoDevicePreset();
	}
	
	if (preset_num >= 10)
	{
		if (!suspendRetrieve)
		{
			initiateLoadingPresetFromExternalMemory();
		}

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
		
		case PORTAMENTO_TIME:
		LED_On(LEFT_POINT_LED);
		LED_Off(RIGHT_POINT_LED);
		LED_On(PREFERENCES_LED);
		Write7Seg(globalGlide);
		normal_7seg_number = globalGlide;
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
		initiateStoringPresetToExternalMemory();
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
	}
	
	if (type_of_device_connected == NoDeviceConnected)
	{
		no_device_gate_in();
	}
	
	if (type_of_device_connected == MIDIComputerConnected)
	{
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
	dacwait1();
	dacwait1();
	dacwait1();
	spi_write(DAC_SPI,0x30);
	spi_write(DAC_SPI,0x00);
	spi_write(DAC_SPI,0x0F);
	dacwait2();
	dacwait2();
	dacwait2();
	dacwait2();
	gpio_set_gpio_pin(DAC2_CS);
	dacwait1();
	dacwait1();
	dacwait1();

	gpio_clr_gpio_pin(DAC3_CS);
	dacwait1();
	dacwait1();
	dacwait1();
	spi_write(DAC_SPI,0x30);
	spi_write(DAC_SPI,0x00);
	spi_write(DAC_SPI,0x0F);
	dacwait2();
	dacwait2();
	dacwait2();
	dacwait2();
	gpio_set_gpio_pin(DAC3_CS);
	dacwait1();
	dacwait1();
	dacwait1();
}

void dacwait1(void)
{
	static uint8_t i = 0;
	static uint8_t wastecounter = 0;
	//cpu_delay_us(12,64000000);//5
	for (i = 0; i < 9; i++) //number arrived at by testing when the timing makes the DAC fail
	{
		wastecounter++;
	}
}
void dacwait2(void)
{
	//cpu_delay_us(12,64000000);//5
	static uint8_t i = 0;
	static uint8_t wastecounter = 0;
	//cpu_delay_us(12,64000000);//5
	for (i = 0; i < 2; i++)
	{
		wastecounter++;
	}
}

void memoryWait(void)
{
	//cpu_delay_us(12,64000000);//5
	static uint8_t i = 0;
	static uint8_t wastecounter = 0;
	//cpu_delay_us(12,64000000);//5
	for (i = 0; i < 20; i++)
	{
		wastecounter++;
	}
}

void enterBootloader(void)
{
	//Reset into Bootloader 
	flashc_erase_gp_fuse_bit(31, true);
	flashc_write_gp_fuse_bit(31, true);

	cpu_irq_disable();
	wdt_opt_t opt = {
		.us_timeout_period = 1000000
	};
	wdt_enable(&opt);
	while(1);
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
		dacwait1(); // necessary wait?
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
		dacwait1(); // necessary wait?
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
	}
	if ((preset_num >= 1) && (preset_num <= 4))
	{
		initMantaKeys(preset_num);
	}
	setCurrentInstrument(InstrumentOne);
}

void loadJoystickPreset(void)
{
	takeover = TRUE;
	//joystick only has preset 0
	if (preset_num != 0)
	{
		preset_num = 0;
		Write7Seg(preset_num);
		normal_7seg_number = preset_num;
	}
}

void loadMIDIComputerPreset(void)
{
	if (preset_num == 0)
	{
		initMIDIArpeggiator();
	}
	if ((preset_num >= 1) && (preset_num <= 4))
	{
		initMIDIKeys(preset_num, TRUE);
	}
}
void loadMIDIKeyboardPreset(void)
{
	if (preset_num == 0)
	{
		//initMIDIArpeggiator();
		initMIDIKeys(1, FALSE);
	}
	if ((preset_num >= 1) && (preset_num <= 4))
	{
		initMIDIKeys(preset_num, TRUE);
	}
}

void loadNoDevicePreset(void)
{
	;
}


