#ifndef DISK_IMAGE_h
#define DISK_IMAGE_h

#include <Arduino.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include "atari.h"
#include "config.h"

#define TYPE_ATR 1
#define TYPE_XFD 2
#ifdef PRO_IMAGES
#define TYPE_PRO 3
#endif
#ifdef ATX_IMAGES
#define TYPE_ATX 4
#endif
#ifdef XEX_IMAGES
#define TYPE_XEX 5
#endif

#define SECTOR_SIZE_SD  128
#define FORMAT_SS_SD_40 92160

#ifdef ATX_IMAGES
struct ATXSectorHeader {
  unsigned int sectorNumber;
  unsigned long fileIndex;
  byte sstatus;
};
#endif

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

#ifdef PRO_IMAGES
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
#endif

class DiskImage {
public:
  DiskImage();
  boolean setFile(SdFile* file);
  byte getType();
  unsigned long getSectorSize();
  SectorDataInfo* getSectorData(unsigned long sector, byte* data);
  unsigned long writeSectorData(unsigned long, byte* data, unsigned long size);
  boolean format(SdFile *file, int density);
  boolean isEnhancedDensity();
  boolean isDoubleDensity();
  boolean isReadOnly();
  boolean hasImage();
  boolean hasCopyProtection();
private:
  boolean loadFile(SdFile* file);
  SdFile*          m_fileRef;
  byte             m_type;
  unsigned long    m_fileSize;
  boolean          m_readOnly;
  unsigned long    m_headerSize;
  unsigned long    m_sectorSize;
  byte             m_sectorReadDelay;
  SectorDataInfo   m_sectorInfo;
  boolean          m_usePhantoms;
  boolean          m_phantomFlip;
#ifdef PRO_IMAGES
  PROSectorHeader  m_proSectorHeader;
#endif  
#ifdef ATX_IMAGES
  ATXSectorHeader  m_sectorHeaders[720];
#endif
};

#endif
