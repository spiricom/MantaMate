/**
 * \file
 *
 * \brief USB Device Human Interface Device (HID) mouse interface.
 *
 * Copyright (c) 2009-2015 Atmel Corporation. All rights reserved.
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

#include "conf_usb.h"
#include "usb_protocol.h"
#include "udd.h"
#include "udc.h"
#include "udi_midi.h"
#include <string.h>

/**
 * \ingroup udi_hid_mouse_group
 * \defgroup udi_hid_mouse_group_udc Interface with USB Device Core (UDC)
 *
 * Structures and functions required by UDC.
 *
 * @{
 */
bool udi_dmidi_enable(void);
void udi_dmidi_disable(void);
bool udi_dmidi_device_setup(void);
uint8_t udi_dmidi_getsetting(void);
bool udi_dmidi_setup(void);

//! Global structure which contains standard UDI interface for UDC
UDC_DESC_STORAGE udi_api_t udi_api_midi = {
	.enable = NULL,//(bool(*)(void))udi_dmidi_enable,
	.disable = NULL,//(void (*)(void))udi_dmidi_disable,
	.setup = NULL,//(bool(*)(void))udi_dmidi_setup,
	.getsetting = NULL,//(uint8_t(*)(void))udi_dmidi_getsetting,
	.sof_notify = NULL,
};
//@}


/**
 * \ingroup udi_hid_mouse_group
 * \defgroup udi_hid_mouse_group_internal Implementation of UDI HID Mouse
 *
 * Class internal implementation
 * @{
 */

/**
 * \name Internal defines and variables to manage HID mouse
 */
//@{

//! Size of report for standard HID mouse
#define UDI_MIDI_REPORT_SIZE   8//4
//! To store current rate of HID mouse
COMPILER_WORD_ALIGNED
		static uint8_t udi_midi_rate;
//! To store current protocol of HID mouse
COMPILER_WORD_ALIGNED
		static uint8_t udi_midi_protocol;
//! To signal if a valid report is ready to send
static bool udi_midi_b_report_valid;
//! Report ready to send
static uint8_t udi_midi_report[UDI_MIDI_REPORT_SIZE];
//! Signal if a report transfer is on going
static bool udi_midi_report_trans_ongoing;
//! Buffer used to send report
COMPILER_WORD_ALIGNED
		static uint8_t
		udi_midi_report_trans[UDI_MIDI_REPORT_SIZE];

/**
 * \brief Callback for set report setup request
 *
 * \return \c 1 always, because it is not used on mouse interface
 */
//static bool udi_hid_mouse_setreport(void);

//@}

/**
 * \name Internal routines
 */
//@{

/**
 * \brief Send the report
 *
 * \return \c 1 if send on going, \c 0 if delay.
 */
static bool udi_midi_send_report(void);

/**
 * \brief Callback called when the report is sent
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finish
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param nb_sent    number of data transfered
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
static void udi_midi_report_sent(udd_ep_status_t status,
		iram_size_t nb_sent, udd_ep_id_t ep);

//@}


//--------------------------------------------
//------ Interface for UDI HID level

bool udi_dmidi_enable(void)
{
	// Initialize internal value
	udi_midi_rate = 0;
	udi_midi_protocol = 0;
	udi_midi_report_trans_ongoing = false;
	memset(udi_midi_report, 0, UDI_MIDI_REPORT_SIZE);
	udi_midi_b_report_valid = false;
	return UDI_MIDI_ENABLE_EXT();
}


void udi_dmidi_disable(void)
{
	UDI_MIDI_DISABLE_EXT();
}


bool udi_dmidi_setup(void)
{
	return true;
	//return udi_hid_setup(&udi_hid_mouse_rate,
	//							&udi_hid_mouse_protocol,
	//							(uint8_t *) &udi_hid_mouse_report_desc,
	//							udi_hid_mouse_setreport);
}


uint8_t udi_dmidi_getsetting(void)
{
	return 0;
}



//--------------------------------------------
//------ Interface for application


static bool udi_midi_send_report(void)
{
	if (udi_midi_report_trans_ongoing)
		return false;	// Transfer on going then send this one after transfer complete

	// Copy report on other array used only for transfer
	memcpy(udi_midi_report_trans, udi_midi_report,
			UDI_MIDI_REPORT_SIZE);
	memset(&udi_midi_report[1], 0, 3);	// Keep status of btn for next report
	udi_midi_b_report_valid = false;

	// Send report
	udi_midi_report_trans_ongoing =
			udd_ep_run(	UDI_MIDI_EP_IN,
							false,
							udi_midi_report_trans,
							UDI_MIDI_REPORT_SIZE,
							udi_midi_report_sent);
	return udi_midi_report_trans_ongoing;
}


static void udi_midi_report_sent(udd_ep_status_t status,
		iram_size_t nb_sent, udd_ep_id_t ep)
{
	UNUSED(ep);
	UNUSED(status);
	UNUSED(nb_sent);
	// Valid report sending
	udi_midi_report_trans_ongoing = false;
	if (udi_midi_b_report_valid) {
		// Send new valid report
		udi_midi_send_report();
	}
}

//@}
