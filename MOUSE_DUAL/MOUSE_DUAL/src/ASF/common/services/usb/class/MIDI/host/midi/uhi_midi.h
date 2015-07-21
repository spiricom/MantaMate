/*
 * uhi_midi.h
 *
 * Created: 7/13/2015 5:08:46 PM
 *  Author: Elaine
 */ 


#ifndef UHI_MIDI_H_
#define UHI_MIDI_H_

#include "conf_usb_host.h"
#include "usb_protocol_midi.h"
//#include "uhc.h"
#include "uhi.h"

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

typedef struct
{
	uint8_t  address; /**< Address of the pipe to configure, or zero if the table entry is to be unused. */
	uint16_t size; /**< Size of the pipe bank, in bytes. */
	uint8_t  epAddress; /**< Address of the endpoint in the connected device. */
	uint8_t  type; /**< Type of the endpoint, a \c EP_TYPE_* mask. */
	uint8_t  banks; /**< Number of hardware banks to use for the pipe. */
} usb_pipe_table_t;

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
void uhi_midi_sof(bool b_micro);

#ifdef __cplusplus
}
#endif

#endif /* UHI_MIDI_H_ */