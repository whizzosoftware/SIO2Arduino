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
#include <SdFat.h>
#include <SdFatUtil.h>
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
DriveControl driveControl(getFileList, mountFileIndex);
SIOChannel sioChannel(PIN_ATARI_CMD, &SIO_UART, &driveAccess, &driveControl);
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file; // TODO: make this unnecessary
DiskDrive drive1;
#ifdef SELECTOR_BUTTON
boolean isSwitchPressed = false;
unsigned long lastSelectionPress;
boolean isFileOpened;
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

  if (!card.init(SPI_HALF_SPEED, PIN_SD_CS)) {
    LOG_MSG_CR(" failed.");
    #ifdef LCD_DISPLAY
      lcd.print("SD Init Error");
    #endif     
    return;
  }
  
  if (!volume.init(&card)) {
    LOG_MSG_CR(" failed.");
    #ifdef LCD_DISPLAY
      lcd.print("SD Volume Error");
    #endif     
    return;
  }

  if (!root.openRoot(&volume)) {
    LOG_MSG_CR(" failed.");
    #ifdef LCD_DISPLAY
      lcd.print("SD Root Error");
    #endif     
    return;
  }

  LOG_MSG_CR(" done.");
  #ifdef LCD_DISPLAY
    lcd.print("READY");
  #endif
}

void loop() {
  // let the SIO channel do its thing
  sioChannel.runCycle();
  
  #ifdef SELECTOR_BUTTON
  // watch the selector button (accounting for debounce)
  if (digitalRead(PIN_SELECTOR) == HIGH && millis() - lastSelectionPress > 250) {
    lastSelectionPress = millis();
    changeDisk();
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

boolean writeSector(int deviceId, unsigned long sector, byte* data, unsigned long length) {
  return (drive1.writeSectorData(sector, data, length) == length);
}

boolean format(int deviceId, int density) {
  char name[13];
  
  // get current filename
  file.getFilename(name);

  // close and delete the current file
  file.close();
  file.remove();

  LOG_MSG("Remove old file: ");
  LOG_MSG_CR(name);

  // open new file for writing
  file.open(&root, name, O_RDWR | O_SYNC | O_CREAT);

  LOG_MSG("Created new file: ");
  LOG_MSG_CR(name);

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
  dir_t dir;
  char name[13];
  boolean imageChanged = false;

  while (!imageChanged) {
    // get next dir entry
    int8_t result = root.readDir((dir_t*)&dir);
    
    // if we got back a 0, rewind the directory and get the first dir entry
    if (!result) {
      root.rewind();
      result = root.readDir((dir_t*)&dir);
    }
    
    // if we have a valid file response code, open it
    if (result > 0 && isValidFilename((char*)&dir.name)) {
      createFilename(name, (char*)dir.name);
      imageChanged = mountFilename(name);
    }
  }
}

boolean isValidFilename(char *s) {
  return (  s[0] != '.' &&    // ignore hidden directories 
            s[0] != '_' && (  // ignore bogus files created by OS X
              (s[8] == 'A' && s[9] == 'T' && s[10] == 'R') || 
              (s[8] == 'P' && s[9] == 'R' && s[10] == 'O') || 
              (s[8] == 'X' && s[9] == 'F' && s[10] == 'D')
#ifdef ATX_IMAGES              
              || (s[8] == 'A' && s[9] == 'T' && s[10] == 'X')
#endif              
            )
         );
}

void createFilename(char* filename, char* name) {
  for (int i=0; i < 8; i++) {
    if (name[i] != ' ') {
      *(filename++) = name[i];
    }
  }
  *(filename++) = '.';
  *(filename++) = name[8];
  *(filename++) = name[9];
  *(filename++) = name[10];
  *(filename++) = '\0';
}

void getFileList(int page, int count, FileEntry *entries) {
  dir_t dir;
  
  root.rewind();

  int ix = 0;
  while (ix < count) {
    if (root.readDir((dir_t*)&dir) < 1) {
      break;
    }
    if (isValidFilename((char*)&dir.name)) {
      memcpy(entries[ix++].name, dir.name, 11);
    }
  }
}

void mountFileIndex(int deviceId, int ix, int count) {
  FileEntry entries[count];
  char name[13];

  // figure out what filename is associated with the index
  getFileList(ix, count, entries);

  // build a full 8.3 filename
  createFilename(name, entries[ix].name);

  // mount the image
  mountFilename(name);
}

boolean mountFilename(char *name) {
  // close previously open file
  if (file.isOpen()) {
    file.close();
  }
  
  if (file.open(&root, name, O_RDWR | O_SYNC) && drive1.setImageFile(&file)) {
    LOG_MSG_CR(name);

    #ifdef LCD_DISPLAY
    lcd.clear();
    lcd.print(name);
    lcd.setCursor(0,1);
    #endif

    return true;
  }
  
  return false;
}
