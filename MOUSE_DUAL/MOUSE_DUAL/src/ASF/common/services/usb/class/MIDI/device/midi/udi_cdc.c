/**
 * \file
 *
 * \brief USB Device Communication Device Class (CDC) interface.
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

#include "note_process.h"
#include "conf_usb.h"
#include "usb_protocol.h"
// #include "usb_protocol_cdc.h"
#include "udd.h"
#include "udc.h"
#include "udi_cdc.h"
#include "midi.h"
#include <string.h>

#define UDI_MIDI_IN_BUFFER_SIZE 64
#define UDI_MIDI_OUT_BUFFER_SIZE 64

#ifndef UDI_MIDI_TX_EMPTY_NOTIFY
#  define UDI_MIDI_TX_EMPTY_NOTIFY(void)
#endif

/**
 * \ingroup udi_cdc_group
 * \defgroup udi_cdc_group_udc Interface with USB Device Core (UDC)
 *
 * Structures and functions required by UDC.
 *
 * @{
 */
bool udi_midi_enable(void);
void udi_midi_disable(void);
bool udi_midi_setup(void);
uint8_t udi_midi_getsetting(void);

bool udi_audiocontrol_enable(void);
void udi_audiocontrol_disable(void);
bool udi_audiocontrol_setup(void);
uint8_t udi_audiocontrol_getsetting(void);
void udi_audiocontrol_sof_notify(void);

void udi_midi_sof_notify(void);
bool udi_midi_multi_is_rx_ready(uint8_t port);

UDC_DESC_STORAGE udi_api_t udi_api_audiocontrol = {
	.enable = udi_audiocontrol_enable,
	.disable = udi_audiocontrol_disable,
	.setup = udi_audiocontrol_setup,
	.getsetting = udi_audiocontrol_getsetting,
	.sof_notify = udi_audiocontrol_sof_notify
};

UDC_DESC_STORAGE udi_api_t udi_api_midi = {
	.enable = udi_midi_enable,
	.disable = udi_midi_disable,
	.setup = udi_midi_setup,
	.getsetting = udi_midi_getsetting,
	.sof_notify = udi_midi_sof_notify
};



//@}

/**
 * \ingroup udi_cdc_group
 * \defgroup udi_cdc_group_internal Implementation of UDI CDC
 *
 * Class internal implementation
 * @{
 */

/**
 * \name Internal routines
 */
//@{

/**
 * \name Routines to control serial line
 */
//@{

/**
 * \brief Returns the port number corresponding at current setup request
 *
 * \return port number
 */

static bool udi_midi_rx_start(uint8_t port);

/**
 * \brief Update rx buffer management with a new data
 * Callback called after data reception on USB line
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finish
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param n          number of data received
 */
static void udi_midi_data_received(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep);

/**
 * \brief Ack sent of tx buffer
 * Callback called after data transfer on USB line
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finished
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param n          number of data transfered
 */
static void udi_midi_data_sent(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep);

/**
 * \brief Send buffer on line or wait a SOF event
 *
 * \param port       Communication port number to manage
 */
static void udi_midi_tx_send(uint8_t port);

//@}

//@}



/**
 * \name Variables to manage RX/TX transfer requests
 * Two buffers for each sense are used to optimize the speed.
 */
//@{

//! Status of CDC DATA interfaces
static volatile uint8_t udi_midi_nb_data_enabled = 0;
static volatile bool udi_midi_data_running = false;
//! Buffer to receive data
COMPILER_WORD_ALIGNED static uint8_t udi_midi_rx_buf[UDI_MIDI_NB][2][UDI_MIDI_IN_BUFFER_SIZE];//is size the last parameter?
//! Data available in RX buffers
static volatile uint16_t udi_midi_rx_buf_nb[UDI_MIDI_NB][2];
//! Give the current RX buffer used (rx0 if 0, rx1 if 1)
static volatile uint8_t udi_midi_rx_buf_sel[UDI_MIDI_NB];
//! Read position in current RX buffer
static volatile uint16_t udi_midi_rx_pos[UDI_MIDI_NB];
//! Signal a transfer on-going
static volatile bool udi_midi_rx_trans_ongoing[UDI_MIDI_NB];

//! Define a transfer halted
#define  UDI_MIDI_TRANS_HALTED    2

//! Buffer to send data
COMPILER_WORD_ALIGNED static uint8_t udi_midi_tx_buf[UDI_MIDI_NB][2][UDI_MIDI_OUT_BUFFER_SIZE];
//! Data available in TX buffers
static uint16_t udi_midi_tx_buf_nb[UDI_MIDI_NB][2];
//! Give current TX buffer used (tx0 if 0, tx1 if 1)
static volatile uint8_t udi_midi_tx_buf_sel[UDI_MIDI_NB];
//! Value of SOF during last TX transfer
static uint16_t udi_midi_tx_sof_num[UDI_MIDI_NB];
//! Signal a transfer on-going
static volatile bool udi_midi_tx_trans_ongoing[UDI_MIDI_NB];
//! Signal that both buffer content data to send
static volatile bool udi_midi_tx_both_buf_to_send[UDI_MIDI_NB];

//@}

bool udi_midi_enable(void)
{
	uint8_t port = 0;
	udi_midi_nb_data_enabled = 0;

	LED_On(USB_CONNECTED_LED);
	initNoteStack();
	
	// Initialize TX management
	udi_midi_tx_trans_ongoing[port] = false;
	udi_midi_tx_both_buf_to_send[port] = false;
	udi_midi_tx_buf_sel[port] = 0;
	udi_midi_tx_buf_nb[port][0] = 0;
	udi_midi_tx_buf_nb[port][1] = 0;
	udi_midi_tx_sof_num[port] = 0;
	udi_midi_tx_send(port);

	// Initialize RX management
	udi_midi_rx_trans_ongoing[port] = false;
	udi_midi_rx_buf_sel[port] = 0;
	udi_midi_rx_buf_nb[port][0] = 0;
	udi_midi_rx_buf_nb[port][1] = 0;
	udi_midi_rx_pos[port] = 0;
	if (!udi_midi_rx_start(port)) {
		return false;
	}
	udi_midi_nb_data_enabled++;
	if (udi_midi_nb_data_enabled == UDI_MIDI_NB) {
		udi_midi_data_running = true;
	}
	type_of_device_connected = MIDIComputerConnected;

	return true;
}

bool udi_audiocontrol_enable(void)
{
	return true;
}



void udi_midi_disable(void)
{
	udi_midi_nb_data_enabled = 0;
	udi_midi_data_running = false;
	type_of_device_connected = NoDeviceConnected;
	LED_Off(USB_CONNECTED_LED);
}

void udi_audiocontrol_disable(void)
{
	;
}

bool udi_audiocontrol_setup(void)
{
	return false;
}

//how often does setup get called? Only when a new interface is enabled?
bool udi_midi_setup(void)
{
	uint8_t port = 0;
/*
	if (Udd_setup_is_in()) {
		// GET Interface Requests
		if (Udd_setup_type() == USB_REQ_TYPE_CLASS) {
			// Requests Class Interface Get
			switch (udd_g_ctrlreq.req.bRequest) {
			case USB_REQ_CDC_GET_LINE_CODING:
				// Get configuration of CDC line
				if (sizeof(usb_cdc_line_coding_t) !=
						udd_g_ctrlreq.req.wLength)
					return false; // Error for USB host
				udd_g_ctrlreq.payload =
						(uint8_t *) &
						udi_cdc_line_coding[port];
				udd_g_ctrlreq.payload_size =
						sizeof(usb_cdc_line_coding_t);
				return true;
			}
		}
	}
	if (Udd_setup_is_out()) {
		// SET Interface Requests
		if (Udd_setup_type() == USB_REQ_TYPE_CLASS) {
			// Requests Class Interface Set
			switch (udd_g_ctrlreq.req.bRequest) {
			case USB_REQ_CDC_SET_LINE_CODING:
				// Change configuration of CDC line
				if (sizeof(usb_cdc_line_coding_t) !=
						udd_g_ctrlreq.req.wLength)
					return false; // Error for USB host
				udd_g_ctrlreq.callback =
						udi_cdc_line_coding_received;
				udd_g_ctrlreq.payload =
						(uint8_t *) &
						udi_cdc_line_coding[port];
				udd_g_ctrlreq.payload_size =
						sizeof(usb_cdc_line_coding_t);
				return true;
			case USB_REQ_CDC_SET_CONTROL_LINE_STATE:
				// According cdc spec 1.1 chapter 6.2.14
				UDI_CDC_SET_DTR_EXT(port, (0 !=
						(udd_g_ctrlreq.req.wValue
						 & CDC_CTRL_SIGNAL_DTE_PRESENT)));
				UDI_CDC_SET_RTS_EXT(port, (0 !=
						(udd_g_ctrlreq.req.wValue
						 & CDC_CTRL_SIGNAL_ACTIVATE_CARRIER)));
				return true;
			}
		}
	}
	*/
	return false;  // request Not supported
}


uint8_t udi_midi_getsetting(void)
{
	return 0;      // MIDI don't have multiple alternate setting
}

uint8_t udi_audiocontrol_getsetting(void)
{
	return 0;
}

void udi_midi_sof_notify(void)
{
	static uint8_t port_notify = 0;

	// A call of udi_cdc_data_sof_notify() is done for each port
	udi_midi_tx_send(port_notify);
}

void udi_audiocontrol_sof_notify(void)
{
	//static uint8_t port_notify = 0;

	// A call of udi_cdc_data_sof_notify() is done for each port
	//udi_midi_tx_send(port_notify);
}


//-------------------------------------------------
//------- Internal routines to process data transfer


static bool udi_midi_rx_start(uint8_t port)
{
	irqflags_t flags;
	uint8_t buf_sel_trans;
	udd_ep_id_t ep;
	
	port = 0;


	flags = cpu_irq_save();
	buf_sel_trans = udi_midi_rx_buf_sel[port];
	if (udi_midi_rx_trans_ongoing[port] ||
		(udi_midi_rx_pos[port] < udi_midi_rx_buf_nb[port][buf_sel_trans])) {
		// Transfer already on-going or current buffer no empty
		cpu_irq_restore(flags);
		return false;
	}

	// Change current buffer
	udi_midi_rx_pos[port] = 0;
	udi_midi_rx_buf_sel[port] = (buf_sel_trans==0)?1:0;

	// Start transfer on RX
	udi_midi_rx_trans_ongoing[port] = true;
	cpu_irq_restore(flags);

	if (udi_midi_multi_is_rx_ready(port)) {
		UDI_MIDI_RX_NOTIFY();
	}
	// Send the buffer with enable of short packet

	ep = UDI_MIDI_EP_OUT_0;  //TODO -- what number should this be??

	return udd_ep_run(ep,
			true,
			udi_midi_rx_buf[port][buf_sel_trans],
			UDI_MIDI_IN_BUFFER_SIZE,
			udi_midi_data_received);
}


static void udi_midi_data_received(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep)
{
	uint8_t buf_sel_trans;
	uint8_t port;


	port = 0;


	if (UDD_EP_TRANSFER_OK != status) {
		// Abort reception
		return;
	}
	buf_sel_trans = (udi_midi_rx_buf_sel[port]==0)?1:0;
	if (!n) {
		udd_ep_run( ep,
				true,
				udi_midi_rx_buf[port][buf_sel_trans],
				UDI_MIDI_IN_BUFFER_SIZE,
				udi_midi_data_received);
		return;
	}
	udi_midi_rx_buf_nb[port][buf_sel_trans] = n;
	udi_midi_rx_trans_ongoing[port] = false;
	udi_midi_rx_start(port);
}


static void udi_midi_data_sent(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep)
{
	uint8_t port;
	UNUSED(n);


	port = 0;


	if (UDD_EP_TRANSFER_OK != status) {
		// Abort transfer
		return;
	}
	udi_midi_tx_buf_nb[port][(udi_midi_tx_buf_sel[port]==0)?1:0] = 0;
	udi_midi_tx_both_buf_to_send[port] = false;
	udi_midi_tx_trans_ongoing[port] = false;

	if (n != 0) {
		UDI_MIDI_TX_EMPTY_NOTIFY();
	}
	udi_midi_tx_send(port);
}


static void udi_midi_tx_send(uint8_t port)
{
	irqflags_t flags;
	uint8_t buf_sel_trans;
	bool b_short_packet;
	udd_ep_id_t ep;
	static uint16_t sof_zlp_counter = 0;
	port = 0;


	if (udi_midi_tx_trans_ongoing[port]) {
		return; // Already ongoing or wait next SOF to send next data
	}

	//how does tx_sof_num get filled?
	if (udi_midi_tx_sof_num[port] == udd_get_frame_number()) {
		return; // Wait next SOF to send next data
	}
	

	flags = cpu_irq_save(); // to protect udi_cdc_tx_buf_sel
	buf_sel_trans = udi_midi_tx_buf_sel[port];
	if (udi_midi_tx_buf_nb[port][buf_sel_trans] == 0) {
		sof_zlp_counter++;
		if (((!udd_is_high_speed()) && (sof_zlp_counter < 100))
				|| (udd_is_high_speed() && (sof_zlp_counter < 800))) {
			cpu_irq_restore(flags);
			return;
		}
	}
	sof_zlp_counter = 0;

	if (!udi_midi_tx_both_buf_to_send[port]) {
		// Send current Buffer
		// and switch the current buffer
		udi_midi_tx_buf_sel[port] = (buf_sel_trans==0)?1:0;
	}else{
		// Send the other Buffer
		// and no switch the current buffer
		buf_sel_trans = (buf_sel_trans==0)?1:0;
	}
	udi_midi_tx_trans_ongoing[port] = true;
	cpu_irq_restore(flags);

	b_short_packet = (udi_midi_tx_buf_nb[port][buf_sel_trans] != UDI_MIDI_OUT_BUFFER_SIZE);
	if (b_short_packet) {
		if (udd_is_high_speed()) {
			udi_midi_tx_sof_num[port] = udd_get_micro_frame_number();
		}else{
			udi_midi_tx_sof_num[port] = udd_get_frame_number();
		}
	}else{
		udi_midi_tx_sof_num[port] = 0; // Force next transfer without wait SOF
	}


	ep = UDI_MIDI_EP_IN_0;
	udd_ep_run( ep,
			b_short_packet,
			udi_midi_tx_buf[port][buf_sel_trans],
			udi_midi_tx_buf_nb[port][buf_sel_trans],
			udi_midi_data_sent);
}


//---------------------------------------------
//------- Application interface


//------- Application interface


iram_size_t udi_midi_multi_get_nb_received_data(uint8_t port)
{
	irqflags_t flags;
	uint16_t pos;
	iram_size_t nb_received;


	port = 0;

	flags = cpu_irq_save();
	pos = udi_midi_rx_pos[port];
	nb_received = udi_midi_rx_buf_nb[port][udi_midi_rx_buf_sel[port]] - pos;
	cpu_irq_restore(flags);
	return nb_received;
}

iram_size_t udi_midi_get_nb_received_data(void)
{
	return udi_midi_multi_get_nb_received_data(0);
}

bool udi_midi_multi_is_rx_ready(uint8_t port)
{
	return (udi_midi_multi_get_nb_received_data(port) > 0);
}

bool udi_midi_is_rx_ready(void)
{
	return udi_midi_multi_is_rx_ready(0);
}

int udi_midi_multi_getc(uint8_t port)
{
	irqflags_t flags;
	int rx_data = 0;
	bool b_databit_9;
	uint16_t pos;
	uint8_t buf_sel;
	bool again;

	port = 0;

udi_midi_getc_process_one_byte:
	// Check available data
	flags = cpu_irq_save();
	pos = udi_midi_rx_pos[port];
	buf_sel = udi_midi_rx_buf_sel[port];
	again = pos >= udi_midi_rx_buf_nb[port][buf_sel];
	cpu_irq_restore(flags);
	while (again) {
		if (!udi_midi_data_running) {
			return 0;
		}
		goto udi_midi_getc_process_one_byte;
	}

	// Read data
	rx_data |= udi_midi_rx_buf[port][buf_sel][pos];
	udi_midi_rx_pos[port] = pos+1;

	udi_midi_rx_start(port);

	return rx_data;
}

int udi_midi_getc(void)
{
	return udi_midi_multi_getc(0);
}

iram_size_t udi_midi_multi_read_buf(uint8_t port, void* buf, iram_size_t size)
{
	irqflags_t flags;
	uint8_t *ptr_buf = (uint8_t *)buf;
	iram_size_t copy_nb;
	uint16_t pos;
	uint8_t buf_sel;
	bool again;

	port = 0;


udi_midi_read_buf_loop_wait:
	// Check available data
	flags = cpu_irq_save();
	pos = udi_midi_rx_pos[port];
	buf_sel = udi_midi_rx_buf_sel[port];
	again = pos >= udi_midi_rx_buf_nb[port][buf_sel];
	cpu_irq_restore(flags);
	while (again) {
		if (!udi_midi_data_running) {
			return size;
		}
		goto udi_midi_read_buf_loop_wait;
	}

	// Read data
	copy_nb = udi_midi_rx_buf_nb[port][buf_sel] - pos;
	if (copy_nb>size) {
		copy_nb = size;
	}
	memcpy(ptr_buf, &udi_midi_rx_buf[port][buf_sel][pos], copy_nb);
	udi_midi_rx_pos[port] += copy_nb;
	ptr_buf += copy_nb;
	size -= copy_nb;
	udi_midi_rx_start(port);

	if (size) {
		goto udi_midi_read_buf_loop_wait;
	}
	return 0;
}

iram_size_t udi_midi_read_buf(void* buf, iram_size_t size)
{
	return udi_midi_multi_read_buf(0, buf, size);
}

iram_size_t udi_midi_multi_get_free_tx_buffer(uint8_t port)
{
	irqflags_t flags;
	iram_size_t buf_sel_nb, retval;
	uint8_t buf_sel;

	port = 0;

	flags = cpu_irq_save();
	buf_sel = udi_midi_tx_buf_sel[port];
	buf_sel_nb = udi_midi_tx_buf_nb[port][buf_sel];
	if (buf_sel_nb == UDI_MIDI_OUT_BUFFER_SIZE) {
		if ((!udi_midi_tx_trans_ongoing[port])
			&& (!udi_midi_tx_both_buf_to_send[port])) {
			/* One buffer is full, but the other buffer is not used.
			 * (not used = transfer on-going)
			 * then move to the other buffer to store data */
			udi_midi_tx_both_buf_to_send[port] = true;
			udi_midi_tx_buf_sel[port] = (buf_sel == 0)? 1 : 0;
			buf_sel_nb = 0;
		}
	}
	retval = UDI_MIDI_OUT_BUFFER_SIZE - buf_sel_nb;  
	cpu_irq_restore(flags);
	return retval;
}

iram_size_t udi_midi_get_free_tx_buffer(void)
{
	return udi_midi_multi_get_free_tx_buffer(0);
}

bool udi_midi_multi_is_tx_ready(uint8_t port)
{
	return (udi_midi_multi_get_free_tx_buffer(port) != 0);
}

bool udi_midi_is_tx_ready(void)
{
	return udi_midi_multi_is_tx_ready(0);
}

int udi_midi_multi_putc(uint8_t port, int value)
{
	irqflags_t flags;
	uint8_t buf_sel;

	port = 0;


udi_midi_putc_process_one_byte:
	// Check available space
	if (!udi_midi_multi_is_tx_ready(port)) {
		if (!udi_midi_data_running) {
			return false;
		}
		goto udi_midi_putc_process_one_byte;
	}

	// Write value
	flags = cpu_irq_save();
	buf_sel = udi_midi_tx_buf_sel[port];
	udi_midi_tx_buf[port][buf_sel][udi_midi_tx_buf_nb[port][buf_sel]++] = value;
	cpu_irq_restore(flags);

	return true;
}

int udi_midi_putc(int value)
{
	return udi_midi_multi_putc(0, value);
}

iram_size_t udi_midi_multi_write_buf(uint8_t port, const void* buf, iram_size_t size)
{
	irqflags_t flags;
	uint8_t buf_sel;
	uint16_t buf_nb;
	iram_size_t copy_nb;
	uint8_t *ptr_buf = (uint8_t *)buf;

	port = 0;

udi_midi_write_buf_loop_wait:
	// Check available space
	if (!udi_midi_multi_is_tx_ready(port)) {
		if (!udi_midi_data_running) {
			return size;
		}
		goto udi_midi_write_buf_loop_wait;
	}

	// Write values
	flags = cpu_irq_save();
	buf_sel = udi_midi_tx_buf_sel[port];
	buf_nb = udi_midi_tx_buf_nb[port][buf_sel];
	copy_nb = UDI_MIDI_OUT_BUFFER_SIZE - buf_nb;
	if (copy_nb > size) {
		copy_nb = size;
	}
	memcpy(&udi_midi_tx_buf[port][buf_sel][buf_nb], ptr_buf, copy_nb);
	udi_midi_tx_buf_nb[port][buf_sel] = buf_nb + copy_nb;
	cpu_irq_restore(flags);

	// Update buffer pointer
	ptr_buf = ptr_buf + copy_nb;
	size -= copy_nb;

	if (size) {
		goto udi_midi_write_buf_loop_wait;
	}

	return 0;
}

iram_size_t udi_midi_write_buf(const void* buf, iram_size_t size)
{
	return udi_midi_multi_write_buf(0, buf, size);
}

//@}
