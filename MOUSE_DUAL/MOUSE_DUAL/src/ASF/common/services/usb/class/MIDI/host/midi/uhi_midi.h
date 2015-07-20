/*
 * uhi_midi.h
 *
 * Created: 7/13/2015 5:08:46 PM
 *  Author: Elaine
 */ 


#ifndef UHI_MIDI_H_
#define UHI_MIDI_H_

#include "USB.h"
#include "conf_usb_host.h"
#include "usb_protocol_midi.h"
#include "midi.h"
#include "uhc.h"
#include "uhi.h"
#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "Pipe_UC3.h"



#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup uhi_msc_group
 * \defgroup uhi_msc_group_uhc Interface with USB Host Core (UHC)
 *
 * Define and functions required by UHC.
 * 
 * @{
 */

//! Global define which contains standard UHI API for UHC.
//! It must be added in USB_HOST_UHI define from conf_usb_host.h file.
#define UHI_MIDI { \
	.install = uhi_midi_install, \
	.enable = uhi_midi_enable, \
	.uninstall = uhi_midi_uninstall, \
	.sof_notify = NULL, \
}

/**
 * \name Functions required by UHC
 * @{
 */
// install
uhc_enum_status_t uhi_midi_install(uhc_device_t* dev);
// enable
void uhi_midi_enable(uhc_device_t* dev);
// uninstall
void uhi_midi_uninstall(uhc_device_t* dev);
// input transfer
bool uhi_midi_write(uint8_t * buf, iram_size_t buf_size,
		uhd_callback_trans_t callback);
// output transfer
bool uhi_midi_read(uint8_t * buf, iram_size_t buf_size,
	uhd_callback_trans_t callback);

// read and spawn events (non-blocking)
void midi_read(void);
// write to MIDI device
void midi_write(uint8_t* data, unsigned int bytes);
// get string descriptions
// void midi_get_strings(char** pManufacturer, char** pProduct, char ** Serial);
//@}
extern bool uhi_midi_in_run(uint8_t * buf, iram_size_t buf_size, uhd_callback_trans_t callback);
// output transfer
extern bool uhi_midi_out_run(uint8_t * buf, iram_size_t buf_size, uhd_callback_trans_t callback);

#ifdef __cplusplus
}
#endif

#endif /* UHI_MIDI_H_ */