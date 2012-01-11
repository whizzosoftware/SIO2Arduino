/*
 * sio2arduino.ino - An Atari 8-bit device emulator for Arduino.
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
#include "config.h"
#include <SD.h>
#include "atari.h"
#include "sio_channel.h"
#include "disk_drive.h"
#ifdef LCD_DISPLAY
#include <LiquidCrystal.h>
#endif

/**
 * Global variables
 */
DriveAccess driveAccess(getDeviceStatus, readSector, writeSector, format);
DriveControl driveControl(getFileList, mountFile);
SIOChannel sioChannel(PIN_ATARI_CMD, &SIO_UART, &driveAccess, &driveControl);
File root;
File file; // TODO: make this unnecessary
DiskDrive drive1;
#ifdef SELECTOR_BUTTON
boolean isSwitchPressed = false;
#endif
#ifdef LCD_DISPLAY
LiquidCrystal lcd(PIN_LCD_RD,PIN_LCD_ENABLE,PIN_LCD_DB4,PIN_LCD_DB5,PIN_LCD_DB6,PIN_LCD_DB7);
#endif

void setup() {
#ifdef DEBUG
  // set up logging serial port
  LOGGING_UART.begin(115200);
#endif

  // initialize serial port to Atari
  SIO_UART.begin(19200);

  // set pin modes
  #ifdef SELECTOR_BUTTON
  pinMode(PIN_SELECTOR, INPUT);
  #endif

  #ifdef LCD_DISPLAY
  // set up LCD if appropriate
  lcd.begin(16, 2);
  lcd.print("SIO2Arduino");
  lcd.setCursor(0,1);
  #endif

  // initialize SD card
  LOG_MSG("Initializing SD card...");
  pinMode(PIN_SD_CS, OUTPUT);
  if (!SD.begin(PIN_SD_CS)) {
    LOG_MSG_CR(" failed.");
    #ifdef LCD_DISPLAY
      lcd.print("SD Init Error");
    #endif     
    return;
  }
  root = SD.open("/");
  if (!root) {
    LOG_MSG_CR("Error opening SD card");
    #ifdef LCD_DISPLAY
      lcd.print("SD Open Error");
    #endif     
  } else {
    LOG_MSG_CR(" done.");
    #ifdef LCD_DISPLAY
      lcd.print("READY");
    #endif
  }
}

void loop() {
  // let the SIO channel do its thing
  sioChannel.runCycle();
  
  #ifdef SELECTOR_BUTTON
  // watch the selector button
  if (digitalRead(PIN_SELECTOR) == HIGH && !isSwitchPressed) {
    isSwitchPressed = true;
    changeDisk();
  } else if (digitalRead(PIN_SELECTOR) == LOW && isSwitchPressed) {
    isSwitchPressed = false;
  }
  #endif
}

void SIO_CALLBACK() {
  // inform the SIO channel that an incoming byte is available
  sioChannel.processIncomingByte();
}

DriveStatus* getDeviceStatus(int deviceId) {
  return drive1.getStatus();
}

SectorPacket* readSector(int deviceId, unsigned long sector) {
  if (drive1.hasImage()) {
    return drive1.getSectorData(sector);
  } else {
    return NULL;
  }
}

boolean writeSector(int deviceId, unsigned long sector, byte* data, unsigned long size) {
  return drive1.writeSectorData(sector, data, size);
}

boolean format(int deviceId, int density) {
  // close and delete the current file
  file.close();
  SD.remove(file.name());

  LOG_MSG("Remove old file: ");
  LOG_MSG_CR(file.name());

  // open new file for writing
  file = SD.open(file.name(), FILE_WRITE);

  LOG_MSG("Created new file: ");
  LOG_MSG_CR(file.name());

  // allow the virtual drive to format the image (and possibly alter its size)
  if (drive1.formatImage(&file, density)) {
    // set the new image file for the drive
    drive1.setImageFile(&file);
    return true;
  } else {
    return false;
  }
}

void changeDisk() {
  boolean imageChanged = false;

  while (!imageChanged) {
    if (file) {
      file.close();
    }

    file = root.openNextFile();

    if (!file) {
      root.rewindDirectory();
      file = root.openNextFile();
    }

    file = SD.open(file.name(), FILE_WRITE);
    imageChanged = drive1.setImageFile(&file);
  }
  
  #ifdef LCD_DISPLAY
  lcd.clear();
  lcd.print(file.name());
  #endif
}
