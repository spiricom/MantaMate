#ifndef _USB_MIDI_H_
#define _USB_MIDI_H_

#include "uhc.h"

void MIDI_HostUSBTask(uhi_midi_dev_t* const MIDIInterfaceInfo);
uint8_t MIDI_Host_Flush(uhi_midi_dev_t* const MIDIInterfaceInfo);
void USB_USBHostTask(void);

#endif
