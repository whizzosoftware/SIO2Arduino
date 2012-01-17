#ifndef SDRIVE_HANDLER_h
#define SDRIVE_HANDLER_h

#include <Arduino.h>
#include "atari.h"
#include "drive_control.h"

const byte DEVICE_SDRIVE           = 0x71;

const byte CMD_SDRIVE_GET20        = 0xC0;
const byte CMD_SDRIVE_IDENT        = 0xE0;
const byte CMD_SDRIVE_INIT         = 0xE1;
const byte CMD_SDRIVE_CHDIR        = 0xE3;
const byte CMD_SDRIVE_GET_ENTRIES  = 0xEB;
const byte CMD_SDRIVE_SWAP_VDN     = 0xEE;
const byte CMD_SDRIVE_GETPARAMS    = 0xEF;
const byte CMD_SDRIVE_MOUNT_D0     = 0xF0;
const byte CMD_SDRIVE_MOUNT_D1     = 0xF1;
const byte CMD_SDRIVE_MOUNT_D2     = 0xF2;
const byte CMD_SDRIVE_MOUNT_D3     = 0xF3;
const byte CMD_SDRIVE_MOUNT_D4     = 0xF4;
const byte CMD_SDRIVE_CHROOT       = 0xFE;

class SDriveHandler {
public:
  SDriveHandler();
  void setDriveControl(DriveControl* driveControl);
  boolean printCmdName(byte cmd);
  boolean isValidCommand(byte cmd);
  boolean isValidDevice(byte device);
  void processCommand(CommandFrame* cmdFrame, Stream* stream);
  void cmdIdent(Stream* stream);
  void cmdInit(Stream* stream);
  void cmdChroot(Stream* stream);
  void cmdSwapVdn(Stream* stream);
  void cmdGetParams(Stream* stream);
  void cmdGetEntries(byte n, Stream* stream);
  void cmdChdir(Stream *stream);
  void cmdGet20(Stream *stream);
  void cmdMountDrive(byte driveNum, byte index, Stream* stream);
  
  DriveControl* m_driveControl;
};

#endif
