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

#ifndef _UHI_HID_MANTA_H_
#define _UHI_HID_MANTA_H_

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "usb_protocol_hid.h"
#include "uhi.h"

#include "utilities.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup uhi_hid_manta_group
 * \defgroup uhi_hid_manta_group_uhc Interface with USB Host Core (UHC)
 *
 * Define and functions required by UHC.
 *
 * @{
 */
#define UHI_MANTA_1ST_ED_EP_OUT_SIZE 10
#define UHI_MANTA_EP_OUT_SIZE 16
#define HEX_BYTES 6
#define SLIDER_BYTES 2
#define MANTA_CONTROL 9
#define FUNTION_BYTES 1

#define HEX_EXT		1		// hexagon and function button LEDs reflect the bits in the first 6 bytes of the report
#define SLIDER_EXT	1<<1	// allows the slider LEDs to respond to computer control
#define TURBO		1<<2	// scan only the bottom 16 sensors, and at a faster, but less accurate, scan rate
#define RAW_MODE	1<<3	// send the actual capacitance measurements the manta is taking
#define HI_RES		1<<4	//  scan only sensors 7 and 8, but at a slow, accurate rate, with higher resolution.)  (DEPRECATED IN 2nd ED)
#define BUTTON_CTRL 1<<5	// only the function buttons respond to computer control

#define LOCAL_CONTROL 0
#define HOST_CONTROL_FULL 1
#define HOST_CONTROL_BUTTON 2
#define HOST_CONTROL_SLIDER 3
#define HOST_CONTROL_HEX_AND_BUTTON 4


//! Global define which contains standard UHI API for UHC
//! It must be added in USB_HOST_UHI define from conf_usb_host.h file.
#define UHI_HID_MANTA { \
	.install = uhi_hid_manta_install, \
	.enable = uhi_hid_manta_enable, \
	.uninstall = uhi_hid_manta_uninstall, \
	.sof_notify = uhi_hid_manta_sof \
}

/**
 * \name Functions required by UHC
 * @{
 */
extern uhc_enum_status_t uhi_hid_manta_install(uhc_device_t* dev);
extern void uhi_hid_manta_enable(uhc_device_t* dev);
extern void uhi_hid_manta_uninstall(uhc_device_t* dev);
extern void uhi_hid_manta_sof(bool);
extern void manta_LED_set_mode(uint8_t mode);
extern void manta_set_LED_hex(uint8_t hex, MantaLEDColor color);
extern void manta_set_LED_slider(uint8_t whichSlider, uint8_t value);
extern void manta_set_LED_slider_bitmask(uint8_t whichSlider, uint8_t value);
extern void manta_set_LED_button(uint8_t button, uint8_t color);
extern void manta_clear_all_LEDs(void);
extern void manta_send_LED(void);
void dimLEDsForFirstEdition(void);
void get_manta_serial_number(void);
void testSerialNumber(void);

extern BOOL firstEdition;
extern uint8_t butt_states[48];
extern uint8_t pastbutt_states[48];
extern uint8_t func_button_states[4];
extern uint8_t past_func_button_states[4];
bool manta_light_LED(uint8_t *lights);
//@}
//@}

/**
 * \ingroup uhi_group
 * \defgroup uhi_hid_manta_group UHI for Human Interface Device Mouse Class
 *
 * Common APIs used by high level application to use this USB host class.
 * 
 * This API requires only callback definitions in conf_usb_host.h file
 * through following defines:
 * - \code  #define UHI_HID_MOUSE_CHANGE(dev,b_plug)
	#define UHI_HID_MOUSE_EVENT_BTN_LEFT(b_state)
	#define UHI_HID_MOUSE_EVENT_BTN_RIGHT(b_state)
	#define UHI_HID_MOUSE_EVENT_BTN_MIDDLE(b_state)
	#define UHI_HID_MOUSE_EVENT_MOUVE(x,y,scroll) \endcode
 *
 * See \ref uhi_hid_manta_quickstart.
 * @{
 */
//@}


/**
 * \page uhi_hid_manta_quickstart Quick start guide for USB host mouse module (UHI mouse)
 *
 * This is the quick start guide for the \ref uhi_hid_manta_group 
 * "USB host mouse module (UHI mouse)" with step-by-step instructions on 
 * how to configure and use the modules in a selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 * 
 * \section uhi_hid_manta_basic_use_case Basic use case
 * In this basic use case, the "USB Host HID Mouse (Single Class support)" module is used.
 * The "USB Host HID Mouse (Multiple Classes support)" module usage is described
 * in \ref uhi_hid_manta_use_cases "Advanced use cases".
 *
 * \section uhi_hid_manta_basic_use_case_setup Setup steps
 * \subsection uhi_hid_manta_basic_use_case_setup_prereq Prerequisites
 * \copydetails uhc_basic_use_case_setup_prereq
 * \subsection uhi_hid_manta_basic_use_case_setup_code Example code
 * \copydetails uhc_basic_use_case_setup_code
 * \subsection uhi_hid_manta_basic_use_case_setup_flow Workflow
 * \copydetails uhc_basic_use_case_setup_flow
 *
 * \section uhi_hid_manta_basic_use_case_usage Usage steps
 *
 * \subsection uhi_hid_manta_basic_use_case_usage_code Example code
 * Content of conf_usb_host.h:
 * \code
	#define USB_HOST_UHI        UHI_HID_MOUSE
	#define UHI_HID_MOUSE_CHANGE(dev, b_plug) my_callback_mouse_change(dev, b_plug)
	extern bool my_callback_mouse_change(uhc_device_t* dev, bool b_plug);
	#define UHI_HID_MOUSE_EVENT_BTN_LEFT(b_state) my_callback_event_btn_left(b_state)
	extern void my_callback_event_btn_left(bool b_state);
	#define UHI_HID_MOUSE_EVENT_BTN_RIGHT(b_state) my_callback_event_btn_right(b_state)
	extern void my_callback_event_btn_right(bool b_state);
	#define UHI_HID_MOUSE_EVENT_BTN_MIDDLE(b_state) my_callback_event_btn_middle(b_state)
	extern void my_callback_event_btn_middle(bool b_state);
	#define UHI_HID_MOUSE_EVENT_MOUVE(x, y, scroll) my_callback_event_mouse(x, y, scroll)
	extern void my_callback_event_mouse(int8_t x, int8_t y, int8_t scroll);
	#include "uhi_hid_mouse.h" // At the end of conf_usb_host.h file
\endcode
 *
 * Add to application C-file:
 * \code
	 bool my_callback_mouse_change(uhc_device_t* dev, bool b_plug)
	 {
	    if (b_plug) {
	       my_display_on_mouse_icon();
	    } else {
	       my_display_off_mouse_icon();
	    }
	 }

	 void my_callback_event_btn_left(bool b_state)
	 {
	    if (b_state) {
	       // Here mouse button left pressed
	    } else {
	       // Here mouse button left released
	    }
	 }

	 void my_callback_event_mouse(int8_t x, int8_t y, int8_t scroll)
	 {
	    if (!x) {
	       // Here mouse are moved on axe X
	       cursor_x += x;
	    }
	    if (!y) {
	       // Here mouse are moved on axe Y
	       cursor_y += y;
	    }
	    if (!scroll) {
	       // Here mouse are moved the wheel
	       wheel += scroll;
	    }
	 }
\endcode
 *
 * \subsection uhi_hid_manta_basic_use_case_setup_flow Workflow
 * -# Ensure that conf_usb_host.h is available and contains the following configuration
 * which is the USB host mouse configuration:
 *   - \code #define USB_HOST_UHI   UHI_HID_MOUSE \endcode
 *     \note It defines the list of UHI supported by USB host.
 *   - \code #define UHI_HID_MOUSE_CHANGE(dev, b_plug) my_callback_mouse_change(dev, b_plug)
	 extern bool my_callback_mouse_change(uhc_device_t* dev, bool b_plug); \endcode
 *     \note This callback is called when a USB device mouse is plugged or unplugged.
 *   - \code #define UHI_HID_MOUSE_EVENT_BTN_LEFT(b_state) my_callback_event_btn_left(b_state)
	extern void my_callback_event_btn_left(bool b_state);
	#define UHI_HID_MOUSE_EVENT_BTN_RIGHT(b_state) my_callback_event_btn_right(b_state)
	extern void my_callback_event_btn_right(bool b_state);
	#define UHI_HID_MOUSE_EVENT_BTN_MIDDLE(b_state) my_callback_event_btn_middle(b_state)
	extern void my_callback_event_btn_middle(bool b_state);
	#define UHI_HID_MOUSE_EVENT_MOUVE(x, y, scroll) my_callback_event_mouse(x, y, scroll)
	extern void my_callback_event_mouse(int8_t x, int8_t y, int8_t scroll) \endcode
 *     \note These callbacks are called when a USB device mouse event is received.
 *
 * \section uhi_hid_manta_use_cases Advanced use cases
 * For more advanced use of the UHI HID mouse module, see the following use cases:
 * - \subpage uhc_use_case_1
 * - \subpage uhc_use_case_2
 * - \subpage uhc_use_case_3
 */


#ifdef __cplusplus
}
#endif
#endif // _UHI_HID_MOUSE_H_
