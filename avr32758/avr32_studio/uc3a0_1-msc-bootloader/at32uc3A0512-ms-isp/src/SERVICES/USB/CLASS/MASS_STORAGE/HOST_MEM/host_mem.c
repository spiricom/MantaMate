/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file ******************************************************************
 *
 * \brief Host management of the USB device memories.
 *
 * This file manages the accesses to the remote USB device memories.
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

#include "conf_access.h"


#if MEM_USB == ENABLE

#include "usb_drv.h"
#include "host_mem.h"
#include "scsi_decoder.h"
#include "usb_host_task.h"
#include "host_mass_storage_task.h"


//_____ M A C R O S ________________________________________________________


//_____ P R I V A T E   D E C L A R A T I O N S ____________________________


//_____ D E F I N I T I O N S ______________________________________________

static U8 buf_cmd[31];

U8 host_selected_lun;

U8  g_pipe_ms_in;
U8  g_pipe_ms_out;
U8  g_u8_sector_size;
U8  g_b_read_run = FALSE;
U32 g_u32_read_addr;
U16 g_u16_read_sector;


//_____ D E C L A R A T I O N S ____________________________________________

static Ctrl_status host_read_10_ram_stop(void);


//! This function manages pipe STALLs
//! @param pipe         Pipe for the CLEAR ENDPOINT FEATURE request
static void host_ms_stall_management(U8 pipe)
{
  U16 nb;

  host_clear_endpoint_feature(Host_get_pipe_endpoint_number(pipe) | MSK_EP_DIR);
  Host_ack_stall(g_pipe_ms_in);
  Host_reset_data_toggle(g_pipe_ms_in);

  // Get CSW
  nb = sizeof(buf_cmd);
  host_get_data(g_pipe_ms_in, &nb, buf_cmd);

  host_ms_request_sense();
}


//! This function sends an SCSI INQUIRY command (0x12) to the device
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status host_ms_inquiry(void)
{
  Ctrl_status status;
  U16 nb;

  if (!Is_host_ms_configured()) return CTRL_FAIL;

  if (g_b_read_run && (status = host_read_10_ram_stop()) != CTRL_GOOD) return status;

  buf_cmd[0]  = 'U';                            //  0 - 0x55
  buf_cmd[1]  = 'S';                            //  1 - 0x53
  buf_cmd[2]  = 'B';                            //  2 - 0x42
  buf_cmd[3]  = 'C';                            //  3 - 0x43
  buf_cmd[4]  = 0x64;                           //  4 - LSB0W(dCBWTag)
  buf_cmd[5]  = 0x00;                           //  5 - LSB1W(dCBWTag)
  buf_cmd[6]  = 0x00;                           //  6 - LSB2W(dCBWTag)
  buf_cmd[7]  = 0x00;                           //  7 - LSB3W(dCBWTag)
  buf_cmd[8]  = 0x24;                           //  8 - LSB0W(dCBWDataTransferLength)
  buf_cmd[9]  = 0x00;                           //  9 - LSB1W(dCBWDataTransferLength)
  buf_cmd[10] = 0x00;                           // 10 - LSB2W(dCBWDataTransferLength)
  buf_cmd[11] = 0x00;                           // 11 - LSB3W(dCBWDataTransferLength)
  buf_cmd[12] = SBC_CMD_DIR_IN;                 // 12 - bmCBWFlags
  buf_cmd[13] = host_selected_lun;              // 13 - bCBWLUN
  buf_cmd[14] = 0x06;                           // 14 - bCBWCBLength
  buf_cmd[15] = SBC_CMD_INQUIRY;                // 15 - CBWCB0 - Operation Code (0x12)
  buf_cmd[16] = 0x00;                           // 16 - CBWCB1 - Obsolete, EVPD
  buf_cmd[17] = 0x00;                           // 17 - CBWCB2 - Page Code
  buf_cmd[18] = 0x00;                           // 18 - CBWCB3 - MSB(Allocation Length)
  buf_cmd[19] = 0x24;                           // 19 - CBWCB4 - LSB(Allocation Length)
  buf_cmd[20] = 0x00;                           // 20 - CBWCW5 - Control
  buf_cmd[21] = 0x00;                           // 21
  buf_cmd[22] = 0x00;                           // 22
  buf_cmd[23] = 0x00;                           // 23
  buf_cmd[24] = 0x00;                           // 24
  buf_cmd[25] = 0x00;                           // 25
  buf_cmd[26] = 0x00;                           // 26
  buf_cmd[27] = 0x00;                           // 27
  buf_cmd[28] = 0x00;                           // 28
  buf_cmd[29] = 0x00;                           // 29
  buf_cmd[30] = 0x00;                           // 30

  // Send command
  if (host_send_data(g_pipe_ms_out, sizeof(buf_cmd), buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  // Transfer data
  nb = sizeof(buf_cmd);
  if (host_get_data(g_pipe_ms_in, &nb, buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  // Get CSW
  nb = sizeof(buf_cmd);
  if (host_get_data(g_pipe_ms_in, &nb, buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  return (buf_cmd[12] == COMMAND_PASSED) ? CTRL_GOOD : CTRL_FAIL;
}


//! This function sends an SCSI REQUEST SENSE command (0x03) to the device
//! @return U8: Sense Key
U8 host_ms_request_sense(void)
{
  volatile U32 delay;
  U16 nb;
  U8 sense_key, sense_key_add;
  Status_t status;

  if (!Is_host_ms_configured()) return SBC_SENSE_KEY_NO_SENSE;

  if (g_b_read_run && host_read_10_ram_stop() != CTRL_GOOD) return SBC_SENSE_KEY_MEDIUM_ERROR;

  buf_cmd[0]  = 'U';                            //  0 - 0x55
  buf_cmd[1]  = 'S';                            //  1 - 0x53
  buf_cmd[2]  = 'B';                            //  2 - 0x42
  buf_cmd[3]  = 'C';                            //  3 - 0x43
  buf_cmd[4]  = 0x65;                           //  4 - LSB0W(dCBWTag)
  buf_cmd[5]  = 0x00;                           //  5 - LSB1W(dCBWTag)
  buf_cmd[6]  = 0x00;                           //  6 - LSB2W(dCBWTag)
  buf_cmd[7]  = 0x00;                           //  7 - LSB3W(dCBWTag)
  buf_cmd[8]  = 0x12;                           //  8 - LSB0W(dCBWDataTransferLength)
  buf_cmd[9]  = 0x00;                           //  9 - LSB1W(dCBWDataTransferLength)
  buf_cmd[10] = 0x00;                           // 10 - LSB2W(dCBWDataTransferLength)
  buf_cmd[11] = 0x00;                           // 11 - LSB3W(dCBWDataTransferLength)
  buf_cmd[12] = SBC_CMD_DIR_IN;                 // 12 - bmCBWFlags
  buf_cmd[13] = host_selected_lun;              // 13 - bCBWLUN
  buf_cmd[14] = 0x06;                           // 14 - bCBWCBLength
  buf_cmd[15] = SBC_CMD_REQUEST_SENSE;          // 15 - CBWCB0 - Operation Code (0x03)
  buf_cmd[16] = 0x00;                           // 16 - CBWCB1 - DESC
  buf_cmd[17] = 0x00;                           // 17 - CBWCB2 - Reserved
  buf_cmd[18] = 0x00;                           // 18 - CBWCB3 - Reserved
  buf_cmd[19] = 0x12;                           // 19 - CBWCB4 - Allocation Length
  buf_cmd[20] = 0x00;                           // 20 - CBWCW5 - Control
  buf_cmd[21] = 0x00;                           // 21
  buf_cmd[22] = 0x00;                           // 22
  buf_cmd[23] = 0x00;                           // 23
  buf_cmd[24] = 0x00;                           // 24
  buf_cmd[25] = 0x00;                           // 25
  buf_cmd[26] = 0x00;                           // 26
  buf_cmd[27] = 0x00;                           // 27
  buf_cmd[28] = 0x00;                           // 28
  buf_cmd[29] = 0x00;                           // 29
  buf_cmd[30] = 0x00;                           // 30

  // Send command
  if (host_send_data(g_pipe_ms_out, sizeof(buf_cmd), buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  // Transfer data
  for (delay = 100000; delay; delay--);
  nb = sizeof(buf_cmd);
  status = host_get_data(g_pipe_ms_in, &nb, buf_cmd);
  if (status == PIPE_STALL)
  {
    host_ms_stall_management(g_pipe_ms_in);
    return CTRL_FAIL;
  }
  else if (status != PIPE_GOOD)
    return SBC_SENSE_KEY_NO_SENSE;

  sense_key = buf_cmd[2] & 0x0F;
  sense_key_add = buf_cmd[12];

  // Get CSW
  nb = sizeof(buf_cmd);
  if (host_get_data(g_pipe_ms_in, &nb, buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  if (buf_cmd[12] != COMMAND_PASSED) return SBC_SENSE_KEY_NO_SENSE;

  switch (sense_key)
  {
  case SBC_SENSE_KEY_NOT_READY:
    if (sense_key_add == SBC_ASC_MEDIUM_NOT_PRESENT)
      return CTRL_NO_PRESENT;
    break;
  case SBC_SENSE_KEY_UNIT_ATTENTION:
    if (sense_key_add == SBC_ASC_NOT_READY_TO_READY_CHANGE)
      return CTRL_BUSY;
    if (sense_key_add == SBC_ASC_MEDIUM_NOT_PRESENT)
      return CTRL_NO_PRESENT;
    break;
  case SBC_SENSE_KEY_HARDWARE_ERROR:
  case SBC_SENSE_KEY_DATA_PROTECT:
    break;
  default:
    sense_key = SBC_SENSE_KEY_NO_SENSE;
    break;
  }

  return sense_key;
}


//! This fonction returns the number of LUNs of the connected mass-storage device
//! @return U8: Number of logical units of the device
U8 host_get_lun(void)
{
  if (!Is_host_ms_configured()) return 0;

  data_stage[0] = 0;
  host_ms_get_max_lun();

  return data_stage[0] + 1;
}


//! This function tests memory state, and starts memory initialization
//! @param lun          Logical Unit Number
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status host_test_unit_ready(U8 lun)
{
  Ctrl_status status;
  U16 nb;
  Status_t get_data_status;

  host_selected_lun = lun;

  if (!Is_host_ms_configured()) return CTRL_FAIL;

  if (g_b_read_run && (status = host_read_10_ram_stop()) != CTRL_GOOD) return status;

  buf_cmd[0]  = 'U';                            //  0 - 0x55
  buf_cmd[1]  = 'S';                            //  1 - 0x53
  buf_cmd[2]  = 'B';                            //  2 - 0x42
  buf_cmd[3]  = 'C';                            //  3 - 0x43
  buf_cmd[4]  = 0x60;                           //  4 - LSB0W(dCBWTag)
  buf_cmd[5]  = 0x00;                           //  5 - LSB1W(dCBWTag)
  buf_cmd[6]  = 0x00;                           //  6 - LSB2W(dCBWTag)
  buf_cmd[7]  = 0x00;                           //  7 - LSB3W(dCBWTag)
  buf_cmd[8]  = 0x00;                           //  8 - LSB0W(dCBWDataTransferLength)
  buf_cmd[9]  = 0x00;                           //  9 - LSB1W(dCBWDataTransferLength)
  buf_cmd[10] = 0x00;                           // 10 - LSB2W(dCBWDataTransferLength)
  buf_cmd[11] = 0x00;                           // 11 - LSB3W(dCBWDataTransferLength)
  buf_cmd[12] = SBC_CMD_DIR_OUT;                // 12 - bmCBWFlags
  buf_cmd[13] = host_selected_lun;              // 13 - bCBWLUN
  buf_cmd[14] = 0x06;                           // 14 - bCBWCBLength
  buf_cmd[15] = SBC_CMD_TEST_UNIT_READY;        // 15 - CBWCB0 - Operation Code (0x00)
  buf_cmd[16] = 0x00;                           // 16 - CBWCB1 - Reserved
  buf_cmd[17] = 0x00;                           // 17 - CBWCB2 - Reserved
  buf_cmd[18] = 0x00;                           // 18 - CBWCB3 - Reserved
  buf_cmd[19] = 0x00;                           // 19 - CBWCB4 - Reserved
  buf_cmd[20] = 0x00;                           // 20 - CBWCW5 - Control
  buf_cmd[21] = 0x00;                           // 21
  buf_cmd[22] = 0x00;                           // 22
  buf_cmd[23] = 0x00;                           // 23
  buf_cmd[24] = 0x00;                           // 24
  buf_cmd[25] = 0x00;                           // 25
  buf_cmd[26] = 0x00;                           // 26
  buf_cmd[27] = 0x00;                           // 27
  buf_cmd[28] = 0x00;                           // 28
  buf_cmd[29] = 0x00;                           // 29
  buf_cmd[30] = 0x00;                           // 30

  // Send command
  if (host_send_data(g_pipe_ms_out, sizeof(buf_cmd), buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  // Get CSW
  nb = sizeof(buf_cmd);
  get_data_status = host_get_data(g_pipe_ms_in, &nb, buf_cmd);
  if (get_data_status == PIPE_STALL)
  {
    host_ms_stall_management(g_pipe_ms_in);
    return CTRL_FAIL;
  }
  else if (get_data_status != PIPE_GOOD)
    return CTRL_FAIL;

  if (buf_cmd[12] != COMMAND_PASSED)
  {
    if (host_ms_request_sense() == CTRL_BUSY)
      return CTRL_BUSY;
    return CTRL_FAIL;
  }

  Host_set_device_ready();

  return CTRL_GOOD;
}


//! This function returns the address of the last valid sector in the \a lun
//! @param lun            Logical Unit Number
//! @param u32_nb_sector  Pointer to number of sectors (sector = 512 bytes)
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status host_read_capacity(U8 lun, U32 *u32_nb_sector)
{
  Ctrl_status status;
  U16 nb;
  Status_t get_data_status;

  host_selected_lun = lun;
  g_u8_sector_size = 0;

  if (!Is_host_ms_configured()) return CTRL_FAIL;

  if (g_b_read_run && (status = host_read_10_ram_stop()) != CTRL_GOOD) return status;

  buf_cmd[0]  = 'U';                            //  0 - 0x55
  buf_cmd[1]  = 'S';                            //  1 - 0x53
  buf_cmd[2]  = 'B';                            //  2 - 0x42
  buf_cmd[3]  = 'C';                            //  3 - 0x43
  buf_cmd[4]  = 0x61;                           //  4 - LSB0W(dCBWTag)
  buf_cmd[5]  = 0x00;                           //  5 - LSB1W(dCBWTag)
  buf_cmd[6]  = 0x00;                           //  6 - LSB2W(dCBWTag)
  buf_cmd[7]  = 0x00;                           //  7 - LSB3W(dCBWTag)
  buf_cmd[8]  = 0x08;                           //  8 - LSB0W(dCBWDataTransferLength)
  buf_cmd[9]  = 0x00;                           //  9 - LSB1W(dCBWDataTransferLength)
  buf_cmd[10] = 0x00;                           // 10 - LSB2W(dCBWDataTransferLength)
  buf_cmd[11] = 0x00;                           // 11 - LSB3W(dCBWDataTransferLength)
  buf_cmd[12] = SBC_CMD_DIR_IN;                 // 12 - bmCBWFlags
  buf_cmd[13] = lun;                            // 13 - bCBWLUN
  buf_cmd[14] = 0x0A;                           // 14 - bCBWCBLength
  buf_cmd[15] = SBC_CMD_READ_CAPACITY_10;       // 15 - CBWCB0 - Operation Code (0x25)
  buf_cmd[16] = 0x00;                           // 16 - CBWCB1 - Obsolete
  buf_cmd[17] = 0x00;                           // 17 - CBWCB2 - MSB0W(Logical Block Address)
  buf_cmd[18] = 0x00;                           // 18 - CBWCB3 - MSB1W(Logical Block Address)
  buf_cmd[19] = 0x00;                           // 19 - CBWCB4 - MSB2W(Logical Block Address)
  buf_cmd[20] = 0x00;                           // 20 - CBWCW5 - MSB3W(Logical Block Address)
  buf_cmd[21] = 0x00;                           // 21 - CBWCW6 - Reserved
  buf_cmd[22] = 0x00;                           // 22 - CBWCW7 - Reserved
  buf_cmd[23] = 0x00;                           // 23 - CBWCW8 - PMI
  buf_cmd[24] = 0x00;                           // 24 - CBWCW9 - Control
  buf_cmd[25] = 0x00;                           // 25
  buf_cmd[26] = 0x00;                           // 26
  buf_cmd[27] = 0x00;                           // 27
  buf_cmd[28] = 0x00;                           // 28
  buf_cmd[29] = 0x00;                           // 29
  buf_cmd[30] = 0x00;                           // 30

  // Send command
  if (host_send_data(g_pipe_ms_out, sizeof(buf_cmd), buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  // Transfer data
  nb = sizeof(buf_cmd);
  get_data_status = host_get_data(g_pipe_ms_in, &nb, buf_cmd);
  if (get_data_status == PIPE_STALL)
  {
    host_ms_stall_management(g_pipe_ms_in);
    return CTRL_FAIL;
  }
  else if (get_data_status != PIPE_GOOD)
    return CTRL_FAIL;

  *u32_nb_sector = sbc_format_scsi_to_mcu_data(32, *(U32 *)(buf_cmd + 0));
  g_u8_sector_size = sbc_format_scsi_to_mcu_data(32, *(U32 *)(buf_cmd + 4)) >> 9;

  // Get CSW
  nb = sizeof(buf_cmd);
  if (host_get_data(g_pipe_ms_in, &nb, buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  if (buf_cmd[12] != COMMAND_PASSED)
    return CTRL_FAIL;

  Host_set_device_ready();

  return CTRL_GOOD;
}


//! This function returns the physical sector size of the \a lun
//! @param lun          Logical Unit Number
//! @return U8: Number of 512-byte blocks per sector
U8 host_read_sector_size(U8 lun)
{
  return g_u8_sector_size;
}


//! This function returns the write-protected mode
//! Only used by memory removal with a HARDWARE-SPECIFIC write-protected detection
//! @warning The customer must unplug the memory to change this write-protected mode.
//! @param lun          Logical Unit Number
//! @return TRUE if the memory is protected
Bool host_wr_protect(U8 lun)
{
  U16 nb;
  Bool write_protect;
  Status_t status;

  host_selected_lun = lun;

  if (!Is_host_ms_configured() ||
      g_u8_sector_size != 1 ||  // LUN not supported in write mode, because sector size != 512 bytes
      (g_b_read_run && host_read_10_ram_stop() != CTRL_GOOD))
    return TRUE;

  buf_cmd[0]  = 'U';                            //  0 - 0x55
  buf_cmd[1]  = 'S';                            //  1 - 0x53
  buf_cmd[2]  = 'B';                            //  2 - 0x42
  buf_cmd[3]  = 'C';                            //  3 - 0x43
  buf_cmd[4]  = 0x63;                           //  4 - LSB0W(dCBWTag)
  buf_cmd[5]  = 0x00;                           //  5 - LSB1W(dCBWTag)
  buf_cmd[6]  = 0x00;                           //  6 - LSB2W(dCBWTag)
  buf_cmd[7]  = 0x00;                           //  7 - LSB3W(dCBWTag)
  buf_cmd[8]  = 0x0C;                           //  8 - LSB0W(dCBWDataTransferLength)
  buf_cmd[9]  = 0x00;                           //  9 - LSB1W(dCBWDataTransferLength)
  buf_cmd[10] = 0x00;                           // 10 - LSB2W(dCBWDataTransferLength)
  buf_cmd[11] = 0x00;                           // 11 - LSB3W(dCBWDataTransferLength)
  buf_cmd[12] = SBC_CMD_DIR_IN;                 // 12 - bmCBWFlags
  buf_cmd[13] = host_selected_lun;              // 13 - bCBWLUN
  buf_cmd[14] = 0x06;                           // 14 - bCBWCBLength
  buf_cmd[15] = SBC_CMD_MODE_SENSE_6;           // 15 - CBWCB0 - Operation Code (0x1A)
  buf_cmd[16] = 0x00;                           // 16 - CBWCB1 - DBD
  buf_cmd[17] = 0x3F;                           // 17 - CBWCB2 - PC, Page Code
  buf_cmd[18] = 0x00;                           // 18 - CBWCB3 - Subpage Code
  buf_cmd[19] = 0x0C;                           // 19 - CBWCB4 - Allocation Length
  buf_cmd[20] = 0x00;                           // 20 - CBWCW5 - Control
  buf_cmd[21] = 0x00;                           // 21
  buf_cmd[22] = 0x00;                           // 22
  buf_cmd[23] = 0x00;                           // 23
  buf_cmd[24] = 0x00;                           // 24
  buf_cmd[25] = 0x00;                           // 25
  buf_cmd[26] = 0x00;                           // 26
  buf_cmd[27] = 0x00;                           // 27
  buf_cmd[28] = 0x00;                           // 28
  buf_cmd[29] = 0x00;                           // 29
  buf_cmd[30] = 0x00;                           // 30

  // Send command
  if (host_send_data(g_pipe_ms_out, sizeof(buf_cmd), buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  // Transfer data
  nb = sizeof(buf_cmd);
  if (host_get_data(g_pipe_ms_in, &nb, buf_cmd) != PIPE_GOOD)
    return CTRL_FAIL;

  write_protect = buf_cmd[2] >> 7;

  // Get CSW
  nb = sizeof(buf_cmd);
  status = host_get_data(g_pipe_ms_in, &nb, buf_cmd);
  if (status == PIPE_STALL)
  {
    host_ms_stall_management(g_pipe_ms_in);
    return FALSE;
  }
  else if (status != PIPE_GOOD)
    return FALSE;

  if (buf_cmd[12] != COMMAND_PASSED) return FALSE;

  return write_protect;
}


//! This function informs about the memory type
//! @return TRUE if the memory is removable
Bool host_removal(void)
{
  return Is_host_ms_configured();
}


//------------ SPECIFIC FUNCTIONS FOR TRANSFER BY RAM --------------------------

#if ACCESS_MEM_TO_RAM == ENABLED

//! This function tranfers 1 data sector from memory to RAM
//! sector = 512 bytes
//! @param addr         Sector address to start read
//! @param ram          Address of RAM buffer
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status host_read_10_ram(U32 addr, void *ram)
{
  Ctrl_status status;
  U16 nb;
  U16 u16_nb_byte_ignore_at_beg;
  U32 u32_host_ms_address;
  U32 u32_host_ms_sector;

  if (!Is_host_ms_configured()) return CTRL_FAIL;

  // It is next read
  if (!g_b_read_run || addr != g_u32_read_addr)
  {
    if (g_b_read_run && (status = host_read_10_ram_stop()) != CTRL_GOOD) return status;

    // Check read capacity OK
    if( 0 == g_u8_sector_size )
    {
       // Update g_u8_sector_size
       if( CTRL_GOOD != host_read_capacity( host_selected_lun, &u32_host_ms_sector))
         return CTRL_FAIL;
    }

    // Compute the address in physical sector units
    u32_host_ms_address       = addr / g_u8_sector_size;

    // Compute the number of bytes to ignore from the beginning of the physical sector
    u16_nb_byte_ignore_at_beg = (addr % g_u8_sector_size) * HOST_SECTOR_SIZE;

    // Compute the number of bytes per physical sector
    u32_host_ms_sector        = g_u8_sector_size * HOST_SECTOR_SIZE;

    // Save current position and remaining sectors
    g_u32_read_addr           = addr;
    g_u16_read_sector         = (u32_host_ms_address + 1) * g_u8_sector_size - addr;

    buf_cmd[0]  = 'U';                            //  0 - 0x55
    buf_cmd[1]  = 'S';                            //  1 - 0x53
    buf_cmd[2]  = 'B';                            //  2 - 0x42
    buf_cmd[3]  = 'C';                            //  3 - 0x43
    buf_cmd[4]  = 0x66;                           //  4 - LSB0W(dCBWTag)
    buf_cmd[5]  = 0x00;                           //  5 - LSB1W(dCBWTag)
    buf_cmd[6]  = 0x00;                           //  6 - LSB2W(dCBWTag)
    buf_cmd[7]  = 0x00;                           //  7 - LSB3W(dCBWTag)
    buf_cmd[8]  = LSB0W(u32_host_ms_sector);      //  8 - LSB0W(dCBWDataTransferLength)
    buf_cmd[9]  = LSB1W(u32_host_ms_sector);      //  9 - LSB1W(dCBWDataTransferLength)
    buf_cmd[10] = LSB2W(u32_host_ms_sector);      // 10 - LSB2W(dCBWDataTransferLength)
    buf_cmd[11] = LSB3W(u32_host_ms_sector);      // 11 - LSB3W(dCBWDataTransferLength)
    buf_cmd[12] = SBC_CMD_DIR_IN;                 // 12 - bmCBWFlags
    buf_cmd[13] = host_selected_lun;              // 13 - bCBWLUN
    buf_cmd[14] = 0x0A;                           // 14 - bCBWCBLength
    buf_cmd[15] = SBC_CMD_READ_10;                // 15 - CBWCB0 - Operation Code (0x28)
    buf_cmd[16] = 0x00;                           // 16 - CBWCB1 - RDPROTECT, DPO, FUA, Obsolete
    buf_cmd[17] = MSB0W(u32_host_ms_address);     // 17 - CBWCB2 - MSB0W(Logical Block Address)
    buf_cmd[18] = MSB1W(u32_host_ms_address);     // 18 - CBWCB3 - MSB1W(Logical Block Address)
    buf_cmd[19] = MSB2W(u32_host_ms_address);     // 19 - CBWCB4 - MSB2W(Logical Block Address)
    buf_cmd[20] = MSB3W(u32_host_ms_address);     // 20 - CBWCW5 - MSB3W(Logical Block Address)
    buf_cmd[21] = 0x00;                           // 21 - CBWCW6 - Reserved
    buf_cmd[22] = 0x00;                           // 22 - CBWCW7 - MSB(Transfer Length)
    buf_cmd[23] = 0x01;                           // 23 - CBWCW8 - LSB(Transfer Length)
    buf_cmd[24] = 0x00;                           // 24 - CBWCW9 - Control
    buf_cmd[25] = 0x00;                           // 25
    buf_cmd[26] = 0x00;                           // 26
    buf_cmd[27] = 0x00;                           // 27
    buf_cmd[28] = 0x00;                           // 28
    buf_cmd[29] = 0x00;                           // 29
    buf_cmd[30] = 0x00;                           // 30

    // Send command
    host_send_data(g_pipe_ms_out, sizeof(buf_cmd), buf_cmd);

    // Ignore data at beginning of physical sector (in case physical sector != 512 bytes)
    if (host_get_data(g_pipe_ms_in, &u16_nb_byte_ignore_at_beg, NULL) == PIPE_STALL)
    {
      host_ms_stall_management(g_pipe_ms_in);
      return CTRL_FAIL;
    }

    g_b_read_run = TRUE;
  }

  // Transfer data
  nb = HOST_SECTOR_SIZE;
  if (host_get_data(g_pipe_ms_in, &nb, ram) == PIPE_STALL)
  {
    host_ms_stall_management(g_pipe_ms_in);
    return CTRL_FAIL;
  }

  g_u32_read_addr++;
  if (!--g_u16_read_sector) return host_read_10_ram_stop();

  return CTRL_GOOD;
}


//! This function terminates the tranfer of a physical sector from memory to RAM
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status host_read_10_ram_stop(void)
{
  U16 nb;

  g_b_read_run = FALSE;

  // Ignore data at end of physical sector (in case physical sector != 512 bytes)
  g_u16_read_sector *= HOST_SECTOR_SIZE;
  if (host_get_data(g_pipe_ms_in, &g_u16_read_sector, NULL) == PIPE_STALL)
  {
    host_ms_stall_management(g_pipe_ms_in);
    return CTRL_FAIL;
  }

  // Get CSW
  nb = sizeof(buf_cmd);
  host_get_data(g_pipe_ms_in, &nb, buf_cmd);

  return (buf_cmd[12] == COMMAND_PASSED) ? CTRL_GOOD : CTRL_FAIL;
}


//! This function tranfers 1 data sector from RAM to memory
//! sector = 512 bytes
//! @param addr         Sector address to start write
//! @param ram          Address of RAM buffer
//! @return                            Ctrl_status
//!   It is ready                ->    CTRL_GOOD
//!   Memory unplug              ->    CTRL_NO_PRESENT
//!   Not initialized or changed ->    CTRL_BUSY
//!   An error occurred          ->    CTRL_FAIL
Ctrl_status host_write_10_ram(U32 addr, const void *ram)
{
  Ctrl_status status;
  U16 nb;

  if (!Is_host_ms_configured() ||
      g_u8_sector_size != 1)  // LUN not supported in write mode, because sector size != 512 bytes
    return CTRL_FAIL;

  if (g_b_read_run && (status = host_read_10_ram_stop()) != CTRL_GOOD) return status;

  buf_cmd[0]  = 'U';                            //  0 - 0x55
  buf_cmd[1]  = 'S';                            //  1 - 0x53
  buf_cmd[2]  = 'B';                            //  2 - 0x42
  buf_cmd[3]  = 'C';                            //  3 - 0x43
  buf_cmd[4]  = 0x66;                           //  4 - LSB0W(dCBWTag)
  buf_cmd[5]  = 0x00;                           //  5 - LSB1W(dCBWTag)
  buf_cmd[6]  = 0x00;                           //  6 - LSB2W(dCBWTag)
  buf_cmd[7]  = 0x00;                           //  7 - LSB3W(dCBWTag)
  buf_cmd[8]  = (U8)HOST_SECTOR_SIZE;           //  8 - LSB0W(dCBWDataTransferLength)
  buf_cmd[9]  = (U8)(HOST_SECTOR_SIZE >> 8);    //  9 - LSB1W(dCBWDataTransferLength)
  buf_cmd[10] = (U8)(HOST_SECTOR_SIZE >> 16);   // 10 - LSB2W(dCBWDataTransferLength)
  buf_cmd[11] = HOST_SECTOR_SIZE >> 24;         // 11 - LSB3W(dCBWDataTransferLength)
  buf_cmd[12] = SBC_CMD_DIR_OUT;                // 12 - bmCBWFlags
  buf_cmd[13] = host_selected_lun;              // 13 - bCBWLUN
  buf_cmd[14] = 0x0A;                           // 14 - bCBWCBLength
  buf_cmd[15] = SBC_CMD_WRITE_10;               // 15 - CBWCB0 - Operation Code (0x2A)
  buf_cmd[16] = 0x00;                           // 16 - CBWCB1 - WRPROTECT, DPO, FUA, Obsolete
  buf_cmd[17] = MSB0W(addr);                    // 17 - CBWCB2 - MSB0W(Logical Block Address)
  buf_cmd[18] = MSB1W(addr);                    // 18 - CBWCB3 - MSB1W(Logical Block Address)
  buf_cmd[19] = MSB2W(addr);                    // 19 - CBWCB4 - MSB2W(Logical Block Address)
  buf_cmd[20] = MSB3W(addr);                    // 20 - CBWCW5 - MSB3W(Logical Block Address)
  buf_cmd[21] = 0x00;                           // 21 - CBWCW6 - Reserved
  buf_cmd[22] = 0x00;                           // 22 - CBWCW7 - MSB(Transfer Length)
  buf_cmd[23] = 0x01;                           // 23 - CBWCW8 - LSB(Transfer Length)
  buf_cmd[24] = 0x00;                           // 24 - CBWCW9 - Control
  buf_cmd[25] = 0x00;                           // 25
  buf_cmd[26] = 0x00;                           // 26
  buf_cmd[27] = 0x00;                           // 27
  buf_cmd[28] = 0x00;                           // 28
  buf_cmd[29] = 0x00;                           // 29
  buf_cmd[30] = 0x00;                           // 30

  // Send command
  host_send_data(g_pipe_ms_out, sizeof(buf_cmd), buf_cmd);

  // Transfer data
  if (host_send_data(g_pipe_ms_out, HOST_SECTOR_SIZE, ram) == PIPE_STALL)
  {
    host_ms_stall_management(g_pipe_ms_out);
    return CTRL_FAIL;
  }

  // Get CSW
  nb = sizeof(buf_cmd);
  host_get_data(g_pipe_ms_in, &nb, buf_cmd);

  return (buf_cmd[12] == COMMAND_PASSED) ? CTRL_GOOD : CTRL_FAIL;
}

#endif  // ACCESS_MEM_TO_RAM == ENABLED


#endif  // MEM_USB == ENABLE
