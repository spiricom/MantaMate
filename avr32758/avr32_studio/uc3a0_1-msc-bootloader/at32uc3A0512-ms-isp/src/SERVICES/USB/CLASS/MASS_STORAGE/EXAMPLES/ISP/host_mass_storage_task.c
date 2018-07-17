/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Management of the USB host mass-storage task.
 *
 * This file manages the USB host mass-storage task.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USB module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ***************************************************************************/

/* Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
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


//_____  I N C L U D E S ___________________________________________________

#include "conf_usb.h"


#if USB_HOST_FEATURE == ENABLED

#include "conf_usb.h"
#include "usb_host_enum.h"
#include "usb_host_task.h"
#include "host_mem.h"
#include "host_mass_storage_task.h"


//_____ M A C R O S ________________________________________________________


//_____ D E F I N I T I O N S ______________________________________________


//_____ D E C L A R A T I O N S ____________________________________________

volatile Bool ms_new_device_connected = FALSE;
volatile Bool ms_connected = FALSE;


//!
//! @brief This function manages the host mass-storage task.
//!
void host_mass_storage_task(void)
{
  // First, check the host controller is in full operating mode with the
  // B-device attached and enumerated
  if (Is_host_ready())
  {
    // New device connection (executed only once after device connection)
    if (ms_new_device_connected)
    {
      U8 i;

      ms_new_device_connected = FALSE;

      // For all supported interfaces
      for (i = 0; i < Get_nb_supported_interface(); i++)
      {
        // If mass-storage class
        if (Get_class(i) == MS_CLASS)
        {
          U8 max_lun;

          ms_connected = TRUE;

          // Get correct physical pipes associated with IN/OUT endpoints
          if (Is_ep_in(i, 0))
          { // Yes, associate it with the IN pipe
            g_pipe_ms_in = Get_ep_pipe(i, 0);
            g_pipe_ms_out = Get_ep_pipe(i, 1);
          }
          else
          { // No, invert...
            g_pipe_ms_in = Get_ep_pipe(i, 1);
            g_pipe_ms_out = Get_ep_pipe(i, 0);
          }

          // Get the number of LUNs in the connected mass-storage device
          max_lun = host_get_lun();

          // Initialize all USB drives
          for (host_selected_lun = 0; host_selected_lun < max_lun; host_selected_lun++)
          {
            U32 capacity;

            host_ms_inquiry();
            host_read_capacity(host_selected_lun, &capacity);
            host_ms_request_sense();
            for (i = 0; i < 3; i++)
            {
              if (host_test_unit_ready(host_selected_lun) == CTRL_GOOD)
              {
                host_read_capacity(host_selected_lun, &capacity);
                break;
              }
            }
          }
          break;
        }
      }
    }
  }
}


#endif  // USB_HOST_FEATURE == ENABLED
