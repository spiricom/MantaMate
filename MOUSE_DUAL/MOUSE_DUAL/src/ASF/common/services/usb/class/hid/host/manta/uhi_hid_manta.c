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
#include "main.h"
#include "manta_keys.h"
#include <string.h>
#include "7Segment.h"
#include "sequencer_process.h"

#ifdef USB_HOST_HUB_SUPPORT
# error USB HUB support is not implemented on UHI mouse
#endif


#define RED_JUST_OFF FALSE
#define TESTING_FIRST_EDITION_BEHAVIOR TRUE

static void processSliders(uint8_t sliderNum, uint16_t val);

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
uint8_t pastSliders[4] = {0,0,0,0};
BOOL slidersTouched[2] = {0,0};
BOOL pastSlidersTouched[2] = {0,0};
BOOL firstEdition = FALSE;
uint8_t which_led_buffer_currently_sending = 0;
uint8_t which_led_buffer_needs_sending = 0;
	
	
uint8_t sof_count = 0;
	
//! To signal if a valid report is ready to send
static bool uhi_manta_b_report_valid;
//! Report ready to send

static uint8_t uhi_manta_report[2][UHI_MANTA_EP_OUT_SIZE];
static uint8_t uhi_manta_report_firstEdition[UHI_MANTA_1ST_ED_EP_OUT_SIZE];

uint8_t LEDsChangedSoFar = 0;

//! Signal if a report transfer is on going
static bool uhi_manta_report_trans_ongoing;

//! Buffer used to send report
static uint8_t uhi_manta_report_trans[UHI_MANTA_EP_OUT_SIZE];
static uint8_t uhi_manta_1st_ed_report_trans[UHI_MANTA_1ST_ED_EP_OUT_SIZE];

/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_hid_manta_install(uhc_device_t* dev)
{
	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;

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
					
					busyWithUSB = TRUE;
					//set version number based on serial number
					if (dev->dev_desc.iSerialNumber < 70)
					{
						if (TESTING_FIRST_EDITION_BEHAVIOR)
						{
							firstEdition = TRUE;
						}
					}
					else
					{
						//firstEdition = TRUE;  //just to check functionality
						firstEdition = FALSE;
					}
					
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
	freeze_LED_update = TRUE;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			uhi_manta_report[i][j] = 0;
		}
	}

	uhi_hid_manta_start_trans_report(dev->address);
	UHI_HID_MANTA_CHANGE(dev, true);
	new_manta_attached = true;
}

void uhi_hid_manta_uninstall(uhc_device_t* dev)
{
	int i = 0;
	
	if (uhi_hid_manta_dev.dev != dev) 
		return; // Device not enabled in this interface

	freeze_LED_update = TRUE;
	for (i = 0; i < 8; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			uhi_manta_report[i][j] = 0;
		}
	}

	uhi_hid_manta_dev.dev = NULL;
	Assert(uhi_hid_manta_dev.report!=NULL);
	free(uhi_hid_manta_dev.report);

	// Decode hexagon buttons
	for(i=0; i<48; i++)
	{
		butt_states[i] = 0;
	}
	//decode sliders
	for(i=0; i<4; i++)
	sliders[i] = 0;
	//decode function buttons
	for(i=0; i<4; i++)
	{
		func_button_states[i] = 0;
	}
	processSliders(0, 0);
	processSliders(1, 0);
	processHexTouch();
	UHI_HID_MANTA_CHANGE(dev, false);
	no_device_mode_active = FALSE;
	type_of_device_connected = NoDeviceConnected;
	//initNoteStack();
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


static void processSliders(uint8_t sliderNum, uint16_t val)
{

	processSliderSequencer(sliderNum, val);

	processSliderKeys(sliderNum, val);

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
	

	
	if ((sliders[0] == 255) && (sliders[1] == 255))
	{
		slidersTouched[0] = FALSE;
	}
	else
	{
		slidersTouched[0] = TRUE;
		if((sliders[0] != pastSliders[0]) || (sliders[1] != pastSliders[1]))
		{
			val = (sliders[0] + (sliders[1] << 8)) & 0xFFF;
			processSliders(0, val);
		}
	}

	
	
	if ((sliders[2] == 255) && (sliders[3] == 255))
	{
		slidersTouched[1] = FALSE;
	}
	else
	{
		slidersTouched[1] = TRUE;
		if((sliders[2] != pastSliders[2])  || (sliders[3] != pastSliders[3]))
		{
			val = (sliders[2] + (sliders[3] << 8)) & 0xFFF;
			processSliders(1, val);
		}
	}
	
	for (int i = 0; i < 4; i++)
	{
		pastSliders[i] = sliders[i];
	}
	for (int i = 0; i < 2; i++)
	{
		if (slidersTouched[i] != pastSlidersTouched[i])
		{
			if (slidersTouched[i])
			{
				mantaSliderTouchAction(i);
			}
			else
			{
				mantaSliderReleaseAction(i);
			}
		}
		
		pastSlidersTouched[i] = slidersTouched[i];
	}

	processHexTouch();

	uhi_hid_manta_start_trans_report(add);
}


//happens every USB frame
int blinkCount = 0;
void uhi_hid_manta_sof(bool b_micro)
{
	UNUSED(b_micro);
	
	if (++blinkCount >= 150)
	{
		blinkCount = 0;
		blink();
	}
	
	if (firstEdition)
	{
		dimLEDsForFirstEdition();
	}
	if (!freeze_LED_update)
	{
		manta_send_LED();
	}

}

//for when you want to update the LED modes or LED states
static bool uhi_manta_send_report(void)
{
	if (uhi_manta_report_trans_ongoing)
	return false;	// Transfer on going then send this one after transfer complete

	if (!firstEdition) //any latter-day Manta
	{
		// Copy report on other array used only for transfer
		memcpy(uhi_manta_report_trans, uhi_manta_report[0], UHI_MANTA_EP_OUT_SIZE);
		uhi_manta_b_report_valid = false;
		// Send report
		return uhi_manta_report_trans_ongoing =
		uhd_ep_run(uhi_hid_manta_dev.dev->address,uhi_hid_manta_dev.ep_out,false,uhi_manta_report_trans,UHI_MANTA_EP_OUT_SIZE,0,uhi_manta_report_sent);
	}
	else //send 10-byte reports for 1st edition mantas without red LEDs
	{
		// Copy report on other array used only for transfer
		memcpy(uhi_manta_1st_ed_report_trans, uhi_manta_report_firstEdition, UHI_MANTA_1ST_ED_EP_OUT_SIZE);
		uhi_manta_b_report_valid = false;
		// Send report
		return uhi_manta_report_trans_ongoing =
		uhd_ep_run(uhi_hid_manta_dev.dev->address,uhi_hid_manta_dev.ep_out,false,uhi_manta_1st_ed_report_trans,UHI_MANTA_1ST_ED_EP_OUT_SIZE,0,uhi_manta_report_sent);
	}
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
		case LOCAL_CONTROL: uhi_manta_report[0][9] = 0x00; break;    
		case HOST_CONTROL_FULL:uhi_manta_report[0][9] = 0x03; break; 
		case HOST_CONTROL_BUTTON: uhi_manta_report[0][9] = 0x20; break;		
		case HOST_CONTROL_SLIDER: uhi_manta_report[0][9] = 0x02; break;	
		case HOST_CONTROL_HEX_AND_BUTTON: uhi_manta_report[0][9] = 0x01; break;		
		default: break;
	}
}

void manta_set_LED_hex(uint8_t hex, MantaLEDColor color)
{
	uint8_t whichbyte = (hex / 8);
	uint8_t whichbit = hex % 8;
	
	if (color == Amber)
	{
		//turn off the red light if it's on
		uhi_manta_report[0][whichbyte+10] &= ~(1 << whichbit);	
		// then turn on the amber light
		uhi_manta_report[0][whichbyte] |= 1 << whichbit;
	}
	else if (color == Red)
	{
		//turn off the amber light if it's on
		uhi_manta_report[0][whichbyte] &= ~(1 << whichbit);
		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[0][whichbyte+10] |= 1 << whichbit;
	}
	
	if (color == AmberOn)
	{
		// turn on the amber light
		uhi_manta_report[0][whichbyte] |= 1 << whichbit;
	}
	else if (color == RedOn)
	{

		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[0][whichbyte+10] |= 1 << whichbit;
	}
	else if (color == BothOn)
	{
		// turn on the amber light
		uhi_manta_report[0][whichbyte] |= 1 << whichbit;
		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[0][whichbyte+10] |= 1 << whichbit;
	}
	else if (color == Off)
	{
		//turn off the amber light
		uhi_manta_report[0][whichbyte]		&= ~(1 << whichbit);
		// turn off the red light
		uhi_manta_report[0][whichbyte+10]	&= ~(1 << whichbit);
	}
	
	else if (color == RedOff)
	{
		// turn off the red light
		uhi_manta_report[0][whichbyte+10] &= ~(1 << whichbit);
	}
	
	else if (color == AmberOff)
	{
		//turn off the amber light
		uhi_manta_report[0][whichbyte] &= ~(1 << whichbit);
	}
	
	else
	{
		// Should not happen.
	}
	//
	//call manta_send_LED() after you have all your lights properly set.

}

void manta_set_LED_slider(uint8_t whichSlider, uint8_t value)
{
	//whichSlider should be 0 (top) or 1 (bottom)
	// value should be 0-8, with 0 meaning off and 1-8 meaning lit LED at that position
	if (value == 0)
	{
		uhi_manta_report[0][whichSlider + 7] = 0;
	}
	else
	{
		uhi_manta_report[0][whichSlider + 7] = 1 << (value - 1);
	}
}

void manta_set_LED_slider_bitmask(uint8_t whichSlider, uint8_t bitmask)
{
	//whichSlider should be 0 (top) or 1 (bottom)
	// bitmask should be an 8-bit word, where the bits represent the led states for that slider
	uhi_manta_report[0][whichSlider + 7] = bitmask;
}

void manta_set_LED_button(uint8_t button, uint8_t color)
{
	uint8_t whichbit = button % 8;
	
	if (color == Amber)
	{
		//turn off the red light if it's on
		uhi_manta_report[0][6] &= ~(1 << (whichbit+4));
		// then turn on the amber light
		uhi_manta_report[0][6] |= 1 << whichbit;
	}
	else if (color == Red)
	{
		//turn off the amber light if it's on
		uhi_manta_report[0][6] &= ~(1 << whichbit);
		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[0][6] |= 1 << (whichbit+4);
	}

	else if (color == Off)
	{
		//turn off the amber light if it's on
		uhi_manta_report[0][6] &= ~(1 << whichbit);
		// ROXXXANNEE  YOU DON"T HAVE TO turn on the red light
		uhi_manta_report[0][6] &= ~(1 << (whichbit+4));
	}

	//
	//call manta_send_LED() after you have all your lights properly set.
}

void manta_clear_all_LEDs(void)
{
	//clear amber LEDs
	for (int i = 0; i < 9; i++)
	{
		uhi_manta_report[0][i] = 0;
	}
	//skip the 10th byte ([9]) because that holds the mode information
	//then clear the red LEDs
	for (int i = 0; i < 6; i++)
	{
		uhi_manta_report[0][i+10] = 0;
	}
}

int which_bit = 0;
int which_byte = 0;

void manta_send_LED(void)
{
	irqflags_t flags = cpu_irq_save();
	
	// Valid and send report
	uhi_manta_b_report_valid = true;
	
	if (roll_LEDs)
	{
		uint8_t countdown = 1; //how many bits we copy each USB frame (how many LEDs can change)
		while(countdown > 0)
		{
			which_bit++;
			if (which_bit > 7)
			{
				which_bit = 0;
				which_byte++;
			}
			
			if (uhi_manta_report[1][which_byte] == uhi_manta_report[0][which_byte])
			{
				which_byte++;
			}
			
			if (which_byte > 15)
			{
				//we're done rolling - exit!
				which_byte = 0;
				roll_LEDs = 0;
				break;
			}


			uint8_t myMask = (1 << which_bit );
			uint8_t masked_n = (uhi_manta_report[0][which_byte] & ( 1 << which_bit ));
		
			if (masked_n > 0) //the bit was on - let's add it to the array
			{
				uhi_manta_report[1][which_byte] = (uhi_manta_report[1][which_byte] | myMask);
			}
			else //the bit was off. Lets clear it in the new array
			{
				uhi_manta_report[1][which_byte] = (uhi_manta_report[1][which_byte] & ~myMask);
			}
			countdown--;
		}
	}
	else
	{
		for (int i = 0; i < 16; i++)
		{
			uhi_manta_report[1][i] = uhi_manta_report[0][i];
		}
	}

	uhi_manta_send_report();
	
	cpu_irq_restore(flags);
	return;
}

uint8_t counterForRedDimming = 0;
uint8_t counterForBothDimming = 0;
uint8_t dimOnRED = 0;
uint8_t dimOnBOTH = 0;
void dimLEDsForFirstEdition(void)
{
	// if RED is on with the hexagons
	for (int i = 0; i< 6; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			

			if ((uhi_manta_report[1][i+10] >> j) & 1)
			{
				
				if (RED_JUST_OFF)
				{
					uhi_manta_report_firstEdition[i]  &= ~(1 << j);
				}
				else
				{
					//if both RED and AMBER are on - dim it darker
					if ((uhi_manta_report[1][i] >> j) & 1)
					{
						if (dimOnBOTH == 1) //turn it on, baby
						{
							uhi_manta_report_firstEdition[i]  |= 1 << j;
						}
						else  //turn it off, baby
						{
							uhi_manta_report_firstEdition[i]  &= ~(1 << j);
						}
					}

					
					else
					{
						if (dimOnRED == 1) //turn it on, baby
						{
							uhi_manta_report_firstEdition[i]  |= 1 << j;
						}
						else  //turn it off, baby
						{
							uhi_manta_report_firstEdition[i]  &= ~(1 << j);
						}
					}
				}
				
				

			}
			else
			{
				if ((uhi_manta_report[1][i] >> j) & 1) //turn it on, baby
				{
					uhi_manta_report_firstEdition[i]  |= 1 << j;
				}
				else  //turn it off, baby
				{
					uhi_manta_report_firstEdition[i]  &= ~(1 << j);
				}
			}
		}
	}
	
	//function buttons
	// if RED is on with the buttons
	for (int j = 0; j < 4; j++)
	{
		
		//if RED is on
		if ((uhi_manta_report[1][6] >> (j+4)) & 1)
		{
			if (RED_JUST_OFF)
			{
				uhi_manta_report_firstEdition[6]  &= ~(1 << j);
			}
			
			else
			{
				//if both RED and AMBER are on - dim it darker
				if ((uhi_manta_report[1][6] >> j) & 1)
				{
					if (dimOnBOTH == 1) //turn it on, baby
					{
						uhi_manta_report_firstEdition[6]  |= 1 << j;
					}
					else  //turn it off, baby
					{
						uhi_manta_report_firstEdition[6]  &= ~(1 << j);
					}
				}

				
				else
				{
					if (dimOnRED == 1) //turn it on, baby
					{
						uhi_manta_report_firstEdition[6]  |= 1 << j;
					}
					else  //turn it off, baby
					{
						uhi_manta_report_firstEdition[6]  &= ~(1 << j);
					}
				}
			}
			uhi_manta_report[1][6]  &= ~(1 << (j + 4));  //clear the red bits in the 6th byte
			
		}
		else
		{
			if ((uhi_manta_report[1][6] >> j) & 1) //turn it on, baby
			{
				uhi_manta_report_firstEdition[6]  |= 1 << j;
			}
			else  //turn it off, baby
			{
				uhi_manta_report_firstEdition[6]  &= ~(1 << j);
			}
		}
	}
	for (int i = 0; i < 3; i++)
	{
		uhi_manta_report_firstEdition[i+7] = uhi_manta_report[1][i+7];
	}
	counterForRedDimming++;
	if (counterForRedDimming > 3)
	{
		counterForRedDimming = 0;
		dimOnRED = !dimOnRED;
	}
	counterForBothDimming++;
	if (counterForBothDimming > 9)
	{
		counterForBothDimming = 0;
		dimOnBOTH = !dimOnBOTH;
	}
}


//@}

//@}
