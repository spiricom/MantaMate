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
#include "uhi_hid_joystick.h"
#include <string.h>
#include "main.h"
#include <fastmath.h>
#include "manta_keys.h"
#include "7Segment.h"
#include <time.h>

#ifdef USB_HOST_HUB_SUPPORT
//# error USB HUB support is not implemented on UHI mouse
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


uint8_t UHI_HID_KEYBOARD_DATA;
uint8_t UHI_HID_KEYBOARD_DATA_SIZE;
uint8_t keyboard_bytes[5];
uint8_t prev_keyboard_bytes[5];
uint8_t isKeyboard = 0;
uint8_t possible_keys[3][12] = {{95, 96, 97, 92, 93, 94, 89, 90, 91, 98, 99, 88},{30,31,32,33,34,35,36,37,38,39,40,45},{0,0,0,0,0,0,0,0,0,0,0}};

uint8_t alreadyFoundEndpoint = 0;
uint8_t alreadyFoundInterface = 0;

//@}

/**
 * \name Structure to store information about USB Device HID mouse
 */
//@{
	


//@}


/**
 * \name Internal routines
 */
//@{
static void get_report_descriptor(void);
static void parse_report_descriptor(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t nb_transfered);
static void uhi_hid_joy_start_trans_report(usb_add_t add);
static void uhi_hid_joy_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);
		
static void findDataOffset(void);
static void ResetParser(void);
int find_highest_bit(int);
//@}

// initialize global report parser
static hid_report_parser_t hid_report_parser = {
	.pos = 0,
	.count = 0,
	.nobject = 0,
	.nreport = 0,
	.usage_size = 0,
};

// initialize global hid joystick struct
static uhi_hid_joy_dev_t uhi_hid_joy_dev = {
	.dev = NULL,
	.report = NULL,
};

const char ItemSize[4]={0,1,2,4};

/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_hid_joy_install(uhc_device_t* dev)
{
	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;
	int i;
	alreadyFoundEndpoint = 0;
	alreadyFoundInterface = 0;
	
	if (MidiDeviceFound == TRUE)
	{
		return UHC_ENUM_UNSUPPORTED;
	}
	if (uhi_hid_joy_dev.dev != NULL) {
		
		return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
	}
	conf_desc_lgt = le16_to_cpu(dev->conf_desc->wTotalLength);
	ptr_iface = (usb_iface_desc_t*) dev->conf_desc;
	b_iface_supported = false;
	
	while(conf_desc_lgt) {
		switch (ptr_iface->bDescriptorType) {

		case USB_DT_INTERFACE: 
			if (ptr_iface->bInterfaceProtocol == HID_PROTOCOL_KEYBOARD)
			{
				isKeyboard = 1;
			}
			if ((ptr_iface->bInterfaceClass   == HID_CLASS)
			&& ((ptr_iface->bInterfaceProtocol == HID_PROTOCOL_GENERIC) || (ptr_iface->bInterfaceProtocol == HID_PROTOCOL_KEYBOARD))
			&& (dev->dev_desc.idProduct != 0x2424)) { // and it's not a manta
				// USB HID Joystick interface found
				// Start allocation endpoint(s)
				b_iface_supported = true;
				uhi_hid_joystick_clear_struct();
			}
			else {
				// Stop allocation endpoint(s)
				b_iface_supported = false;
			}
			break;
			
			
		
		case USB_DT_HID:
			if (!b_iface_supported) {
				break;
			}
			//how many descriptors and what type, make some space to store
			uhi_hid_joy_dev.numDesc = ((usb_hid_descriptor_t*) ptr_iface)->bNumDescriptors;
			uhi_hid_joy_dev.DescType = (uint8_t *) malloc(uhi_hid_joy_dev.numDesc);
			uhi_hid_joy_dev.DescSize = (uint16_t *) malloc(2*uhi_hid_joy_dev.numDesc);
			for (i = 0; i < uhi_hid_joy_dev.numDesc; i++)
			{
				uhi_hid_joy_dev.DescType[i] = ((usb_hid_descriptor_t*) ptr_iface)->bRDescriptorType;
				uhi_hid_joy_dev.DescSize[i] = (((usb_hid_descriptor_t*) ptr_iface)->wDescriptorLength)>>8;
			}
			break;
		
		case USB_DT_ENDPOINT:
			//  Allocation of the endpoint
			if (!b_iface_supported) {
				break;
			}
			if (!alreadyFoundEndpoint)
			{
				//only pay attention to input endpoints, ignore output endpoints
				if (((usb_ep_desc_t*) ptr_iface)->bEndpointAddress & USB_EP_DIR_IN)
				{
					if (!uhd_ep_alloc(dev->address, (usb_ep_desc_t*) ptr_iface)) {
						return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail
					}
								

					Assert(((usb_ep_desc_t*) ptr_iface)->bEndpointAddress & USB_EP_DIR_IN);
					uhi_hid_joy_dev.ep_in = ((usb_ep_desc_t*) ptr_iface)->bEndpointAddress;
					uhi_hid_joy_dev.report_size =
					le16_to_cpu(((usb_ep_desc_t*) ptr_iface)->wMaxPacketSize);
					uhi_hid_joy_dev.report = malloc(uhi_hid_joy_dev.report_size);
					if (uhi_hid_joy_dev.report == NULL) {
						Assert(false);
						return UHC_ENUM_MEMORY_LIMIT; // Internal RAM allocation fail
					}
					alreadyFoundEndpoint = 1;
				}
			}


		default:
			// Ignore descriptor
			break;
		}
		Assert(conf_desc_lgt>=ptr_iface->bLength);
		conf_desc_lgt -= ptr_iface->bLength;
		ptr_iface = (usb_iface_desc_t*) ((uint8_t*) ptr_iface + ptr_iface->bLength);
	}
	if (!b_iface_supported) {
		return UHC_ENUM_UNSUPPORTED; // No interface supported
	}
	uhi_hid_joy_dev.dev = dev;
	// All endpoints of all interfaces supported allocated
	
	return UHC_ENUM_SUCCESS;
}

// called after uhi_hid_joy_install has run checks and after space has been allocated
// this appears to step through the report, allocating space in hid_report_parser.reportDesc
static void get_report_descriptor()
{
	usb_setup_req_t req;
	int i;
	
	for (i = 0; i < uhi_hid_joy_dev.numDesc; i++)
	{	
		ResetParser();
		
		req.bmRequestType = USB_REQ_RECIP_INTERFACE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
		req.bRequest = USB_REQ_GET_DESCRIPTOR;
		req.wValue = (USB_DT_HID_REPORT << 8);
		req.wIndex = 0;
		req.wLength = uhi_hid_joy_dev.DescSize[i] + 0x40;
		
		hid_report_parser.reportDesc_size = uhi_hid_joy_dev.DescSize[i];
		
		if (hid_report_parser.reportDesc != NULL)
			hid_report_parser.reportDesc = (uint8_t *) realloc(hid_report_parser.reportDesc, uhi_hid_joy_dev.DescSize[i]);
		else hid_report_parser.reportDesc = (uint8_t *) malloc(uhi_hid_joy_dev.DescSize[i]);

		// After a USB reset, the reallocation is required
		if (!uhd_setup_request(1, &req, (uint8_t *) hid_report_parser.reportDesc, 
			uhi_hid_joy_dev.DescSize[i], NULL, parse_report_descriptor)) 
		{
			//uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
			//MEMORY_printf_string("ERROR");
			return;
		}
	}
}

static void ResetLocalState(hid_report_parser_t * hid_report_parser)
{
	hid_report_parser->usage_size = 0;
	memset(hid_report_parser->usage_tab,0,sizeof(hid_report_parser->usage_tab));
}

uint8_t* GetReportOffset(const uint8_t ireport, const uint8_t ReportType)
{
	uint16_t pos = 0;
	while(pos < MAX_REPORT && hid_report_parser.offset_tab[pos][1] != 0)
	{
		if(hid_report_parser.offset_tab[pos][0] == ireport && hid_report_parser.offset_tab[pos][1] == ReportType)
			return &hid_report_parser.offset_tab[pos][2];
		pos++;
	}
	if(pos < MAX_REPORT)
	{
		/* Increment Report count */
		hid_report_parser.nreport++;
		hid_report_parser.offset_tab[pos][0] = ireport;
		hid_report_parser.offset_tab[pos][1] = ReportType;
		hid_report_parser.offset_tab[pos][2] = 0;
		return &hid_report_parser.offset_tab[pos][2];
	}
	return NULL;
}


static void ResetParser(void)
{
	hid_report_parser.pos=0;
	hid_report_parser.count=0;
	hid_report_parser.nobject=0;
	hid_report_parser.nreport=0;

	hid_report_parser.usage_size=0;
	memset(hid_report_parser.usage_tab,0,sizeof(hid_report_parser.usage_tab));

	memset(hid_report_parser.offset_tab,0,sizeof(hid_report_parser.offset_tab));
	memset(&hid_report_parser.data,0,sizeof(hid_report_parser.data));
	
	//memset(&NUM_BUTTS,0,sizeof(NUM_BUTTS));
}

static void findDataOffset(void)
{
	uint8_t offset = hid_report_parser.data.offset;
	switch(hid_report_parser.data.path.node[hid_report_parser.data.path.size-1].u_page)
	{
		case GENERIC_DESKTOP:
			switch(hid_report_parser.data.path.node[hid_report_parser.data.path.size-1].usage)
			{
				case USAGE_X: case USAGE_Y: case USAGE_Z: case USAGE_RX: case USAGE_RY: case USAGE_RZ: case USAGE_HAT_SWITCH: case USAGE_SLIDER: case USAGE_DIAL: case USAGE_WHEEL:
					if (find_highest_bit(hid_report_parser.data.log_max) == 3)
					{
						//then it's a DPAD, not a joystick - store it as such
						myJoystick.dPads[myJoystick.numDPads].offset = offset;
						myJoystick.dPads[myJoystick.numDPads].size = hid_report_parser.data.size;
						myJoystick.dPads[myJoystick.numDPads].logical_max_bits = 3;
						
						myJoystick.numDPads++;
						break;
					}
					else
					{
						myJoystick.joyAxes[myJoystick.numJoyAxis].offset = offset;
						if (hid_report_parser.data.size == 1)
						{
							myJoystick.joyAxes[myJoystick.numJoyAxis].size = 8;
							myJoystick.joyAxes[myJoystick.numJoyAxis].logical_max_bits = 8;
						}
						else
						{
							myJoystick.joyAxes[myJoystick.numJoyAxis].size = hid_report_parser.data.size;
							myJoystick.joyAxes[myJoystick.numJoyAxis].logical_max_bits = find_highest_bit(hid_report_parser.data.log_max);
						}
					
						myJoystick.numJoyAxis++;
						break;
					}

				
					/*   this is the code to default to 8 bits if it doesn't actually report the size
					UHI_HID_JOY_SLIDER = offset;
					SLIDER_SIZE = hid_report_parser.data.size;
					//fixes problem where data size defaults to 1 if not defined, we think that 8 bits must be defaults
					if (SLIDER_SIZE == 1)
					{
						SLIDER_SIZE = 8;
						SLIDER_LOGICAL_MAX_BITS = 8;
					} else
					{
						SLIDER_LOGICAL_MAX_BITS = find_highest_bit(hid_report_parser.data.log_max);
					}
					*/
					
				case KEYBOARD:
					//would be nice to figure out how to parse this
					UHI_HID_KEYBOARD_DATA = offset;
					UHI_HID_KEYBOARD_DATA_SIZE = hid_report_parser.data.size;
					break;
				default:
					break;
			}
			break;
		case SIMULATION:
			switch(hid_report_parser.data.path.node[hid_report_parser.data.path.size-1].usage)
			{
				case USAGE_THROTTLE:
					myJoystick.joyAxes[myJoystick.numJoyAxis].offset = offset;
					myJoystick.joyAxes[myJoystick.numJoyAxis].size = hid_report_parser.data.size;
					myJoystick.joyAxes[myJoystick.numJoyAxis].logical_max_bits = find_highest_bit(hid_report_parser.data.log_max);
					myJoystick.numJoyAxis++;
					break;
				default:
					break;
			}
		case BUTTON:
			myJoystick.joyButtons[myJoystick.numJoyButton].offset = offset;
			myJoystick.joyButtons[myJoystick.numJoyButton].count = hid_report_parser.report_count;
			myJoystick.joyButtons[myJoystick.numJoyButton].size =  hid_report_parser.data.size;
			myJoystick.numJoyButton++;
			/*
			if(NUM_BUTTS[ibutt] == 0 && hid_report_parser.report_count == hid_report_parser.count)
			{
				UHI_HID_JOY_BUTT[ibutt] = offset;
				NUM_BUTTS[ibutt] = hid_report_parser.report_count;
				BUTT_SIZE[ibutt++] = hid_report_parser.data.size;
			}
			*/
			break;
			

		default:
			break;	
	}
}



static void parse_report_descriptor(usb_add_t add,  uhd_trans_status_t status,
	uint16_t nb_transfered)
{	
	while(hid_report_parser.pos < hid_report_parser.reportDesc_size)
	{
		/* Get new hid_report_parser.item if current hid_report_parser.count is empty */
		if(hid_report_parser.count == 0)
		{
			hid_report_parser.item = hid_report_parser.reportDesc[hid_report_parser.pos++];
			hid_report_parser.val = 0;
			int i;
			unsigned long valTmp=0;
			for (i=0;i<ItemSize[hid_report_parser.item & SIZE_MASK];i++)
			{
				memcpy(&valTmp, &hid_report_parser.reportDesc[(hid_report_parser.pos)+i], 1);
				hid_report_parser.val += valTmp >> ((3-i)*8);
				valTmp=0;
			}
			/* Pos on next item */
			hid_report_parser.pos += ItemSize[hid_report_parser.item & SIZE_MASK];
		}
		
		switch(hid_report_parser.item & ITEM_MASK)
		{
			case ITEM_UPAGE :
				/* Copy u_page in Usage stack */
				hid_report_parser.u_page = (uint16_t) hid_report_parser.val;
				hid_report_parser.usage_tab[hid_report_parser.usage_size].u_page = hid_report_parser.u_page;
				break;
			case ITEM_USAGE :
			{
				// hacky fix to weird report descriptor
				if(hid_report_parser.u_page == BUTTON)
					hid_report_parser.u_page = GENERIC_DESKTOP;
					
				/* Copy global or local u_page if any, in Usage stack */
				if((hid_report_parser.item & SIZE_MASK) > 2)
					hid_report_parser.usage_tab[hid_report_parser.usage_size].u_page = (uint16_t) (hid_report_parser.val >> 16);
				else
					hid_report_parser.usage_tab[hid_report_parser.usage_size].u_page = hid_report_parser.u_page;
				
				/* Copy Usage in Usage stack */
				hid_report_parser.usage_tab[hid_report_parser.usage_size].usage = (uint16_t) (hid_report_parser.val & 0xFFFF);

				/* Increment Usage stack size */
				hid_report_parser.usage_size++;

				break;
			}
			case ITEM_COLLECTION :
			{
				/* Get u_page/Usage from UsageTab and store them in hid_report_parser.data.Path */
				hid_report_parser.data.path.node[hid_report_parser.data.path.size].u_page = hid_report_parser.usage_tab[0].u_page;
				hid_report_parser.data.path.node[hid_report_parser.data.path.size].usage = hid_report_parser.usage_tab[0].usage;
				hid_report_parser.data.path.size++;

				/* Unstack u_page/Usage from UsageTab (never remove the last) */
				if(hid_report_parser.usage_size > 0)
				{
					uint8_t j=0;
					while(j < hid_report_parser.usage_size)
					{
						hid_report_parser.usage_tab[j].usage = hid_report_parser.usage_tab[j+1].usage;
						hid_report_parser.usage_tab[j].u_page = hid_report_parser.usage_tab[j+1].u_page;
						j++;
					}
					/* Remove Usage */
					hid_report_parser.usage_size--;
				}

				/* Get Index if any */
				if(hid_report_parser.val >= 0x80)
				{
					hid_report_parser.data.path.node[hid_report_parser.data.path.size].u_page = 0xFF;
					hid_report_parser.data.path.node[hid_report_parser.data.path.size].usage = hid_report_parser.val & 0x7F;
					hid_report_parser.data.path.size++;
				}
				ResetLocalState(&hid_report_parser); 
				break;
			}
			case ITEM_END_COLLECTION :
			{
				hid_report_parser.data.path.size--;
				/* Remove Index if any */
				if(hid_report_parser.data.path.node[hid_report_parser.data.path.size].u_page == 0xFF)
					hid_report_parser.data.path.size--;
				ResetLocalState(&hid_report_parser); 
				break;
			}
			case ITEM_FEATURE :
			case ITEM_INPUT :
			case ITEM_OUTPUT :
			{
				// An object was found

				// Increment object count
				hid_report_parser.nobject++;

				// Get new hid_report_parser.count from global value
				if(hid_report_parser.count == 0)
				{
					hid_report_parser.count = hid_report_parser.report_count;
				}

				// Get u_page/Usage from UsageTab and store them in hid_report_parser.data.Path
				hid_report_parser.data.path.node[hid_report_parser.data.path.size].u_page = hid_report_parser.usage_tab[0].u_page;
				hid_report_parser.data.path.node[hid_report_parser.data.path.size].usage = hid_report_parser.usage_tab[0].usage;
				hid_report_parser.data.path.size++;

				// Unstack u_page/Usage from UsageTab (never remove the last)
				if(hid_report_parser.usage_size > 0)
				{
					int j = 0;
					while(j < hid_report_parser.usage_size)
					{
						hid_report_parser.usage_tab[j].u_page = hid_report_parser.usage_tab[j+1].u_page;
						hid_report_parser.usage_tab[j].usage = hid_report_parser.usage_tab[j+1].usage;
						j++;
					}
					// Remove Usage
					hid_report_parser.usage_size--;
				}

				// Copy data type
				hid_report_parser.data.type = (uint8_t) (hid_report_parser.item & ITEM_MASK);

				// Copy data attribute
				hid_report_parser.data.attribute = (uint8_t) hid_report_parser.val;

				// Store offset
				hid_report_parser.data.offset = *GetReportOffset(hid_report_parser.data.ireport, (uint8_t) (hid_report_parser.item & ITEM_MASK));

				// Get Object in pData
				// --------------------------------------------------------------------------
				//memcpy(&pData[hid_report_parser.nobject - 1], &hid_report_parser.data, sizeof(hid_data));
				findDataOffset();
				// --------------------------------------------------------------------------

				// Increment Report Offset
				*GetReportOffset(hid_report_parser.data.ireport, (uint8_t) (hid_report_parser.item & ITEM_MASK)) += hid_report_parser.data.size;

				// Remove path last node
				hid_report_parser.data.path.size--;

				// Decrement count
				hid_report_parser.count--;
				if (hid_report_parser.count == 0) {
					ResetLocalState(&hid_report_parser);
				}
				break;
			}
			case ITEM_REP_ID :
				hid_report_parser.data.ireport = (uint8_t) hid_report_parser.val;
				break;
			case ITEM_REP_SIZE :
				hid_report_parser.data.size = (uint8_t) hid_report_parser.val;
				break;
			case ITEM_REP_COUNT :
				hid_report_parser.report_count = (uint8_t) hid_report_parser.val;
				break;
			case ITEM_UNIT_EXP :
				hid_report_parser.data.unit_exp = (int8_t) hid_report_parser.val;
				// Convert 4 bits signed value to 8 bits signed value
				if (hid_report_parser.data.unit_exp > 7)
					hid_report_parser.data.unit_exp |= 0xF0;
				break;
			case ITEM_UNIT :
				hid_report_parser.data.unit = hid_report_parser.val;
				break;
			case ITEM_LOG_MIN :
				hid_report_parser.data.log_min = hid_report_parser.val;
				break;
			case ITEM_LOG_MAX :
				hid_report_parser.data.log_max = hid_report_parser.val;
				break;
			case ITEM_PHY_MIN :
				hid_report_parser.data.phy_min = hid_report_parser.val;
				break;
			case ITEM_PHY_MAX :
				hid_report_parser.data.phy_max = hid_report_parser.val;
				break;
			case ITEM_LONG :
				/* can't handle long items, but should at least skip them */
				hid_report_parser.pos += (uint8_t) (hid_report_parser.val & 0xff);
		}	
	}
}

void uhi_hid_joy_enable(uhc_device_t* dev)
{
	if (uhi_hid_joy_dev.dev != dev) {
		return;  // No interface to enable
	}
	get_report_descriptor();
	// Init value
	uhi_hid_joy_start_trans_report(dev->address);
	type_of_device_connected = JoystickConnected;
	
	clearDACoutputs();
	
	UHI_HID_JOY_CHANGE(dev, true);
	updatePreset();
}


void uhi_hid_joystick_clear_struct(void)
{
	for (int i = 0; i < MAX_AXES; i++)
	{
		myJoystick.joyAxes[i].logical_max_bits = 0;
		myJoystick.joyAxes[i].previous_value = 0;
		myJoystick.joyAxes[i].offset = 0;
		myJoystick.joyAxes[i].size = 0;
	}
	for (int i = 0; i < MAX_DPADS; i++)
	{
		myJoystick.dPads[i].logical_max_bits = 0;
		myJoystick.dPads[i].previous_value = 0;
		myJoystick.dPads[i].offset = 0;
		myJoystick.dPads[i].size = 0;
	}
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		myJoystick.joyButtons[i].count = 0;
		myJoystick.joyButtons[i].previous_value = 0;
		myJoystick.joyButtons[i].offset = 0;
		myJoystick.joyButtons[i].size = 0;
	}
	myJoystick.numJoyAxis = 0;
	myJoystick.numDPads = 0;
	myJoystick.numJoyButton = 0;
}

void uhi_hid_joy_uninstall(uhc_device_t* dev)
{
	if (uhi_hid_joy_dev.dev != dev) {
		return; // Device not enabled in this interface
	}
	uhi_hid_joy_dev.dev = NULL; 
	Assert(uhi_hid_joy_dev.report!=NULL);
	free(uhi_hid_joy_dev.report);
	free(uhi_hid_joy_dev.DescType);
	free(uhi_hid_joy_dev.DescSize);
	free(hid_report_parser.reportDesc);
	uhi_hid_joystick_clear_struct();
	UHI_HID_JOY_CHANGE(dev, false);
	no_device_mode_active = FALSE;
	type_of_device_connected = NoDeviceConnected;
	updatePreset();
	clearDACoutputs();
}
//@}

/**
 * \name Internal routines
 */
//@{

/**
 * \brief Starts the reception of the HID mouse report
 *
 * \param add   USB address to use
 */
static void uhi_hid_joy_start_trans_report(usb_add_t add)
{
	// Start transfer on interrupt endpoint IN
	// right now, this hack forces reception on endpoint 1 (129) because some devices with multiple endpoints would go to the last endpoint instead of 1.
	/*uhd_ep_run(add, 129, true, uhi_hid_joy_dev.report,
		uhi_hid_joy_dev.report_size, 0, uhi_hid_joy_report_reception);
		*/
	uhd_ep_run(add, uhi_hid_joy_dev.ep_in, true, uhi_hid_joy_dev.report,
			uhi_hid_joy_dev.report_size, 0, uhi_hid_joy_report_reception);
			
}

void keyboard_hack_grab(void) {
	
	//clear our the array
	for (int i = 0; i < 12; i++)
	{
		possible_keys[2][i] = 0;
	}
	//fill the array with the found keys
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 12; j++)
		{
			if ((uhi_hid_joy_dev.report[i] == possible_keys[0][j]) || (uhi_hid_joy_dev.report[i] == possible_keys[1][j]))
			{
				possible_keys[2][j] = 1;
			}
		}
	}
	//send out the proper outputs for the keys that were found
	for (int i = 0; i < 12; i++)
	{
		if (possible_keys[2][i] == 1)
		{
			sendDataToOutput(i, 0, 0xffff);
		}
		else
		{
			sendDataToOutput(i, 0, 0);
		}

	}
	
	
}



uint32_t findDataInReport(uint8_t size, uint8_t offset) {
	uint8_t total;
	uint8_t index;
	uint32_t state_new;
	uint8_t i;
	
	total = size;
	index = offset/8;
	state_new = (uhi_hid_joy_dev.report[index]>>(offset%8));  // add the first (partial) byte
	i = 8 - offset % 8;
	
	while(i <= total - 8)  // add a full byte
	{
		state_new += uhi_hid_joy_dev.report[++index] << i;
		i += 8;
	}
	if(i < total)  // add the last partial byte
	{
		state_new += (uhi_hid_joy_dev.report[++index] & (0xFF>>(8-(offset+size)%8)))<<i;
		i += (offset+size)%8;
	}
	
	return state_new;
}

int find_highest_bit(int val)
{
	unsigned int count = 1;
	while (val >>= 1)
		count++;
	return count;
}

/**
 * \brief Decodes the HID joystick report received. based on the hid mouse example by atmel
 *
 * \param add           USB address used by the transfer
 * \param status        Transfer status
 * \param nb_transfered Number of data transfered
 */
static void uhi_hid_joy_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered)
{
	
	UNUSED(ep);

	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_hid_joy_start_trans_report(add);
		return; // HID mouse transfer restart
	}

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
		return; // HID mouse transfer aborted
	}
	
	if (isKeyboard)
	{
		keyboard_hack_grab();	
	}
	else
	{
		uint16_t tempValue = 0;
		
		if (!joystickIgnoreAxes)
		{		
			axesOffset = myJoystick.numJoyAxis;
			for (int i = 0; i < myJoystick.numJoyAxis; i++)
			{

				if (myJoystick.joyAxes[i].logical_max_bits <= 16)
				{
					tempValue = ((findDataInReport(myJoystick.joyAxes[i].size, myJoystick.joyAxes[i].offset)) << (16-myJoystick.joyAxes[i].logical_max_bits));
				}
				else
				{
					tempValue = ((findDataInReport(myJoystick.joyAxes[i].size, myJoystick.joyAxes[i].offset)) >> (myJoystick.joyAxes[i].logical_max_bits - 16));
				}

				sendDataToOutput(i, globalCVGlide, tempValue);
			}
		}
		else
		{
			axesOffset = 0;
		}
		
		if (dPadStyle == asButtons)
		{
			uint8_t tempDPad = findDataInReport(myJoystick.dPads[0].size, myJoystick.dPads[0].offset);
			for (int i = 0; i < 8; i++)
			{
				uint16_t tempOutput = 0;
				if (i == tempDPad)
				{
					tempOutput = 65535;
				}
				sendDataToOutput(i + axesOffset, 0, tempOutput);
			}
		}
		else if (dPadStyle == asAxes)
		{
			tempValue = ((findDataInReport(myJoystick.dPads[0].size, myJoystick.dPads[0].offset)) << (16-myJoystick.dPads[0].logical_max_bits));
			sendDataToOutput(axesOffset, globalCVGlide, tempValue);
		}
		//otherwise DPad is ignored and skipped in the output
		if (myJoystick.numDPads > 0)
		{
			if (dPadStyle == asButtons)
			{
				dPadOffset = 8;
			}
			else if (dPadStyle == asAxes)
			{
				dPadOffset = 1;
			}
			else
			{
				dPadOffset = 0;
			}
		}
		for (int i = 0; i < myJoystick.numJoyButton; i++)
		{
			tempValue = ((findDataInReport(myJoystick.joyButtons[i].size, myJoystick.joyButtons[i].offset)) << (16-myJoystick.joyButtons[i].size));
			if (tempValue != myJoystick.joyButtons[i].previous_value)
			{
				sendDataToOutput(i +  axesOffset + dPadOffset, 0, tempValue);
				if ((joystickTriggers) && (tempValue > 0))
				{
					myJoystick.trigCount[(i + axesOffset + dPadOffset)] = TRIGGER_TIMING;
				}
			}

			myJoystick.joyButtons[i].previous_value = tempValue;
			
		}

	}
	uhi_hid_joy_start_trans_report(add);
}


//@}
//@}