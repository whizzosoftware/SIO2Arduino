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
#include <SD.h>
#include "atari.h"
#include "sio_channel.h"
#include "disk_drive.h"
#include "log.h"

#define CMD_PIN     2
#define SD_PIN      4
#define SWITCH_PIN  8

#define STATE_INIT           1
#define STATE_WAIT_CMD_START 2
#define STATE_READ_CMD       3
#define STATE_WAIT_CMD_END   4

// globals
DriveAccess driveAccess(getDeviceStatus, readSector, writeSector, format);
DriveControl driveControl(getFileList, mountFile);
SIOChannel sioChannel(CMD_PIN, &Serial1, &driveAccess, &driveControl);
File root;
File file; // TODO: make this unnecessary
DiskDrive drive1;
boolean isSwitchPressed = false;

void setup() {
  // set up logging serial port
  Serial.begin(115200);

  // initialize serial port to Atari
  Serial1.begin(19200);

  // set pin modes
  pinMode(SWITCH_PIN, INPUT);

  // initialize SD card
  LOG_MSG("Initializing SD card...");
  pinMode(SD_PIN, OUTPUT);
  if (!SD.begin(SD_PIN)) {
    LOG_MSG_CR(" failed.");
    return;
  }
  LOG_MSG_CR(" done.");
  root = SD.open("/");
  if (!root) {
    LOG_MSG_CR("Error opening SD card");
  }
}

void loop() {
  // let the SIO channel do its thing
  sioChannel.runCycle();
  
  // watch the control switch
  if (digitalRead(SWITCH_PIN) == HIGH && !isSwitchPressed) {
    isSwitchPressed = true;
    changeDisk();
  } else if (digitalRead(SWITCH_PIN) == LOW && isSwitchPressed) {
    isSwitchPressed = false;
  }
}

void serialEvent1() {
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
}
