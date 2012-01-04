#ifndef DISK_IMAGE_h
#define DISK_IMAGE_h

#include <Arduino.h>
#include <SD.h>
#include "atari.h"

const byte TYPE_ATR = 1;
const byte TYPE_XFD = 2;
const byte TYPE_PRO = 3;

const unsigned long SECTOR_SIZE_SD  = 128;

const unsigned long FORMAT_SS_SD_35 = 80640;
const unsigned long FORMAT_SS_SD_40 = 92160;
const unsigned long FORMAT_SS_ED_35 = 116480;
const unsigned long FORMAT_SS_ED_40 = 133120;
const unsigned long FORMAT_SS_DD_35 = 160896;
const unsigned long FORMAT_SS_DD_40 = 183936;

// ATR format
#define ATR_SIGNATURE 0x0296
struct ATRHeader {
  unsigned int signature;
  unsigned int pars;
  unsigned int secSize;
  byte parsHigh;
  unsigned long crc;
  unsigned long unused;
  byte flags;
};

// PRO format
struct PROFileHeader {
  byte sectorCountHi;
  byte sectorCountLo;
  byte magic;
  byte imageType;
  byte phantomSectorMode;
  byte sectorReadDelay;
  byte b0;
  byte b1;
  byte b2;
  byte b3;
  byte b4;
  byte b5;
  byte b6;
  byte b7;
  byte b8;
  byte b9;
};
struct PROSectorHeader {
  StatusFrame statusFrame;
  byte b0;
  byte b1;
  byte b2;
  byte b3;
  byte b4;
  byte b5;
  byte b6;
  byte b7;
};

class DiskImage {
public:
  DiskImage();
  boolean setFile(File* file);
  unsigned long getSectorSize();
  SectorPacket* getSectorData(unsigned long sector);
  boolean writeSectorData(unsigned long, byte* data, unsigned long size);
  boolean format(File *file, int density);
  boolean isEnhancedDensity();
  boolean isDoubleDensity();
  boolean hasImage();
private:
  boolean loadFile(File* file);
  File*            m_fileRef;
  byte             m_type;
  unsigned long    m_fileSize;
  unsigned long    m_headerSize;
  unsigned long    m_sectorSize;
  byte             m_sectorReadDelay;
  SectorPacket     m_sectorBuffer;
  PROSectorHeader  m_proSectorHeader;
};

#endif
