/*
 * disk_drive.cpp - Virtual disk drive class.
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
#include "disk_drive.h"

DiskDrive::DiskDrive() {
  // reset device status
  memset(&m_driveStatus.statusFrame, 0, sizeof(m_driveStatus.statusFrame));

  // set standard attributes
  m_driveStatus.statusFrame.timeout_lsb = 0xE0;
}

DriveStatus* DiskDrive::getStatus() {
  m_driveStatus.statusFrame.commandStatus.writeProtect = m_diskImage.isReadOnly() ? 0x01 : 0x00;
  return &m_driveStatus;
}

boolean DiskDrive::setImageFile(SdFile *file) {
  boolean result = m_diskImage.setFile(file);
  if (result) {
    // set device status
    memset(&m_driveStatus.statusFrame, 0, sizeof(m_driveStatus.statusFrame));
    m_driveStatus.statusFrame.commandStatus.enhancedDensity = m_diskImage.isEnhancedDensity() ? 0x01 : 0x00;
    m_driveStatus.statusFrame.commandStatus.doubleDensity = m_diskImage.isDoubleDensity() ? 0x01 : 0x00;
    m_driveStatus.statusFrame.hardwareStatus.writeProtect = m_diskImage.isReadOnly() ? 0x00 : 0x01;
    m_driveStatus.sectorSize = m_diskImage.getSectorSize();
  }
  return result;
}

SectorPacket* DiskDrive::getSectorData(unsigned long sector) {
  if (m_diskImage.hasImage()) {
    unsigned long startTime = micros();
    
    SectorPacket *packet = m_diskImage.getSectorData(sector);
    // store the status frame if valid
    if (packet->validStatusFrame) {
      memcpy(&m_driveStatus.statusFrame, &(packet->statusFrame), sizeof(m_driveStatus.statusFrame));
    }
    
    // for PRO images, make read time consistent across all sector reads to 
    // allow skew-based protection to work
    if (m_diskImage.getType() == TYPE_PRO) {
      unsigned long time = micros() - startTime;
      unsigned long delta = (MIN_PRO_SECTOR_READ * ((time - 1) / MIN_PRO_SECTOR_READ + 1)) - time;
      delay(delta / 1000);             // Arduino's delayMicroseconds() can't reliably 
      delayMicroseconds(delta % 1000); // handle an argument larger than 16,383
    }
    
    return packet;
  } else {
    return NULL;
  }
}

unsigned long DiskDrive::writeSectorData(unsigned long sector, byte *data, unsigned long len) {
  return m_diskImage.writeSectorData(sector, data, len);
}

boolean DiskDrive::formatImage(SdFile *file, int density) {
  return m_diskImage.format(file, density);
}

boolean DiskDrive::hasImage() {
  return m_diskImage.hasImage();
}
