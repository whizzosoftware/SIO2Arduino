#ifndef DRIVE_ACCESS_h
#define DRIVE_ACCESS_h

#include <Arduino.h>
#include "atari.h"

class DriveAccess {
public:
  DriveAccess(DriveStatus*(*deviceStatusFunc)(int), SectorDataInfo*(*readSectorFunc)(int,unsigned long,byte*), boolean(*writeSectorFunc)(int,unsigned long,byte*,unsigned long), boolean(*formatFunc)(int,int));
  DriveStatus*      (*deviceStatusFunc)(int);
  SectorDataInfo*   (*readSectorFunc)(int,unsigned long,byte*);
  boolean           (*writeSectorFunc)(int,unsigned long, byte*,unsigned long);
  boolean           (*formatFunc)(int,int);
};

#endif
