#ifndef DRIVE_ACCESS_h
#define DRIVE_ACCESS_h

#include <Arduino.h>
#include "atari.h"

class DriveAccess {
public:
  DriveAccess(DriveStatus*(*deviceStatusFunc)(int), SectorPacket*(*readSectorFunc)(int,unsigned long), boolean(*writeSectorFunc)(int,unsigned long,byte*,unsigned long), boolean(*formatFunc)(int,int));
  DriveStatus*      (*deviceStatusFunc)(int);
  SectorPacket*     (*readSectorFunc)(int,unsigned long);
  boolean           (*writeSectorFunc)(int,unsigned long, byte*,unsigned long);
  boolean           (*formatFunc)(int,int);
};

#endif
