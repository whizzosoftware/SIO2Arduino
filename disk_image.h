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
const byte PSM_SIMPLE            = 0;
const byte PSM_MINDSCAPE_SPECIAL = 1;
const byte PSM_GLOBAL_FLIP_FLOP  = 2;
const byte PSM_GLOBAL_FLOP_FLIP  = 3;
const byte PSM_HEURISTIC         = 4;
const byte PSM_STICKY            = 5;
const byte PSM_REVERSE_STICKY    = 6;
const byte PSM_SHIMMERING        = 7;
const byte PSM_REVERSE_SHIMMER   = 8;
const byte PSM_ROLLING_THUNDER   = 9;

struct PROFileHeader {
  byte sectorCountHi;
  byte sectorCountLo;
  byte magic;
  byte imageType;
  byte phantomSectorMode;
  byte sectorReadDelay;
  byte g;
  byte h;
  byte i;
  byte j;
  byte k;
  byte l;
  byte m;
  byte n;
  byte o;
  byte p;
};
struct PROSectorHeader {
  StatusFrame statusFrame;
  byte m;
  byte totalPhantoms;
  byte phantom4;
  byte phantom1;
  byte phantom2;
  byte phantom3;
  byte n;
  byte phantom5;
};

class DiskImage {
public:
  DiskImage();
  boolean setFile(File* file);
  byte getType();
  unsigned long getSectorSize();
  SectorPacket* getSectorData(unsigned long sector);
  boolean writeSectorData(unsigned long, byte* data, unsigned long size);
  boolean format(File *file, int density);
  boolean isEnhancedDensity();
  boolean isDoubleDensity();
  boolean isReadOnly();
  boolean hasImage();
private:
  boolean loadFile(File* file);
  File*            m_fileRef;
  byte             m_type;
  unsigned long    m_fileSize;
  boolean          m_readOnly;
  unsigned long    m_headerSize;
  unsigned long    m_sectorSize;
  byte             m_sectorReadDelay;
  SectorPacket     m_sectorBuffer;
  // PRO specific fields
  PROSectorHeader  m_proSectorHeader;
  boolean          m_usePhantoms;
  boolean          m_phantomFlip;
};

#endif
