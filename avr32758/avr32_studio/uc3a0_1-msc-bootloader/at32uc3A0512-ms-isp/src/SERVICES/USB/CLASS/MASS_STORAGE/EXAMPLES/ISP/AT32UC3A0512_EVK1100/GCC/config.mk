# Hey Emacs, this is a -*- makefile -*-

# The purpose of this file is to define the build configuration variables used
# by the generic Makefile. See Makefile header for further information.

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


# Base paths
PRJ_PATH = ../../../../../../../..
APPS_PATH = $(PRJ_PATH)/APPLICATIONS
BRDS_PATH = $(PRJ_PATH)/BOARDS
COMP_PATH = $(PRJ_PATH)/COMPONENTS
DRVR_PATH = $(PRJ_PATH)/DRIVERS
SERV_PATH = $(PRJ_PATH)/SERVICES
UTIL_PATH = $(PRJ_PATH)/UTILS

# CPU architecture: {ap|ucr1|ucr2}
ARCH = ucr2

# Part: {none|ap7xxx|uc3xxxxx}
PART = uc3a0512

# Flash memories: [{cfi|internal}@address,size]...
FLASH = internal@0x80000000,512Kb

# Clock source to use when programming: [{xtal|extclk|int}]
PROG_CLOCK = xtal

# Device/Platform/Board include path
PLATFORM_INC_PATH = \
  $(BRDS_PATH)/

# Target name: {*.a|*.elf}
TARGET = $(PART)-isp.elf

# Definitions: [-D name[=definition]...] [-U name...]
# Things that might be added to DEFS:
#   BOARD             Board used: see $(BRDS_PATH)/board.h
#   EXT_BOARD         Extension board used (if any): see $(BRDS_PATH)/board.h
#   ISP_OSC           Oscillator that the ISP will use: {0|1}
DEFS = -D BOARD=EVK1100 \
       -D ISP_OSC=0

# ISP: forcing word
ISP_FORCE = MSIF

# Include path
INC_PATH = \
  $(UTIL_PATH)/ \
  $(UTIL_PATH)/PREPROCESSOR/ \
  $(SERV_PATH)/MEMORY/CTRL_ACCESS/ \
  $(SERV_PATH)/FAT/ \
  $(DRVR_PATH)/PM/ \
  $(DRVR_PATH)/WDT/ \
  $(DRVR_PATH)/FLASHC/ \
  $(DRVR_PATH)/USBB/ \
  $(DRVR_PATH)/USBB/ENUM/ \
  $(DRVR_PATH)/USBB/ENUM/HOST/ \
  $(SERV_PATH)/USB/ \
  ../../../../SCSI_DECODER/ \
  ../../../../HOST_MEM/ \
  ../../ \
  ../../CONF/ \
  ../../BOOT/ \
  ../../INTC/

# C source files
CSRCS = \
  $(SERV_PATH)/MEMORY/CTRL_ACCESS/ctrl_access.c \
  $(SERV_PATH)/FAT/fat.c \
  $(SERV_PATH)/FAT/fat_unusual.c \
  $(SERV_PATH)/FAT/file.c \
  $(SERV_PATH)/FAT/navigation.c \
  $(DRVR_PATH)/PM/pm.c \
  ../../flashc.c \
  $(DRVR_PATH)/USBB/usb_drv.c \
  $(DRVR_PATH)/USBB/ENUM/usb_task.c \
  $(DRVR_PATH)/USBB/ENUM/HOST/usb_host_enum.c \
  $(DRVR_PATH)/USBB/ENUM/HOST/usb_host_task.c \
  ../../../../SCSI_DECODER/scsi_decoder.c \
  ../../../../HOST_MEM/host_mem.c \
  ../../host_mass_storage_task.c \
  ../../crc32.c \
  ../../isp.c \
  ../../INTC/intc.c
#   $(DRVR_PATH)/WDT/wdt.c

# Assembler source files
ASSRCS = \
  ../../BOOT/boot.S

# Library path
LIB_PATH =

# Libraries to link with the project
LIBS =

# Linker script file if any
LINKER_SCRIPT = ./link_uc3a0512-isp.lds

# Options to request or suppress warnings: [-fsyntax-only] [-pedantic[-errors]] [-w] [-Wwarning...]
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
WARNINGS = -Wall

# Options for debugging: [-g]...
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
DEBUG = -g

# Options that control optimization: [-O[0|1|2|3|s]]...
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
OPTIMIZATION = -Os -ffunction-sections -fdata-sections

# Extra flags to use when preprocessing
CPP_EXTRA_FLAGS =

# Extra flags to use when compiling
C_EXTRA_FLAGS =

# Extra flags to use when assembling
AS_EXTRA_FLAGS =

# Extra flags to use when linking
LD_EXTRA_FLAGS = -Wl,--gc-sections -nostartfiles

# Documentation path
DOC_PATH = \
  ../../DOC/

# Documentation configuration file
DOC_CFG = \
  ../doxyfile.doxygen
