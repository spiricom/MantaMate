#!/bin/sh

# Usage: dfuprogram-uc3a-ms_bl-user_appli.sh {<user_hexfile>}
#
# This shell script programs through the DFU bootloader the USB MSC bootloader (flash array),
# the user application (flash array), the MS ISP forcing word (User page),
# the general-purpose fuse bits and the Security bit.

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
echo Programming MCU memory with 'USB MSC bootloader' and 'User application'

srec_cat \
  msc-bl/at32uc3a0512-ms-isp-beta.hex -intel \
  -crop 0x80002000 0x80008000 \
  $1 -intel \
  -crop 0x80008000 0x80080000 \
  -o temp.hex -intel
batchisp \
  -device at32uc3a0512 \
  -hardware usb \
  -operation \
    erase f \
    memory flash \
    blankcheck \
    loadbuffer temp.hex \
    program \
    verify \
    memory user \
    addrange 0x000001F8 0x000001FB \
    fillbuffer 0xFF \
    program \
    verify \
    memory configuration \
    addrange 0x00000000 0x00000000 \
    fillbuffer 0x00 \
    program \
    verify \
    memory security \
    addrange 0x00000000 0x00000000 \
    fillbuffer 0x01 \
    program \
    start reset 0
rm -f temp.hex
