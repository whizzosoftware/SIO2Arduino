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

DriveStatus* DiskDrive::getStatus() {
  // reset device status
  memset(&m_driveStatus.statusFrame, 0, sizeof(m_driveStatus.statusFrame));

  // set appropriate bits
  m_driveStatus.statusFrame.hardwareStatus.writeProtected = 1;
  m_driveStatus.statusFrame.commandStatus.motorStatus = 1;
  m_driveStatus.statusFrame.commandStatus.enhancedDensity = m_diskImage.isEnhancedDensity() ? 1 : 0;
  m_driveStatus.statusFrame.commandStatus.doubleDensity = m_diskImage.isDoubleDensity() ? 1 : 0;;
  m_driveStatus.statusFrame.timeout_lsb = 128;

  return &m_driveStatus;
}

boolean DiskDrive::setImageFile(File *file) {
  boolean result = m_diskImage.setFile(file);
  if (result) {
    m_driveStatus.sectorSize = m_diskImage.getSectorSize();
  }
  return result;
}

SectorPacket* DiskDrive::getSectorData(unsigned long sector) {
  return m_diskImage.getSectorData(sector);
}

boolean DiskDrive::writeSectorData(unsigned long sector, byte *data, unsigned long len) {
  return m_diskImage.writeSectorData(sector, data, len);
}

boolean DiskDrive::formatImage(File *file, int density) {
  return m_diskImage.format(file, density);
}
