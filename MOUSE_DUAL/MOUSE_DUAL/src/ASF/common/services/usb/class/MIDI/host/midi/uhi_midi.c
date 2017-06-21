/*
 * uhi_midi.c
 *
 * Created: 7/13/2015 5:08:13 PM
 *  Author: Elaine
 */ 


#include <asf.h>
#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "uhi_midi.h"
#include "uhd.h"
#include "uhc.h"
#include "main.h"
#include "note_process.h"
#include "7Segment.h"
#include "midi.h"
#include <string.h>

#ifdef USB_HOST_HUB_SUPPORT
# error USB HUB support is not implemented on UHI mouse
#endif


#ifndef UHI_MIDI_RX_NOTIFY
#  define UHI_MIDI_RX_NOTIFY()
#endif

//#define UHI_MIDI_BUFFER_SIZE 64
// cdc has low speed rate with 64, and non low with 5 * 64
//#define UHI_MIDI_BUFFER_SIZE 64 * 5
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
	uint16_t nb;   // number of data bytes available in buffer
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

//device information
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
bool uhi_midi_is_rx_ready(void);
iram_size_t uhi_midi_get_nb_received(void);
int uhi_midi_getc(void);
iram_size_t uhi_midi_read_buf(void* buf, iram_size_t size);

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
	
	// we added this because sometime USB MIDI devices give garbage in the first transfer
	firstMIDIMessage = 1;
	
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
	type_of_device_connected = MIDIKeyboardConnected;
	// Init value
	initNoteStack();
	manta_mapper = 0; // make sure there is no weird MIDI note mapping going on if the manta was previously being communicated with.
	UHI_MIDI_CHANGE(dev, true);
}

void uhi_midi_uninstall(uhc_device_t* dev)
{
	if (uhi_midi_dev.dev != dev) {
		return; // Device not enabled in this interface
	}
	uhi_midi_dev.dev = NULL;
	type_of_device_connected = NoDeviceConnected;
	uhi_midi_free_device();
	UHI_MIDI_CHANGE(dev, false);
	initNoteStack();
}
//@}


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

uint8_t whichPos;

void uhi_midi_sof(bool b_micro)
{
	UNUSED(b_micro);

	if (uhi_midi_dev.dev == NULL) {
		return; // No interface to installed
	}
	if (!uhi_midi_dev.b_enabled) {
		return; // Interface not enabled
	}


	
	if (clock_speed != 0)
	{
		if (USB_frame_counter == clock_speed) {
			clockHappened();
			USB_frame_counter = 0;
		}
		USB_frame_counter++;
	}
	// Update transfers
	//JS - seems like this function checks whether the currently selected buffer is done reading, and it resets it if so
	// then, it starts the EP transfer if there is an available buffer
	uhi_midi_line_t *line = &uhi_midi_dev.line_rx;
	uhi_midi_rx_update(line);
		/*
	uhi_midi_buf_t *buf;

	if (line->buf_sel == 0)
	{
		buf = &line->buffer[1];
		line->buf_sel = 1;
	}
	else 
	{
		buf = &line->buffer[0];
		line->buf_sel = 0;
	}
	*/
		// this stuff isn't part of CDC - is it from HID? 
	//uhi_midi_buf_t *buf = &line->buffer[(line->buf_sel == 0) ? 1 : 0];
	//uhi_midi_buf_t *buf = &line->buffer[0];
	//whichPos = parseMIDI(buf->nb);
	//buf->pos = buf->nb;
	//uhi_midi_tx_update(&uhi_midi_dev.line_tx);
	//uhi_midi_read_buf(buf, buf->nb);
}

static bool uhi_midi_rx_update(uhi_midi_line_t *line)
{
	irqflags_t flags;
	uhi_midi_buf_t *buf_nosel;
	uhi_midi_buf_t *buf_sel;

	flags = cpu_irq_save();
	// Check if transfer is already on-going
	if (line->b_trans_ongoing) 
	{
		cpu_irq_restore(flags);
		return false;
	}
	
	// Search a empty buffer to start a transfer

	//this makes local variables for the buffers that relate to sel and nosel
	buf_sel = &line->buffer[line->buf_sel];
	buf_nosel = &line->buffer[(line->buf_sel == 0)? 1 : 0];
	
	//if the position in the buffer has gone above or is equal to the number of bytes that were last transferred, reset the number of bytes transferred and the position to 0.
	if (buf_sel->pos >= buf_sel->nb) 
	{
		// The current buffer has been read
		// then reset it
		buf_sel->pos = 0;
		buf_sel->nb = 0;
		
	}
	
	//if the selected buffer is now empty and the other buffer has bytes to transfer, switch the selected and nonselected buffers, and assume there is new data to grab??
	if (!buf_sel->nb && buf_nosel->nb) 
	{
		// New data available then change current buffer
		line->buf_sel = (line->buf_sel == 0)? 1 : 0;
		buf_nosel = buf_sel;
		//LED_On(LED1);
		UHI_MIDI_RX_NOTIFY();

	}
	
	// if the nonselected buffer has stuff in it (but the currently selected buffer isn't empty!) then throw an error because there isn't an empty buffer available to start a transfer.
	if (buf_nosel->nb) 
	{
		// No empty buffer available to start a transfer
		cpu_irq_restore(flags);
		//LED_On(LED0);
		return false;
	}

	// Check if transfer must be delayed after the next SOF
	if (uhi_midi_dev.dev->speed == UHD_SPEED_HIGH) 
	{
		if (line->sof == uhd_get_microframe_number()) 
		{
			cpu_irq_restore(flags);
			return false;
		}
	} 
	else 
	{
		if (line->sof == uhd_get_frame_number()) 
		{
			cpu_irq_restore(flags);
			return false;
		}
	}

	// Start transfer on empty buffer

	// ok, either both buffers are empty, or there is an empty buffer that we just re-assigned as "nonselected". 
	// in this case, we are ready to set up an endpoint transfer into the empty buffer
	// let the system know that there is a transmission ongoing
	line->b_trans_ongoing = true;
	cpu_irq_restore(flags);
	
	//stupid test so that the LEDs show which buffer is selected
	/*
	if (line->buf_sel == 0)
	{
		LED_On(LED3);
	}
	else
	{
		LED_Off(LED3);
	}
	if (line->buf_sel == 1)
	{
		LED_On(LED2);
	}
	else
	{
		LED_Off(LED2);
	}
*/
	///  
	/// it appears that the ep run function gets data from the endpoint and puts it in the "nosel" buffer. Then it runs the rx_received function once this has finished.
	// there was at one point an issue, where if it times out then it thinks it has new data. I lengthened the timeout, and also made the rx_received function ignore data in the event of a timeout. JS
	return uhd_ep_run(
	uhi_midi_dev.dev->address,
	line->ep_data,
	true,
	buf_nosel->ptr,
	line->buffer_size,
	1000,
	uhi_midi_rx_received);
	//timeout was 10 originally
	

	
	/*
	//buf_sel = &line->buffer[0];
	//buf_nosel = &line->buffer[0];
	
	if (buf_sel->pos >= buf_sel->nb) {
		// The current buffer has been read
		// then reset it
		buf_sel->pos = 0;
		buf_sel->nb = 0;
		LED_On(LED0);
	}
	if (!buf_sel->nb && buf_nosel->nb) {
		// New data available then change current buffer
		line->buf_sel = (line->buf_sel == 0)? 1 : 0;
		//buf_nosel = buf_sel;
		LED_On(LED1);
	}					

	if (buf_nosel->nb) {
		// No empty buffer available to start a transfer
		cpu_irq_restore(flags);
		LED_On(LED3);
		return false;
	}
	
	if(buf_sel->nb)
	{
		cpu_irq_restore(flags);
		LED_On(LED3);
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
	*/
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

	// Search port corresponding at endpoint - (used to be searching port (when it was CDC), now just sets the transmission line to the one we know)
	line = &(uhi_midi_dev.line_rx);
  
	if ((UHD_TRANS_TIMEOUT == status) && nb_transferred) {
		//LED_On(LED5);
		line->b_trans_ongoing  = false;
		return;
		// this used to not return, it went on and saved the bytes even in the case of a timeout. That caused the software to constantly think it was getting new data even though they were just timeouts.
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
	buf = &line->buffer[(line->buf_sel == 0) ? 1 : 0];
	buf->pos = 0;
	buf->nb = nb_transferred;
	line->b_trans_ongoing  = false;
	// Manage new transfer
	// note that rx_update calls this function, or at least sets up the endpoint transfer that will call this function when it is done. 
	uhi_midi_rx_update(line);
}


bool uhi_midi_is_rx_ready(void)
{
	return (0 != uhi_midi_get_nb_received());
}

iram_size_t uhi_midi_get_nb_received(void)
{
	uhi_midi_buf_t *buf;
	uhi_midi_line_t *line;
	int howMany = 0;
	// Check available data
	line = &(uhi_midi_dev.line_rx);
	buf = &line->buffer[line->buf_sel];
	/*
	if (buf->pos == 0)
	{
		LED_On(LED1);
	}
	else
	{
		LED_Off(LED1);
	}
	*/

	howMany = buf->nb - buf->pos;
	//Write7Seg(howMany);
	return (howMany);
}

int uhi_midi_getc(void)
{
	uhi_midi_line_t *line;
	uhi_midi_buf_t *buf;
	int rx_data = 0;
	int tempbuffer[UHI_MIDI_BUFFER_SIZE];
	
	line = &(uhi_midi_dev.line_rx);

	uhi_midi_getc_process_one_byte:
	// Check available data
	buf = &line->buffer[line->buf_sel];
	while (buf->pos >= buf->nb) {
		uhi_midi_rx_update(line);
		goto uhi_midi_getc_process_one_byte;
	}

	// Read data
	//rx_data |= buf->ptr[buf->pos];
	rx_data = buf->ptr[buf->pos]; // why is this or-equalled in the original CDC code??? That doesn't make sense to me.
	buf->pos++;
	
	for (int i = 0; i < UHI_MIDI_BUFFER_SIZE; i++)
	{
		tempbuffer[i] = buf->ptr[i];
	}
	
	uhi_midi_rx_update(line);

	return rx_data;
}

iram_size_t uhi_midi_read_buf(void* buf, iram_size_t size)
{
	uhi_midi_line_t *line;
	uhi_midi_buf_t *midi_buf;
	iram_size_t copy_nb;
	uint16_t newBytes = 0;
	
	//set the line to be the receive line
	line = &(uhi_midi_dev.line_rx);

	//this is the loop that ????
	uhi_midi_read_buf_loop_wait:
	// Check available data
	// set midi_buf to the currently selected buffer (this should be the one that has new data to be read)
	midi_buf = &line->buffer[line->buf_sel];
	
	newBytes = midi_buf->nb;
	// keep running "rx_update" as long as the position in the buffer is greater-than-or-equal-to the number of bytes (why?)
	/*
	while (midi_buf->pos >= midi_buf->nb) {
		uhi_midi_rx_update(line);
		goto uhi_midi_read_buf_loop_wait;
	}
	*/
	//Write7Seg(midi_buf->nb);
	// Read data
	//make a frozen copy of the number of bytes minus the position (this represents the number of bytes left to read)
	copy_nb = midi_buf->nb - midi_buf->pos;
	// if the number of bytes left to read is bigger than the size of the read that was passed to the function, then only read the amount passed as size (seems like the passed amount is like a chunk size?)
	if (copy_nb > size) {
		copy_nb = size;
	}
	//copy (the number of bytes left to read) bytes starting at the current buffer position to the "my_buffer" that was passed.
	memcpy(buf, &midi_buf->ptr[midi_buf->pos], copy_nb);
	
	//now move the position in the buffer to be past the number of bytes you just copied.
	midi_buf->pos += copy_nb;
	//this is confusing... I guess this is moving the pointer to the "my_buffer" up to the point past the data we just copied in case it loops again because the chunk size was smaller than the amount of data that needed to be copied
	buf = (uint8_t*)buf + copy_nb;
	//now we subtract the amount of bytes we just copied from size, so I guess "size" is the size of the chunk you want to read into the buffer on each iteration of this loop.
	size -= copy_nb;

	// now call "rx_update" -- why??
	uhi_midi_rx_update(line);

    /*
	// if you've still got bytes left to transfer (because copy_nb was bigger than "size" that was passed in") then go back and continue copying out the buffer.
	if (size) {
		goto uhi_midi_read_buf_loop_wait;
	}
	*/
	// return zero if the function was successful
	return newBytes;
}
