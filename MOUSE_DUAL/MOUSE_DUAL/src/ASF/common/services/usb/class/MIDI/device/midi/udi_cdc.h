/**
 * \file
 *
 * \brief USB Device Communication Device Class (CDC) interface definitions.
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

#ifndef _UDI_CDC_H_
#define _UDI_CDC_H_

#include "conf_usb.h"
#include "usb_protocol.h"
#include "usb_protocol_cdc.h"
#include "udd.h"
#include "udc_desc.h"
#include "udi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup udi_cdc_group_udc
 * @{
 */

//! Global structure which contains standard UDI API for UDC
extern UDC_DESC_STORAGE udi_api_t udi_api_midi;

extern UDC_DESC_STORAGE udi_api_t udi_api_audiocontrol;
//@}

/**
 * \ingroup udi_cdc_group
 * \defgroup udi_cdc_group_desc USB interface descriptors
 *
 * The following structures provide predefined USB interface descriptors.
 * It must be used to define the final USB descriptors.
 */
//@{

/**
 * \brief MIDI! interface descriptor
 *
 * Interface descriptor
 */

typedef struct{
	// MIDI Audio Control Interface
	usb_iface_desc_t iface0;
	usb_audio_desc_t aciface;

	// MIDI Audio Streaming Interface
	usb_iface_desc_t iface1;
	usb_stream_desc_t msiface;
	usb_jack_in_descriptor_t jack_in_emb;
	usb_jack_in_descriptor_t jack_in_ext;
	usb_jack_out_descriptor_t jack_out_emb;
	usb_jack_out_descriptor_t jack_out_ext;
	usb_midi_ep_desc_t ep_out;
	usb_midi_ep_desc_cs ep_out_cs;
	usb_midi_ep_desc_t ep_in;
	usb_midi_ep_desc_cs ep_in_cs;
}udi_midi_desc_t;


//! Structure for USB Device Configuration Descriptor
COMPILER_PACK_SET(1)
typedef struct{
	usb_conf_desc_t     config;
	udi_midi_desc_t     midi;
} udc_midi_conf_desc_t;
COMPILER_PACK_RESET()


//! CDC communication endpoints size for all speeds
#define UDI_MIDI_EP_SIZE        64

// I now understand that the UDI_MIDI_EP_SIZE is reported in little endian correctly. The computer reads it right, and the uc3a reads it into it's own memory using a LE16 converter.
//! Content of the USB MIDI descriptor
#define UDI_MIDI_DESC {\
	.iface0.bLength                  = sizeof(usb_iface_desc_t),\
	.iface0.bDescriptorType          = USB_DT_INTERFACE,\
	.iface0.bInterfaceNumber         = UDI_AUDIOCTRL_IFACE_NUMBER,\
	.iface0.bAlternateSetting        = 0,\
	.iface0.bNumEndpoints            = 0,\
	.iface0.bInterfaceClass          = AUDIO_CLASS,\
	.iface0.bInterfaceSubClass       = AUDIO_SUBCLASS_AUDIOCTRL,\
	.iface0.bInterfaceProtocol       = 0,\
	.iface0.iInterface               = UDI_MIDI_STRING_ID,\
	.aciface.bLength                 = sizeof(usb_audio_desc_t),\
	.aciface.bDescriptorType         = CS_INTERFACE,\
	.aciface.bDescriptorSubtype      = HEADER,\
	.aciface.bcdADC                  = LE16(USB_MIDI_BDC_V1_00),\
	.aciface.wTotalLength            = LE16(sizeof(usb_audio_desc_t)),\
	.aciface.bInCollection           = 1,\
	.aciface.baInterfaceNr           = 1,\
	.iface1.bLength                  = sizeof(usb_iface_desc_t),\
	.iface1.bDescriptorType          = USB_DT_INTERFACE,\
	.iface1.bInterfaceNumber         = UDI_MIDISTREAM_IFACE_NUMBER,\
	.iface1.bAlternateSetting        = 0,\
	.iface1.bNumEndpoints            = 2,\
	.iface1.bInterfaceClass          = AUDIO_CLASS,\
	.iface1.bInterfaceSubClass       = AUDIO_SUBCLASS_MIDISTREAM,\
	.iface1.bInterfaceProtocol       = 0,\
	.iface1.iInterface               = UDI_MIDI_STRING_ID,\
	.msiface.bLength                 = sizeof(usb_stream_desc_t),\
	.msiface.bDescriptorType         = CS_INTERFACE,\
	.msiface.bDescriptorSubtype      = MS_HEADER,\
	.msiface.bcdADC                  = LE16(USB_MIDI_BDC_V1_00),\
	.msiface.wTotalLength            = LE16((sizeof(udc_midi_conf_desc_t) - offsetof(udc_midi_conf_desc_t, midi.msiface))),\
	.jack_in_emb.bLength             = sizeof(usb_jack_in_descriptor_t),\
	.jack_in_emb.bDescriptorType     = CS_INTERFACE,\
	.jack_in_emb.bDescriptorSubtype  = MIDI_IN_JACK,\
	.jack_in_emb.bJackType           = EMBEDDED,\
	.jack_in_emb.bjackID             = 0x01,\
	.jack_in_emb.iJack               = 0x00,\
	.jack_in_ext.bLength             = sizeof(usb_jack_in_descriptor_t),\
	.jack_in_ext.bDescriptorType     = CS_INTERFACE,\
	.jack_in_ext.bDescriptorSubtype  = MIDI_IN_JACK,\
	.jack_in_ext.bJackType           = EXTERNAL,\
	.jack_in_ext.bjackID             = 0x02,\
	.jack_in_ext.iJack               = 0x00,\
	.jack_out_emb.bLength            = sizeof(usb_jack_out_descriptor_t),\
	.jack_out_emb.bDescriptorType    = CS_INTERFACE,\
	.jack_out_emb.bDescriptorSubtype = MIDI_OUT_JACK,\
	.jack_out_emb.bJackType          = EMBEDDED,\
	.jack_out_emb.bjackID            = 0x03,\
	.jack_out_emb.bNrInputPins       = 1,\
	.jack_out_emb.baSourceID         = 0x02,\
	.jack_out_emb.baSourcePin        = 0x01,\
	.jack_out_emb.iJack              = 0x00,\
	.jack_out_ext.bLength            = sizeof(usb_jack_out_descriptor_t),\
	.jack_out_ext.bDescriptorType    = CS_INTERFACE,\
	.jack_out_ext.bDescriptorSubtype = MIDI_OUT_JACK,\
	.jack_out_ext.bJackType          = EXTERNAL,\
	.jack_out_ext.bjackID            = 0x04,\
	.jack_out_ext.bNrInputPins       = 0x01,\
	.jack_out_ext.baSourceID         = 0x01,\
	.jack_out_ext.baSourcePin        = 0x01,\
	.jack_out_ext.iJack              = 0x00,\
	.ep_out.ep.bLength               = sizeof(usb_midi_ep_desc_t),\
	.ep_out.ep.bDescriptorType       = USB_DT_ENDPOINT,\
	.ep_out.ep.bEndpointAddress      = UDI_MIDI_EP_OUT_0,\
	.ep_out.ep.bmAttributes          = USB_EP_TYPE_BULK,\
	.ep_out.ep.wMaxPacketSize        = LE16(UDI_MIDI_EP_SIZE),\
	.ep_out.ep.bInterval             = 0x00,\
	.ep_out.bRefresh                 = 0x00,\
	.ep_out.bSynchAddress            = 0x00,\
	.ep_out_cs.bLength               = sizeof(usb_midi_ep_desc_cs),\
	.ep_out_cs.bDescriptorType       = CS_ENDPOINT,\
	.ep_out_cs.bDescriptorSubtype    = MS_GENERAL,\
	.ep_out_cs.bNumEmbMIDIJack       = 0x01,\
	.ep_out_cs.BaAssocJackID         = 0x01,\
	.ep_in.ep.bLength                = sizeof(usb_midi_ep_desc_t),\
	.ep_in.ep.bDescriptorType        = USB_DT_ENDPOINT,\
	.ep_in.ep.bEndpointAddress       = UDI_MIDI_EP_IN_0,\
	.ep_in.ep.bmAttributes           = USB_EP_TYPE_BULK,\
	.ep_in.ep.wMaxPacketSize         = LE16(UDI_MIDI_EP_SIZE),\
	.ep_in.ep.bInterval              = 0x00,\
	.ep_in.bRefresh                  = 0x00,\
	.ep_in.bSynchAddress             = 0x00,\
	.ep_in_cs.bLength                = sizeof(usb_midi_ep_desc_cs),\
	.ep_in_cs.bDescriptorType        = CS_ENDPOINT,\
	.ep_in_cs.bDescriptorSubtype     = MS_GENERAL,\
	.ep_in_cs.bNumEmbMIDIJack        = 0x01,\
	.ep_in_cs.BaAssocJackID          = 0x03,\
}


	

/**
 * \brief Notify a state change of DCD signal
 *
 * \param b_set      DCD is enabled if true, else disabled
 */
void udi_midi_ctrl_signal_dcd(bool b_set);

/**
 * \brief Notify a state change of DSR signal
 *
 * \param b_set      DSR is enabled if true, else disabled
 */
void udi_midi_ctrl_signal_dsr(bool b_set);

/**
 * \brief Notify a framing error
 */
void udi_midi_signal_framing_error(void);

/**
 * \brief Notify a parity error
 */
void udi_midi_signal_parity_error(void);

/**
 * \brief Notify a overrun
 */
void udi_midi_signal_overrun(void);

/**
 * \brief Gets the number of byte received
 *
 * \return the number of data available
 */
iram_size_t udi_midi_get_nb_received_data(void);

/**
 * \brief This function checks if a character has been received on the CDC line
 *
 * \return \c 1 if a byte is ready to be read.
 */
bool udi_midi_is_rx_ready(void);

/**
 * \brief Waits and gets a value on CDC line
 *
 * \return value read on CDC line
 */
int udi_midi_getc(void);

/**
 * \brief Reads a RAM buffer on CDC line
 *
 * \param buf       Values read
 * \param size      Number of value read
 *
 * \return the number of data remaining
 */
iram_size_t udi_midi_read_buf(void* buf, iram_size_t size);

/**
 * \brief Gets the number of free byte in TX buffer
 *
 * \return the number of free byte in TX buffer
 */
iram_size_t udi_midi_get_free_tx_buffer(void);

/**
 * \brief This function checks if a new character sent is possible
 * The type int is used to support scanf redirection from compiler LIB.
 *
 * \return \c 1 if a new character can be sent
 */
bool udi_midi_is_tx_ready(void);

/**
 * \brief Puts a byte on CDC line
 * The type int is used to support printf redirection from compiler LIB.
 *
 * \param value      Value to put
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
int udi_midi_putc(int value);

/**
 * \brief Writes a RAM buffer on CDC line
 *
 * \param buf       Values to write
 * \param size      Number of value to write
 *
 * \return the number of data remaining
 */
iram_size_t udi_midi_write_buf(const void* buf, iram_size_t size);
//@}

/**
 * \name Interface for application with multi CDC interfaces support
 */
//@{

/**
 * \brief Notify a state change of DCD signal
 *
 * \param port       Communication port number to manage
 * \param b_set      DCD is enabled if true, else disabled
 */
void udi_midi_multi_ctrl_signal_dcd(uint8_t port, bool b_set);

/**
 * \brief Notify a state change of DSR signal
 *
 * \param port       Communication port number to manage
 * \param b_set      DSR is enabled if true, else disabled
 */
void udi_midi_multi_ctrl_signal_dsr(uint8_t port, bool b_set);

/**
 * \brief Notify a framing error
 *
 * \param port       Communication port number to manage
 */
void udi_midi_multi_signal_framing_error(uint8_t port);

/**
 * \brief Notify a parity error
 *
 * \param port       Communication port number to manage
 */
void udi_midi_multi_signal_parity_error(uint8_t port);

/**
 * \brief Notify a overrun
 *
 * \param port       Communication port number to manage
 */
void udi_midi_multi_signal_overrun(uint8_t port);

/**
 * \brief Gets the number of byte received
 *
 * \param port       Communication port number to manage
 *
 * \return the number of data available
 */
iram_size_t udi_midi_multi_get_nb_received_data(uint8_t port);

/**
 * \brief This function checks if a character has been received on the CDC line
 *
 * \param port       Communication port number to manage
 *
 * \return \c 1 if a byte is ready to be read.
 */
bool udi_midi_multi_is_rx_ready(uint8_t port);

/**
 * \brief Waits and gets a value on CDC line
 *
 * \param port       Communication port number to manage
 *
 * \return value read on CDC line
 */
int udi_midi_multi_getc(uint8_t port);

/**
 * \brief Reads a RAM buffer on CDC line
 *
 * \param port       Communication port number to manage
 * \param buf       Values read
 * \param size      Number of values read
 *
 * \return the number of data remaining
 */
iram_size_t udi_midi_multi_read_buf(uint8_t port, void* buf, iram_size_t size);

/**
 * \brief Gets the number of free byte in TX buffer
 *
 * \param port       Communication port number to manage
 *
 * \return the number of free byte in TX buffer
 */
iram_size_t udi_midi_multi_get_free_tx_buffer(uint8_t port);

/**
 * \brief This function checks if a new character sent is possible
 * The type int is used to support scanf redirection from compiler LIB.
 *
 * \param port       Communication port number to manage
 *
 * \return \c 1 if a new character can be sent
 */
bool udi_midi_multi_is_tx_ready(uint8_t port);

/**
 * \brief Puts a byte on CDC line
 * The type int is used to support printf redirection from compiler LIB.
 *
 * \param port       Communication port number to manage
 * \param value      Value to put
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
int udi_midi_multi_putc(uint8_t port, int value);

/**
 * \brief Writes a RAM buffer on CDC line
 *
 * \param port       Communication port number to manage
 * \param buf       Values to write
 * \param size      Number of value to write
 *
 * \return the number of data remaining
 */
iram_size_t udi_midi_multi_write_buf(uint8_t port, const void* buf, iram_size_t size);



#ifdef __cplusplus
}
#endif
#endif // _UDI_CDC_H_
