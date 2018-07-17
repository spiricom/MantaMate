/* This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief USB MSC bootloader start example application for AVR32 UC3.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USART module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/*! \page License
 * Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// Include Files 
#include "board.h"
#include "conf_isp.h"
#include "gpio.h"
#include "flashc.h"
#include "wdt.h"


 
/*! \brief Start the bootloader execution.
 *
 * Program the ISP_FORCE_VALUE pattern to the Flash user page.
 * Force the part to reset using the watchdog timer.
 */
void usb_msc_bl_start (void)
{
   Disable_global_interrupt();
   // Write at destination (AVR32_FLASHC_USER_PAGE + ISP_FORCE_OFFSET) the value
   // ISP_FORCE_VALUE. Size of ISP_FORCE_VALUE is 4 bytes. 
   flashc_memset32(AVR32_FLASHC_USER_PAGE + ISP_FORCE_OFFSET, ISP_FORCE_VALUE, 4, TRUE);
   wdt_enable(17777);
   while (1);           // wait WDT time-out to reset and start the MSC bootloader
}

int main(void)
{
int i = 0;              // tick counter

   while (TRUE)         // MAIN SCHEDULE LOOP
   {
      i++;              // loop tick
     
      // Toggle LED1 every ~3S under RC osc
      if( i == 1000 )
      {
         // --> toggle the LED0
         gpio_tgl_gpio_pin(LED0_GPIO);

         i= 0;
      }
	   // test PB0 status, if pressed, launch the USB MSC bootloader 
      if (gpio_get_pin_value(GPIO_PUSH_BUTTON_0) == 0)
      {
         gpio_set_gpio_pin(LED0_GPIO); // LED0 OFF
         gpio_clr_gpio_pin(LED1_GPIO); // LED1 ON
	     usb_msc_bl_start();
      }
   }
}
