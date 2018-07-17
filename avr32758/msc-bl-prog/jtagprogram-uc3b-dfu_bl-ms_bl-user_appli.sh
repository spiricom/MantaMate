#!/bin/sh

# Usage: jtagprogram-uc3b-dfu_bl-ms_bl-user_appli.sh {<user_hexfile>}
#
# This shell script programs through JTAG the USB DFU bootloader (flash array),
# the USB MS bootloader(flash array), the user application (flash array),
# the MS ISP forcing word (User page), the general-purpose fuse bits and the Security bit.

# Copyright (C) 2006-2008, Atmel Corporation All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/
# or other materials provided with the distribution.
#
# 3. The name of ATMEL may not be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
# SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


echo
echo Performing a JTAG Chip Erase command.
avr32program chiperase

echo
echo Programming MCU memory with 'USB DFU bootloader', 'USB MSC bootloader' and 'User application'
srec_cat \
  dfu-bl/at32uc3b-isp-1.0.2.hex -intel \
  -crop 0x80000000 0x80002000 \
  -offset -0x80000000 \
  msc-bl/at32uc3b0256-ms-isp-beta.hex -intel \
  -crop 0x80002000 0x80008000 \
  -offset -0x80000000 \
  $1 -intel \
  -crop 0x80008000 0x80080000 \
  -offset -0x80000000 \
  -o temp.bin -binary
avr32program program -finternal@0x80000000,256Kb -cxtal -v -O0x80000000 -Fbin temp.bin
rm -f temp.bin

echo
echo Programming USB DFU bootloader configuration word
avr32program program -finternal@0x80000000,256Kb -cxtal -e -v -O0x808001FC -Fbin dfu-bl/at32uc3b-isp_cfg-1.0.2.bin

echo
echo Programming general-purpose fuse bits
avr32program writefuses -finternal@0x80000000,256Kb gp=0x7C03FFFF

echo
echo Executing application
avr32program run -R
