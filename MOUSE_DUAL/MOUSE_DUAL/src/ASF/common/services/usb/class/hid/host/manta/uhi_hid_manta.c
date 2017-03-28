/**
 * \file
 *
 * \brief USB host Human Interface Device (HID) mouse driver.
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

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "uhd.h"
#include "uhc.h"
#include "uhi_hid_manta.h"
#include "dip204.h"
#include "main.h"
#include "note_process.h"
#include <string.h>
#include "7Segment.h"
#include "sequencer_process.h"

#ifdef USB_HOST_HUB_SUPPORT
# error USB HUB support is not implemented on UHI mouse
#endif




/**
 * \ingroup uhi_hid_manta_group
 * \defgroup uhi_hid_manta_group_internal Implementation of UHI HID Mouse
 *
 * Class internal implementation
 * @{
 */
//@}

/**
 * \name Structure to store information about USB Device HID manta
 */
//@{
typedef struct {
	uhc_device_t *dev;
	usb_ep_t ep_in;
	usb_ep_t ep_out;
	uint8_t report_size;
	uint8_t *report;
	uint8_t report_btn_prev;
}uhi_hid_manta_dev_t;

static uhi_hid_manta_dev_t uhi_hid_manta_dev = {
	.dev = NULL,
	.report = NULL,
};

//@}


/**
 * \name Internal routines
 */
//@{
static void uhi_hid_manta_start_trans_report(usb_add_t add);

static void uhi_hid_manta_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);
		
		
static void processSliders(uint8_t sliderNum, uint16_t val);

static void processSliderKeys(uint8_t sliderNum, uint16_t val);

static void processKeys(void);


static bool uhi_manta_send_report(void);

/**
 * \brief Callback called when the report is sent
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finish
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param nb_sent    number of data transfered
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
static void uhi_manta_report_sent(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);
//@}

uint8_t butt_states[48];
uint8_t pastbutt_states[48];
uint8_t func_button_states[4] = {0,0,0,0};
uint8_t past_func_button_states[4] = {0,0,0,0};
uint8_t sliders[4] = {0,0,0,0};
uint8_t pastsliders[4] = {0,0,0,0};
	
//! To signal if a valid report is ready to send
static bool uhi_manta_b_report_valid;
//! Report ready to send
static uint8_t uhi_manta_report[UHI_MANTA_EP_OUT_SIZE];
//! Signal if a report transfer is on going
static bool uhi_manta_report_trans_ongoing;
//! Buffer used to send report
static uint8_t uhi_manta_report_trans[UHI_MANTA_EP_OUT_SIZE];

/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_hid_manta_install(uhc_device_t* dev)
{
	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;
	//char *product = NULL;

	//product = uhc_dev_get_string(dev,dev->dev_desc.iProduct);
	//while(product == NULL);
	
	//lcd_clear_line(2);
	//dip204_printf_string("%x",dev->dev_desc.idProduct);
	Write7Seg(55);
	if (uhi_hid_manta_dev.dev != NULL)
		return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
	conf_desc_lgt = le16_to_cpu(dev->conf_desc->wTotalLength);
	ptr_iface = (usb_iface_desc_t*)dev->conf_desc;
	b_iface_supported = false;
	
	while(conf_desc_lgt)
	{
		switch (ptr_iface->bDescriptorType) 
		{
			case USB_DT_INTERFACE:
				if ((ptr_iface->bInterfaceClass   == HID_CLASS)
					&& (ptr_iface->bInterfaceProtocol == HID_PROTOCOL_GENERIC)
					&& dev->dev_desc.idProduct == 0x2424)
				{
					int i;
					// USB HID Manta interface found
					// Start allocation endpoint(s)
					b_iface_supported = true;
					// initialize button states to 0
					for(i=0; i<48; i++)
					{
						butt_states[i]=0;
						pastbutt_states[i]=0;
						//notestack[i] = -1;
					}
				} 
				else
				{
					b_iface_supported = false; // Stop allocation endpoint(s)
					return UHC_ENUM_UNSUPPORTED; // No interface supported
				}
				
			break;

			case USB_DT_ENDPOINT:
				//  Allocation of the endpoint
				if (!b_iface_supported) 
					break;

				if (!uhd_ep_alloc(dev->address, (usb_ep_desc_t*)ptr_iface))
					return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail

				if(((usb_ep_desc_t*)ptr_iface)->bEndpointAddress & USB_EP_DIR_IN)
				{
					uhi_hid_manta_dev.ep_in = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
					uhi_hid_manta_dev.report_size =
					le16_to_cpu(((usb_ep_desc_t*)ptr_iface)->wMaxPacketSize);
					uhi_hid_manta_dev.report = malloc(uhi_hid_manta_dev.report_size);
					
				}
				else
					uhi_hid_manta_dev.ep_out = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
				break;
			// Ignore descriptor
			default: break;
		}	
		Assert(conf_desc_lgt>=ptr_iface->bLength);
		conf_desc_lgt -= ptr_iface->bLength;
		ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
	}
	if (uhi_hid_manta_dev.report == NULL) {
		Assert(false);
		return UHC_ENUM_MEMORY_LIMIT; // Internal RAM allocation fail
	}
	
	if(uhi_hid_manta_dev.ep_in != 0 && uhi_hid_manta_dev.ep_out != 0)		
	{		
		uhi_hid_manta_dev.dev = dev;
		// All endpoints of all interfaces supported allocated
		return UHC_ENUM_SUCCESS;
	}
	
	return UHC_ENUM_UNSUPPORTED; // No interface supported
}

void uhi_hid_manta_enable(uhc_device_t* dev)
{
	if (uhi_hid_manta_dev.dev != dev) 
		return;  // No interface to enable

	// Init value
	uhi_hid_manta_dev.report_btn_prev = 0;
	initNoteStack();
	manta_mapper = 1; // lets the note_process functions know that it's a manta, and therefore the note numbers need to be mapped to actual MIDI pitches using one of the notemaps
	//memset(lights,0,HEX_BYTES*2+SLIDER_BYTES); removed this because it seems more efficient to manipulate the manta hid send report directly. It's possible that there's a problem with that, which means will need to bring back this separate array.
	

	// FOR NOW, THE LEDS ARE SET TO BEING HOST CONTROLLED
	manta_LED_set_mode(HOST_CONTROL_FULL);


	uhi_hid_manta_start_trans_report(dev->address);
	UHI_HID_MANTA_CHANGE(dev, true);
}



	




void uhi_hid_manta_uninstall(uhc_device_t* dev)
{
	if (uhi_hid_manta_dev.dev != dev) 
		return; // Device not enabled in this interface

	uhi_hid_manta_dev.dev = NULL;
	Assert(uhi_hid_manta_dev.report!=NULL);
	free(uhi_hid_manta_dev.report);
	UHI_HID_MANTA_CHANGE(dev, false);
	manta_mapper = 0;
	initNoteStack();
}

/**
 * \brief Starts the reception of the HID mouse report
 *
 * \param add   USB address to use
 */
static void uhi_hid_manta_start_trans_report(usb_add_t add)
{
	// Start transfer on interrupt endpoint IN
	uhd_ep_run(add, uhi_hid_manta_dev.ep_in, true, uhi_hid_manta_dev.report,
			uhi_hid_manta_dev.report_size, 0, uhi_hid_manta_report_reception);
}

/**
 * \brief Decodes the HID mouse report received
 *
 * \param add           USB address used by the transfer
 * \param status        Transfer status
 * \param nb_transfered Number of data transfered
 */
static void uhi_hid_manta_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered)
{
	uint8_t i;
	unsigned short val;
	UNUSED(ep);
	
	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_hid_manta_start_trans_report(add);
		return; // HID mouse transfer restart
	}

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 64)) {
		return; // HID mouse transfer aborted
	}
	manta_data_lock = 1;
	// Decode hexagon buttons
	for(i=0; i<48; i++)
	{
		butt_states[i] = uhi_hid_manta_dev.report[i+1] + 0x80;	
	}
	//decode sliders
	for(i=0; i<4; i++)
		sliders[i] = uhi_hid_manta_dev.report[i+53] + 0x80;
	//decode function buttons
	for(i=0; i<4; i++)
	{
		func_button_states[i] = uhi_hid_manta_dev.report[i+49] + 0x80;
	}
	manta_data_lock = 0;
	
	if(((sliders[0] != pastsliders[0]) && (sliders[0] != 255)) || ((sliders[1] != pastsliders[1]) && (sliders[1] != 255)))
	{
		val = (sliders[0] + (sliders[1] << 8)) & 0xFFF;
		processSliders(0, val);
	}

	if(((sliders[2] != pastsliders[2]) && (sliders[2] != 255)) || ((sliders[3] != pastsliders[3]) && (sliders[3] != 255)))
	{
		val = (sliders[2] + (sliders[3] << 8)) & 0xFFF;
		processSliders(1, val);
	}
	
	//check if we're in sequencer mode
	if (sequencer_mode == 1)
	{
		processSequencer();
	}
	else
	{
		processKeys();
	}
	
	
		
	blinkersToggle();

	
	manta_send_LED();
	
	uhi_hid_manta_start_trans_report(add);
}

static void processSliders(uint8_t sliderNum, uint16_t val)
{
	if (sequencer_mode == 1)
	{
		processSliderSequencer(sliderNum, val);
	}
	else
	{
		processSliderKeys(sliderNum, val);
	}
}

static void processSliderKeys(uint8_t sliderNum, uint16_t val)
{
	dacsend((sliderNum),0,val);
}

static void processKeys(void)
{
	uint8_t i;
	uint8_t hex_max = 0;

	for (i = 0; i < 48; i++)
	{
		//if the current sensor value of a key is positive and it was zero on last count
		if (manta_data_lock == 0)
		{
			
			if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
			{
				addNote(i,butt_states[i]);
				manta_set_LED_hex(i, Amber);
			}

			else if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
			{
				removeNote(i);	
				manta_set_LED_hex(i, Off);
			}
			
			hexMax[i] = butt_states[i];

			// update the past keymap array (stores the previous values of every key's sensor reading)
			pastbutt_states[i] = butt_states[i];
		}
	
	}

	noteOut();
}

//happens every USB frame
void uhi_hid_manta_sof(bool b_micro)
{
	UNUSED(b_micro);

	if (clock_speed != 0)
	{
		if (USB_frame_counter == clock_speed) {
			clockHappened();
			USB_frame_counter = 0;
		}
		USB_frame_counter++;
	}
	
		
}

//for when you want to update the LED modes or LED states
static bool uhi_manta_send_report(void)
{
	if (uhi_manta_report_trans_ongoing)
	return false;	// Transfer on going then send this one after transfer complete

	// Copy report on other array used only for transfer
	memcpy(uhi_manta_report_trans, uhi_manta_report, UHI_MANTA_EP_OUT_SIZE);
	uhi_manta_b_report_valid = false;

	// Send report
	return uhi_manta_report_trans_ongoing =
	uhd_ep_run(uhi_hid_manta_dev.dev->address,uhi_hid_manta_dev.ep_out,false,uhi_manta_report_trans,UHI_MANTA_EP_OUT_SIZE,0,uhi_manta_report_sent);
}

static void uhi_manta_report_sent(usb_add_t add, usb_ep_t ep,
		uhd_trans_status_t status, iram_size_t nb_transfered)
{
	UNUSED(ep);
	UNUSED(status);
	UNUSED(nb_transfered);
	// Valid report sending
	uhi_manta_report_trans_ongoing = false;
	if (uhi_manta_b_report_valid) {
		// Send new valid report
		uhi_manta_send_report();
	}
}

void manta_LED_set_mode(uint8_t mode)
{
	switch(mode)
	{
		case LOCAL_CONTROL: uhi_manta_report[9] = 0x00; break;    
		case HOST_CONTROL_FULL:uhi_manta_report[9] = 0x03; break; 
		case HOST_CONTROL_BUTTON: uhi_manta_report[9] = 0x20; break;		
		case HOST_CONTROL_SLIDER: uhi_manta_report[9] = 0x02; break;	
		case HOST_CONTROL_HEX_AND_BUTTON: uhi_manta_report[9] = 0x01; break;		
		default: break;         // TODO fix these modes to be the correct bits
	}
}

void manta_set_LED_hex(uint8_t hex, MantaLEDColor color)
{
	uint8_t whichbyte = (hex / 8);
	uint8_t whichbit = hex % 8;
	
	if (color == Amber)
	{
		//turn off the red light if it's on
		uhi_manta_report[whichbyte+10] &= ~(1 << whichbit);	
		// then turn on the amber light
		uhi_manta_report[whichbyte] |= 1 << whichbit;
	}
	else if (color == Red)
	{
		//turn off the amber light if it's on
		uhi_manta_report[whichbyte] &= ~(1 << whichbit);
		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[whichbyte+10] |= 1 << whichbit;
	}
	
	if (color == AmberOn)
	{
		// turn on the amber light
		uhi_manta_report[whichbyte] |= 1 << whichbit;
	}
	else if (color == RedOn)
	{

		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[whichbyte+10] |= 1 << whichbit;
	}

	else if (color == Off)
	{
		//turn off the amber light
		uhi_manta_report[whichbyte]		&= ~(1 << whichbit);
		// turn off the red light
		uhi_manta_report[whichbyte+10]	&= ~(1 << whichbit);
	}
	
	else if (color == RedOff)
	{
		// turn off the red light
		uhi_manta_report[whichbyte+10] &= ~(1 << whichbit);
	}
	
	else if (color == AmberOff)
	{
		//turn off the amber light
		uhi_manta_report[whichbyte] &= ~(1 << whichbit);
	}
	
	else
	{
		// Should not happen.
	}

	//
	//call manta_send_LED(lights) after you have all your lights properly set.

}

void manta_set_LED_slider(uint8_t whichSlider, uint8_t value)
{
	//whichSlider should be 0 (top) or 1 (bottom)
	// value should be 0-8, with 0 meaning off and 1-8 meaning lit LED at that position
	if (value == 0)
	{
		uhi_manta_report[whichSlider + 7] = 0;
	}
	else
	{
		uhi_manta_report[whichSlider + 7] = 1 << (value - 1);
	}
}

void manta_set_LED_slider_bitmask(uint8_t whichSlider, uint8_t bitmask)
{
	//whichSlider should be 0 (top) or 1 (bottom)
	// bitmask should be an 8-bit word, where the bits represent the led states for that slider
	uhi_manta_report[whichSlider + 7] = bitmask;
}

void manta_set_LED_button(uint8_t button, uint8_t color)
{
	uint8_t whichbit = button % 8;
	
	if (color == Amber)
	{
		//turn off the red light if it's on
		uhi_manta_report[6] &= ~(1 << (whichbit+4));
		// then turn on the amber light
		uhi_manta_report[6] |= 1 << whichbit;
	}
	else if (color == Red)
	{
		//turn off the amber light if it's on
		uhi_manta_report[6] &= ~(1 << whichbit);
		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[6] |= 1 << (whichbit+4);
	}

	else if (color == Off)
	{
		//turn off the amber light if it's on
		uhi_manta_report[6] &= ~(1 << whichbit);
		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[6] &= ~(1 << (whichbit+4));
	}

	//
	//call manta_send_LED(lights) after you have all your lights properly set.
}


void manta_send_LED(void)
{
	irqflags_t flags = cpu_irq_save();
	
	// Valid and send report
	uhi_manta_b_report_valid = true;
	uhi_manta_send_report();

	cpu_irq_restore(flags);
	return;
}


//@}

//@}
