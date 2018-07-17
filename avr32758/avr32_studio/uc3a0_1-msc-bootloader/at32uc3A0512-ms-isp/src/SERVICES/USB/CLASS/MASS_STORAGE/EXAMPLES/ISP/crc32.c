/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief 32-bit ANSI X3.66 CRC checksum.
 *
 * This module may be used to compute the 32-bit CRC used as the frame check
 * sequence in ADCCP (ANSI X3.66, also known as FIPS PUB 71 and FED-STD-1003,
 * the US versions of CCITT's X.25 link-level protocol).
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USB module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

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


#include <stddef.h>
#include "compiler.h"
#include "crc32.h"


//! CCITT's CRC32 seed.
#define CRC32_CCITT_SEED  0xFFFFFFFF

//! The CRC32 polynomial.
//!
//! The polynomial is
//! X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
//! (0x104C11DB7).
//!
//! \note We take it `backwards' and put the highest-order term in the
//!       lowest-order bit. The X^32 term is `implied'; the lsb is the X^31
//!       term, etc. The X^0 term (usually shown as `+1') results in the msb
//!       being 1.
#define CRC32_POLYNOMIAL 0xEDB88320


//! The table of feedback terms of the CRC32 polynomial.
//!
//! \note The usual hardware shift register implementation, which is what we're
//! using (we're merely optimizing it by doing eight-bit chunks at a time)
//! shifts bits into the lowest-order term. In our implementation, that means
//! shifting towards the right. Why do we do it this way? Because the calculated
//! CRC must be transmitted in order from highest-order term to lowest-order
//! term. UARTs transmit characters in order from LSB to MSB. By storing the CRC
//! this way, we hand it to the UART in the order low-byte to high-byte; the
//! UART sends each low-bit to high-bit; and the result is transmission bit by
//! bit from highest- to lowest-order term without requiring any bit shuffling
//! on our part. Reception works similarly.
//!
//! The feedback terms table consists of 256 32-bit entries.
//!
//! The values must be right-shifted by eight bits by the
//! \ref crc32_iterate_byte logic; the shift must be unsigned (bring in zeroes).
static crc32_table_t *crc32_table = NULL;

//! Variable used to remember the running value of the 32-bit CRC.
static U32 crc32;


void crc32_init(crc32_table_t *buffer)
{
  unsigned int b;

  crc32_table = buffer;

  for (b = 0; b < 256; b++)
  {
    U32 v = b;
    int i;

    for (i = 7; i >= 0; i--)
      v = (v & 1) ? ((v >> 1) ^ CRC32_POLYNOMIAL) : (v >> 1);

    (*crc32_table)[b] = v;
  }

  crc32_reset();
}


void crc32_reset(void)
{
  crc32 = CRC32_CCITT_SEED;
}


void crc32_iterate_byte(U8 byte)
{
  crc32 = (*crc32_table)[(crc32 ^ byte) & 0xFF] ^ (crc32 >> 8);
}


void crc32_iterate_buffer(const void *buffer, size_t nbytes)
{
  const U8 *pbyte = buffer;

  while (nbytes--)
    crc32_iterate_byte(*pbyte++);
}


U32 crc32_get_value(void)
{
  return ~crc32;
}
