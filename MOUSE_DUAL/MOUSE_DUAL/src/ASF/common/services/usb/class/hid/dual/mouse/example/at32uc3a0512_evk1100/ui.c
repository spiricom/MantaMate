/**
 * \file
 *
 * \brief User Interface
 *
 * Copyright (C) 2011 - 2014 Atmel Corporation. All rights reserved.
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

#include <asf.h>
#include "ui.h"
#include "main.h"
#include "7Segment.h"
#include "udi_cdc.h"
#include "note_process.h"

uint8_t myReceiveBuf[64];
uint8_t mySendBuf[64];


int lastbutton0 = 0;
int lastbutton1 = 0;
extern unsigned char preset_num;
int howManyFilled = 0;
uint8_t my_buf[128];

	
/**
 * \name Internal routines to manage asynchronous interrupt pin change
 * This interrupt is connected to a switch and allows to wakeup CPU in low sleep mode.
 * This wakeup the USB devices connected via a downstream resume.
 * @{
 */
static void ui_enable_asynchronous_interrupt(void);
static void ui_disable_asynchronous_interrupt(void);

/**
 * \brief Interrupt handler for interrupt pin change
 */
ISR(ui_wakeup_isr, AVR32_GPIO_IRQ_GROUP, 0)
{
	// Clear GPIO interrupt.
	gpio_clear_pin_interrupt_flag(GPIO_JOYSTICK_PUSH);

	// Clear External Interrupt Line else Wakeup event always enabled
	eic_clear_interrupt_line(&AVR32_EIC, EXT_NMI);

	ui_disable_asynchronous_interrupt();

	// Wakeup host and device
	uhc_resume();
}

/**
 * \brief Initializes and enables interrupt pin change
 */
static void ui_enable_asynchronous_interrupt(void)
{
	//! Structure holding the configuration parameters of the EIC low level driver.
	eic_options_t eic_options = {
		// Choose External Interrupt Controller Line
		.eic_line = EXT_NMI,
		// Enable level-triggered interrupt.
		.eic_mode = EIC_MODE_LEVEL_TRIGGERED,
		// Don't care value because the chosen eic mode is level-triggered.
		.eic_edge = 0,
		// Interrupt will trigger on low-level.
		.eic_level = EIC_LEVEL_LOW_LEVEL,
		// Enable filter.
		.eic_filter = EIC_FILTER_ENABLED,
		// For Wake Up mode, initialize in asynchronous mode
		.eic_async = EIC_ASYNCH_MODE
	};

	/* register joystick handler on level 0 */
	irqflags_t flags = cpu_irq_save();
	irq_register_handler(ui_wakeup_isr,
			AVR32_GPIO_IRQ_0 + (GPIO_JOYSTICK_PUSH / 8), 0);
	cpu_irq_restore(flags);

	/* configure joystick to produce IT on all state change */
	gpio_enable_pin_interrupt(GPIO_JOYSTICK_PUSH, GPIO_PIN_CHANGE);

	/*
	 * Configure pin change interrupt for asynchronous wake-up (required to
	 * wake up from the STATIC sleep mode).
	 *
	 * First, map the interrupt line to the GPIO pin with the right
	 * peripheral function.
	 */
	gpio_enable_module_pin(GPIO_JOYSTICK_PUSH, AVR32_EIC_EXTINT_8_FUNCTION);

	/*
	 * Enable the internal pull-up resistor on that pin (because the EIC is
	 * configured such that the interrupt will trigger on low-level, see
	 * eic_options.eic_level).
	 */
	gpio_enable_pin_pull_up(GPIO_JOYSTICK_PUSH);

	// Init the EIC controller with the set options.
	eic_init(&AVR32_EIC, &eic_options, sizeof(eic_options) /
			sizeof(eic_options_t));

	// Enable External Interrupt Controller Line
	eic_enable_line(&AVR32_EIC, EXT_NMI);
}

/**
 * \brief Disables interrupt pin change
 */
static void ui_disable_asynchronous_interrupt(void)
{
	eic_disable_line(&AVR32_EIC, EXT_NMI);

	/* Disable joystick input change ITs. */
	gpio_disable_pin_interrupt(GPIO_JOYSTICK_PUSH);
}
//! @}

/**
 * \name Main user interface functions
 * @{
 */
void ui_init(void)
{
	LED_Off(LED0);
	LED_Off(LED1);
	LED_Off(LED2);
	LED_Off(LED3);
	LED_Off(LED4);
	LED_Off(LED5);
}

void ui_usb_mode_change(bool b_host_mode)
{
	ui_init();
	if (b_host_mode) {
		//LED_On(LED0);
	}
}
//! @}

/**
 * \name Host mode user interface functions
 * @{
 */

//! Status of device enumeration
static uhc_enum_status_t ui_enum_status=UHC_ENUM_DISCONNECT;
//! Blink frequency depending on device speed
static uint16_t ui_device_speed_blink;
//! Notify the presence of a USB device mouse
static bool ui_hid_joy_plug = false;
static bool ui_hid_manta_plug = false;
//! Manages device mouse moving
static int8_t ui_x, ui_y, ui_scroll;
//! Notify the presence of a USB device MIDI
bool ui_midi_plug = false;

void ui_host_vbus_change(bool b_vbus_present)
{
	UNUSED(b_vbus_present);
}

void ui_host_vbus_error(void)
{
}

void ui_host_connection_event(uhc_device_t *dev, bool b_present)
{
	LED_Off(LED0);
	LED_Off(LED1);
	LED_Off(LED2);
	LED_Off(LED3);
	LED_Off(LED4);
	LED_Off(LED5);

	if (b_present) {
		LED_On(LED4);
	} else {
		ui_enum_status = UHC_ENUM_DISCONNECT;
		LED_Off(LED4);
	}
}

void ui_host_enum_event(uhc_device_t * dev, uhc_enum_status_t status)
{
	ui_enum_status = status;
	if (ui_enum_status == UHC_ENUM_SUCCESS) {
		switch (dev->speed) {
		case UHD_SPEED_HIGH:
			ui_device_speed_blink = 250;
			break;
		case UHD_SPEED_FULL:
			ui_device_speed_blink = 500;
			break;
		case UHD_SPEED_LOW:
		default:
			ui_device_speed_blink = 1000;
			break;
		}
	}
}

void ui_uhi_hid_joy_change(uhc_device_t * dev, bool b_plug)
{
	ui_hid_joy_plug = b_plug;
}

void ui_uhi_hid_manta_change(uhc_device_t * dev, bool b_plug)
{
	ui_hid_manta_plug = b_plug;
}

void ui_uhi_midi_change(uhc_device_t * dev, bool b_plug)
{
	ui_midi_plug = b_plug;
}

void my_callback_midi_rx_notify(void)
{
	uint16_t bytesToRead = 0;
	//LED_On(LED0);
	while (uhi_midi_is_rx_ready()) {
		 //LED_On(LED1);
		 bytesToRead = uhi_midi_read_buf(&my_buf, 64);
		 parseMIDI(bytesToRead);
 		 //Write7Seg(my_buf[1]);
	}
};

void ui_host_wakeup_event(void)
{
	ui_disable_asynchronous_interrupt();
}

void ui_host_sof_event(void)
{
	bool b_btn_state;
	static bool btn_suspend = false;
	static bool btn_suspend_and_remotewakeup = false;
	static uint16_t counter_sof = 0;

	if (ui_enum_status == UHC_ENUM_SUCCESS) 
	{

		// Display device enumerated and in active mode
		if (++counter_sof > ui_device_speed_blink) 
		{
			counter_sof = 0;
			if (ui_hid_joy_plug) {
				//LED_Toggle(LED7);
			}
			if (ui_midi_plug) {
				//LED_Toggle(LED3);
			}
			if (ui_hid_manta_plug){
				//LED_Toggle(LED2);
			}
		}
		/*
		if ((!gpio_get_pin_value(GPIO_PUSH_BUTTON_0)) && (lastbutton0 == 0))
		{
			preset_num--;
			Write7Seg(preset_num);
		}
		if ((!gpio_get_pin_value(GPIO_PUSH_BUTTON_1)) && (lastbutton1 == 0))
		{
			preset_num++;
			Write7Seg(preset_num);
		}
		lastbutton0 = !gpio_get_pin_value(GPIO_PUSH_BUTTON_0);
		lastbutton1 = !gpio_get_pin_value(GPIO_PUSH_BUTTON_1);
		/*
		// Scan button to enter in suspend mode
		b_btn_state = !gpio_get_pin_value(GPIO_PUSH_BUTTON_0);
		if (b_btn_state != btn_suspend) {
			// Button have changed
			btn_suspend = b_btn_state;
			if (b_btn_state) {
				// Button has been pressed
				LED_Off(LED2);
				LED_Off(LED3);
				LED_Off(LED_BI0_GREEN);
				LED_Off(LED_BI0_RED);
				LED_Off(LED_BI1_GREEN);
				LED_Off(LED_BI1_RED);
				ui_enable_asynchronous_interrupt();
				uhc_suspend(false);
				return;
			}
		}
		*/
	/*
		// Scan button to enter in suspend mode and remote wakeup
		b_btn_state = !gpio_get_pin_value(GPIO_PUSH_BUTTON_1);
		if (b_btn_state != btn_suspend_and_remotewakeup) {
			// Button have changed
			btn_suspend_and_remotewakeup = b_btn_state;
			if (b_btn_state) {
				// Button has been pressed
				LED_Off(LED2);
				LED_Off(LED3);
				LED_Off(LED_BI0_GREEN);
				LED_Off(LED_BI0_RED);
				LED_Off(LED_BI1_GREEN);
				LED_Off(LED_BI1_RED);
				ui_enable_asynchronous_interrupt();
				uhc_suspend(true);
				return;
			}
		}
		*/
		/*
		if (ui_hid_mouse_plug) {
			// Power on a LED when the mouse move
			if (!ui_x && !ui_y && !ui_scroll) {
				LED_Off(LED_BI0_GREEN);
			}else{
				ui_x = ui_y = ui_scroll = 0;
				LED_On(LED_BI0_GREEN);
			}
		}*/
	}
}




static bool ui_d_midi_enable = false;

void ui_test_flag_reset(void)
{
	//LED_Off(LED_BI1_GREEN);
	//LED_Off(LED_BI1_RED);
}

void ui_test_finish(bool b_success)
{
	if (b_success) {
		//LED_On(LED_BI1_GREEN);
	} else {
		//LED_On(LED_BI1_RED);
	}
}
//! @}

void ui_midi_rx_start(void)
{
	LED_On(LED2);
	
	//udi_midi_tx_send(0);
}

void ui_midi_rx_stop(void)
{
	LED_Off(LED2);
}

void ui_midi_tx_start(void)
{
	//LED_On(LED_AMBER1);
}

void ui_midi_tx_stop(void)
{
	//LED_Off(LED_AMBER1);
}

void ui_midi_error(void)
{
	//LED_On(LED_BI0_RED);
}

void ui_midi_overflow(void)
{
	//LED_On(LED_BI1_RED);
}

void ui_my_midi_receive(void)
{
	
	uint16_t bytesToRead = 0;
	
	if (udi_midi_is_rx_ready()) {
		int numReceivedBytes = udi_midi_get_nb_received_data();
		udi_midi_read_buf(myReceiveBuf, numReceivedBytes);
		for (int i = 0; i < numReceivedBytes; i++)
		{
			my_buf[i] = myReceiveBuf[i];
		}
		if(numReceivedBytes && udi_midi_is_tx_ready())
		{
			//udi_midi_write_buf(mySendBuf, numReceivedBytes);   /// uncomment to echo back to computer -JS

			parseMIDI(numReceivedBytes);
		}
		} else {
		// Fifo empty then Stop
		;
	}
}

void ui_my_midi_send(void)
{
	ui_midi_tx_start();
	//uint8_t myValue = 0x09;
	// Transfer UART RX fifo to CDC TX
	if (!udi_midi_is_tx_ready()) {
		// Fifo full
		;
		}else{
		udi_midi_write_buf(mySendBuf, 4);
	}
	ui_midi_tx_stop();
}

uint32_t USB_frame_counter = 0;

void ui_process(uint16_t framenumber)
{
	ui_my_midi_receive();
	
	
	USB_frame_counter++;
	
	if (USB_frame_counter == 500) {

	}
	if (USB_frame_counter == 1000) {

		USB_frame_counter = 0;
	}
	
}

void ui_ext_gate_in(void)
{
	if(eiccounter % 2 == 1)
	{
		LED_On(LED4);
		
		tuningTest(tuning_count);
		//mySendBuf[0] = 0x09;
		//mySendBuf[1] = 0x90;
		//mySendBuf[2] = 0x3c;
		//mySendBuf[3] = 0x78;
		
		//ui_my_midi_send();
		//udi_midi_write_buf(mySendBuf, 4);
	}
	else
	{
		LED_Off(LED4);
		tuningTest(tuning_count);
		//mySendBuf[0] = 0x09;
		//mySendBuf[1] = 0x90;
		//mySendBuf[2] = 0x3c;
		//mySendBuf[3] = 0x00;
		//ui_my_midi_receive();
		//ui_my_midi_send();
		//udi_midi_write_buf(mySendBuf, 4);
	}
	tuning_count++;
	if (tuning_count > 6)
	{
		tuning_count = 0;
	}
}

