#ifndef SIO_CHANNEL_h
#define SIO_CHANNEL_h

#include <Arduino.h>
#include "atari.h"

const byte COMMAND_FRAME_SIZE   = 5;

const byte STATE_INIT           = 1;
const byte STATE_WAIT_CMD_START = 2;
const byte STATE_READ_CMD       = 3;
const byte STATE_READ_SECTOR    = 4;
const byte STATE_WAIT_CMD_END   = 5;

const byte CMD_FORMAT           = 0x21;
const byte CMD_FORMAT_MD        = 0x22;
const byte CMD_POLL             = 0x3F;
const byte CMD_PUT              = 0x50;
const byte CMD_READ             = 0x52;
const byte CMD_STATUS           = 0x53;
const byte CMD_WRITE            = 0x57;

const byte DEVICE_D1            = 0x31;
const byte DEVICE_D2            = 0x32;
const byte DEVICE_D3            = 0x33;
const byte DEVICE_D4            = 0x34;
const byte DEVICE_D5            = 0x35;
const byte DEVICE_D6            = 0x36;
const byte DEVICE_D7            = 0x37;
const byte DEVICE_D8            = 0x38;
const byte DEVICE_R1            = 0x50;

class SIOChannel {
public:
  SIOChannel(int cmdPin, Stream* stream, DriveStatus*(*deviceStatusFunc)(int), SectorPacket*(*readSectorFunc)(int,unsigned long), boolean(*writeSectorFunc)(int,unsigned long,byte*,unsigned long), boolean(*formatFunc)(int,int));
  void runCycle();
  void processIncomingByte();
  void sendDeviceStatus(DriveStatus *deviceStatus);
  byte* readSectorDataFrame();
private:
  boolean isChecksumValid();
  boolean isCommandForThisDevice();
  boolean isValidDevice(byte b);
  boolean isValidCommand();
  boolean isValidAuxData();
  byte checksum(byte* chunk, int size);
  void processCommand();
  void dumpCommandFrame();
  void cmdGetSector(int deviceId);
  void cmdPutSector(int deviceId);
  void cmdPutSectorWithVerify(int deviceId);
  void cmdGetStatus(int deviceId);
  void cmdFormat(int deviceId, int density);
  unsigned long getCommandSector();
  void doPutSector();

  int               m_cmdPin;
  Stream*           m_stream;
  int               m_cmdPinState;
  CommandFrame      m_cmdFrame;
  byte*             m_cmdFramePtr;
  byte*             m_putSectorBuffer;
  byte*             m_putSectorBufferPtr;
  int               m_putBytesRemaining;
  boolean           m_dataFrameReadInProgress;
  DriveStatus*      (*m_deviceStatusFunc)(int);
  SectorPacket*     (*m_readSectorFunc)(int,unsigned long);
  boolean           (*m_writeSectorFunc)(int,unsigned long, byte*,unsigned long);
  boolean           (*m_formatFunc)(int,int);
};

#endif
