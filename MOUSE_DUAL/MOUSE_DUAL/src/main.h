/**
 * \file
 *
 * \brief Declaration of main function used by example
 *
 * Copyright (C) 2014-2015 Atmel Corporation. All rights reserved.
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

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdint.h>

#include "usb_protocol_cdc.h"
#include "utilities.h"


// TIMER 
volatile avr32_tc_t *tc1;
volatile avr32_tc_t *tc2;
volatile avr32_tc_t *tc3;

tRamp pitchGlideOne, pitchGlideTwo;
tRamp cv1GlideOne, cv1GlideTwo;
tRamp cv2GlideOne, cv2GlideTwo;
tRamp cv3GlideOne, cv3GlideTwo;
tRamp cv4GlideOne, cv4GlideTwo;

#define TIMERS 1

#define TC1                 (&AVR32_TC)
#define TC1_CHANNEL         0
#define TC1_IRQ             AVR32_TC_IRQ0
#define TC1_IRQ_GROUP       AVR32_TC_IRQ_GROUP
#define TC1_IRQ_PRIORITY    AVR32_INTC_INT0

#define TC2                 (&AVR32_TC)
#define TC2_CHANNEL         1
#define TC2_IRQ             AVR32_TC_IRQ1
#define TC2_IRQ_GROUP       AVR32_TC_IRQ_GROUP
#define TC2_IRQ_PRIORITY    AVR32_INTC_INT0

#define TC3                 (&AVR32_TC)
#define TC3_CHANNEL         2
#define TC3_IRQ             AVR32_TC_IRQ2
#define TC3_IRQ_GROUP       AVR32_TC_IRQ_GROUP
#define TC3_IRQ_PRIORITY    AVR32_INTC_INT0

void initTimers (void);

//DEBUG CODE
extern uint16_t lengthDB;
extern int slider;


//global variables that everything which includes main.h should be able to see
extern uint32_t dummycounter;
extern uint8_t manta_mapper;
extern uint8_t tuning_count;

extern uint8_t manta_data_lock;
extern unsigned char preset_num;
extern uint32_t clock_speed;
extern uint8_t sequencer_mode;
extern uint32_t USB_frame_counter;

//set up the external interrupt for the gate input
void setupEIC(void);
void updatePreset(void);
void Preset_Switch_Check(uint8_t whichSwitch);
void USB_Mode_Switch_Check(void);
void clockHappened(void);
void initI2C(void);
void enterBootloader(void);
void sendI2CtoEEPROM(void);

/*! \brief Opens the communication port
 * This is called by CDC interface when USB Host enable it.
 *
 * \retval true if cdc startup is successfully done
 */
bool main_midi_enable(void);

/*! \brief Closes the communication port
 * This is called by CDC interface when USB Host disable it.
 */
void main_midi_disable(void);

/*! \brief Manages the leds behaviors
 * Called when a start of frame is received on USB line each 1ms.
 */
void main_sof_action(void);

/*! \brief Enters the application in low power mode
 * Callback called when USB host sets USB line in suspend state
 */
void main_suspend_action(void);

/*! \brief Turn on a led to notify active mode
 * Called when the USB line is resumed from the suspend state
 */
void main_resume_action(void);



//function prototypes//
void dacwait1(void);
void dacwait2(void);
void DACsetup(void);
void dacsend(unsigned char DACvoice, unsigned char DACnum, unsigned short DACval);

void DAC16Send(unsigned char DAC16voice, unsigned short DAC16val);

void lcd_clear_line(uint8_t linenum);
#endif // _MAIN_H_
