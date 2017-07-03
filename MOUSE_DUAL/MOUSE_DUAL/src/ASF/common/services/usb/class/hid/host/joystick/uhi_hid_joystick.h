/**
 * \file
 *
 * \brief USB host driver for Human Interface Device (HID) mouse interface.
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

#ifndef _UHI_HID_JOY_H_
#define _UHI_HID_JOY_H_

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "usb_protocol_hid.h"
#include "uhi.h"
#include "uhc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BUTTONS		64
#define MAX_AXES		12

#define GENERIC_DESKTOP		1
#define SIMULATION			2
#define KEYBOARD			6
#define BUTTON				9

#define USAGE_X 0x30
#define USAGE_Y 0x31
#define USAGE_Z 0x32
#define USAGE_RX 0x33
#define USAGE_RY 0x34
#define USAGE_RZ 0x35
#define USAGE_SLIDER 0x36
#define USAGE_DIAL 0x37
#define USAGE_WHEEL 0x38
#define USAGE_HAT_SWITCH 0x39
#define USAGE_THROTTLE 0xBB


/*
* Constants
*/
#define PATH_SIZE               10
#define USAGE_TAB_SIZE          50
#define MAX_REPORT              30

#define REPORT_DSC_SIZE       6144

/*
* Items
* -------------------------------------------------------------------------- */
#define SIZE_0                0x00
#define SIZE_1                0x01
#define SIZE_2                0x02
#define SIZE_4                0x03
#define SIZE_MASK             0x03

#define TYPE_MAIN             0x00
#define TYPE_GLOBAL           0x04
#define TYPE_LOCAL            0x08
#define TYPE_MASK             0x0C

/* Main items */
#define ITEM_COLLECTION       0xA0
#define ITEM_END_COLLECTION   0xC0
#define ITEM_FEATURE          0xB0
#define ITEM_INPUT            0x80
#define ITEM_OUTPUT           0x90

/* Global items */
#define ITEM_UPAGE            0x04
#define ITEM_LOG_MIN          0x14
#define ITEM_LOG_MAX          0x24
#define ITEM_PHY_MIN          0x34
#define ITEM_PHY_MAX          0x44
#define ITEM_UNIT_EXP         0x54
#define ITEM_UNIT             0x64
#define ITEM_REP_SIZE         0x74
#define ITEM_REP_ID           0x84
#define ITEM_REP_COUNT        0x94

/* Local items */
#define ITEM_USAGE            0x08
#define ITEM_STRING           0x78

/* Long item */
#define ITEM_LONG			  0xFC

#define ITEM_MASK             0xFC

/* Attribute Flags */
#define ATTR_DATA_CST         0x01
#define ATTR_NVOL_VOL         0x80

typedef struct {
	uhc_device_t *dev;
	usb_ep_t ep_in;
	uint16_t report_butt_prev;
	uint16_t report_slider_prev;
	uint16_t report_hat_prev;
	uint16_t report_Rz_prev;
	uint16_t report_x_prev;
	uint16_t report_y_prev;
	uint8_t report_size;
	uint8_t *DescType;
	uint8_t *report;
	uint16_t *DescSize;
	uint8_t numDesc;
}uhi_hid_joy_dev_t;

typedef struct {
	uint16_t u_page;
	uint16_t usage;
}hid_node;

typedef struct {
	uint8_t size;
	hid_node node[PATH_SIZE];
}hid_path;

typedef struct {
	int32_t val;
	hid_path path;
	uint8_t ireport;
	uint8_t offset;
	uint8_t size;
	uint8_t type;
	uint8_t attribute;
	uint32_t unit;
	int8_t unit_exp;
	int32_t log_min;
	int32_t log_max;
	int32_t phy_min;
	int32_t phy_max;
}hid_data;

typedef struct {
	uint8_t *reportDesc;
	uint16_t reportDesc_size;
	uint16_t pos; //
	uint8_t item;
	int32_t val;
	hid_data data; //
	uint8_t offset_tab[MAX_REPORT][3]; //
	uint8_t report_count;
	uint8_t count; //
	uint16_t u_page;
	hid_node usage_tab[USAGE_TAB_SIZE]; //
	uint8_t usage_size; //
	uint8_t nobject; //
	uint8_t nreport; //
}hid_report_parser_t;

typedef struct _tJoystickAxis
{
	uint8_t offset;
	uint8_t size;
	uint8_t logical_max_bits;
	uint16_t previous_value;
} tJoystickAxis;

typedef struct _tJoystickButton
{
	uint8_t offset;
	uint8_t count;
	uint8_t size;
	uint16_t previous_value;
} tJoystickButton;

typedef struct _tJoystick
{
	tJoystickAxis joyAxes[MAX_AXES];
	tJoystickButton joyButtons[MAX_BUTTONS];
	uint8_t numJoyAxis;
	uint8_t numJoyButton;
}  tJoystick;

tJoystick myJoystick;

uint8_t possible_keys[3][12];


//! Global define which contains standard UHI API for UHC
//! It must be added in USB_HOST_UHI define from conf_usb_host.h file.
#define UHI_HID_JOYSTICK { \
	.install = uhi_hid_joy_install, \
	.enable = uhi_hid_joy_enable, \
	.uninstall = uhi_hid_joy_uninstall, \
	.sof_notify = NULL, \
}

/**
 * \name Functions required by UHC
 * @{
 */
extern uhc_enum_status_t uhi_hid_joy_install(uhc_device_t* dev);
extern void uhi_hid_joy_enable(uhc_device_t* dev);
extern void uhi_hid_joy_uninstall(uhc_device_t* dev);
uint8_t* GetReportOffset(const uint8_t ireport, const uint8_t ReportType);
void clearJoystick(tJoystick theJoystick);
static uint32_t findDataInReport(uint8_t size, uint8_t offset);

#ifdef __cplusplus
}
#endif
#endif // _UHI_HID_MOUSE_H_
