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
		
static void processKeys(void);
/**
 * \brief Send the report
 *
 * \return \c 1 if send on going, \c 0 if delay.
 */
static bool uhi_midi_send_report(void);

/**
 * \brief Callback called when the report is sent
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finish
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param nb_sent    number of data transfered
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
static void uhi_midi_report_sent(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);
//@}

uint8_t butt_states[48];
static uint8_t pastbutt_states[48];
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
static uint8_t lights[2*HEX_BYTES+SLIDER_BYTES+FUNTION_BYTES];

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
					&& (dev->dev_desc.idProduct == 0x2424))
				{
					int i;
					// USB HID Mouse interface found
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
				else b_iface_supported = false; // Stop allocation endpoint(s)
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
	memset(lights,0,HEX_BYTES*2+SLIDER_BYTES);
	lights[0] = 0x01;
	lights[9] = 0x03;
	lights[6] = 0x03;
	lights[7] = 0x0A;
	manta_light_LED(lights);
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

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
		return; // HID mouse transfer aborted
	}
	
	// Decode buttons
	for(i=0; i<48; i++)
		butt_states[i] = uhi_hid_manta_dev.report[i+1] + 0x80;	
	for(i=0; i<4; i++)
		sliders[i] = uhi_hid_manta_dev.report[i+53] + 0x80;
		
    i = 0;
	
	while(i < 48 && butt_states[i] == 0)
		i++;
		
	if(i < 48)
	{
		lcd_clear_line(1);
		dip204_printf_string("button: %u = %u",i+1,butt_states[i]);
		dip204_hide_cursor();
		UHI_HID_MOUSE_EVENT_BTN_LEFT(1);
	}
	else
		UHI_HID_MOUSE_EVENT_BTN_LEFT(0);
		
	i++;
	
	while(i < 48 && butt_states[i] == 0)
		i++;
	
	if(i < 48)
	{
		lcd_clear_line(2);
		dip204_printf_string("button: %u = %u",i+1,butt_states[i]);
		dip204_hide_cursor();
		UHI_HID_MOUSE_EVENT_BTN_RIGHT(1);
	}
	else UHI_HID_MOUSE_EVENT_BTN_RIGHT(0);
	
	if((sliders[0] != pastsliders[0] && sliders[0] != 255) || (sliders[1] != pastsliders[1] && sliders[1] != 255))
	{
		val = (sliders[0] + (sliders[1] << 8)) & 0xFFF;/*
		dip204_clear_line(3);
		dip204_printf_string("slider: %u = %u",1,val);
		dip204_hide_cursor();*/
		dacsend(0,2,val);
	}
	
	if((sliders[2] != pastsliders[2] && sliders[2] != 255) || (sliders[3] != pastsliders[3] && sliders[3] != 255))
	{
		val = (sliders[2] + (sliders[3] << 8)) & 0xFFF;/*
		dip204_clear_line(4);
		dip204_printf_string("slider: %u = %u",2,val);
		dip204_hide_cursor();*/
		dacsend(2,2,val);
	}
	
	processKeys();
	//mantaVol(butt_states);
	/*val  = calculateDACvalue();
	DAC16Send(1, val);*/
	noteOut();
	
	uhi_hid_manta_start_trans_report(add);
}

static void processKeys(void)
{
	uint8_t i;

	for (i = 0; i < 48; i++)
	{
		//if the current sensor value of a key is positive and it was zero on last count
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			addNote(i,butt_states[i]);
		}

		else if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			removeNote(i);	
		}

		// update the past keymap array (stores the previous values of every key's sensor reading)
		pastbutt_states[i] = butt_states[i];
	}
}

static bool uhi_midi_send_report(void)
{
	if (uhi_manta_report_trans_ongoing)
	return false;	// Transfer on going then send this one after transfer complete

	// Copy report on other array used only for transfer
	memcpy(uhi_manta_report_trans, uhi_manta_report, UHI_MANTA_EP_OUT_SIZE);
	uhi_manta_b_report_valid = false;

	// Send report
	return uhi_manta_report_trans_ongoing =
	uhd_ep_run(uhi_hid_manta_dev.dev->address,uhi_hid_manta_dev.ep_out,false,uhi_manta_report_trans,UHI_MANTA_EP_OUT_SIZE,0,uhi_midi_report_sent);
}

static void uhi_midi_report_sent(usb_add_t add, usb_ep_t ep,
		uhd_trans_status_t status, iram_size_t nb_transfered)
{
	UNUSED(ep);
	UNUSED(status);
	UNUSED(nb_transfered);
	// Valid report sending
	uhi_manta_report_trans_ongoing = false;
	if (uhi_manta_b_report_valid) {
		// Send new valid report
		uhi_midi_send_report();
	}
}

bool manta_light_LED(uint8_t *lights)
{
	uint8_t i;
	irqflags_t flags = cpu_irq_save();


	// amber lights + function button LEDs
	for(i=0; i<HEX_BYTES+FUNTION_BYTES+SLIDER_BYTES; i++)
		uhi_manta_report[i] = lights[i];
	// control byte
	uhi_manta_report[MANTA_CONTROL] = BUTTON_CTRL;
	
	// red lights	
	for(; i<SLIDER_BYTES+2*HEX_BYTES; i++)
	{
		// don't turn on red if amber already on beacuse too much current!!!! explosions
		uhi_manta_report[i+1] = lights[i] & (~lights[i-SLIDER_BYTES-HEX_BYTES-FUNTION_BYTES]);
	}
	
	
	// Valid and send report
	uhi_manta_b_report_valid = true;
	uhi_midi_send_report();

	cpu_irq_restore(flags);
	return true;
}
//@}

//@}
