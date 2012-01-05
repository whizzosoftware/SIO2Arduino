#ifndef DRIVE_H
#define DRIVE_H

#include <Arduino.h>
#include "atari.h"
#include "disk_image.h"

class DiskDrive {
public:
  DiskDrive();
  DriveStatus* getStatus();
  boolean setImageFile(File* file);
  unsigned long getImageSectorSize();
  SectorPacket* getSectorData(unsigned long sector);
  boolean writeSectorData(unsigned long sector, byte* data, unsigned long len);
  boolean formatImage(File* file, int density);
  boolean hasImage();
private:
  DriveStatus  m_driveStatus;
  DiskImage    m_diskImage;
};

#endif
