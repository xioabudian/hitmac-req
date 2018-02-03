/*
 * Copyright (c) 2016, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/** \addtogroup launchpad-peripherals
 * @{
 *
 * \defgroup launchpad-cc1310-specific CC1310 LaunchPad Peripherals
 *
 * Defines related to the CC1310 LaunchPad
 *
 * This file provides connectivity information on LEDs, Buttons, UART and
 * other peripherals
 *
 * This file is not meant to be modified by the user.
 * @{
 *
 * \file
 * Header file with definitions related to the I/O connections on the TI
 * CC1310 LaunchPad
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */
/*---------------------------------------------------------------------------*/
#ifndef BOARD_H_
#define BOARD_H_
/*---------------------------------------------------------------------------*/
#include "ioc.h"
/*---------------------------------------------------------------------------*/
/**
 * \name LED configurations
 *
 * Those values are not meant to be modified by the user
 * @{
 */
/* Some files include leds.h before us, so we need to get rid of defaults in
 * leds.h before we provide correct definitions */
#undef LEDS_GREEN
#undef LEDS_YELLOW
#undef LEDS_RED
#undef LEDS_CONF_ALL

#define LEDS_RED       1
#define LEDS_GREEN     LEDS_RED
#define LEDS_YELLOW    LEDS_RED
#define LEDS_ORANGE    LEDS_RED

#define LEDS_CONF_ALL  1

/* Notify various examples that we have LEDs */
#define PLATFORM_HAS_LEDS        1
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name LED IOID mappings
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#define BOARD_IOID_LOGIC            IOID_29
#define BOARD_IOID_RED_LED          IOID_17
#define BOARD_RED_LED              (1 << BOARD_IOID_RED_LED)
#define BOARD_LED_ALL             (BOARD_IOID_RED_LED)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name UART IOID mapping
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#define BOARD_IOID_UART_RX        IOID_28
#define BOARD_IOID_UART_TX        IOID_27
#define BOARD_IOID_UART_CTS       IOID_UNUSED
#define BOARD_IOID_UART_RTS       IOID_UNUSED

/* #define BOARD_IOID_UART_RTS       IOID_18 */
/* #define BOARD_IOID_UART_CTS       IOID_19 */
#define BOARD_UART_RX             (1 << BOARD_IOID_UART_RX)
#define BOARD_UART_TX             (1 << BOARD_IOID_UART_TX)

#define BOARD_UART_CTS            (1 << BOARD_IOID_UART_CTS)
#define BOARD_UART_RTS            (1 << BOARD_IOID_UART_RTS)

/** @} */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/**
 * \name CC1310 434 PA
 * operated by root
 *
 * @{
 */
#if PA_434MHZ
#define CC1310_PA_CSD        IOID_30
#define CC1310_PA_CTX        IOID_2
#define BOARD_IOID_DIO2      IOID_UNUSED
#define BOARD_IOID_DIO29     IOID_UNUSED
#else
#define BOARD_IOID_DIO2           IOID_2
#define BOARD_IOID_DIO29           IOID_30
#endif

/** @} */
/*---------------------------------------------------------------------------*/

/**
 * \brief ROM bootloader configuration
 *
 * Change SET_CCFG_BL_CONFIG_BL_PIN_NUMBER to BOARD_IOID_KEY_xyz to select
 * which button triggers the bootloader on reset.
 *
 * The remaining values are not meant to be modified by the user
 * @{
 */
#if ROM_BOOTLOADER_ENABLE
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE            0xC5
#define SET_CCFG_BL_CONFIG_BL_LEVEL                     0x00
#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER                BOARD_IOID_KEY_LEFT
#define SET_CCFG_BL_CONFIG_BL_ENABLE                    0xC5
#else
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE            0x00
#define SET_CCFG_BL_CONFIG_BL_LEVEL                     0x01
#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER                0xFF
#define SET_CCFG_BL_CONFIG_BL_ENABLE                    0xFF
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \brief Remaining pins
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#define BOARD_IOID_DIO1           IOID_1
/* #define BOARD_IOID_DIO2           IOID_2 */
#define BOARD_IOID_DIO3           IOID_3
#define BOARD_IOID_DIO4           IOID_4
#define BOARD_IOID_DIO5           IOID_5
#define BOARD_IOID_DIO6           IOID_6
#define BOARD_IOID_DIO7           IOID_7
#define BOARD_IOID_DIO8           IOID_8
#define BOARD_IOID_DIO9           IOID_9
#define BOARD_IOID_DIO10           IOID_10
#define BOARD_IOID_DIO11           IOID_11
#define BOARD_IOID_DIO12           IOID_12
#define BOARD_IOID_DIO13           IOID_13
#define BOARD_IOID_DIO14           IOID_14

#define BOARD_IOID_DIO15           IOID_15
#define BOARD_IOID_DIO16           IOID_16
#define BOARD_IOID_DIO18           IOID_18
#define BOARD_IOID_DIO19           IOID_19
#define BOARD_IOID_DIO20           IOID_20
#define BOARD_IOID_DIO21           IOID_21
#define BOARD_IOID_DIO22           IOID_22
#define BOARD_IOID_DIO23           IOID_23
#define BOARD_IOID_DIO24           IOID_24
#define BOARD_IOID_DIO25           IOID_25
#define BOARD_IOID_DIO26           IOID_26
/* #define BOARD_IOID_DIO29           IOID_30 */

#define BOARD_UNUSED_PINS { \
    BOARD_IOID_DIO1, BOARD_IOID_DIO2, BOARD_IOID_DIO3, BOARD_IOID_DIO4, BOARD_IOID_DIO5, \
    BOARD_IOID_DIO6, BOARD_IOID_DIO7, BOARD_IOID_DIO8, BOARD_IOID_DIO9, BOARD_IOID_DIO10, \
    BOARD_IOID_DIO11, BOARD_IOID_DIO12, BOARD_IOID_DIO13, BOARD_IOID_DIO15, \
    BOARD_IOID_DIO16, BOARD_IOID_DIO18, BOARD_IOID_DIO19, BOARD_IOID_DIO20, \
    BOARD_IOID_DIO21, BOARD_IOID_DIO22, BOARD_IOID_DIO23, BOARD_IOID_DIO24, BOARD_IOID_DIO25, \
    BOARD_IOID_DIO26, BOARD_IOID_DIO29, \
    IOID_UNUSED \
}
/** @} */

/*---------------------------------------------------------------------------*/
/**
 * \name RF Front End configuration
 *
 * Those values are not meant to be modified by the user
 * @{
 */

#define DOT_15_4G_CONF_FREQUENCY_BAND_ID          14  /* 433MHz CC1310 A version*/
#define RF_CORE_CONF_PROP_FRONT_END_MODE        0 /* Setting by SmartRF */
#define RF_CORE_CONF_PROP_BIAS_MODE             0x01 /* External */
/** @} */

/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "HIT CPSGroup Evaluation Board"

/** @} */
/*---------------------------------------------------------------------------*/
#endif /* BOARD_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
