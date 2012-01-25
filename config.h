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
#ifndef CONFIG_H
#define CONFIG_H

/**
 * These are SIO2Arduino feature definitions.
 */

// These are the Arduino devices that can be used. I'm sure others would work,
// but these are the only ones I have to test with. Only one of these should
// be uncommented.
#define ARDUINO_UNO              // Arduino Uno board
//#define ARDUINO_MEGA           // Arduino Mega 2560/ADK board

// Uncomment this line if you are using an LCD display
//#define LCD_DISPLAY

// Uncomment this line if you are using a hardware button for image selection
#define SELECTOR_BUTTON

// uncomment if using an Ethernet shield for SD capabilities
//#define ETHERNET_SHIELD

// uncomment for ATX image format support (Mega 2560 only)
//#define ATX_IMAGES

// uncomment for XEX "image" support
#define XEX_IMAGES

// uncomment this to enable debug logging -- make sure the HARDWARE_UART isn't the same as
// the LOGGING_UART defined at the bottom of the file
#define DEBUG 

/*
 * These are the Arduino pin definitions.
 */
 
#define PIN_ATARI_CMD         2    // the Atari SIO command line - usually the purple wire on the SIO cable

// for now, you can't change these pin definitions
#ifdef ETHERNET_SHIELD
  #define PIN_SD_CS           4    // the SD CS line
#else
  #ifdef ARDUINO_MEGA
    #define PIN_SD_CS         53   // the SD breakout board's CS (chip select) pin
    #define PIN_SD_DI         51   // the SD breakout board's DI pin
    #define PIN_SD_DO         50   // the SD breakout board's DO pin
    #define PIN_SD_CLK        52   // the SD breakout board's CLK pin
  #else
    #define PIN_SD_CS         10   // the SD breakout board's CS (chip select) pin
    #define PIN_SD_DI         11   // the SD breakout board's DI pin
    #define PIN_SD_DO         12   // the SD breakout board's DO pin
    #define PIN_SD_CLK        13   // the SD breakout board's CLK pin
  #endif
#endif

#ifdef SELECTOR_BUTTON
  #define PIN_SELECTOR        3    // the selector button pin
#endif

#ifdef LCD_DISPLAY
  #ifdef ARDUINO_MEGA
    #define PIN_LCD_RD          5    // *
    #define PIN_LCD_ENABLE      6    // *
    #define PIN_LCD_DB4         10   // * LCD display pins
    #define PIN_LCD_DB5         9    // *
    #define PIN_LCD_DB6         8    // *
    #define PIN_LCD_DB7         7    // *
  #else
    #define PIN_LCD_RD          4    // *
    #define PIN_LCD_ENABLE      5    // *
    #define PIN_LCD_DB4         9    // * LCD display pins
    #define PIN_LCD_DB5         8    // *
    #define PIN_LCD_DB6         7    // *
    #define PIN_LCD_DB7         6    // *
  #endif
#endif

// the hardware UART to use for SIO bus communication
#ifdef ARDUINO_MEGA
  #define SIO_UART     Serial1
  #define SIO_CALLBACK serialEvent1
#else
  #define SIO_UART     Serial
  #define SIO_CALLBACK serialEvent
#endif

/**
 * Logging/debug config
 */
#ifdef DEBUG
  #define LOGGING_UART Serial
  #define LOG_MSG(...) LOGGING_UART.print(__VA_ARGS__)
  #define LOG_MSG_CR(...) LOGGING_UART.println(__VA_ARGS__)
#else
  #define LOG_MSG(...)
  #define LOG_MSG_CR(...)
#endif

#endif
