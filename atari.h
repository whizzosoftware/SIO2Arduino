#ifndef ATARI_H
#define ATARI_H

const byte DELAY_T2 = 5;
const byte DELAY_T3 = 2;
const byte DELAY_T4 = 1;
const byte DELAY_T5 = 1;

const byte ACK      = 0x41;
const byte NAK      = 0x4E;
const byte COMPLETE = 0x43;
const byte ERR      = 0x45;

const byte DENSITY_SD = 1;
const byte DENSITY_ED = 2;
const byte DENSITY_DD = 3;

const unsigned long SD_SECTOR_SIZE  = 128;
const unsigned long DD_SECTOR_SIZE  = 256;
const unsigned long MAX_SECTOR_SIZE = 1024;

struct CommandFrame {
  byte deviceId;
  byte command;
  byte aux1;
  byte aux2;
  byte checksum;
};

struct HardwareStatus {
  byte controllerBusy:1;
  byte dataRequestOrIndex:1;
  byte dataLostOrTrack0:1;
  byte crcError:1;
  byte recordNotFound:1;
  byte recordType:1;
  byte writeProtected:1;
  byte notReady:1;
};

struct CommandStatus {
  byte invalidCommandFrame:1;
  byte invalidDataFrame:1;
  byte writeFailure:1;
  byte writeProtect:1;
  byte motorStatus:1;
  byte doubleDensity:1;
  byte unused:1;
  byte enhancedDensity:1;  
};

struct StatusFrame {
  CommandStatus  commandStatus;
  HardwareStatus hardwareStatus;
  byte           timeout_lsb;
  byte           timeout_msb;
};

struct DriveStatus {
  unsigned long sectorSize;
  StatusFrame   statusFrame;
};

struct SectorPacket {
  unsigned long sectorSize;
  byte* sectorData;
};

#endif
