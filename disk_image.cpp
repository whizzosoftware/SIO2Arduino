/*
 * disk_image.cpp - Handles disk images in various formats.
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
#include "disk_image.h"
#include "log.h"

DiskImage::DiskImage() {
  m_fileRef = NULL;
}

boolean DiskImage::setFile(File* file) {
  m_fileRef = file;
  m_fileSize = file->size();

  // free up previous sector buffer if we had one
  if (m_sectorBuffer.sectorData) {
    free(m_sectorBuffer.sectorData);
    m_sectorBuffer.sectorData = NULL;
  }
  
  // if image is valid...
  if (loadFile(file)) {
    // create new sector buffer
    m_sectorBuffer.sectorData = (byte*)malloc(sizeof(byte) * m_sectorSize);
    LOG_MSG_CR(file->name());
    return true;
  } else {
    m_fileRef = NULL;
  }
  
  return false;
}

unsigned long DiskImage::getSectorSize() {
  return m_sectorSize;
}

/**
 * Read data from drive image.
 */
SectorPacket* DiskImage::getSectorData(unsigned long sector) {
  m_sectorBuffer.sectorSize = m_sectorSize;
  m_sectorBuffer.error = false;
  m_sectorBuffer.validStatusFrame = false;

  // seek to proper offset in file
  if (m_type == TYPE_PRO) {
    // if this is a PRO image, we seek based on the sector number + the sector header size (omitting the header)
    m_fileRef->seek(m_headerSize + ((sector - 1) * (m_sectorSize + sizeof(m_proSectorHeader))));

    // then we read the sector header
    for (int i=0; i < sizeof(m_proSectorHeader); i++) {
      ((byte*)&m_proSectorHeader)[i] = (byte)m_fileRef->read();
    }

    // return the status frame so the drive can return it on a subsequent status command
    memcpy(&m_sectorBuffer.statusFrame, &m_proSectorHeader, sizeof(m_sectorBuffer.statusFrame));
    m_sectorBuffer.validStatusFrame = true;
    
    // if the header shows there was an error in this sector, return an error
    if (!m_proSectorHeader.statusFrame.hardwareStatus.crcError || !m_proSectorHeader.statusFrame.hardwareStatus.dataLostOrTrack0 || !m_proSectorHeader.statusFrame.hardwareStatus.recordNotFound) {
      m_sectorBuffer.error = true;
    } else {
      // if there are phantom sector(s) associated with this sector, decide what to return
      if (m_usePhantoms && m_proSectorHeader.totalPhantoms > 0 && m_phantomFlip) {
        m_fileRef->seek(m_headerSize + (((720 + m_proSectorHeader.phantom1) - 1) * (m_sectorSize + sizeof(PROSectorHeader))) + sizeof(PROSectorHeader));
      }
    }
    m_phantomFlip = !m_phantomFlip; // TODO: do bad sectors cause this to flip?
  } else {
    // if this is an ATR image, we seek based on the sector number (omitting the header)
    m_fileRef->seek(m_headerSize + ((sector - 1) * m_sectorSize));
  }

  // delay if necessary
  if (m_sectorReadDelay) {
    delay(m_sectorReadDelay);
  }

  // read sector data into buffer
  for (int i=0; i < m_sectorSize; i++) {
    m_sectorBuffer.sectorData[i] = (byte)m_fileRef->read();
  }

  return &m_sectorBuffer;
}

/**
 * Write data to drive image.
 */
boolean DiskImage::writeSectorData(unsigned long sector, byte* data, unsigned long len) {
  if (!m_readOnly) {
    // seek to proper offset in file
    unsigned long offset = m_headerSize + ((sector - 1) * m_sectorSize);
    m_fileRef->seek(offset);
  
    // write the data
    unsigned long wrote = m_fileRef->write(data, len);
    m_fileRef->flush();
    return (wrote == len);
  }
  
  return false;
}

/**
 * Format drive image.
 */
boolean DiskImage::format(File *file, int density) {
  if (!m_readOnly) {
    // determine file length
    unsigned long length = FORMAT_SS_SD_40;
    if (density == DENSITY_ED) {
      length = FORMAT_SS_ED_40;
    }
  
    // make sure we're at beginning of file
    file->seek(0);
  
    // if disk is an ATR, write the header
    if (m_type == TYPE_ATR) {
      ATRHeader header;
      memset(&header, 0, sizeof(header));
      header.signature = ATR_SIGNATURE;
      header.pars = length / 0x10;
      header.secSize = SECTOR_SIZE_SD;
      file->write((byte*)&header, sizeof(header));
    }
  
    // create empty byte buffer
    for (unsigned long i=0; i < length; i++) {
      file->write((byte)0);
    }
    file->flush();
  
    return true;
  }
  
  return false;
}

boolean DiskImage::loadFile(File *file) {
  // make sure we're at the beginning of file
  file->seek(0);
  
  // read first 16 bytes of file & rewind again
  byte header[16];
  for (int i=0; i < 16; i++) {
    header[i] = (byte)file->read();
  }
  file->seek(0);
  
  // check if it's an ATR
  ATRHeader* atrHeader = (ATRHeader*)&header;
  if (atrHeader->signature == ATR_SIGNATURE) {
    m_type = TYPE_ATR;
    m_headerSize = 16;
    m_readOnly = false;
    m_sectorSize = atrHeader->secSize;
    m_sectorReadDelay = 0;
    
    LOG_MSG("Loaded ATR with sector size ");
    LOG_MSG(atrHeader->secSize);
    LOG_MSG(": ");
    
    return true;
  }

  // check if it's an APE PRO image
  PROFileHeader* proHeader = (PROFileHeader*)&header;
  if (proHeader->sectorCountHi * 256 + proHeader->sectorCountLo == ((m_fileSize-16)/(SECTOR_SIZE_SD+sizeof(m_proSectorHeader))) && proHeader->magic == 'P') {
    m_type = TYPE_PRO;
    m_readOnly = true;
    m_headerSize = 16;
    m_sectorSize = SECTOR_SIZE_SD;
    m_sectorReadDelay = proHeader->sectorReadDelay * (1000/60);
    
    // set the phantom emulation mode
    switch (proHeader->phantomSectorMode) {
      case PSM_SIMPLE:
      case PSM_MINDSCAPE_SPECIAL:
      case PSM_STICKY:
      case PSM_SHIMMERING:
      case PSM_REVERSE_SHIMMER:
        m_usePhantoms = false;
        break;
      case PSM_GLOBAL_FLIP_FLOP:
        m_usePhantoms = true;
        m_phantomFlip = false;
        break;
      case PSM_GLOBAL_FLOP_FLIP:
        m_usePhantoms = true;
        m_phantomFlip = true;
        break;
    }

    LOG_MSG("Loaded PRO with sector size 128: ");

    return true;
  }

  // check if it's an XFD
  // (since an XFD is just a raw data dump, we can only determine this by file name and size)
  char *filename = file->name();
  int len = strlen(filename);
  char *extension = filename + len - 4;
  if ((!strcmp(".XFD", extension) || !strcmp(".xfd", extension)) && (m_fileSize == FORMAT_SS_SD_40)) {
    m_type = TYPE_XFD;
    m_readOnly = false;
    m_headerSize = 0;
    m_sectorSize = SECTOR_SIZE_SD;
    m_sectorReadDelay = 0;

    LOG_MSG("Loaded XFD with sector size 128: ");

    return true;
  }
  
  return false;
}

boolean DiskImage::hasImage() {
  return (m_fileRef != NULL);
}

boolean DiskImage::isEnhancedDensity() {
  return (m_fileSize == FORMAT_SS_ED_35 + m_headerSize || m_fileSize == FORMAT_SS_ED_40 + m_headerSize);
}

boolean DiskImage::isDoubleDensity() {
  return (m_fileSize == FORMAT_SS_DD_35 + m_headerSize || m_fileSize == FORMAT_SS_DD_40 + m_headerSize);
}

boolean DiskImage::isReadOnly() {
  return m_readOnly;
}
