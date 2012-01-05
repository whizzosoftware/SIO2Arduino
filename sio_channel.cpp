/*
 * sio_channel.cpp - Handles communication with the Atari SIO bus.
 *
 * Copyright (c) 2012 Whizzo Software LLC (Daniel Noguerol)
 *
 * This file is part of the SIO2Arduino project which emulates
 * Atari 8-bit SIO devices on Arduino hardware.
 *
 * SIO2Arduino is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SIO2Arduino is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SIO2Arduino; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "sio_channel.h"
#include "log.h"

SIOChannel::SIOChannel(int cmdPin, Stream* stream, DriveStatus*(*deviceStatusFunc)(int), SectorPacket*(*readSectorFunc)(int,unsigned long), boolean(*writeSectorFunc)(int,unsigned long,byte*,unsigned long), boolean(*formatFunc)(int,int)) {
  m_cmdPin = cmdPin;
  m_stream = stream;
  m_deviceStatusFunc = deviceStatusFunc;
  m_readSectorFunc = readSectorFunc;
  m_writeSectorFunc = writeSectorFunc;
  m_formatFunc = formatFunc;
  m_dataFrameReadInProgress = false;
  m_putSectorBuffer = (byte*)malloc(sizeof(byte) * MAX_SECTOR_SIZE);

  // set command pin to be read
  pinMode(m_cmdPin, INPUT);

  m_cmdPinState = STATE_INIT;
}

void SIOChannel::runCycle() {
    // watch the Atari command line
    switch (m_cmdPinState) {
      case STATE_INIT:
        if (digitalRead(m_cmdPin) == HIGH) {
          m_cmdPinState = STATE_WAIT_CMD_START;
        }
        break;
      case STATE_WAIT_CMD_START:
        if (digitalRead(m_cmdPin) == LOW) {
          m_cmdPinState = STATE_READ_CMD;
          // reset last command frame info
          memset(&m_cmdFrame, 0, sizeof(m_cmdFrame));
          m_cmdFramePtr = (byte*)&m_cmdFrame;
          m_dataFrameReadInProgress = false;
        }
        break;
      case STATE_READ_CMD:
        // if command frame is complete...
        if (m_cmdFramePtr - (byte*)&m_cmdFrame == COMMAND_FRAME_SIZE) {
          dumpCommandFrame();
          // process command frame
          if (isChecksumValid() && isCommandForThisDevice()) {
            if (isValidCommand() && isValidAuxData()) {
              processCommand();
            } else {
              m_stream->write(NAK);
            }
          }
          m_cmdPinState = STATE_WAIT_CMD_END;
        }
        break;
      case STATE_WAIT_CMD_END:
        if (digitalRead(m_cmdPin) == HIGH) {
          m_cmdPinState = STATE_WAIT_CMD_START;
        }
        break;      
    }
}

void SIOChannel::processIncomingByte() {
  // read the next byte from the bus
  byte b = m_stream->read();

  // if we're waiting for a command, read the data into the command frame
  if (m_cmdPinState == STATE_READ_CMD) {
    int idx = (int)m_cmdFramePtr - (int)&m_cmdFrame;
    // sometimes we see extra bytes on the bus while reading a command and things get lost --
    // the isValidDevice() check prevents a command frame read from getting corrupted by them
    if (idx < COMMAND_FRAME_SIZE && (idx > 0 || (idx == 0 && isValidDevice(b)))) {
      *m_cmdFramePtr = b;
      m_cmdFramePtr++;
      return;
    }
  } else if (m_dataFrameReadInProgress) {
    *m_putSectorBufferPtr = b;
    m_putSectorBufferPtr++;
    m_putBytesRemaining--;
    if (m_putBytesRemaining == 0) {
      doPutSector();
    }
    return;
  }
  // otherwise, ignore it
  LOG_MSG("Ignoring: ");
  LOG_MSG(b, HEX);
  LOG_MSG_CR();
}

boolean SIOChannel::isChecksumValid() {
  byte chkSum = checksum((byte*)&m_cmdFrame, 4);
  if (chkSum != m_cmdFrame.checksum) {
    LOG_MSG("Checksum failed. Calculated: ");
    LOG_MSG(chkSum);
    LOG_MSG("; received: ");
    LOG_MSG_CR(m_cmdFrame.checksum);

    return false;
  } else {
    return true;
  }
}

boolean SIOChannel::isCommandForThisDevice() {
  // we only emulate drive 1 right now
  return (m_cmdFrame.deviceId == DEVICE_D1);
}

boolean SIOChannel::isValidCommand() {
  return (m_cmdFrame.command == CMD_READ || 
          m_cmdFrame.command == CMD_WRITE ||
          m_cmdFrame.command == CMD_STATUS ||
          m_cmdFrame.command == CMD_PUT ||
          m_cmdFrame.command == CMD_FORMAT ||
          m_cmdFrame.command == CMD_FORMAT_MD);
}

boolean SIOChannel::isValidDevice(byte b) {
  return (b == DEVICE_D1 ||
          b == DEVICE_D2 ||
          b == DEVICE_D3 ||
          b == DEVICE_D4 ||
          b == DEVICE_D5 ||
          b == DEVICE_D6 ||
          b == DEVICE_D7 ||
          b == DEVICE_D8 ||
          b == DEVICE_R1);
}

boolean SIOChannel::isValidAuxData() {
  return true;
}

byte SIOChannel::checksum(byte* chunk, int size) {
  int chkSum = 0;
  for(int i=0; i < size; i++) {
    chkSum = ((chkSum+chunk[i])>>8) + ((chkSum+chunk[i])&0xff);
  }
  return (byte)chkSum;
}

void SIOChannel::processCommand() {
  int deviceId = 1;
  
  switch (m_cmdFrame.command) {
    case CMD_READ:
      cmdGetSector(deviceId);
      break;
    case CMD_WRITE:
    case CMD_PUT:
      cmdPutSector(deviceId);
      break;
    case CMD_STATUS:
      cmdGetStatus(deviceId);
      break;
    case CMD_FORMAT:
      cmdFormat(deviceId, DENSITY_SD);
      break;
    case CMD_FORMAT_MD:
      cmdFormat(deviceId, DENSITY_ED);
      break;
  }
}

void SIOChannel::cmdGetSector(int deviceId) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  // write data frame + checksum
  SectorPacket *p = m_readSectorFunc(deviceId, getCommandSector());
  if (p != NULL && !p->error) {
    // send complete 
    delay(DELAY_T5);
    m_stream->write(COMPLETE);
  } else {
    // send error
    delay(95);
    m_stream->write(ERR);
  }

  delayMicroseconds(700);

  // write data
  byte *b = p->sectorData;
  for (int i=0; i < p->sectorSize; i++) {
    m_stream->write(*b);
    b++;
  }
  
  // write checksum
  m_stream->write(checksum(p->sectorData, p->sectorSize));
  m_stream->flush();
}

void SIOChannel::cmdPutSector(int deviceId) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  DriveStatus *status = m_deviceStatusFunc(deviceId);
  m_putBytesRemaining = status->sectorSize + 1;
  m_putSectorBufferPtr = m_putSectorBuffer;
  m_dataFrameReadInProgress = true;
}
  
void SIOChannel::doPutSector() {
  m_dataFrameReadInProgress = false;

  int sectorSize = m_putSectorBufferPtr - m_putSectorBuffer - 1;

  // calculate checksum
  byte chksum = checksum(m_putSectorBuffer, sectorSize);

  // if checksum is good...
  if (m_putSectorBuffer[sectorSize] == chksum) {
    // send ACK
    delay(DELAY_T4);
    m_stream->write(ACK);

    // write sector to disk image
    delay(DELAY_T5);
    if (m_writeSectorFunc(1, getCommandSector(), m_putSectorBuffer, sectorSize)) {
      // send COMPLETE
      m_stream->write(COMPLETE);
    } else {
      LOG_MSG_CR("Write to device error");
      m_stream->write(ERR);
    }
  // otherwise, NAK it
  } else {
    delay(DELAY_T4);
    m_stream->write(NAK);

    LOG_MSG_CR("Data frame checksum error");
  }
}

void SIOChannel::cmdGetStatus(int deviceId) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  // send complete 
  delay(DELAY_T5);
  m_stream->write(COMPLETE);

  // get device status
  DriveStatus* driveStatus = m_deviceStatusFunc(deviceId);

  // calculate checksum
  int frameLength = sizeof(driveStatus->statusFrame);
  byte chksum = checksum((byte*)&driveStatus->statusFrame, frameLength);

  // send status to bus
  byte* b = (byte*)&driveStatus->statusFrame;
  for (int i=0; i < frameLength; i++) {
    m_stream->write(*b);
    LOG_MSG(*b, HEX);
    LOG_MSG(" ");
    b++;
  }
  m_stream->write(chksum);
  
  LOG_MSG_CR();
}

void SIOChannel::cmdFormat(int deviceId, int density) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  // perform image format
  if (m_formatFunc(deviceId, density)) {
    // send COMPLETE
    delay(DELAY_T5);
    m_stream->write(COMPLETE);
    
    // send 128 (SD) or 256 (DD) data frame prefixed/suffixed with 0xFF,0xFF
    int frameLen = (density == DD_SECTOR_SIZE) ? DD_SECTOR_SIZE : SD_SECTOR_SIZE;

    LOG_MSG("Sending data frame of length ");
    LOG_MSG_CR(frameLen);

    m_stream->write(0xFF);
    m_stream->write(0xFF);
    for (int i=0; i < frameLen - 3; i++) {
      m_stream->write((byte)0x00);
    }
    m_stream->write(0xFF);
    m_stream->write(0xFF);
  } else {
    delay(DELAY_T5);
    m_stream->write(ERR);
  }
}

void SIOChannel::dumpCommandFrame() {
  LOG_MSG(m_cmdFrame.deviceId, HEX);
  LOG_MSG(" ");
  LOG_MSG(m_cmdFrame.command, HEX);
  LOG_MSG(" ");
  LOG_MSG(m_cmdFrame.aux1, HEX);
  LOG_MSG(" ");
  LOG_MSG(m_cmdFrame.aux2, HEX);
  LOG_MSG(" ");
  LOG_MSG(m_cmdFrame.checksum, HEX);
  LOG_MSG(" : ");
  
  switch (m_cmdFrame.command) {
    case CMD_STATUS:
      LOG_MSG("STATUS");
      break;
    case CMD_POLL:
      LOG_MSG("POLL");
      break;
    case CMD_READ:
      LOG_MSG("READ ");
      LOG_MSG(getCommandSector());
      break;
    case CMD_WRITE:
      LOG_MSG("WRITE ");
      LOG_MSG(getCommandSector());
      break;
    case CMD_PUT:
      LOG_MSG("PUT ");
      LOG_MSG(getCommandSector());
      break;
    case CMD_FORMAT:
      LOG_MSG("FORMAT");
      break;
    case CMD_FORMAT_MD:
      LOG_MSG("FORMAT MD");
      break;
    default:
      LOG_MSG("??");
  }
  
  LOG_MSG_CR();
}

unsigned long SIOChannel::getCommandSector() {
  return (unsigned long)(m_cmdFrame.aux2 << 8) + (m_cmdFrame.aux1 & 0xff);
}

