/**
 * \file
 *
 * \brief User Interface
 *
 * Copyright (C) 2011 - 2014 Atmel Corporation. All rights reserved.
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
 /**
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include <asf.h>
#include "ui.h"
#include "main.h"
#include "7Segment.h"
#include "udi_cdc.h"
#include "note_process.h"

uint8_t myReceiveBuf[64];
uint8_t mySendBuf[64];


int lastbutton0 = 0;
int lastbutton1 = 0;
extern unsigned char preset_num;
int howManyFilled = 0;
uint8_t my_buf[128];

	
/**
 * \name Internal routines to manage asynchronous interrupt pin change
 * This interrupt is connected to a switch and allows to wakeup CPU in low sleep mode.
 * This wakeup the USB devices connected via a downstream resume.
 * @{
 */
static void ui_disable_asynchronous_interrupt(void);




/**
 * \brief Disables interrupt pin change
 */
static void ui_disable_asynchronous_interrupt(void)
{
	eic_disable_line(&AVR32_EIC, EXT_NMI);
}
//! @}

/**
 * \name Main user interface functions
 * @{
 */
void ui_init(void)
{
	LED_Off(LED0);
	LED_Off(LED1);
	LED_Off(LED2);
	LED_Off(LED3);
	LED_Off(LED4);
	LED_Off(LED5);
}

void ui_usb_mode_change(bool b_host_mode)
{
	ui_init();
	if (b_host_mode) {
		;
	}
}
//! @}

/**
 * \name Host mode user interface functions
 * @{
 */

//! Status of device enumeration
static uhc_enum_status_t ui_enum_status=UHC_ENUM_DISCONNECT;
//! Notify the presence of a USB device mouse
static bool ui_hid_joy_plug = false;
static bool ui_hid_manta_plug = false;
//! Notify the presence of a USB device MIDI
bool ui_midi_plug = false;

void ui_host_vbus_change(bool b_vbus_present)
{
	UNUSED(b_vbus_present);
}

void ui_host_vbus_error(void)
{
}

void ui_host_connection_event(uhc_device_t *dev, bool b_present)
{
	if (b_present) {
		LED_On(USB_CONNECTED_LED);
	} else {
		ui_enum_status = UHC_ENUM_DISCONNECT;
		LED_Off(USB_CONNECTED_LED);
	}
}

void ui_host_enum_event(uhc_device_t * dev, uhc_enum_status_t status)
{
	ui_enum_status = status;
}

void ui_uhi_hid_joy_change(uhc_device_t * dev, bool b_plug)
{
	ui_hid_joy_plug = b_plug;
}

void ui_uhi_hid_manta_change(uhc_device_t * dev, bool b_plug)
{
	ui_hid_manta_plug = b_plug;
}

void ui_uhi_midi_change(uhc_device_t * dev, bool b_plug)
{
	ui_midi_plug = b_plug;
}

void my_callback_midi_rx_notify(void)
{
	uint16_t bytesToRead = 0;
	//LED_On(LED0);
	while (uhi_midi_is_rx_ready()) {
		 //LED_On(LED1);
		 bytesToRead = uhi_midi_read_buf(&my_buf, 64);
		 parseMIDI(bytesToRead);
 		 if (DEBUG) Write7Seg(my_buf[1]);
	}
};

void ui_host_wakeup_event(void)
{
	ui_disable_asynchronous_interrupt();
}




static bool ui_d_midi_enable = false;

void ui_test_flag_reset(void)
{
	//LED_Off(LED_BI1_GREEN);
	//LED_Off(LED_BI1_RED);
}

void ui_test_finish(bool b_success)
{
	if (b_success) {
		//LED_On(LED_BI1_GREEN);
	} else {
		//LED_On(LED_BI1_RED);
	}
}
//! @}

void ui_midi_rx_start(void)
{
	LED_On(LED2);
	
	//udi_midi_tx_send(0);
}

void ui_midi_rx_stop(void)
{
	LED_Off(LED2);
}

void ui_midi_tx_start(void)
{
	//LED_On(LED_AMBER1);
}

void ui_midi_tx_stop(void)
{
	//LED_Off(LED_AMBER1);
}

void ui_midi_error(void)
{
	//LED_On(LED_BI0_RED);
}

void ui_midi_overflow(void)
{
	//LED_On(LED_BI1_RED);
}

void ui_my_midi_receive(void)
{
	//receive midi from a computer
	uint16_t bytesToRead = 0;
	
	if (udi_midi_is_rx_ready()) {
		int numReceivedBytes = udi_midi_get_nb_received_data();
		udi_midi_read_buf(myReceiveBuf, numReceivedBytes);
		for (int i = 0; i < numReceivedBytes; i++)
		{
			my_buf[i] = myReceiveBuf[i];
		}
		if(numReceivedBytes && udi_midi_is_tx_ready())
		{
			//udi_midi_write_buf(mySendBuf, numReceivedBytes);   /// uncomment to echo back to computer -JS

			parseMIDI(numReceivedBytes);
		}
		} else {
		// Fifo empty then Stop
		;
	}
}

void ui_my_midi_send(void)
{
	ui_midi_tx_start();
	// Transfer UART RX fifo to CDC TX
	if (!udi_midi_is_tx_ready()) {
		// Fifo full
		;
		}else{
		udi_midi_write_buf(mySendBuf, 4);
	}
	ui_midi_tx_stop();
}


uint32_t upHeld = 0;
uint32_t downHeld = 0;
uint32_t holdTimeThresh = 8;
static uint32_t buttonFrameCounter = 0;
static uint32_t buttonHoldSpeed = 60;

void USB_frame_action(uint16_t framenumber)
{
	
	
	//put things here that need to happen on every single USB frame   :D
	
	if (type_of_device_connected == MIDIComputerConnected)
	{
		ui_my_midi_receive(); //seems like we need to poll to receive on every SOF in device mode
	}
	
	//step the internal clock
	if (clock_speed != 0)
	{
		if (USB_frame_counter >= clock_speed)
		{
			clockHappened();
			USB_frame_counter = 0;
		}
		USB_frame_counter++;
	}


	//watch the up and down buttons to catch the "hold down" action and speed up the preset scrolling
	if (!gpio_get_pin_value(GPIO_PRESET_UP_SWITCH))
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
	
	if (!gpio_get_pin_value(GPIO_PRESET_DOWN_SWITCH))
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
		
	

	
}

void ui_ext_gate_in(void)
{
	//allow a gate in to create a clock on the computer (via a MIDI note)

	//send note off for middle C
	mySendBuf[0] = 0x09;
	mySendBuf[1] = 0x90;
	mySendBuf[2] = 0x3c;
	mySendBuf[3] = 0x00;
	ui_my_midi_send();
	//then send a note on for middle C
	mySendBuf[0] = 0x09;
	mySendBuf[1] = 0x90;
	mySendBuf[2] = 0x3c;
	mySendBuf[3] = 0x78;
	ui_my_midi_send();
}

