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
  // seek to proper offset in file
  unsigned long offset = m_headerSize + ((sector - 1) * m_sectorSize);
  m_fileRef->seek(offset);

  m_sectorBuffer.sectorSize = m_sectorSize;

  // write sector data
  for (int i=0; i < m_sectorSize; i++) {
    m_sectorBuffer.sectorData[i] = (byte)m_fileRef->read();
  }

  return &m_sectorBuffer;
}

/**
 * Write data to drive image.
 */
boolean DiskImage::writeSectorData(unsigned long sector, byte* data, unsigned long len) {
  // seek to proper offset in file
  unsigned long offset = m_headerSize + ((sector - 1) * m_sectorSize);
  m_fileRef->seek(offset);

  // write the data
  unsigned long wrote = m_fileRef->write(data, len);
  m_fileRef->flush();
  return (wrote == len);
}

/**
 * Format drive image.
 */
boolean DiskImage::format(File *file, int density) {
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
    header.secSize = 128;
    file->write((byte*)&header, sizeof(header));
  }

  // create empty byte buffer
  for (unsigned long i=0; i < length; i++) {
    file->write((byte)0);
  }
  file->flush();

  return true;
}

boolean DiskImage::loadFile(File *file) {
  // first, we'll check if it's an ATR since that is a structured format
  // (we examine the actual file header in case the file is mis-named)
  
  // read file header (first 16 bytes)
  file->seek(0);
  ATRHeader atrHeader;
  byte* b = (byte*)&atrHeader;
  for (int i=0; i < sizeof(atrHeader); i++) {
    *b = (byte)file->read();
    b++;
  }
  file->seek(0);
  
  // check for a valid ATR header
  if (atrHeader.signature == ATR_SIGNATURE) {
    m_type = TYPE_ATR;
    m_headerSize = 16;
    m_sectorSize = atrHeader.secSize;
    
    LOG_MSG("Loaded an ATR with sector size ");
    LOG_MSG(atrHeader.secSize);
    LOG_MSG(": ");
    
    return true;
  }

  // if it wasn't an ATR, check if it's an XFD
  // (since an XFD is just a raw data dump, we can only determine this by file name and size)
  char *filename = file->name();
  int len = strlen(filename);
  char *extension = filename + len - 4;
  if ((!strcmp(".XFD", extension) || !strcmp(".xfd", extension)) && (m_fileSize == FORMAT_SS_SD_40)) {
    m_type = TYPE_XFD;
    m_headerSize = 0;
    m_sectorSize = 128;

    LOG_MSG("Loaded an XFD with sector size 128: ");

    return true;
  }
  
  return false;
}

boolean DiskImage::isEnhancedDensity() {
  return (m_fileSize == FORMAT_SS_ED_35 + m_headerSize || m_fileSize == FORMAT_SS_ED_40 + m_headerSize);
}

boolean DiskImage::isDoubleDensity() {
  return (m_fileSize == FORMAT_SS_DD_35 + m_headerSize || m_fileSize == FORMAT_SS_DD_40 + m_headerSize);
}
