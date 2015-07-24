/*
 * uhi_midi.c
 *
 * Created: 7/13/2015 5:08:13 PM
 *  Author: Elaine
 */ 



#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "uhi_midi.h"
#include "uhd.h"
#include "uhc.h"
#include "dip204.h"
#include "main.h"
#include "note_process.h"
#include <string.h>

#ifdef USB_HOST_HUB_SUPPORT
# error USB HUB support is not implemented on UHI mouse
#endif

#define UHI_MIDI_BUFFER_SIZE 64

/**
 * \ingroup uhi_hid_mouse_group
 * \defgroup uhi_hid_mouse_group_internal Implementation of UHI HID Mouse
 *
 * Class internal implementation
 * @{
 */

/**
 * \name Structure to store information about USB Device MIDI
 */
//@{

// internal buffer information
typedef struct {
	uint16_t pos;  // position of consumer in this buffer
	uint16_t nb;   // number of data bytes in available in buffer
	uint8_t* ptr;  // pointer on internal buffer	
}uhi_midi_buf_t;

// communication line information
typedef struct{
	usb_ep_t ep_data;  // bulk endpoint number used to transfer data
	bool b_trans_ongoing; 
	uint16_t sof;  // frame of last transfer
	volatile uint8_t buf_sel;  // current internal buffer used to store data
	uint16_t buffer_size;  // size of internal buffer in bytes
	uhi_midi_buf_t buffer[2]; // internal buffer information	
}uhi_midi_line_t;

typedef struct {
	uhc_device_t *dev;
	uint8_t iface_num;
	bool b_enabled;
	uhi_midi_line_t line_rx;
	uhi_midi_line_t line_tx;
}uhi_midi_dev_t;
//@}

static uhi_midi_dev_t uhi_midi_dev = {
	.dev = NULL,
};

static void uhi_midi_free_device(void);
static bool uhi_midi_rx_update(uhi_midi_line_t *line);
static void uhi_midi_rx_received(usb_add_t add, usb_ep_t ep,
		uhd_trans_status_t status, iram_size_t nb_transferred);
static bool uhi_midi_tx_update(uhi_midi_line_t *line);
static void uhi_midi_tx_send(usb_add_t add, usb_ep_t ep,
	uhd_trans_status_t status, iram_size_t nb_transferred);
static uint8_t parseMIDI(uint8_t maxBytes);
static void handleKey(uint8_t ctrlByte, uint8_t msgByte1, uint8_t msgByte2);

uint8_t firstMsg = 1;

/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_midi_install(uhc_device_t* dev)
{
	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;
	uhi_midi_line_t *ptr_line;
	
	
	if (uhi_midi_dev.dev != NULL) {
		return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
	}
	conf_desc_lgt = le16_to_cpu(dev->conf_desc->wTotalLength);
	ptr_iface = (usb_iface_desc_t*)dev->conf_desc;
	b_iface_supported = false;
	while(conf_desc_lgt) {
		switch (ptr_iface->bDescriptorType) {

		case USB_DT_INTERFACE:
			if ((ptr_iface->bInterfaceClass == AUDIO_CLASS)
			&& (ptr_iface->bInterfaceProtocol == 0) ) {
				// USB MIDI interface found
				// Start allocation endpoint(s)
				b_iface_supported = true;
				uhi_midi_dev.iface_num = ptr_iface->bInterfaceNumber;
				uhi_midi_dev.line_rx.ep_data = 0;
				uhi_midi_dev.line_tx.ep_data = 0;
			} else {
				// Stop allocation endpoint(s)
				b_iface_supported = false;
			}
			break;

		case USB_DT_ENDPOINT:
			//  Allocation of the endpoint
			if (!b_iface_supported) {
				break;
			}
			if (!uhd_ep_alloc(dev->address, (usb_ep_desc_t*)ptr_iface)) {
				return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail
			}
			switch(((usb_ep_desc_t*)ptr_iface)->bmAttributes & USB_EP_TYPE_MASK)
			{
				case USB_EP_TYPE_BULK:
					if(((usb_ep_desc_t*)ptr_iface)->bEndpointAddress & USB_EP_DIR_IN)
						ptr_line = &uhi_midi_dev.line_rx;
					else
						ptr_line = &uhi_midi_dev.line_tx;
					ptr_line->ep_data = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
					ptr_line->b_trans_ongoing = false;
					ptr_line->buf_sel = 0;
					
					// Allocate and initialize buffers
					uint16_t buf_size = Max( le16_to_cpu(
							((usb_ep_desc_t*)ptr_iface)->wMaxPacketSize),
							UHI_MIDI_BUFFER_SIZE );
					ptr_line->buffer_size = buf_size;
					ptr_line->buffer[0].pos = 0;
					ptr_line->buffer[0].nb = 0;
					ptr_line->buffer[0].ptr = calloc(buf_size,sizeof(uint8_t));
					if (ptr_line->buffer[0].ptr == NULL) {
						Assert(false);
						uhi_midi_free_device();
						return UHC_ENUM_SOFTWARE_LIMIT;
					}
					ptr_line->buffer[1].pos = 0;
					ptr_line->buffer[1].nb = 0;
					ptr_line->buffer[1].ptr = calloc(buf_size,sizeof(uint8_t));
					if (ptr_line->buffer[1].ptr == NULL) {
						Assert(false);
						uhi_midi_free_device();
						return UHC_ENUM_SOFTWARE_LIMIT;
					}
					break;
				default:
					break;
			}
			break;

		default:
			// Ignore descriptor
			break;
		}
		Assert(conf_desc_lgt>=ptr_iface->bLength);
		conf_desc_lgt -= ptr_iface->bLength;
		ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
	}
		
	// All endpoints of all interfaces supported allocated
	if(uhi_midi_dev.line_rx.ep_data)
	{
		uhi_midi_dev.b_enabled = false;
		uhi_midi_dev.dev = dev;
			
		return UHC_ENUM_SUCCESS;
	}
	
	uhi_midi_free_device();
	return UHC_ENUM_UNSUPPORTED; // No interface supported
}

void uhi_midi_enable(uhc_device_t* dev)
{
	if (uhi_midi_dev.dev != dev) {
		return;  // No interface to enable
	}

	uhi_midi_dev.b_enabled = true;

	// Init value
	initNoteStack();
	firstMsg = 1;
	//uhi_midi_sof(false);
	UHI_MIDI_CHANGE(dev, true);
}

void uhi_midi_uninstall(uhc_device_t* dev)
{
	if (uhi_midi_dev.dev != dev) {
		return; // Device not enabled in this interface
	}
	uhi_midi_dev.dev = NULL;
	uhi_midi_free_device();
	UHI_MIDI_CHANGE(dev, false);
	initNoteStack();
}
//@}

/**
 * \name Internal routines
 */
//@{

static void uhi_midi_free_device(void)
{
	if(uhi_midi_dev.line_rx.buffer[0].ptr)
		free(uhi_midi_dev.line_rx.buffer[0].ptr);
	if (uhi_midi_dev.line_rx.buffer[1].ptr)
		free(uhi_midi_dev.line_rx.buffer[1].ptr);
	if (uhi_midi_dev.line_tx.buffer[0].ptr)
		free(uhi_midi_dev.line_tx.buffer[0].ptr);
	if (uhi_midi_dev.line_tx.buffer[1].ptr)
		free(uhi_midi_dev.line_tx.buffer[1].ptr);
}

void uhi_midi_sof(bool b_micro)
{
	UNUSED(b_micro);

	if (uhi_midi_dev.dev == NULL) {
		return; // No interface to installed
	}
	if (!uhi_midi_dev.b_enabled) {
		return; // Interface not enabled
	}

	// Update transfers
	uhi_midi_line_t *line = &uhi_midi_dev.line_rx;
	uhi_midi_rx_update(line);
	//uhi_midi_buf_t *buf = &line->buffer[(line->buf_sel == 0) ? 1 : 0];
	uhi_midi_buf_t *buf = &line->buffer[0];
	parseMIDI(buf->nb);
	buf->pos = buf->nb;
	//uhi_midi_tx_update(&uhi_midi_dev.line_tx);
}

static bool uhi_midi_rx_update(uhi_midi_line_t *line)
{
	irqflags_t flags;
	uhi_midi_buf_t *buf_nosel;
	uhi_midi_buf_t *buf_sel;

	flags = cpu_irq_save();
	// Check if transfer is already on-going
	if (line->b_trans_ongoing) {
		cpu_irq_restore(flags);
		return false;
	}

	// Search an empty buffer to start a transfer
	//buf_sel = &line->buffer[line->buf_sel];
	//buf_nosel = &line->buffer[(line->buf_sel == 0)? 1 : 0];
	buf_sel = &line->buffer[0];
	buf_nosel = &line->buffer[0];
	if (buf_sel->pos >= buf_sel->nb) {
		// The current buffer has been read
		// then reset it
		buf_sel->pos = 0;
		buf_sel->nb = 0;
	}/*
	if (!buf_sel->nb && buf_nosel->nb) {
		// New data available then change current buffer
		line->buf_sel = (line->buf_sel == 0)? 1 : 0;
		buf_nosel = buf_sel;
	}					

	if (buf_nosel->nb) {
		// No empty buffer available to start a transfer
		cpu_irq_restore(flags);
		return false;
	}*/
	if(buf_sel->nb)
	{
		cpu_irq_restore(flags);
		return false;
	}

	// Check if transfer must be delayed after the next SOF
	if (uhi_midi_dev.dev->speed == UHD_SPEED_HIGH) {
		if (line->sof == uhd_get_microframe_number()) {
			cpu_irq_restore(flags);
			return false;
		}
	} else {
		if (line->sof == uhd_get_frame_number()) {
			cpu_irq_restore(flags);
			return false;
		}
	}

	// Start transfer on empty buffer
	line->b_trans_ongoing = true;
	cpu_irq_restore(flags);

	return uhd_ep_run(
	uhi_midi_dev.dev->address,
	line->ep_data,
	true,
	buf_nosel->ptr,
	line->buffer_size,
	10,
	uhi_midi_rx_received);
}

static void uhi_midi_rx_received(
usb_add_t add,
usb_ep_t ep,
uhd_trans_status_t status,
iram_size_t nb_transferred)
{
	uhi_midi_line_t *line;
	uhi_midi_buf_t *buf;
	
	UNUSED(add);

	// Search port corresponding at endpoint
	line = &(uhi_midi_dev.line_rx);
  
	if ((UHD_TRANS_TIMEOUT == status) && nb_transferred) {
		// Save transfered
	}
	else if (UHD_TRANS_NOERROR != status) {
		// Abort transfer
		line->b_trans_ongoing  = false;
		return;
	}
	
	// Update SOF tag, if it is a short packet
	if (nb_transferred != line->buffer_size) {
		if (uhi_midi_dev.dev->speed == UHD_SPEED_HIGH) {
			line->sof = uhd_get_microframe_number();
			} else {
			line->sof = uhd_get_frame_number();
		}
	}
	
	// Update buffer structure
	//buf = &line->buffer[(line->buf_sel == 0) ? 1 : 0];
	buf = &line->buffer[0];
	buf->pos = 0;
	buf->nb = nb_transferred;
	line->b_trans_ongoing  = false;
	
	// Manage new transfer
	uhi_midi_rx_update(line);
}


static bool uhi_midi_tx_update(uhi_midi_line_t *line)
{
	irqflags_t flags;
	uhi_midi_buf_t *buf;

	flags = cpu_irq_save();
	// Check if transfer is already on-going
	if (line->b_trans_ongoing) {
		cpu_irq_restore(flags);
		return false;
	}
	// Check if transfer must be delayed after the next SOF
	if (uhi_midi_dev.dev->speed == UHD_SPEED_HIGH) {
		if (line->sof == uhd_get_microframe_number()) {
			cpu_irq_restore(flags);
			return false;
		}
		} else {
		if (line->sof == uhd_get_frame_number()) {
			cpu_irq_restore(flags);
			return false;
		}
	}

	// Send the current buffer if not empty
	buf = &line->buffer[line->buf_sel];
	if (buf->nb == 0) {
		cpu_irq_restore(flags);
		return false;
	}

	// Change current buffer to next buffer
	line->buf_sel = (line->buf_sel == 0)? 1 : 0;

	// Start transfer
	line->b_trans_ongoing = true;
	cpu_irq_restore(flags);

	return uhd_ep_run(
	uhi_midi_dev.dev->address,
	line->ep_data,
	true,
	buf->ptr,
	buf->nb,
	1000,
	uhi_midi_tx_send);
}


static void uhi_midi_tx_send(
usb_add_t add,
usb_ep_t ep,
uhd_trans_status_t status,
iram_size_t nb_transferred)
{
	uhi_midi_line_t *line;
	uhi_midi_buf_t *buf;
	irqflags_t flags;
	UNUSED(add);

	flags = cpu_irq_save();

	// Search port corresponding at endpoint
	while (1) {
		line = &(uhi_midi_dev.line_tx);
		if (ep != line->ep_data) {
			cpu_irq_restore(flags);
			return;  // something went wrong
		}
	}

	if (UHD_TRANS_NOERROR != status) {
		// Abort transfer
		line->b_trans_ongoing  = false;
		cpu_irq_restore(flags);
		return;
	}

	// Update SOF tag, if it is a short packet
	if (nb_transferred != line->buffer_size) {
		if (uhi_midi_dev.dev->speed == UHD_SPEED_HIGH) {
			line->sof = uhd_get_microframe_number();
			} else {
			line->sof = uhd_get_frame_number();
		}
	}

	// Update buffer structure
	buf = &line->buffer[(line->buf_sel == 0) ? 1 : 0 ];
	buf->nb = 0;
	line->b_trans_ongoing  = false;
	cpu_irq_restore(flags);

	// Manage new transfer
	uhi_midi_tx_update(line);
}

uint8_t prevCtrlByte = 0;
uint8_t prevMsgByte1 = 0;
uint8_t prevMsgByte2 = 0;

static uint8_t parseMIDI(uint8_t maxBytes)
{
	uhi_midi_line_t *line;
	uhi_midi_buf_t *buf;
	uint8_t ctrlByte;
	uint8_t msgByte1;
	uint8_t msgByte2;
	uint8_t i;
	
	line = &(uhi_midi_dev.line_rx);
	//buf = &line->buffer[(line->buf_sel == 0) ? 1 : 0];
	buf = &line->buffer[0];
	i = buf->pos;
	
	ctrlByte = buf->ptr[++i];
	buf->ptr[i]=0;  // mark as read
	msgByte1 = buf->ptr[++i];
	msgByte2 = buf->ptr[++i];

	// throw away first message
	if(ctrlByte != 0 && firstMsg)
	{
		firstMsg = 0;
		prevCtrlByte = ctrlByte;
		prevMsgByte1 = msgByte1;
		prevMsgByte2 = msgByte2;
		return 0;
	}
	
	if(ctrlByte != 0)
	{
		prevCtrlByte = ctrlByte;
		prevMsgByte1 = msgByte1;
		prevMsgByte2 = msgByte2;
		i++;
		do
		{
			handleKey(ctrlByte,msgByte1,msgByte2);
			ctrlByte = buf->ptr[++i];
			msgByte1 = buf->ptr[++i];
			msgByte2 = buf->ptr[++i];
			i++;
		} while(ctrlByte != 0 && i < line->buffer_size);
		if(ctrlByte != 0)
			handleKey(ctrlByte,msgByte1,msgByte2);
			
			lcd_clear_line(4);
			dip204_printf_string("%u of %u",i,maxBytes);			

	}
	return i;
}

static void handleKey(uint8_t ctrlByte, uint8_t msgByte1, uint8_t msgByte2)
{
	uint8_t control = ctrlByte & 0xf0;
	//uint8_t channel = ctrlByte & 0x0f;
	
	/*
	lcd_clear_line(1);
	dip204_printf_string("control: %u",ctrlByte);
	lcd_clear_line(2);
	dip204_printf_string("note: %u", msgByte1);*/

	switch(control)
	{
		case 144:
			addNote(msgByte1,msgByte2);

			//DAC16Send(1,calculateDACvalue());
			noteOut();
			midiVol();
			break;
		case 128:
			removeNote(msgByte1);
			//DAC16Send(1,calculateDACvalue());
			noteOut();
			midiVol();
			break;
		// control change
		case 176:
			controlChange(msgByte1,msgByte2);
			break;
		// program change	
		case 192:
			programChange(msgByte1);
		default:
			break;
	}
}
