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
#include "conf_usb_host.h"
#include "ui.h"
#include "main.h"
#include "note_process.h"
#include "7Segment.h"
#include "sequencer_process.h"


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
static void setSPI(spi_options_t spiOptions);

static volatile bool main_b_midi_enable = false;
uint32_t dummycounter = 0;
uint8_t manta_mapper = 0;
uint8_t tuning_count = 0;
uint8_t manta_data_lock = 0; // probably not necessary, added this when I was worried about read and write happening simultaneously, but it wasn't the case.
uint8_t spi_mode = 0;

uint32_t clock_speed = 0; // this is the speed of the internal sequencer clock - not totally sure of the units, it's not actually ms, but its some measure of the period between clicks. IF you want to use external gates only, set this number to zero.
uint32_t USB_frame_counter = 0; // used by the internal sequencer clock to count USB frames (which are the source of the internal sequencer metronome)
uint8_t sequencer_mode = 1;  // Hey Reid, this is the variable to change to put it into "sequencer" mode.

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
	initSequencer();
	//initialize the SPI bus for DAC
	initSPIbus();
	
	//initialize the I2C bus (TWI) for preset user memory storage (on external EEPROM)
	initI2C();
	
	//send the messages to the DACs to make them update without software LDAC feature
	DACsetup();
	
	// a test write
	//sendI2CtoEEPROM();
	
	tRampInit(&pitchGlideOne, 2000, 500, 1);
	tRampInit(&pitchGlideTwo, 2000, 500, 1);
	tRampInit(&cv1GlideOne, 2000, 500, 1);
	tRampInit(&cv2GlideOne, 2000, 500, 1);
	tRampInit(&cv3GlideOne, 2000, 500, 1);
	tRampInit(&cv4GlideOne, 2000, 500, 1);
	tRampInit(&cv1GlideTwo, 2000, 500, 1);
	tRampInit(&cv2GlideTwo, 2000, 500, 1);
	tRampInit(&cv3GlideTwo, 2000, 500, 1);
	tRampInit(&cv4GlideTwo, 2000, 500, 1);
	
	// Start USB host stack
	uhc_start();
	
	// figure out if we're supposed to be in host mode or device mode for the USB
	USB_Mode_Switch_Check();
	
	//start off on preset 0;
	updatePreset();

	// The USB management is entirely managed by interrupts.
	// As a consequence, the user application only has to play with the power modes.
	
	while (true) {	
		sleepmgr_enter_sleep();
	}
}


U8 data_received[PATTERN_TEST_LENGTH];

// SequencerOne timer interrupt.
__attribute__((__interrupt__))
static void tc1_irq(void)
{
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(TC1, TC1_CHANNEL);
	

	LED_Off(LED1);
	
	if (pitch_vs_trigger == PitchMode)
	{
		DAC16Send(1, 0);
	}
	else // TriggerMode
	{
		// Set 4 trigger outputs low
		dacsend(0, 1, 0);
		dacsend(1, 1, 0);
		DAC16Send(0, 0);
		DAC16Send(1, 0);
	}
}

// SequencerTwo timer interrupt.
__attribute__((__interrupt__))
static void tc2_irq(void)
{
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(TC2, TC2_CHANNEL);
		
	if (pitch_vs_trigger == PitchMode)
	{
		DAC16Send(3, 0);
	}
	else // TriggerMode
	{
		// Set 4 trigger outputs low
		dacsend(2, 1, 0);
		dacsend(3, 1, 0);
		DAC16Send(2, 0);
		DAC16Send(3, 0);
	}
}

int count3 = 0;
// Glide timer.
__attribute__((__interrupt__))
static void tc3_irq(void)
{
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	int sr = tc_read_sr(TC3, TC3_CHANNEL);

	// SequencerOne & sequencerTwo Pitch
	DAC16Send(0, tRampTick(&pitchGlideOne) * UINT16_MAX); 
	DAC16Send(2, tRampTick(&pitchGlideTwo) * UINT16_MAX);
	
	// SequencerOne CV1-CV2
	dacsend(0, 0, tRampTick(&cv1GlideOne));
	dacsend(1, 0, tRampTick(&cv2GlideOne));
	// SequencerOne CV3-CV4
	dacsend(0, 1, tRampTick(&cv3GlideOne));
	dacsend(1, 1, tRampTick(&cv4GlideOne));
	// SequencerTwo CV1-CV2
	dacsend(2, 0, tRampTick(&cv1GlideTwo));
	dacsend(3, 0, tRampTick(&cv2GlideTwo));
	// SequencerTwo CV3-CV4
	dacsend(2, 1, tRampTick(&cv3GlideTwo));
	dacsend(3, 1, tRampTick(&cv4GlideTwo));	
}

static void tc1_init(volatile avr32_tc_t *tc)
{

	static const tc_waveform_opt_t waveform_opt = {
		// Channel selection.
		.channel  = TC1_CHANNEL,
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
		.cpcstop  = true,
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
	tc_write_rc(tc, TC1_CHANNEL, (sysclk_get_pba_hz() / 8 / 10000)); // was 1000
	// configure the timer interrupt
	tc_configure_interrupts(tc, TC1_CHANNEL, &tc_interrupt);

	
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
		.cpcstop  = true,
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
	tc_write_rc(tc, TC2_CHANNEL, (sysclk_get_pba_hz() / 8 / 10000)); // was 1000
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
	tc_write_rc( tc, TC3_CHANNEL, 2000); // approximately .5 ms
	
	// configure the timer interrupt
	tc_configure_interrupts(tc, TC3_CHANNEL, &tc_interrupt);

}

void initTimers (void)
{
	
	tc1 = TC1;
	tc2 = TC2;
	tc3 = TC3;
	
	// Enable the clock to the selected example Timer/counter peripheral module.
	sysclk_enable_peripheral_clock(TC1);
	sysclk_enable_peripheral_clock(TC2);
	sysclk_enable_peripheral_clock(TC3);
	
	tc1_init(tc1);
	tc2_init(tc2);
	tc3_init(tc3);

}

  
// interrupt handler for external gate signal input from synthesizer.
__attribute__((__interrupt__))
static void eic_int_handler1(void)
{
	eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE1);
	clockHappened();
}


// interrupt handler to find the state of the HOST/DEVICE switch on the panel
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
	
	else if( gpio_get_pin_interrupt_flag( GPIO_PRESET_SWITCH1) )
	{		
		//down switch
		delay_us(500); // to de-bounce
		Preset_Switch_Check(0);
		LED_On(LED2);
		LED_Off(LED1);
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_PRESET_SWITCH1);
	}
	
	else if( gpio_get_pin_interrupt_flag( GPIO_PRESET_SWITCH2) )
	{		
		//up switch
		delay_us(500); // to de-bounce
		Preset_Switch_Check(1);
		LED_Off(LED2);
		LED_On(LED1);
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_PRESET_SWITCH2);
	}
	
	else if( gpio_get_pin_interrupt_flag( GPIO_PREFERENCES_SWITCH) )
	{
		//up switch
		delay_us(500); // to de-bounce
		//Preset_Switch_Check(1);
		LED_Off(LED4);
		LED_On(LED0);
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_PREFERENCES_SWITCH);
	}
	
	else if( gpio_get_pin_interrupt_flag( GPIO_SAVE_SWITCH) )
	{
		//up switch
		delay_us(500); // to de-bounce
		//Preset_Switch_Check(1);
		LED_Off(LED0);
		LED_On(LED4);
		// Clear the interrupt flag of the pin PB2 is mapped to.
		gpio_clear_pin_interrupt_flag(GPIO_SAVE_SWITCH);
	}
	
}

void USB_Mode_Switch_Check(void)
{
		
		int myTemp = gpio_get_pin_value(GPIO_HOST_DEVICE_SWITCH);
		
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
			uhc_start();
			myUSBMode = HOSTMODE;
		}	
}

void Preset_Switch_Check(uint8_t whichSwitch)
{
	if (whichSwitch)
	{
		if (!gpio_get_pin_value(GPIO_PRESET_SWITCH2))
		{
			if (preset_num >= 99)
			{
				preset_num = 99;
			}
			else
			{
				preset_num++;
			}
			
		}
	}
	else 
	{
		if (!gpio_get_pin_value(GPIO_PRESET_SWITCH1))
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
}

void updatePreset(void)
{
	Write7Seg(preset_num);
	switch (preset_num)
	{
		case 0:
		initNoteStack();
		noteOut();
		polymode = 0;
		break;
		
		case 1:
		initNoteStack();
		noteOut();
		polymode = 1;
		polynum = 2;
		break;
		
		case 2:
		initNoteStack();
		noteOut();
		polymode = 1;
		polynum = 3;
		break;
		
		case 3:
		initNoteStack();
		noteOut();
		polymode = 1;
		polynum = 4;
		break;
		
		default: 
		break;	
	}
}

void clockHappened(void)
{
	sequencerStep();

}


void initI2C(void)
{
	U8 status = 0;
	// I2C options settings
	my_opt.pba_hz = 33000000; // is this correct?
	my_opt.speed = TWI_SPEED;
	my_opt.chip = EEPROM_ADDRESS;
	status = twi_master_init(&AVR32_TWI, &my_opt);
	if (status == TWI_SUCCESS)
	{
		// display test result to user
		Write7Seg(77);
	}
	else
	{
		// display test result to user
		Write7Seg(70);
	}

	static const gpio_map_t TWI_GPIO_MAP =
	{
		{AVR32_TWI_SDA_0_0_PIN, AVR32_TWI_SDA_0_0_FUNCTION},
		{AVR32_TWI_SCL_0_0_PIN, AVR32_TWI_SCL_0_0_FUNCTION}
	};
	gpio_enable_module(TWI_GPIO_MAP,
	sizeof(TWI_GPIO_MAP) / sizeof(TWI_GPIO_MAP[0]));

}

void sendI2CtoEEPROM(void)
{
	U8 data_received[PATTERN_TEST_LENGTH] = {0};
	U8 status = 0;
	U8 i = 0;
	// TWI chip address to communicate with
	I2Cpacket_sent.chip = EEPROM_ADDRESS;
	// TWI address/commands to issue to the other chip (node)
	I2Cpacket_sent.addr[0] = VIRTUALMEM_ADDR_START >> 16;
	I2Cpacket_sent.addr[1] = VIRTUALMEM_ADDR_START >> 8;
	I2Cpacket_sent.addr[2] = VIRTUALMEM_ADDR_START;
	// Length of the TWI data address segment (1-3 bytes)
	I2Cpacket_sent.addr_length = EEPROM_ADDR_LGT;
	// Where to find the data to be written
	I2Cpacket_sent.buffer = (void*) test_pattern;
	// How many bytes do we want to write
	I2Cpacket_sent.length = PATTERN_TEST_LENGTH;

	// perform a write access
	status = twi_master_write(&AVR32_TWI, &I2Cpacket_sent);

	// check write result
	if (status == TWI_SUCCESS)
	{
		// display test result to user
		Write7Seg(17);
	}
	else
	{
		// display test result to user
		Write7Seg(10);
	}
	delay_ms(1);

	// TWI chip address to communicate with
	I2Cpacket_received.chip = EEPROM_ADDRESS ;
	// Length of the TWI data address segment (1-3 bytes)
	I2Cpacket_received.addr_length = EEPROM_ADDR_LGT;
	// How many bytes do we want to write
	I2Cpacket_received.length = PATTERN_TEST_LENGTH;
	// TWI address/commands to issue to the other chip (node)
	I2Cpacket_received.addr[0] = VIRTUALMEM_ADDR_START >> 16;
	I2Cpacket_received.addr[1] = VIRTUALMEM_ADDR_START >> 8;
	I2Cpacket_received.addr[2] = VIRTUALMEM_ADDR_START;
	// Where to find the data to be written
	I2Cpacket_received.buffer = data_received;

	// perform a read access
	status = twi_master_read(&AVR32_TWI, &I2Cpacket_received);

	// check read result
	if (status == TWI_SUCCESS)
	{
		// display test result to user
		Write7Seg(27);
	}
	else
	{
		// display test result to user
		Write7Seg(20);
	}

	// check received data against sent data
	for (i = 0 ; i < PATTERN_TEST_LENGTH; i++)
	{
		if (data_received[i] != test_pattern[i])
		{
			// a char isn't consistent
			Write7Seg(30);
		}
	}

	// everything was OK
	Write7Seg(37);
	//this will be to send preset data to the EEPROM chip.
}

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

unsigned short dacouthigh = 0;
unsigned short dacoutlow = 0;
unsigned short DAC1outhigh = 0;
unsigned short DAC1outlow = 0;
unsigned char SPIbusy = 0;
unsigned char preset_num = 0;

void DACsetup(void)
{
	//let the portamento interrupt know the SPI is busy
	SPIbusy = 1;
	
	setSPI(spiOptions12DAC);
	spi_mode = TWELVEBIT;
	
	gpio_clr_gpio_pin(DAC2_CS);
	dacwait1();
	dacwait1();
	dacwait1();
	spi_write(SPARE_SPI,0x30);
	spi_write(SPARE_SPI,0x00);
	spi_write(SPARE_SPI,0x0F);
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
	spi_write(SPARE_SPI,0x30);
	spi_write(SPARE_SPI,0x00);
	spi_write(SPARE_SPI,0x0F);
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

static void setSPI(spi_options_t spiOptions)
{
	SPIbusy = 1;
	spi_disable(SPARE_SPI);
	spi_initMaster(SPARE_SPI, &spiOptions);
	spi_selectionMode(SPARE_SPI, 0, 0, 0);
	spi_selectChip(SPARE_SPI, 0);
	spi_setupChipReg(SPARE_SPI, &spiOptions, TARGET_PBACLK_FREQ_HZ);
	spi_enable(SPARE_SPI);
}

void dacsend(unsigned char DACvoice, unsigned char DACnum, unsigned short DACval)
{
	//send a value to one of the DAC channels to be converted to analog voltage
	//DACnum is which type of output it goes to (0 = B, 1 = C)
	
	//for now, to correct for a mistake on the panel where the jacks are upside down, we reverse the DACvoice and DACnum numbers
	//KLUDGE
	//DACvoice = (3 - DACvoice);
	//DACnum = (1 - DACnum);
	//KLUDGE
	
	SPIbusy = 1;
	if (spi_mode != TWELVEBIT)
	{
		setSPI(spiOptions12DAC);
		spi_mode = TWELVEBIT;
	}

	dacouthigh = (DACval >> 4) & 0xFF;
	dacoutlow = ((DACval << 4) & 0xF0);

	if (DACnum == 0)
	{
		gpio_clr_gpio_pin(DAC2_CS);
		dacwait1();
		while((spi_write(SPARE_SPI,DACvoice)) != 0);
		while((spi_write(SPARE_SPI,dacouthigh)) !=0);
		while((spi_write(SPARE_SPI,dacoutlow)) != 0);
		dacwait1();
		gpio_set_gpio_pin(DAC2_CS);
		//dacwait1();
	}

	if (DACnum == 1)
	{
		gpio_clr_gpio_pin(DAC3_CS);
		dacwait1();
		while((spi_write(SPARE_SPI,DACvoice)) != 0);
		while((spi_write(SPARE_SPI,dacouthigh)) !=0);
		while((spi_write(SPARE_SPI,dacoutlow)) != 0);
		dacwait1();
		gpio_set_gpio_pin(DAC3_CS);
		dacwait1();
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
		setSPI(spiOptions16DAC);
		spi_mode = SIXTEENBIT;
	}
	
	//DAC16voice = (3 - DAC16voice); //for now, since the panel jacks are accidentally upside down, we'll reverse the DAC voice number
	daccontrol = (16 | (DAC16voice << 1));
	DAC1outhigh = ((daccontrol << 8) + (DAC16val >> 8));
	DAC1outlow = ((DAC16val & 255) << 8);
	gpio_clr_gpio_pin(DAC1_CS);
	dacwait2();
	spi_write(SPARE_SPI, DAC1outhigh);
	spi_write(SPARE_SPI, DAC1outlow);
	dacwait2();
	gpio_set_gpio_pin(DAC1_CS);
	SPIbusy = 0;

}

static void initSPIbus(void)
{
	SPIbusy = 1;
	//prepare the pins the control the DAC and set them to default positions
	gpio_set_gpio_pin(DAC1_CS);
	gpio_set_gpio_pin(DAC2_CS);
	gpio_set_gpio_pin(DAC3_CS);
	//gpio_set_gpio_pin(DAC4_CS);
	gpio_clr_gpio_pin(REF1);
	//gpio_clr_gpio_pin(REF2);

	//allow pins to settle
	delay_ms(1);

	// Initialize as master
	setSPI(spiOptions12DAC);
	
	// Assign I/Os to SPI
	static const gpio_map_t DIP204_SPI_GPIO_MAP =
	{
		{DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock.
		{DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
		{DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
		{DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};
	gpio_enable_module(DIP204_SPI_GPIO_MAP,
	sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));
	// Assign I/Os to SPI
	static const gpio_map_t SPARE_SPI_GPIO_MAP =
	{
		{SPARE_SPI_SCK_PIN,  SPARE_SPI_SCK_FUNCTION },  // SPI Clock.
		{SPARE_SPI_MISO_PIN, SPARE_SPI_MISO_FUNCTION},  // MISO.
		{SPARE_SPI_MOSI_PIN, SPARE_SPI_MOSI_FUNCTION},  // MOSI.
		{SPARE_SPI_NPCS_PIN, SPARE_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};
	gpio_enable_module(SPARE_SPI_GPIO_MAP,
	sizeof(SPARE_SPI_GPIO_MAP) / sizeof(SPARE_SPI_GPIO_MAP[0]));
}

int i = 0;
uint16_t testvoltage = 0;
uint16_t testvoltage16 = 0;

void testLoop(void)
 {
	while(1)
	{
		testvoltage16 = 65530;
		//cpu_delay_ms(1,64000000);//5
		for(i=0; i<4; i++)
		{
			dacsend(i,1,testvoltage);
			dacsend(i,0,(4096 - testvoltage));
			DAC16Send(i,testvoltage16);
		}
		testvoltage++;
		//testvoltage16++;
		
		if (testvoltage > 2095)
		{
			LED_Off(LED5);
		}
		if (testvoltage > 4095)
		{
			testvoltage = 0;
			LED_On(LED5);
		}
		if (testvoltage16 > 65534)
		{
			testvoltage16 = 0;
		}
		Write7Seg(testvoltage16 / 656);
		
	}
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
	  
	gpio_enable_pin_interrupt(GPIO_PRESET_SWITCH1 , GPIO_PIN_CHANGE);	// PB02
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_PRESET_SWITCH1/8), AVR32_INTC_INT0);
	
	gpio_enable_pin_interrupt(GPIO_PRESET_SWITCH2 , GPIO_PIN_CHANGE);	// PA20
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_PRESET_SWITCH2/8), AVR32_INTC_INT0);
	
	gpio_enable_pin_interrupt(GPIO_SAVE_SWITCH , GPIO_PIN_CHANGE);	 //PX39
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_SAVE_SWITCH/8), AVR32_INTC_INT0);
	
	gpio_enable_pin_interrupt(GPIO_PREFERENCES_SWITCH , GPIO_PIN_CHANGE);	//PX02
	INTC_register_interrupt( &int_handler_switches, AVR32_GPIO_IRQ_0 + (GPIO_PREFERENCES_SWITCH/8), AVR32_INTC_INT0);
	
	
	// Register the RTC interrupt handler to the interrupt controller.
	INTC_register_interrupt(&tc1_irq, TC1_IRQ, TC1_IRQ_PRIORITY);
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
	if (!main_b_midi_enable)
	return;
	ui_process(udd_get_frame_number());
}


bool main_midi_enable(void)
{
	main_b_midi_enable = true;
	return true;
}

void main_midi_disable(void)
{
	main_b_midi_enable = false;
}



