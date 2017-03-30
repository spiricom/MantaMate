/*****************************************************************************
 *
 * \file
 *
 * \brief Configuration template file for the AVR UC3 LCD MEMORY driver.
 *
 *
 * Copyright (c) 2014-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 *****************************************************************************/
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */


#ifndef __CONF_MEMORY_H__
#define __CONF_MEMORY_H__

#include "board.h"

/*! \name SPI Connections of the MEMORY LCD
 */
//! @{
#if !defined(MEMORY_SPI)           || \
!defined(MEMORY_SPI_NPCS)          || \
!defined(MEMORY_SPI_SCK_PIN)       || \
!defined(MEMORY_SPI_SCK_FUNCTION)  || \
!defined(MEMORY_SPI_MISO_PIN)      || \
!defined(MEMORY_SPI_MISO_FUNCTION) || \
!defined(MEMORY_SPI_MOSI_PIN)      || \
!defined(MEMORY_SPI_MOSI_FUNCTION) || \
!defined(MEMORY_SPI_NPCS_PIN)      || \
!defined(MEMORY_SPI_NPCS_FUNCTION) 
#  if UC3A
#    define MEMORY_SPI                  (&AVR32_SPI1)
#    define MEMORY_SPI_NPCS             1
#    define MEMORY_SPI_SCK_PIN          AVR32_SPI1_SCK_0_0_PIN
#    define MEMORY_SPI_SCK_FUNCTION     AVR32_SPI1_SCK_0_0_FUNCTION
#    define MEMORY_SPI_MISO_PIN         AVR32_SPI1_MISO_0_0_PIN
#    define MEMORY_SPI_MISO_FUNCTION    AVR32_SPI1_MISO_0_0_FUNCTION
#    define MEMORY_SPI_MOSI_PIN         AVR32_SPI1_MOSI_0_0_PIN
#    define MEMORY_SPI_MOSI_FUNCTION    AVR32_SPI1_MOSI_0_0_FUNCTION
#    define MEMORY_SPI_NPCS_PIN         AVR32_SPI1_NPCS_2_0_PIN
#    define MEMORY_SPI_NPCS_FUNCTION    AVR32_SPI1_NPCS_2_0_FUNCTION
#  else
#    define MEMORY_SPI_NPCS             1
#    define MEMORY_SPI                  (&AVR32_SPI)
#    define MEMORY_SPI_SCK_PIN          AVR32_SPI_SCK_0_0_PIN
#    define MEMORY_SPI_SCK_FUNCTION     AVR32_SPI_SCK_0_0_FUNCTION
#    define MEMORY_SPI_MISO_PIN         AVR32_SPI_MISO_0_0_PIN
#    define MEMORY_SPI_MISO_FUNCTION    AVR32_SPI_MISO_0_0_FUNCTION
#    define MEMORY_SPI_MOSI_PIN         AVR32_SPI_MOSI_0_0_PIN
#    define MEMORY_SPI_MOSI_FUNCTION    AVR32_SPI_MOSI_0_0_FUNCTION
#    define MEMORY_SPI_NPCS_PIN         AVR32_SPI_NPCS_2_0_PIN
#    define MEMORY_SPI_NPCS_FUNCTION    AVR32_SPI_NPCS_2_0_FUNCTION
#  endif
#  warning The MEMORY SPI configuration does not exist in the board definition file. Using default settings.
#endif
//! @}

/*! \name GPIO and PWM Connections of the MEMORY LCD Backlight
 */
//! @{
#if !defined(MEMORY_BACKLIGHT_PIN)
#  if UC3A
#    define MEMORY_BACKLIGHT_PIN        AVR32_PIN_PB18
#  else
#    define MEMORY_BACKLIGHT_PIN        AVR32_PWM_6_1_PIN
#  endif
#  warning The MEMORY Backlight configuration does not exist in the board definition file. Using default settings.
#endif


#if !defined(MEMORY_PWM_CHANNEL) || \
!defined(MEMORY_PWM_PIN)         || \
!defined(MEMORY_PWM_FUNCTION)
#  if UC3A
#    define MEMORY_PWM_CHANNEL          6
#    define MEMORY_PWM_PIN              AVR32_PWM_6_PIN
#    define MEMORY_PWM_FUNCTION         AVR32_PWM_6_FUNCTION
#  else
#    define MEMORY_PWM_CHANNEL          6
#    define MEMORY_PWM_PIN              AVR32_PWM_6_1_PIN
#    define MEMORY_PWM_FUNCTION         AVR32_PWM_6_1_FUNCTION
#  endif
#  warning The MEMORY PWM configuration does not exist in the board definition file. Using default settings.
#endif
//! @}

#endif // __CONF_MEMORY_H__
