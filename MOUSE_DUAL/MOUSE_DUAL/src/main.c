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
//#include "pipe.h"
//#include "midihost.h"
#include "main.h"
#include "note_process.h"
#include "7Segment.h"


#  define TARGET_PBACLK_FREQ_HZ 32000000 // master clock divided by 2 (64MHZ/2 = 32MHz)

static void initSPIbus(void);
static void setSPI(spi_options_t spiOptions);

// add the spi options driver structure for the LCD DIP204
spi_options_t DIP_spiOptions =
{
	.reg          = DIP204_SPI_NPCS,
	.baudrate     = 1000000,
	.bits         = 8,
	.spck_delay   = 0,
	.trans_delay  = 0,
	.stay_act     = 1,
	.spi_mode     = 0,
	.modfdis      = 1
};

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
	cpu_delay_us(12,64000000);//5
}
void dacwait2(void)
{
	cpu_delay_us(12,64000000);//5
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
	DACvoice = (3 - DACvoice);
	DACnum = (1 - DACnum);
	//KLUDGE
	
	SPIbusy = 1;
	setSPI(spiOptions12DAC);

	dacouthigh = (DACval >> 4) & 0xFF;
	dacoutlow = ((DACval << 4) & 0xF0);

	if (DACnum == 0)
	{
		gpio_clr_gpio_pin(DAC2_CS);
		dacwait1();
		while((spi_write(SPARE_SPI,DACvoice)) != 0);
		while((spi_write(SPARE_SPI,dacouthigh)) !=0);
		while((spi_write(SPARE_SPI,dacoutlow)) != 0);
		dacwait2();
		gpio_set_gpio_pin(DAC2_CS);
		dacwait1();
	}

	if (DACnum == 1)
	{
		gpio_clr_gpio_pin(DAC3_CS);
		dacwait1();
		while((spi_write(SPARE_SPI,DACvoice)) != 0);
		while((spi_write(SPARE_SPI,dacouthigh)) !=0);
		while((spi_write(SPARE_SPI,dacoutlow)) != 0);
		dacwait2();
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
	setSPI(spiOptions16DAC);
	
	DAC16voice = (3 - DAC16voice); //for now, since the panel jacks are accidentally upside down, we'll reverse the DAC voice number
	daccontrol = (16 | (DAC16voice << 1));
	DAC1outhigh = ((daccontrol << 8) + (DAC16val >> 8));
	DAC1outlow = ((DAC16val & 255) << 8);
	gpio_clr_gpio_pin(DAC1_CS);
	cpu_delay_us(2,64000000);
	spi_write(SPARE_SPI,DAC1outhigh);
	spi_write(SPARE_SPI,DAC1outlow);
	cpu_delay_us(12,64000000);
	gpio_set_gpio_pin(DAC1_CS);
	SPIbusy = 0;
}

void lcd_clear_line(uint8_t linenum)
{
	dip204_set_cursor_position(1,linenum);
	dip204_write_string("                    ");
	dip204_set_cursor_position(1,linenum);
	//dip204_hide_cursor();
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

int i =0;
uint16_t testvoltage = 0;
uint16_t testvoltage16 = 0;

void testLoop(void)
{
	while(1)
	{
		//cpu_delay_ms(1,64000000);//5
		for(i=0; i<4; i++)
		{
			dacsend(i,1,testvoltage);
			dacsend(i,0,(4096 - testvoltage));
			DAC16Send(i,testvoltage16);
		}
		testvoltage++;
		testvoltage16++;
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


/*! \brief Main function. Execution starts here.
 */
int main(void){
	sysclk_init();
	board_init();
	
	irq_initialize_vectors();
	cpu_irq_enable();

	// Initialize the sleep manager
	sleepmgr_init();

	ui_init();

	//initialize the SPI bus for DAC and LCD
	initSPIbus();
	
	//send the messages to the DACs to make them update without software LDAC feature
	DACsetup();
	// Write7Seg(56);
	// Start USB host stack
	uhc_start();
	
	//testLoop();
	
	// Start USB device stack
	//udc_start();
	initNoteStack();
	// The USB management is entirely managed by interrupts.
	// As a consequence, the user application only has to play with the power modes.
	
	while (true) {	
		sleepmgr_enter_sleep();
	}
}
