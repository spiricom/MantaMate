/*
 * uhi_midi.c
 *
 * Created: 7/13/2015 5:08:13 PM
 *  Author: Elaine
 */ 


#include "uhi_midi.h"
#include "uhc.h"
#include <string.h>

#ifdef USB_HOST_HUB_SUPPORT
# error USB HUB support is not implemented on UHI mouse
#endif
#include "led.h"

#define UHI_MIDI_TIMEOUT 20000
#define MIDI_RX_BUF_SIZE 64
#define MIDI_TX_BUF_SIZE 64

/**
 * \ingroup uhi_hid_mouse_group
 * \defgroup uhi_hid_mouse_group_internal Implementation of UHI HID Mouse
 *
 * Class internal implementation
 * @{
 */

#define MIDI_EP_DESC_REQ_TYPE ( (USB_REQ_DIR_IN) | (USB_REQ_TYPE_STANDARD) | (USB_REQ_RECIP_DEVICE))

#define MIDI_EP_OUT_DESC_REQ
/**
 * \name Structure to store information about USB Device MIDI
 */
//@{
typedef struct {
	uhc_device_t *dev;
	uint8_t *report;
	uint8_t report_size;
	usb_ep_t ep_in;
	usb_ep_t ep_out;
	USB_Pipe_Table_t data_in_pipe;
	USB_Pipe_Table_t data_out_pipe;
	bool isActive;
	uint8_t ifaceNum;
}uhi_midi_dev_t;

static uhi_midi_dev_t uhi_midi_dev = {
	.dev = NULL,
	.report = NULL,
	.ep_in = 0xf,
	.ep_out = 0xf,
};

//@}


/**
 * \name Internal routines
 */
//@{
static void uhi_midi_start_trans_report(usb_add_t add);
static void uhi_midi_report_reception(
	usb_add_t add,
	usb_ep_t ep,
	uhd_trans_status_t status,
	iram_size_t nb_transfered);
//@}

//static uint8_t *uhi_midi_data;
volatile uint8_t     USB_HostState = 0;
/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_midi_install(uhc_device_t* dev)
{
	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;
	
	
	if (uhi_midi_dev.dev != NULL) {
		return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
	}
	conf_desc_lgt = le16_to_cpu(dev->conf_desc->wTotalLength);
	ptr_iface = (usb_iface_desc_t*)dev->conf_desc;
	b_iface_supported = false;
	while(conf_desc_lgt) {
		switch (ptr_iface->bDescriptorType) {

		case USB_DT_INTERFACE:
			USB_HostState = 0;
			if ((ptr_iface->bInterfaceClass == AUDIO_CLASS) && (ptr_iface->bInterfaceProtocol == 0) ) {
				// USB MIDI interface found
				// Start allocation endpoint(s)
				USB_HostState = 11;
				b_iface_supported = true;
				uhi_midi_dev.ep_in = 0;
				uhi_midi_dev.ep_out = 0;
			} 
			
			else 
			{
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
						uhi_midi_dev.ep_in = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
					else if(((usb_ep_desc_t*)ptr_iface)->bEndpointAddress & USB_EP_DIR_OUT)
						uhi_midi_dev.ep_out = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
					break;
				default:
					break;
			}
			uhi_midi_dev.dev = dev;
			// All endpoints of all interfaces supported allocated
			return UHC_ENUM_SUCCESS;
			
		default:
			// Ignore descriptor
			break;
		}
		Assert(conf_desc_lgt>=ptr_iface->bLength);
		conf_desc_lgt -= ptr_iface->bLength;
		ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
	}
	return UHC_ENUM_UNSUPPORTED; // No interface supported
}

void uhi_midi_enable(uhc_device_t* dev)
{
	if (uhi_midi_dev.dev != dev) {
		return;  // No interface to enable
	}

	// Init value
	uhi_midi_start_trans_report(dev->address);
	UHI_MIDI_CHANGE(dev, true);
}

void uhi_midi_uninstall(uhc_device_t* dev)
{
	if (uhi_midi_dev.dev != dev) {
		return; // Device not enabled in this interface
	}
	uhi_midi_dev.dev = NULL;
	UHI_MIDI_CHANGE(dev, false);
	USB_HostState = 0;
}

static void uhi_midi_start_trans_report(usb_add_t add)
{
	// Start transfer on interrupt endpoint IN
	uhd_ep_run(add, uhi_midi_dev.ep_in, true, uhi_midi_dev.report,
	uhi_midi_dev.report_size, 0, uhi_midi_report_reception);
}
//@}

/**
 * \name Internal routines
 */
//@{

/**
 * \brief Starts the reception of the MIDI mouse report
 *
 * \param add   USB address to use
 */
bool uhi_midi_write(uint8_t * buf, iram_size_t buf_size, uhd_callback_trans_t callback)
{
	// Start transfer on interrupt endpoint IN
	return uhd_ep_run(uhi_midi_dev.dev->address, uhi_midi_dev.ep_in, true, buf,
			buf_size, UHI_MIDI_TIMEOUT, callback);
}

bool uhi_midi_read(uint8_t * buf, iram_size_t buf_size, uhd_callback_trans_t callback)
{
	/*
    from uhd.h
    * \warning About \a b_shortpacket, for OUT endpoint it means that
    * a short packet or a Zero Length Packet must be sent to the USB line
    * to properly close the usb transfer at the end of the data transfer.
    * For Bulk and Interrupt IN endpoint, it will automatically stop the transfer
    * at the end of the data transfer (received short packet).
    *
   */
	return uhd_ep_run(uhi_midi_dev.dev->address, uhi_midi_dev.ep_out, true,
			buf, buf_size, UHI_MIDI_TIMEOUT, callback);
}
bool uhi_midi_in_run(uint8_t * buf, iram_size_t buf_size,
		     uhd_callback_trans_t callback) {
  return uhd_ep_run(
		    uhi_midi_dev.dev->address,
		    uhi_midi_dev.ep_in,  //		    false,  // shortpacket... //// TEST:
		    true, 
		    buf, buf_size,
		    UHI_MIDI_TIMEOUT, 
		    callback);
}

bool uhi_midi_out_run(uint8_t * buf, iram_size_t buf_size,
		      uhd_callback_trans_t callback) {
  /*
    from uhd.h
    * \warning About \a b_shortpacket, for OUT endpoint it means that
    * a short packet or a Zero Length Packet must be sent to the USB line
    * to properly close the usb transfer at the end of the data transfer.
    * For Bulk and Interrupt IN endpoint, it will automatically stop the transfer
    * at the end of the data transfer (received short packet).
    *
   */
  return uhd_ep_run(
		    uhi_midi_dev.dev->address,
		    uhi_midi_dev.ep_out, 
		    true, // automatic shortpacket for buf < wlen
		    buf, buf_size,
		    UHI_MIDI_TIMEOUT, 
		    callback);
}

static void uhi_midi_report_reception(
	usb_add_t add,
	usb_ep_t ep,
	uhd_trans_status_t status,
	iram_size_t nb_transfered)

{
	uint8_t i;
	unsigned short val;
	UNUSED(ep);

	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_midi_start_trans_report(add);
		return; // MIDI transfer restart
	}

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
		return; // MIDI transfer aborted
	}
}
/*uint8_t MIDI_Host_Flush(uhi_midi_dev_t* const MIDIInterfaceInfo)
{
	if(USB_HostState != 11 || !(MIDIInterfaceInfo->isActive))
		return 1;

	uint8_t ErrorCode;
	Pipe_SelectPipe(MIDIInterfaceInfo->data_out_pipe.address);
	
	Pipe_Unfreeze();

	if (Pipe_BytesInPipe())
	{
		Pipe_ClearOUT();

		if ((ErrorCode = Pipe_WaitUntilReady()) != PIPE_READYWAIT_NoError)
		{
			Pipe_Freeze();
			return ErrorCode;
		}
	}

	Pipe_Freeze();

	return PIPE_READYWAIT_NoError;
}
*/
/*void MIDI_HostUSBTask(uhi_midi_dev_t* const MIDIInterfaceInfo)
{
	if(USB_HostState != 11  || !(MIDIInterfaceInfo->isActive))
		return;
	MIDI_Host_Flush(MIDIInterfaceInfo);
}*/



//@}

//@}
