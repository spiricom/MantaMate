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
#include "uhi_hid_mouse.h"
#include "dip204.h"
#include "main.h"
#include <string.h>

#ifdef USB_HOST_HUB_SUPPORT
# error USB HUB support is not implemented on UHI mouse
#endif

/**
 * \ingroup uhi_hid_mouse_group
 * \defgroup uhi_hid_mouse_group_internal Implementation of UHI HID Mouse
 *
 * Class internal implementation
 * @{
 */

/**
 * \name Index in HID report for usual HID mouse events
 * @{
 */
#define UHI_HID_MOUSE_BTN        0
#define UHI_HID_MOUSE_MOV_X      1
#define UHI_HID_MOUSE_MOV_Y      2
#define UHI_HID_MOUSE_MOV_SCROLL 3
//@}

/**
 * \name Structure to store information about USB Device HID manta
 */
//@{
typedef struct {
	uhc_device_t *dev;
	usb_ep_t ep_in;
	uint8_t report_size;
	uint8_t *report;
	uint8_t report_btn_prev;
}uhi_hid_mouse_dev_t;

static uhi_hid_mouse_dev_t uhi_hid_mouse_dev = {
	.dev = NULL,
	.report = NULL,
	};

enum maps_t
{
	NO_MAP,
	WICKI_HAYDEN,
	HARMONIC
};


//@}


/**
 * \name Internal routines
 */
//@{
static void uhi_hid_mouse_start_trans_report(usb_add_t add);

static void uhi_hid_mouse_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);
		
static unsigned char calculateNoteStack(void);
unsigned short calculateDACvalue(unsigned int noteval);

//@}
		
		
unsigned long twelvetet[12] = {0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100};
unsigned long overtonejust[12] = {0, 111, 203, 316, 386, 498, 551, 702, 813, 884, 968, 1088};
unsigned long kora1[12] = {0, 185, 230, 325, 405, 498, 551, 702, 885, 930, 1025, 1105};
unsigned long meantone[12] = {0, 117, 193, 310, 386, 503, 579, 697, 773, 890, 966, 1083};
unsigned long werckmeister1[12] = {0, 90, 192, 294, 390, 498, 588, 696, 792, 888, 996, 1092};
unsigned long werckmeister3[12] = {0, 96, 204, 300, 396, 504, 600, 702, 792, 900, 1002, 1098};
	
unsigned int whmap[48] = {0,2,4,6,8,10,12,14,7,9,11,13,15,17,19,21,12,14,16,18,20,\
22,24,26,19,21,23,25,27,29,31,33,24,26,28,30,32,34,36,38,31,33,35,37,39,41,43,45};
unsigned int harmonicmap[48] = {0,4,8,12,16,20,24,28,7,11,15,19,23,27,31,35,10,14,\
18,22,26,30,34,38,17,21,25,29,33,37,41,45,20,24,28,32,36,40,44,48,27,31,35,39,43,47,51,55};
	
enum maps_t whichmap = NO_MAP;
unsigned long scaledoctaveDACvalue = 54612;
unsigned char tuning = 0;
signed char transpose = 0;
unsigned char octaveoffset = 0;

uint8_t butt_states[48];
static uint8_t pastbutt_states[48];
uint8_t sliders[4] = {0,0,0,0};
uint8_t pastsliders[4] = {0,0,0,0};
signed char notestack[48];
unsigned char numnotes = 0;
unsigned char currentnote = 0;
uint8_t amplitude = 0;
unsigned char lastButtVCA = 0; //0 if you want to turn this off

/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_hid_mouse_install(uhc_device_t* dev)
{
	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;

	if (uhi_hid_mouse_dev.dev != NULL)
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
					&& (ptr_iface->bInterfaceProtocol == HID_PROTOCOL_GENERIC) ) 
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
						notestack[i] = -1;
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

				Assert(((usb_ep_desc_t*)ptr_iface)->bEndpointAddress & USB_EP_DIR_IN);
				uhi_hid_mouse_dev.ep_in = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
				uhi_hid_mouse_dev.report_size =
						le16_to_cpu(((usb_ep_desc_t*)ptr_iface)->wMaxPacketSize);
				uhi_hid_mouse_dev.report = malloc(uhi_hid_mouse_dev.report_size);
			
				if (uhi_hid_mouse_dev.report == NULL) {
					Assert(false);
					return UHC_ENUM_MEMORY_LIMIT; // Internal RAM allocation fail
				}
			
				uhi_hid_mouse_dev.dev = dev;
				// All endpoints of all interfaces supported allocated
				return UHC_ENUM_SUCCESS;

			// Ignore descriptor
			default: break;
		}	
		Assert(conf_desc_lgt>=ptr_iface->bLength);
		conf_desc_lgt -= ptr_iface->bLength;
		ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
	}
	return UHC_ENUM_UNSUPPORTED; // No interface supported
}

void uhi_hid_mouse_enable(uhc_device_t* dev)
{
	if (uhi_hid_mouse_dev.dev != dev) 
		return;  // No interface to enable

	// Init value
	uhi_hid_mouse_dev.report_btn_prev = 0;
	uhi_hid_mouse_start_trans_report(dev->address);
	UHI_HID_MOUSE_CHANGE(dev, true);
}

void uhi_hid_mouse_uninstall(uhc_device_t* dev)
{
	if (uhi_hid_mouse_dev.dev != dev) 
		return; // Device not enabled in this interface

	uhi_hid_mouse_dev.dev = NULL;
	Assert(uhi_hid_mouse_dev.report!=NULL);
	free(uhi_hid_mouse_dev.report);
	UHI_HID_MOUSE_CHANGE(dev, false);
}

/**
 * \brief Starts the reception of the HID mouse report
 *
 * \param add   USB address to use
 */
static void uhi_hid_mouse_start_trans_report(usb_add_t add)
{
	// Start transfer on interrupt endpoint IN
	uhd_ep_run(add, uhi_hid_mouse_dev.ep_in, true, uhi_hid_mouse_dev.report,
			uhi_hid_mouse_dev.report_size, 0, uhi_hid_mouse_report_reception);
}

/**
 * \brief Decodes the HID mouse report received
 *
 * \param add           USB address used by the transfer
 * \param status        Transfer status
 * \param nb_transfered Number of data transfered
 */
static void uhi_hid_mouse_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered)
{
	uint8_t i;
	unsigned short val;
	UNUSED(ep);

	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_hid_mouse_start_trans_report(add);
		return; // HID mouse transfer restart
	}

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
		return; // HID mouse transfer aborted
	}
	// Decode buttons
	for(i=0; i<52; i++)
		butt_states[i] = uhi_hid_mouse_dev.report[i+1] + 0x80;		
	for(i=0; i<4; i++)
		sliders[i] = uhi_hid_mouse_dev.report[i+53] + 0x80;
		
    i = 0;
	
	while(i < 52 && butt_states[i] == 0)
		i++;
		
	if(i < 52)
	{
		dip204_set_cursor_position(1,1);
		dip204_write_string("                ");
		dip204_set_cursor_position(1,1);
		dip204_printf_string("b: %u = %u",i+1,butt_states[i]);
		dip204_hide_cursor();
		UHI_HID_MOUSE_EVENT_BTN_LEFT(1);
	}
	else 
	{
		dip204_set_cursor_position(1,1);
		dip204_write_string("              ");
		UHI_HID_MOUSE_EVENT_BTN_LEFT(0);
	}
	

	i++;
	
	while(i < 52 && butt_states[i] == 0)
		i++;
	
	if(i < 52)
	{
		dip204_set_cursor_position(1,2);
		dip204_write_string("         ");
		dip204_set_cursor_position(1,2);
		dip204_printf_string("a");
		dip204_hide_cursor();
		UHI_HID_MOUSE_EVENT_BTN_RIGHT(1);
	}
	else UHI_HID_MOUSE_EVENT_BTN_RIGHT(0);
	
	if((sliders[0] != pastsliders[0] && sliders[0] != 255) || (sliders[1] != pastsliders[1] && sliders[1] != 255))
	{
		val = (sliders[0] + (sliders[1] << 8)) & 0xFFF;/*
		dip204_set_cursor_position(1,3);
		dip204_write_string("                    ");
		dip204_set_cursor_position(1,3);
		dip204_printf_string("slider: %u = %u",1,val);
		dip204_hide_cursor();*/
		dacsend(0,2,val);
	}
	
	if((sliders[2] != pastsliders[2] && sliders[2] != 255) || (sliders[3] != pastsliders[3] && sliders[3] != 255))
	{
		val = (sliders[2] + (sliders[3] << 8)) & 0xFFF;/*
		dip204_set_cursor_position(1,4);
		dip204_write_string("                    ");
		dip204_set_cursor_position(1,4);
		dip204_printf_string("slider: %u = %u",2,val);
		dip204_hide_cursor();*/
		dacsend(2,2,val);
	}
	
	if(calculateNoteStack())
	{
		val  = calculateDACvalue((unsigned int)currentnote);
		DAC16Send(2, val);
	} uhi_hid_mouse_start_trans_report(add);
}

unsigned char calculateNoteStack(void)
{
	unsigned char i, j, k;
	unsigned char changed = 0;
	//unsigned char polynum = 1;
	//unsigned char polyVoiceNote[4];
	//unsigned char polyVoiceBusy[4];
	//unsigned char changevoice[4];
	unsigned char notehappened = 0;
	unsigned char noteoffhappened = 0;
	
	//unsigned char voicefound = 0;
	//unsigned char voicecounter = 0;
	//unsigned char alreadythere = 0;
	signed char checkstolen = -1;

	//ADDING A NOTE
	//first figure out how many notes are currently in the stack
	// next, take the last note in the stack and copy it into the position one index number past it
	// now, do that for each note as you go down the list
	// when you get the index number 0, after copying it, put the new note in it's place

	//REMOVING A NOTE
	//first, find the note in the stack
	//then, remove it
	//move everything to the right of it (if it's not negative 1) one index number less
	//replace the last position with -1

	//create a stack that stores the currently touched notes in order they were touched
	for (i = 0; i < 48; i++)
	{
		checkstolen = -1;
		//if the current sensor value of a key is positive and it was zero on last count
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			//it's a note-on -- add it to the monophonic stack
			changed = 1;
			if(numnotes == 0)		
				dacsend(1,2,0xFFF);

			//first move notes that are already in the stack one position to the right
			for (j = numnotes; j > 0; j--)
				notestack[j] = notestack[(j - 1)];

			//then, insert the new note into the front of the stack
			notestack[0] = i;

			//also, assign a new polyphony voice to the note on for the polyphony handling
			/*voicefound = 0;
			voicecounter = 0;
			for (j = 0; j < polynum; j++)
			{
				if ((polyVoiceBusy[j] == 0) && (voicefound == 0))
				{
					polyVoiceNote[j] = i;  // store the new note in a voice if a voice is free - store it without the offset and transpose (just 0-31).
					polyVoiceBusy[j] = 1;
					changevoice[j] = 1;
					voicefound = 1;
				}	
				voicecounter++;
				
				if ((voicecounter == polynum) && (voicefound == 0))
				{
					polyVoiceNote[(polynum - 1)] = i;  // store the new note in a voice if a voice is free - store it without the offset and transpose (just 0-31).
					polyVoiceBusy[(polynum - 1)] = 1;
					changevoice[(polynum - 1)] = 1;
					voicefound = 1;
				}
			}*/
			numnotes++;
			notehappened = 1;
			currentnote = notestack[0];
		}

		else if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			//it's a note-off, remove it from the stack
			changed = 1;
			//go through the notes that are currently held down to find the one that released
			for (j = 0; j < numnotes; j++)
			{
				//if it's the note that just got released
				if (notestack[j] == i)
				{
					for (k = 0; k < (numnotes - j); k++)
					{
						notestack[k + j] = notestack[k + j + 1];
						//if it's the last one, write negative 1 beyond it (it's already been copied to the position to the left of it)
						if (k == ((numnotes - j) - 1))
							notestack[k + j + 1] = -1;
					}
				}
			}

			//also, remove that note from the polyphony array if it's there.
			/*for (j = 0; j < polynum; j++)
			{
				if (polyVoiceNote[j] == i)
				{
					polyVoiceBusy[j] = 0;
					changevoice[j] = 1;
					checkstolen = j;
				}
			}*/
			
			//remove it from the notestack and decrement numnotes
			numnotes--;
			notehappened = 1;
			noteoffhappened = 1;
			if (notestack[0] != -1)
				currentnote = notestack[0];

			// if we removed a note from the polyphony array
			/*if (checkstolen != -1)
			{
				//now check if there are any polyphony voices waiting that got stolen.
				for (j = 0; j < numnotes; j++)
				{
					//if you find a held note in the notestack
					if (notestack[j] != -1)
					{
						//check if it has no voice associated with it
						alreadythere = 0;
						for (k = 0; k < polynum; k++)
						{
							if ((polyVoiceNote[k] == notestack[j]) && (polyVoiceBusy[k] == 1))
								alreadythere = 1;
						}
						// if you didn't find it, use the voice that was just released to sound it.
						if (alreadythere == 0)
						{
							polyVoiceNote[checkstolen] = notestack[j];
							polyVoiceBusy[checkstolen] = 1;
							changevoice[checkstolen] = 1;
							notehappened = 1;
						}
					}
				}
			}*/
			
			if(numnotes == 0)
				dacsend(1,2,0);
			
		}
		// update the past keymap array (stores the previous values of every key's sensor reading)
		pastbutt_states[i] = butt_states[i];

	}
	
	// volume control
	amplitude = 0;		
	if (lastButtVCA == 1)
	{
		if(numnotes > 0)
			amplitude = butt_states[notestack[0]];
	}
	else
	{
		for(j=0; j<numnotes; j++)
		{
			uint8_t val = butt_states[notestack[j]];
			if(val > amplitude)
			amplitude = val;
		}
	}
	
	dacsend(3,2,amplitude<<4);/*
	if(amplitude != 0)
	{
		dip204_set_cursor_position(1,2);
		dip204_printf_string("                    ");
		dip204_set_cursor_position(1,2);
		dip204_printf_string("amplitude: %u", amplitude);
		dip204_hide_cursor();
	}*/
	return changed;
}

unsigned short calculateDACvalue(unsigned int noteval)
{
	signed long pitchclass;
	unsigned long templongnote = 0;
	unsigned int virtualnote;
	unsigned long templongoctave;
	unsigned short DAC1val;
	unsigned int note;
	
	switch(whichmap)
	{
		case WICKI_HAYDEN: note = whmap[noteval]; break;    // wicki-hayden
		case HARMONIC: note = harmonicmap[noteval]; break;  // harmonic
		default: note = noteval; break;                     // no map
	}
	
	//templong = ((noteval + offset + transpose) * 54612);  // original simple equal temperament
	pitchclass = ((note + transpose + 21) % 12);  // add 21 to make it positive and centered on C
	virtualnote = (note + 13 + transpose - pitchclass);
	if (tuning == 0)
		templongnote = (twelvetet[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 1)
		templongnote = (overtonejust[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 2)
		templongnote = (kora1[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 3)
		templongnote = (meantone[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 4)
		templongnote = (werckmeister1[pitchclass] * scaledoctaveDACvalue);
	else if (tuning == 5)
		templongnote = (werckmeister3[pitchclass] * scaledoctaveDACvalue);
	
	templongnote = (templongnote / 10000);
	templongoctave = ((virtualnote + octaveoffset) * scaledoctaveDACvalue);
	templongoctave = (templongoctave / 100);
	DAC1val = templongnote + templongoctave;
	return DAC1val*2;
}

//@}

//@}
