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


#ifndef _CRC32_H_
#define _CRC32_H_

#include <stddef.h>
#include "compiler.h"


//! Type of the pre-computed CRC32 table.
typedef U32 crc32_table_t[1 << (sizeof(U8) * 8)];


/*! \brief Initializes the CRC32 module.
 *
 * \param buffer  Pointer to a writeable buffer to use as pre-computed CRC32
 *                table.
 */
extern void crc32_init(crc32_table_t *buffer);

/*! \brief Resets the CRC32 module.
 */
extern void crc32_reset(void);

/*! \brief Iterates the CRC32 by one byte.
 *
 * \param byte  Byte to iterate the CRC32 by.
 */
extern void crc32_iterate_byte(U8 byte);

/*! \brief Iterates the CRC32 by a buffer.
 *
 * \param buffer  Pointer to the buffer to iterate the CRC32 by.
 * \param nbytes  Size of the buffer in bytes.
 */
extern void crc32_iterate_buffer(const void *buffer, size_t nbytes);

/*! \brief Obtains the running value of the CRC32.
 *
 * \return The running value of the CRC32.
 */
extern U32 crc32_get_value(void);


#endif  // _CRC32_H_
