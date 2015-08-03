/**
 * \file
 *
 * \brief USB Human Interface Device (HID) protocol definitions.
 *
 * Copyright (c) 2009-2014 Atmel Corporation. All rights reserved.
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
 /**
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef _USB_PROTOCOL_MIDI_H_
#define _USB_PROTOCOL_MIDI_H_

#include "usb_protocol.h"
/**
 * \ingroup usb_protocol_group
 * \defgroup usb_hid_protocol USB Human Interface Device (HID)
 * protocol definitions
 * \brief USB Human Interface Device (HID) protocol definitions
 *
 * @{
 */

//! \name Possible Class value
//@{
#define  AUDIO_CLASS                            0x01
//@}

//! \name Possible SubClass value
//@{

#define  AUDIO_SUBCLASS_MIDISTREAM                 0x03
#define  AUDIO_SUBCLASS_AUDIOCTRL                   0x01
//@}

//! \name Possible protocol value
//@{
//! Protocol generic standard
#define  MIDI_PROTOCOL_GENERIC                 0x00
//@}


//! \brief Audio USB requests (bRequest)
enum usb_reqid_audio {
	USB_REQ_MIDI_CODE_UNDEFINED = 0x00,
	USB_REQ_MIDI_SET_CUR = 0x01,
	USB_REQ_MIDI_SET_MIN = 0x02,
	USB_REQ_MIDI_SET_MAX = 0x03,
	USB_REQ_MIDI_SET_RES = 0x04,
	USB_REQ_MIDI_SET_MEM = 0x05,
	USB_REQ_MIDI_GET_CUR = 0x81,
	USB_REQ_MIDI_GET_MIN = 0x82,
	USB_REQ_MIDI_GET_MAX = 0x83,
	USB_REQ_MIDI_GET_RES = 0x84,
	USB_REQ_MIDI_GET_MEM = 0x85,
	USB_REQ_MIDI_GET_STAT = 0xFF,
};

//! \brief HID USB descriptor types
enum usb_descriptor_type_midi {
	CS_INTERFACE                    = 0x24, /**< Indicates that the descriptor is a class specific interface descriptor. */
	CS_ENDPOINT                     = 0x25, /**< Indicates that the descriptor is a class specific endpoint descriptor. */
};

//! \brief HID protocol
enum usb_midi_protocol {
	USB_MIDI_PROCOTOL_BOOT = 0,
};

enum usb_ms_iface_desc_subtype {
	MS_DESCRIPTOR_UNDEFINED = 0x00,
	MS_HEADER = 0x01,
	MIDI_IN_JACK = 0x02,
	MIDI_OUT_JACK = 0x03,
	ELEMENT = 0x04,
};

enum usb_ac_iface_desc_subtype {
	AC_DESCRIPTOR_UNDEFINED = 0x00,
	HEADER = 0x01,
};

enum usb_ms_ep_desc_subtype {
	DESCRIPTOR_UNDEFINED = 0x00,
	MS_GENERAL = 0x01,
};

enum usb_midi_jack_type {
	JACK_TYPE_UNDEFINED = 0x00,
	EMBEDDED = 0x01,
	EXTERNAL = 0x02,	
};

enum usb_midi_ep_ctrl_sel {
	EP_CONTROL_UNDEFINED = 0x00,
	ASSOCIATION_CONTROL = 0x01,
};

COMPILER_PACK_SET(1)

typedef struct {
	uint8_t bLength;            //!< Size of this descriptor in bytes
	uint8_t bDescriptorType;    //!< descriptor type
	uint8_t bDescriptorSubtype; //!< descriptor subtype
	le16_t bcdMSC;             //!< midi streaming specification release number
	le16_t wTotalLength;       //!< combined length of this descriptor header and all jack and element descriptors
}usb_ms_descriptor_t;

//! \brief MIDI Descriptor
typedef struct {
	uint8_t bLength;           //!< Size of this descriptor in bytes
	uint8_t bDescriptorType;   //!< HID descriptor type
	uint8_t bElementID;        //!< constant ID
	uint8_t bNrInputPins;  //!< number of input pins of this MIDI OUT jack: p
	uint8_t baSourceID;		   //!< ID of item that input pin of this jack is connected to
	uint8_t baSourcePin;       //!< output pin that this jack is connected to
	uint8_t bNrOutputPins; //!< number of input pins of this MIDI OUT jack: p
	uint8_t bInTerminalLink;   //!< ID of item that input pin of this jack is connected to
	uint8_t bOutTerminalLink;  //!< output pin that this jack is connected to
	uint8_t bElCapsSize;       //!< size in bytes of bmElementCaps field
	uint8_t bmElementCaps;     //!< 
	uint8_t iElement;          //!< index of string descriptor
} usb_midi_descriptor_t;

typedef struct{
	uint8_t bLength;            //!< Size of this descriptor in bytes
	uint8_t bDescriptorType;    //!< descriptor type
	uint8_t bDescriptorSubtype; //!< descriptor subtype
	uint8_t bJackType;          //!< embedded or external
	uint8_t bjackID;            //!< constant ID
	uint8_t iJack;              //!< index of string descriptor
}usb_jack_in_descriptor_t;

typedef struct{
	uint8_t bLength;            //!< Size of this descriptor in bytes
	uint8_t bDescriptorType;    //!< descriptor type
	uint8_t bDescriptorSubtype; //!< descriptor subtype
	uint8_t bJackType;          //!< embedded or external
	uint8_t bjackID;            //!< constant ID
	uint8_t bNrInputPins;   //!< number of input pins of this MIDI OUT jack: p
	uint8_t baSourceID;		    //!< ID of item that input pin of this jack is connected to
	uint8_t baSourcePin;        //!< output pin that this jack is connected to
	uint8_t iJack;              //!< index of string descriptor
}usb_jack_out_descriptor_t;

//typedef struct
//{
//	uint8_t Size; /**< Size of the descriptor, in bytes. */
//	uint8_t Type; /**< Type of the descriptor, either a value in \ref USB_DescriptorTypes_t or a value
///				    *   given by the specific class.
//				    */
//} USB_Descriptor_Header_t;

typedef struct
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype; //< Sub type value used to distinguish between audio class-specific descriptors,
								//		*   a value from the \ref Audio_CSInterface_AS_SubTypes_t enum.*/
	uint16_t bcdADC; //< Binary Coded Decimal value, indicating the supported Audio Class specification version.
								//				*   \see \ref VERSION_BCD() utility macro.*/
	uint16_t wTotalLength; //< Total length of the Audio class-specific descriptors, including this descriptor. */
	uint8_t bInCollection; //< Total number of Audio Streaming interfaces linked to this Audio Control interface (must be 1). */
	uint8_t baInterfaceNr; //< Interface number of the associated Audio Streaming interface. */
} usb_audio_desc_t;

typedef struct
{
	usb_ep_desc_t ep; /**< Standard endpoint descriptor describing the audio endpoint. */
	uint8_t bRefresh; /**< Always set to zero for Audio class devices. */
	uint8_t bSynchAddress; /**< Endpoint address to send synchronization information to, if needed (zero otherwise). */
} usb_midi_ep_desc_t;


typedef struct
{
	uint8_t bLength;            //!< Size of this descriptor in bytes
	uint8_t bDescriptorType;    //!< descriptor type
	uint8_t bDescriptorSubtype; //!< descriptor subtype
	uint16_t bcdADC; 
	uint16_t wTotalLength; /**< Total length of the Audio class-specific descriptors, including this descriptor. */
} usb_stream_desc_t;	

typedef struct
{
	uint8_t bLength;            //!< Size of this descriptor in bytes
	uint8_t bDescriptorType;    //!< descriptor type
	uint8_t bDescriptorSubtype; //!< descriptor subtype
	uint8_t bNumEmbMIDIJack; /**< Total number of jacks inside this endpoint. */
	uint8_t BaAssocJackID; /**< IDs of each jack inside the endpoint. */
} usb_midi_ep_desc_cs;	

COMPILER_PACK_RESET()

   //! \name Constants of field DESCRIPTOR_HID
   //! @{
//! Numeric expression identifying the HID Class
//! Specification release (here V1.00)
#define  USB_MIDI_BDC_V1_00                    0x0100
//! Numeric expression specifying the number of class descriptors
//! Note: Always at least one i.e. Report descriptor.
#define  USB_MIDI_NUM_DESC                     0x01

#endif // _USB_PROTOCOL_MIDI_H_
