/**
 * \file
 *
 * \brief Common User Interface for application
 *
 * Copyright (C) 2014-2015 Atmel Corporation. All rights reserved.
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

#ifndef _UI_H_
#define _UI_H_

#include "uhc.h"


/**
 * \name Main user interface functions
 * @{
 */

//! \brief Initializes the user interface
void ui_init(void);

/*! \brief Notify that the USB mode are switched automatically.
 * This is possible only when ID pin is available.
 *
 * \param b_host_mode true, if the host mode has been selected
 */
void ui_usb_mode_change(bool b_host_mode);
//! @}



/**
 * \name Host mode user interface functions
 * @{
 */

/*! \brief Notify that a Vbus are changed
 * Available only in USB hardware with Vbus monitoring.
 *
 * \param b_vbus_present true, if Vbus is high.
 */
void ui_host_vbus_change(bool b_vbus_present);

/*! \brief Notify that a Vbus error has occurred
 * Available only in USB hardware with Vbus monitoring.
 */
void ui_host_vbus_error(void);

/*! \brief Notify that a USB device has been connected or disconnected.
 *
 * \param dev         Pointer on USB device information
 * \param b_present   true, if the device has been connected
 */
void ui_host_connection_event(uhc_device_t *dev, bool b_present);

//! \brief Notify that a USB device or the host has wake up the USB line.
void ui_host_wakeup_event(void);

/*! \brief Notify the end of a USB device enumeration
 *
 * \param dev         Pointer on USB device information
 * \param status      Status of the USB enumeration
 */
void ui_host_enum_event(uhc_device_t *dev, uhc_enum_status_t status);


/**
 * \name Device mode user interface functions
 * @{
 */

/*! \brief Enters the user interface in low power mode
 * Callback called when USB host sets USB line in suspend state
 */
void ui_device_suspend_action(void);

/*! \brief Exits the user interface if the low power mode
 * Callback called when USB host sets USB line in IDLE state
 */
void ui_device_resume_action(void);

//! \brief Enables the remotewakeup feature
void ui_device_remotewakeup_enable(void);

//! \brief Disables the remotewakeup feature
void ui_device_remotewakeup_disable(void);

//! \brief Enables the device mouse interface
bool ui_device_midi_enable(void);

//! \brief Disables the device mouse interface
void ui_device_midi_disable(void);

//! \brief Scans the device mouse controls and shows the device status each USB SOF
void ui_device_sof_action(void);
//! @}

void ui_uhi_hid_joy_change(uhc_device_t * dev, bool b_plug);
void ui_uhi_hid_manta_change(uhc_device_t * dev, bool b_plug);
void  ui_uhi_midi_change(uhc_device_t *dev, bool b_plug);

void my_callback_midi_rx_notify(void);

void ui_test_flag_reset(void);
void ui_test_finish(bool b_success);

extern bool ui_midi_plug;
extern uint8_t my_buf[128];
extern uint8_t mySendBuf[64];

/*! \brief Called when communication port is opened
 */
void ui_midi_open(uint8_t port);

/*! \brief Called when communication port is closed
 */
void ui_midi_close(uint8_t port);

/*! \brief Called when a data is received on CDC
 */
void ui_midi_rx_start(void);

/*! \brief Called when a data is received on port com
 */
void ui_midi_tx_start(void);

/*! \brief Called when all data pending are sent on port com
 */
void ui_midi_rx_stop(void);

/*! \brief Called when all data pending are sent on CDC
 */
void ui_midi_tx_stop(void);

/*! \brief Called when a communication error occur
 */
void ui_midi_error(void);

/*! \brief Called when a overflow occur
 */
void ui_midi_overflow(void);

void ui_my_midi_receive(void);

void ui_my_midi_send(void);

void ui_my_sysex_send(iram_size_t size);

/*! \brief This process is called each 1ms
 * It is called only if the USB interface is enabled.
 *
 * \param framenumber  Current frame number
 */
void USB_frame_action(uint16_t framenumber);

void midi_ext_gate_in(void);



#endif // _UI_H_
