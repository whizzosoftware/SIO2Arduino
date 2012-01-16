#ifndef DRIVE_H
#define DRIVE_H

#include <Arduino.h>
#include "atari.h"
#include "disk_image.h"

const unsigned long MIN_PRO_SECTOR_READ = 25000 - DELAY_T5 * 1000;

class DiskDrive {
public:
  DiskDrive();
  DriveStatus* getStatus();
  boolean setImageFile(SdFile* file);
  unsigned long getImageSectorSize();
  SectorPacket* getSectorData(unsigned long sector);
  unsigned long writeSectorData(unsigned long sector, byte* data, unsigned long len);
  boolean formatImage(SdFile* file, int density);
  boolean hasImage();
private:
  DriveStatus  m_driveStatus;
  DiskImage    m_diskImage;
};

#endif
