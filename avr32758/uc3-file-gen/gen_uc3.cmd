@echo off
setlocal enableextensions enabledelayedexpansion

: Usage: gen_uc3.cmd {<hexfile>|<binfile>}
:
: This command script converts a UC3 memory image to an avr32fwupgrade.uc3 file
: programmable by the AVR32 USB MS ISP.
:
: <hexfile>   Input Intel HEX file to convert.
:
: <binfile>   Input raw binary file to convert. This file must start at the
:             beginning of the UC3 internal flash.

: Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
:
: Redistribution and use in source and binary forms, with or without
: modification, are permitted provided that the following conditions are met:
:
: 1. Redistributions of source code must retain the above copyright notice, this
: list of conditions and the following disclaimer.
:
: 2. Redistributions in binary form must reproduce the above copyright notice,
: this list of conditions and the following disclaimer in the documentation and/
: or other materials provided with the distribution.
:
: 3. The name of ATMEL may not be used to endorse or promote products derived
: from this software without specific prior written permission.
:
: THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
: WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
: MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
: SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
: INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
: BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
: DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
: OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
: NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
: EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


: If the input file is in Intel HEX format,
if /i %~x1 equ .hex (
  : convert the input Intel HEX file to avr32fwupgrade.uc3,
  srec_cat ^
    ^( %1 -intel ^
       -crop 0x80008000 0x80080000 ^
       -fill 0x00 0x80008000 -max %1 -intel -crop 0x80008000 0x80080000 ^) ^
    -offset -0x80007FE7 ^
    -b-e-crc32 0x00000015 ^
    -gen 0x00000005 0x00000015 -repeat-data 0xA3 0x21 0xB4 0x20 0x3E 0xE9 0x11 0xDD 0xAE 0x16 0x08 0x00 0x20 0x0C 0x9A 0x66 ^
    -gen 0x00000000 0x00000005 -repeat-string AVR32 ^
    -o avr32fwupgrade.uc3 -binary
) else (
: else
  : convert the input raw binary file to avr32fwupgrade.uc3.
  srec_cat ^
    %1 -binary ^
    -crop 0x00008000 0x00080000 ^
    -offset -0x00007FE7 ^
    -b-e-crc32 0x00000015 ^
    -gen 0x00000005 0x00000015 -repeat-data 0xA3 0x21 0xB4 0x20 0x3E 0xE9 0x11 0xDD 0xAE 0x16 0x08 0x00 0x20 0x0C 0x9A 0x66 ^
    -gen 0x00000000 0x00000005 -repeat-string AVR32 ^
    -o avr32fwupgrade.uc3 -binary
)
