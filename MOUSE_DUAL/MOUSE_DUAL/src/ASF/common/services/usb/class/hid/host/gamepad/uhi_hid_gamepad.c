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
#include "note_process.h"
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
#define MAX_BUTTS		8

#define UHI_HID_MOUSE_MOV_SCROLL 3

#define GENERIC_DESKTOP		1
#define SIMULATION			2
#define BUTTON				9

#define USAGEX 0x30
#define USAGEY 0x31
#define HAT_SWITCH 0x39
#define THROTTLE 0xBB
#define Rz 0x35
#define SLIDER 0x36

uint8_t UHI_HID_JOY_MOV_X;
uint8_t UHI_HID_JOY_MOV_Y;
uint8_t X_SIZE;
uint8_t Y_SIZE;
uint8_t UHI_HID_JOY_BUTT[MAX_BUTTS];
uint8_t NUM_BUTTS[MAX_BUTTS];
uint8_t BUTT_SIZE[MAX_BUTTS];
uint8_t UHI_HID_JOY_HAT;
uint8_t HAT_SIZE;
uint8_t UHI_HID_JOY_SLIDER;
uint8_t SLIDER_SIZE;
uint8_t UHI_HID_JOY_THROTTLE;
uint8_t THROTTLE_SIZE;
uint8_t UHI_HID_JOY_Rz; //joystick twistiness-ness
uint8_t Rz_SIZE;
uint8_t X_LOGICAL_MAX_BITS;
uint8_t Y_LOGICAL_MAX_BITS;
uint8_t Rz_LOGICAL_MAX_BITS;
uint8_t SLIDER_LOGICAL_MAX_BITS;

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
		
static uint32_t shift(uint8_t size, uint8_t offset);
static void findDataOffset(void);
static void ResetParser(void);
int find_highest_bit(int);
static void display_joystick_sizes(void);
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
static uhi_hid_joy_dev_t uhi_hid_game_dev = {
	.dev = NULL,
	.report = NULL,
};

const char ItemSize[4]={0,1,2,4};

/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_hid_game_install(uhc_device_t* dev)
{
	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;
	int i;

	if (uhi_hid_game_dev.dev != NULL) {
		
		return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
	}
	//if (DEBUG) Write7Seg(57);
	conf_desc_lgt = le16_to_cpu(dev->conf_desc->wTotalLength);
	ptr_iface = (usb_iface_desc_t*) dev->conf_desc;
	b_iface_supported = false;
	
	while(conf_desc_lgt) {
		switch (ptr_iface->bDescriptorType) {

		case USB_DT_INTERFACE: // if it's some kind of interface (my interpretation)
			if ((ptr_iface->bInterfaceClass   == HID_CLASS)
			&& (ptr_iface->bInterfaceProtocol == HID_PROTOCOL_GENERIC)
			&& dev->dev_desc.idProduct != 0x2424) { // and it's not a manta
				// USB HID Joystick interface found
				// Start allocation endpoint(s)
				b_iface_supported = true;
			} else {
				// Stop allocation endpoint(s)
				b_iface_supported = false;
			}
			break;
		
		case USB_DT_HID:
			if (!b_iface_supported) {
				break;
			}
			//how many descriptors and what type, make some space to store
			uhi_hid_game_dev.numDesc = ((usb_hid_descriptor_t*) ptr_iface)->bNumDescriptors;
			uhi_hid_game_dev.DescType = (uint8_t *) malloc(uhi_hid_game_dev.numDesc);
			uhi_hid_game_dev.DescSize = (uint16_t *) malloc(2*uhi_hid_game_dev.numDesc);
			for (i = 0; i < uhi_hid_game_dev.numDesc; i++)
			{
				uhi_hid_game_dev.DescType[i] = ((usb_hid_descriptor_t*) ptr_iface)->bRDescriptorType;
				uhi_hid_game_dev.DescSize[i] = (((usb_hid_descriptor_t*) ptr_iface)->wDescriptorLength)>>8;
			}
			break;
		
		case USB_DT_ENDPOINT:
			//  Allocation of the endpoint
			if (!b_iface_supported) {
				break;
			}
			if (!uhd_ep_alloc(dev->address, (usb_ep_desc_t*) ptr_iface)) {
				return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail
			}
			Assert(((usb_ep_desc_t*) ptr_iface)->bEndpointAddress & USB_EP_DIR_IN);
			uhi_hid_game_dev.ep_in = ((usb_ep_desc_t*) ptr_iface)->bEndpointAddress;
			uhi_hid_game_dev.report_size =
					le16_to_cpu(((usb_ep_desc_t*) ptr_iface)->wMaxPacketSize);
			uhi_hid_game_dev.report = malloc(uhi_hid_game_dev.report_size);
			if (uhi_hid_game_dev.report == NULL) {
				Assert(false);
				return UHC_ENUM_MEMORY_LIMIT; // Internal RAM allocation fail
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
	uhi_hid_game_dev.dev = dev;
	// All endpoints of all interfaces supported allocated
	
	get_report_descriptor();
	
	
	return UHC_ENUM_SUCCESS;
}

// called after uhi_hid_joy_install has run checks and after space has been allocated
// but wtf is it actually doing?
// this appears to step through the report, allocating space in hid_report_parser.reportDesc
// but it also seems to be overwritten with each iteration, so it seems like nothing is kept? 
// confused.
static void get_report_descriptor()
{
	usb_setup_req_t req;
	int i;
	
	for (i = 0; i < uhi_hid_game_dev.numDesc; i++)
	{	
		ResetParser();
		
		req.bmRequestType = USB_REQ_RECIP_INTERFACE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
		req.bRequest = USB_REQ_GET_DESCRIPTOR;
		req.wValue = (USB_DT_HID_REPORT << 8);
		req.wIndex = 0;
		req.wLength = uhi_hid_game_dev.DescSize[i] + 0x40;
		
		hid_report_parser.reportDesc_size = uhi_hid_game_dev.DescSize[i];
		
		if (hid_report_parser.reportDesc != NULL)
			hid_report_parser.reportDesc = (uint8_t *) realloc(hid_report_parser.reportDesc, uhi_hid_game_dev.DescSize[i]);
		else hid_report_parser.reportDesc = (uint8_t *) malloc(uhi_hid_game_dev.DescSize[i]);

		// After a USB reset, the reallocation is required
		if (!uhd_setup_request(1, &req, (uint8_t *) hid_report_parser.reportDesc, 
			uhi_hid_game_dev.DescSize[i], NULL, parse_report_descriptor)) 
		{
			//uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
			//MEMORY_printf_string("ERROR");
			//if (DEBUG) Write7Seg(77);
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
	
	memset(&NUM_BUTTS,0,sizeof(NUM_BUTTS));
}

//hid_data pData[16]; //random large number - idk it seems very hex :D
uint8_t ibutt = 0;

static void findDataOffset(void)
{
	uint8_t offset = hid_report_parser.data.offset;
	switch(hid_report_parser.data.path.node[hid_report_parser.data.path.size-1].u_page)
	{
		case GENERIC_DESKTOP:
			switch(hid_report_parser.data.path.node[hid_report_parser.data.path.size-1].usage)
			{
				case USAGEX:
					UHI_HID_JOY_MOV_X = offset;
					X_SIZE = hid_report_parser.data.size;
					X_LOGICAL_MAX_BITS = find_highest_bit(hid_report_parser.data.log_max);
					break;
				case USAGEY:
					UHI_HID_JOY_MOV_Y = offset;
					Y_SIZE = hid_report_parser.data.size;
					Y_LOGICAL_MAX_BITS = find_highest_bit(hid_report_parser.data.log_max);
					break;
				case HAT_SWITCH:
					UHI_HID_JOY_HAT = offset;
					HAT_SIZE = hid_report_parser.data.size;
					break;
				case SLIDER:
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
					
					break;
				case Rz:
					UHI_HID_JOY_Rz = offset;
					Rz_SIZE = hid_report_parser.data.size;
					Rz_LOGICAL_MAX_BITS = find_highest_bit(hid_report_parser.data.log_max);
					break;
				default:
					break;
			}
			break;
		case SIMULATION:
			switch(hid_report_parser.data.path.node[hid_report_parser.data.path.size-1].usage)
			{
				case THROTTLE:
					UHI_HID_JOY_THROTTLE = offset;
					THROTTLE_SIZE = hid_report_parser.data.size;
					break;
				default:
					break;
			}
		case BUTTON:
			if(NUM_BUTTS[ibutt] == 0 && hid_report_parser.report_count == hid_report_parser.count)
			{
				UHI_HID_JOY_BUTT[ibutt] = offset;
				NUM_BUTTS[ibutt] = hid_report_parser.report_count;
				BUTT_SIZE[ibutt++] = hid_report_parser.data.size;
			}
			break;
		default:
			break;	
	}
}


// i think this is where the descriptor is parsed. 
// perhaps this will illuminate the earlier functions

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

void uhi_hid_game_enable(uhc_device_t* dev)
{
	if (uhi_hid_game_dev.dev != dev) {
		return;  // No interface to enable
	}

	// Init value
	uhi_hid_game_dev.report_butt_prev = 0;
	uhi_hid_game_dev.report_y_prev = 0;
	uhi_hid_game_dev.report_x_prev = 0;
	uhi_hid_game_dev.report_slider_prev = 0;
	uhi_hid_game_dev.report_Rz_prev = 0;
	uhi_hid_game_dev.report_hat_prev = 0;
	uhi_hid_game_start_trans_report(dev->address);
	type_of_device_connected = JoystickConnected;
	UHI_HID_GAME_CHANGE(dev, true);
	joystick_mode = true; 
}

void uhi_hid_game_uninstall(uhc_device_t* dev)
{
	if (uhi_hid_game_dev.dev != dev) {
		return; // Device not enabled in this interface
	}
	uhi_hid_game_dev.dev = NULL; 
	Assert(uhi_hid_game_dev.report!=NULL);
	free(uhi_hid_game_dev.report);
	free(uhi_hid_game_dev.DescType);
	free(uhi_hid_game_dev.DescSize);
	free(hid_report_parser.reportDesc);
	ibutt = 0;
	UHI_HID_GAME_CHANGE(dev, false);
	type_of_device_connected = NoDeviceConnected;
	joystick_mode = false;
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
static void uhi_hid_game_start_trans_report(usb_add_t add)
{
	// Start transfer on interrupt endpoint IN
	uhd_ep_run(add, uhi_hid_game_dev.ep_in, true, uhi_hid_game_dev.report,
			uhi_hid_game_dev.report_size, 0, uhi_hid_game_report_reception);
}

static uint32_t shift(uint8_t size, uint8_t offset) {
	uint8_t total;
	uint8_t index;
	uint32_t state_new;
	uint8_t i;
	
	total = size;
	index = offset/8;
	state_new = (uhi_hid_game_dev.report[index]>>(offset%8));  // add the first (partial) byte
	i = 8 - offset % 8;
	
	while(i <= total - 8)  // add a full byte
	{
		state_new += uhi_hid_game_dev.report[++index] << i;
		i += 8;
	}
	if(i < total)  // add the last partial byte
	{
		state_new += (uhi_hid_game_dev.report[++index] & (0xFF>>(8-(offset+size)%8)))<<i;
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
static void uhi_hid_game_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered)
{
	uint16_t butt_prev;
	uint16_t butt_new;
	uint16_t slider_prev;
	uint16_t slider_new;
	uint16_t hat_prev;
	uint16_t hat_new;
	uint16_t Rz_prev;
	uint16_t Rz_new;
	uint16_t x_prev;
	uint16_t x_new;
	uint16_t y_prev;
	uint16_t y_new;
	uint8_t i = 0, b;
	
	UNUSED(ep);

	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_hid_game_start_trans_report(add);
		return; // HID mouse transfer restart
	}

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
		return; // HID mouse transfer aborted
	}
	
	// Decode buttons
	butt_prev = uhi_hid_game_dev.report_butt_prev;
	slider_prev = uhi_hid_game_dev.report_slider_prev;
	hat_prev = uhi_hid_game_dev.report_hat_prev;
	Rz_prev = uhi_hid_game_dev.report_Rz_prev;
	x_prev = uhi_hid_game_dev.report_x_prev;
	y_prev = uhi_hid_game_dev.report_y_prev;
	
	butt_new = 0;
	i = 0;
	int n_total_buttons = 0;
	
	// This combines all the "banks" of buttons into a single series of bits
	// button 1 is the lowest bit 
	for(b=0; b < ibutt; b++)
	{
		butt_new += shift(NUM_BUTTS[b]*BUTT_SIZE[b], UHI_HID_JOY_BUTT[b]) << i;
		i += NUM_BUTTS[b]*BUTT_SIZE[b];
		n_total_buttons += NUM_BUTTS[b];
	}
	
	x_new = shift(X_SIZE, UHI_HID_JOY_MOV_X);
	y_new = shift(Y_SIZE, UHI_HID_JOY_MOV_Y);
	hat_new = shift(HAT_SIZE, UHI_HID_JOY_HAT);
	Rz_new = shift(Rz_SIZE, UHI_HID_JOY_Rz);
	slider_new = shift(SLIDER_SIZE, UHI_HID_JOY_SLIDER);
	
	// We assume that there are 4 continuous controls X,Y,Rz,slider
	// this might not be true for all controllers
	if (x_prev != x_new) {
		uhi_hid_joy_dev.report_x_prev = x_new;
		DAC16Send(0, x_new << (16-X_LOGICAL_MAX_BITS));
	}
	
	if (y_prev != y_new) {
		uhi_hid_joy_dev.report_y_prev = y_new;
		int y_value = ((2 << Y_SIZE) - 1) - y_new; // this inverts the y direction
		DAC16Send(1, y_value << (16-Y_LOGICAL_MAX_BITS));
	}
	
	if(Rz_prev != Rz_new) {
		uhi_hid_joy_dev.report_Rz_prev = Rz_new;
		DAC16Send(2, Rz_new << (16-Rz_LOGICAL_MAX_BITS));
	}
	
	if(slider_prev != slider_new )
	{
		uhi_hid_joy_dev.report_slider_prev = slider_new;
		DAC16Send(3, slider_new << (16-SLIDER_LOGICAL_MAX_BITS));
	}	
	
	if(butt_prev != butt_new)
	{
		uhi_hid_joy_dev.report_butt_prev = butt_new;
		int my_mask = 1;
		int n_butts_mapped_so_far = 0;
		int n_dacs_available = 8; //TODO: magic. because we assumed 4 continuous controllers, that leaves 8 dacs available
		
		while ((n_butts_mapped_so_far < n_dacs_available) && (n_butts_mapped_so_far < n_total_buttons))
		{
			int butt_state = (butt_new >> n_butts_mapped_so_far) & my_mask;
			dacsend(n_butts_mapped_so_far % 4, 
					n_butts_mapped_so_far / 4,
					butt_state * 0xFFF); 
			
			if (n_butts_mapped_so_far == 0)
			{
				if (DEBUG) Write7Seg(butt_state);
			}
			n_butts_mapped_so_far++;
		}
	}
	
	// unused for now
	if(hat_prev != hat_new) {
		uhi_hid_joy_dev.report_hat_prev = hat_new;
	}
	
	// start the next transfer. this looks like it starts some crazy loop,
	// but it follows the example code given by atmel
	uhi_hid_joy_start_trans_report(add);
}

static int loop_counter = 0;
static void display_joystick_sizes(void)
{
	switch((loop_counter / 100) % 5)
	{
		case 0:
			Write7Seg(UHI_HID_JOY_MOV_X);
			break;
		case 1:
			Write7Seg(UHI_HID_JOY_MOV_Y);
			break;
		case 2:
			Write7Seg(UHI_HID_JOY_HAT);
			break;
		case 3:
			Write7Seg(UHI_HID_JOY_Rz);
			break;
		case 4: 
			Write7Seg(UHI_HID_JOY_SLIDER);
			break;
		default:
			Write7Seg(99);
	}
	loop_counter++;
}

//@}
//@}