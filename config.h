/*
 * config.h - Configuration for the SIO2Arduino build.
 *
 * Copyright (c) 2012 Whizzo Software LLC (Daniel Noguerol)
 *
 * This file is part of the SIO2Arduino project which emulates
 * Atari 8-bit SIO devices on Arduino hardware.
 *
 * SIO2Arduino is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SIO2Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SIO2Arduino; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
 * These are SIO2Arduino feature definitions.
 */

// uncomment to use an LCD display
//#define LCD_DISPLAY

// uncomment to use a selector button
#define SELECTOR_BUTTON

// uncomment (and name appropriately) to use a hardware UART for serial communication (e.g. Arduino Mega)
//#define HARDWARE_UART   Serial1

/*
 * These are the Arduino pin definitions.
 */
 
#define PIN_ATARI_CMD   2    // the Atari SIO command line
#define PIN_SD_CARD     4    // the SD card pin

#ifndef HARDWARE_UART
#define PIN_SERIAL_RX   5
#define PIN_SERIAL_TX   6
#endif

#ifdef SELECTOR_BUTTON
#define PIN_SELECTOR    8    // the selector button pin
#endif

#ifdef LCD_DISPLAY
#define PIN_LCD_RD      30   // *
#define PIN_LCD_ENABLE  32   // *
#define PIN_LCD_DB4     40   // * LCD display pins
#define PIN_LCD_DB5     38   // *
#define PIN_LCD_DB6     36   // *
#define PIN_LCD_DB7     34   // *
#endif
