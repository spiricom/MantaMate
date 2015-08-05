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

#ifndef _UDI_MIDI_H_
#define _UDI_MIDI_H_

#include "conf_usb.h"
#include "usb_protocol.h"
#include "usb_protocol_midi.h"
#include "udc_desc.h"
#include "udi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup udi_hid_mouse_group_udc
 * @{
 */
//! Global structure which contains standard UDI API for UDC
extern UDC_DESC_STORAGE udi_api_t udi_api_midi;
//@}

/**
 * \ingroup udi_hid_mouse_group
 * \defgroup udi_hid_mouse_group_desc USB interface descriptors
 *
 * The following structures provide predefined USB interface descriptors.
 * It must be used to define the final USB descriptors.
 */
//@{

//! Interface descriptor structure for HID MIDI (mouse)
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

//! By default no string associated to this interface
#ifndef UDI_MIDI_STRING_ID
#define UDI_MIDI_STRING_ID 0
#endif

//! HID mouse endpoints size
#define UDI_MIDI_EP_SIZE  64

//! Content of HID mouse interface descriptor for all speed
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
	.jack_out_emb.bNrInputPins       = 0,\
	.jack_out_emb.baSourceID         = 0x00,\
	.jack_out_emb.baSourcePin        = 0x00,\
	.jack_out_emb.iJack              = 0x00,\
	.jack_out_ext.bLength            = sizeof(usb_jack_out_descriptor_t),\
	.jack_out_ext.bDescriptorType    = CS_INTERFACE,\
	.jack_out_ext.bDescriptorSubtype = MIDI_OUT_JACK,\
	.jack_out_ext.bJackType          = EXTERNAL,\
	.jack_out_ext.bjackID            = 0x04,\
	.jack_out_emb.bNrInputPins       = 0x01,\
	.jack_out_emb.baSourceID         = 0x01,\
	.jack_out_emb.baSourcePin        = 0x01,\
	.jack_out_ext.iJack              = 0x00,\
	.ep_out.ep.bLength               = sizeof(usb_midi_ep_desc_t),\
	.ep_out.ep.bDescriptorType       = USB_DT_ENDPOINT,\
	.ep_out.ep.bEndpointAddress      = UDI_MIDI_EP_OUT,\
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
	.ep_in.ep.bEndpointAddress       = UDI_MIDI_EP_IN,\
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
//@}
//@}

#ifdef __cplusplus
}
#endif

#endif // _UDI_HID_MOUSE_H_