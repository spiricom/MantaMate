/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Main file of the USB MS ISP.
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

#include <avr32/io.h>
#include <string.h>
#include "preprocessor.h"
#include "board.h"
#include "pm.h"
#include "flashc.h"
#include "conf_usb.h"
#include "usb_task.h"
#if USB_HOST_FEATURE == ENABLED
#include "host_mass_storage_task.h"
#endif
#include "fs_com.h"
#include "file.h"
#include "navigation.h"
#include "crc32.h"
#include "conf_isp.h"


//_____ M A C R O S ________________________________________________________


//_____ D E F I N I T I O N S ______________________________________________

#define FGCLK_USBB                48000000

#define FOSC                      ATPASTE2(FOSC, ISP_OSC)
#define OSC_STARTUP               ATPASTE3(OSC, ISP_OSC, _STARTUP)

#define pm_enable_osc_crystal     ATPASTE3(pm_enable_osc, ISP_OSC, _crystal)
#define pm_enable_clk             ATPASTE2(pm_enable_clk, ISP_OSC)


#if USB_HOST_FEATURE == ENABLED
static crc32_table_t crc32_table;
#endif


/*!
 *  Start the generation of system clocks
 */
static void sys_clk_gen_start(void)
{
#if FOSC !=  8000000 &&                                                        \
    FOSC != 12000000 &&                                                        \
    FOSC != 16000000
  #error Unsupported oscillator frequency
#endif

  volatile avr32_pm_t *const pm = &AVR32_PM;

  // Start the oscillator
  pm_enable_osc_crystal(pm, FOSC);
  pm_enable_clk(pm, OSC_STARTUP);

  // Set PLL0 VCO @ 96 MHz
  pm_pll_setup(pm, 0,                     // pll
                   FGCLK_USBB / FOSC - 1, // mul
                   0,                     // div
                   ISP_OSC,               // osc
                   63);                   // lockcount

  // Set PLL0 @ 48 MHz
  pm_pll_set_option(pm, 0,  // pll
                        1,  // pll_freq
                        1,  // pll_div2
                        0); // pll_wbwdisable

  // Enable PLL0
  pm_pll_enable(pm, 0);

  // Wait for PLL0 locked
  pm_wait_for_pll0_locked(pm);

  // fPBA: 12 MHz
  // fPBB: 12 MHz
  // fHSB: 12 MHz
  pm_cksel(pm, 1,   // pbadiv
               1,   // pbasel
               1,   // pbbdiv
               1,   // pbbsel
               1,   // hsbdiv
               1);  // hsbsel

  // Use 0 flash wait state
  flashc_set_wait_state(0);

  // Switch the main clock to PLL0
  pm_switch_to_clock(pm, AVR32_PM_MCCTRL_MCSEL_PLL0);

  // Setup USB GCLK
  pm_gc_setup(pm, AVR32_PM_GCLK_USBB, // gc
                  1,                  // osc_or_pll: use Osc (if 0) or PLL (if 1)
                  0,                  // pll_osc: select Osc0/PLL0 or Osc1/PLL1
                  0,                  // diven
                  0);                 // div

  // Enable USB GCLK
  pm_gc_enable(pm, AVR32_PM_GCLK_USBB);
}


#if USB_HOST_FEATURE == ENABLED

static void isp_task(void)
{
  #define PROGRAM_START_PAGE  (PROGRAM_START_OFFSET / AVR32_FLASHC_PAGE_SIZE)
  #define HEADER_SIZE         (sizeof(ID) + sizeof(UUID) + sizeof(crc32))

  static const U8 ID[] = ISP_FW_UPGRADE_FILE_ID;
  static const U8 UUID[] = ISP_FW_UPGRADE_FILE_UUID;

  static enum
  {
    ISP_STATE_ERROR               = -1,
    ISP_STATE_IDLE                =  0,
    ISP_STATE_MOUNT               =  1,
    ISP_STATE_CHECK               =  2,
    ISP_STATE_CRC32               =  3,
    ISP_STATE_ERASE_AND_VERIFY    =  4,
    ISP_STATE_PROGRAM_AND_VERIFY  =  5
  } isp_state = ISP_STATE_IDLE;

  static U32 crc32;

#if __GNUC__ && __AVR32__
  __attribute__((__aligned__(4)))
#elif __ICCAVR32__
  #pragma data_alignment = 4
#endif
  U8 buffer[AVR32_FLASHC_PAGE_SIZE];
  volatile U64 *flash_ptr;
  S32 i;

  if (!Is_host_ms_configured())
  {
    isp_state = ISP_STATE_IDLE;
    return;
  }

  switch (isp_state)
  {
  case ISP_STATE_ERROR:
    break;

  case ISP_STATE_IDLE:
    isp_state = ISP_STATE_MOUNT;
    break;

  case ISP_STATE_MOUNT:
    nav_reset();
    if (!nav_drive_set(0) ||
        !nav_partition_mount())
    {
      isp_state = ISP_STATE_ERROR;
      break;
    }
    isp_state = ISP_STATE_CHECK;
    break;

  case ISP_STATE_CHECK:
    if (!nav_filelist_single_enable(FS_FILE) ||
        !nav_filelist_set(0, FS_FIND_NEXT) ||
        !nav_file_name(ISP_FW_UPGRADE_FILENAME, 0, FS_NAME_CHECK, TRUE))
    {
      isp_state = ISP_STATE_ERROR;
      break;
    }

    i = nav_file_lgt() - HEADER_SIZE;
    if (i < 0 ||
        i > flashc_get_flash_size() - PROGRAM_START_OFFSET ||
        !file_open(FOPEN_MODE_R) ||
        file_read_buf(buffer, sizeof(ID)) != sizeof(ID) ||
        memcmp(buffer, ID, sizeof(ID)) ||
        file_read_buf(buffer, sizeof(UUID)) != sizeof(UUID) ||
        memcmp(buffer, UUID, sizeof(UUID)) ||
        file_read_buf((U8 *)&crc32, sizeof(crc32)) != sizeof(crc32))
    {
      isp_state = ISP_STATE_ERROR;
      break;
    }

    crc32_reset();
    isp_state = ISP_STATE_CRC32;
    break;

  case ISP_STATE_CRC32:
    switch (file_eof())
    {
    case 0x00:
      if (!(i = file_read_buf(buffer, sizeof(buffer))))
      {
        isp_state = ISP_STATE_ERROR;
        break;
      }
      crc32_iterate_buffer(buffer, i);
      break;
    case 0x01:
      if (crc32 != crc32_get_value())
      {
        isp_state = ISP_STATE_ERROR;
        break;
      }
      isp_state = ISP_STATE_ERASE_AND_VERIFY;
      break;
    case 0xFF:
      isp_state = ISP_STATE_ERROR;
      break;
    }
    break;

  case ISP_STATE_ERASE_AND_VERIFY:
    for (i = flashc_get_page_region(PROGRAM_START_PAGE); i < AVR32_FLASHC_REGIONS; i++)
    {
      flashc_lock_region(i, FALSE);
      if (flashc_is_region_locked(i))
      {
        isp_state = ISP_STATE_ERROR;
        return;
      }
    }

    for (i = flashc_get_page_count() - 1; i >= PROGRAM_START_PAGE; i--)
    {
      if (!flashc_erase_page(i, TRUE))
      {
        isp_state = ISP_STATE_ERROR;
        return;
      }
    }

    if (!file_seek(HEADER_SIZE, FS_SEEK_SET))
    {
      isp_state = ISP_STATE_ERROR;
      break;
    }
    isp_state = ISP_STATE_PROGRAM_AND_VERIFY;
    break;

  case ISP_STATE_PROGRAM_AND_VERIFY:
    switch (file_eof())
    {
    case 0x00:
      if (!(i = file_getpos()))
      {
        isp_state = ISP_STATE_ERROR;
        break;
      }
      flash_ptr = (U64 *)(PROGRAM_START_ADDRESS + i - HEADER_SIZE);
      if (!(i = file_read_buf(buffer, AVR32_FLASHC_PAGE_SIZE)))
      {
        isp_state = ISP_STATE_ERROR;
        break;
      }
      memset(&buffer[i], 0xFF, AVR32_FLASHC_PAGE_SIZE - i);
      flashc_clear_page_buffer();
      for (i = 0; i < AVR32_FLASHC_PAGE_SIZE / sizeof(*flash_ptr); i++)
        flash_ptr[i] = ((U64 *)buffer)[i];
      flashc_write_page(-1);
      if (memcmp((void *)flash_ptr, buffer, AVR32_FLASHC_PAGE_SIZE))
      {
        isp_state = ISP_STATE_ERROR;
        break;
      }
      break;
    case 0x01:
      file_close();
      flash_ptr = (U64 *)AVR32_FLASHC_USER_PAGE;
      memcpy(buffer, (void *)flash_ptr, AVR32_FLASHC_PAGE_SIZE);
      *(U32 *)&buffer[ISP_FORCE_OFFSET] = 0xFFFFFFFF;
      flashc_clear_page_buffer();
      for (i = 0; i < AVR32_FLASHC_PAGE_SIZE / sizeof(*flash_ptr); i++)
        *flash_ptr++ = ((U64 *)buffer)[i];
      flashc_erase_user_page(FALSE);
      flashc_write_user_page();
      Disable_global_interrupt();
      AVR32_WDT.ctrl = AVR32_WDT_CTRL_EN_MASK |
                       (10 << AVR32_WDT_CTRL_PSEL_OFFSET) |
                       (AVR32_WDT_KEY_VALUE << AVR32_WDT_CTRL_KEY_OFFSET);
      asm ("");
      AVR32_WDT.ctrl = AVR32_WDT_CTRL_EN_MASK |
                       (10 << AVR32_WDT_CTRL_PSEL_OFFSET) |
                       ((~AVR32_WDT_KEY_VALUE << AVR32_WDT_CTRL_KEY_OFFSET) & AVR32_WDT_CTRL_KEY_MASK);
      while (1);
    case 0xFF:
      isp_state = ISP_STATE_ERROR;
      break;
    }
    break;
  }
}

#endif  // USB_HOST_FEATURE == ENABLED


int main(void)
{
  sys_clk_gen_start();

  crc32_init(&crc32_table);

  usb_task_init();

  while (TRUE)
  {
    usb_task();
#if USB_HOST_FEATURE == ENABLED
    host_mass_storage_task();
    isp_task();
#endif
  }
}
