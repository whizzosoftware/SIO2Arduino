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
#include "config.h"

SIOChannel::SIOChannel(int cmdPin, Stream* stream, DriveAccess *driveAccess, DriveControl *driveControl) {
  m_cmdPin = cmdPin;
  m_stream = stream;
  m_driveAccess = driveAccess;
  m_driveControl = driveControl;
  
  m_sdriveHandler.setDriveControl(m_driveControl);

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
          resetCommandFrameBuffer();
        }
        break;
      case STATE_READ_CMD:
        m_startTimeoutInterval = millis();
        // if command frame is fully read...
        if (m_cmdFramePtr - (byte*)&m_cmdFrame == COMMAND_FRAME_SIZE) {
          dumpCommandFrame();
          // process command frame
          if (isChecksumValid() && isCommandForThisDevice()) {
            if (isValidCommand() && isValidAuxData()) {
              m_cmdPinState = processCommand();
            } else {
              m_stream->write(NAK);
              m_cmdPinState = STATE_WAIT_CMD_START;
            }
          } else {
            m_cmdPinState = STATE_WAIT_CMD_START;
          }
        // otherwise, check for command read timeout
        } else if (millis() - m_startTimeoutInterval > READ_CMD_TIMEOUT) {
          m_cmdPinState = STATE_WAIT_CMD_START;
        }
        break;
      case STATE_READ_DATAFRAME:
        // check for timeout
        if (millis() - m_startTimeoutInterval > READ_FRAME_TIMEOUT) {
          m_cmdPinState = STATE_WAIT_CMD_START;
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

  switch (m_cmdPinState) {
    // if we read a valid device byte and are in a "command wait" state, come out of it and
    // process the byte
    case STATE_INIT:
    case STATE_WAIT_CMD_START:
    case STATE_WAIT_CMD_END:
      if (digitalRead(m_cmdPin) == LOW && isValidDevice(b)) {
        m_cmdPinState = STATE_READ_CMD;
        resetCommandFrameBuffer();
      } else {
        break;
      }
    // if we're reading a command frame...
    case STATE_READ_CMD: {
      // read the data into the command frame
      int idx = (int)m_cmdFramePtr - (int)&m_cmdFrame;
      // sometimes we see extra bytes between command frames on the bus while reading a command and things get lost --
      // the isValidDevice() check prevents a command frame read from getting corrupted by them
      if (idx < COMMAND_FRAME_SIZE && (idx > 0 || (idx == 0 && isValidDevice(b)))) {
        *m_cmdFramePtr = b;
        m_cmdFramePtr++;
        return;
      }
      break;
    }
    // if we're reading a data frame...
    case STATE_READ_DATAFRAME: {
      // add byte to read sector buffer
      *m_putSectorBufferPtr = b;
      m_putSectorBufferPtr++;
      m_putBytesRemaining--;
      if (m_putBytesRemaining == 0) {
        doPutSector();
      }
      break;
    }
    default:
      LOG_MSG("Ignoring byte ");
      LOG_MSG(b, HEX);
      LOG_MSG(" in state ");
      LOG_MSG_CR(m_cmdPinState);
      break;
  }
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
  return (m_cmdFrame.deviceId == DEVICE_D1 || m_cmdFrame.deviceId == DEVICE_SDRIVE);
}

boolean SIOChannel::isValidCommand() {
  boolean result = (m_cmdFrame.command == CMD_READ || 
                    m_cmdFrame.command == CMD_WRITE ||
                    m_cmdFrame.command == CMD_STATUS ||
                    m_cmdFrame.command == CMD_PUT ||
                    m_cmdFrame.command == CMD_FORMAT ||
                    m_cmdFrame.command == CMD_FORMAT_MD);

  if (!result) {
    result = m_sdriveHandler.isValidCommand(m_cmdFrame.command);
  }
  
  return result;
}

boolean SIOChannel::isValidDevice(byte b) {
  boolean result = (b == DEVICE_D1 ||
                    b == DEVICE_D2 ||
                    b == DEVICE_D3 ||
                    b == DEVICE_D4 ||
                    b == DEVICE_D5 ||
                    b == DEVICE_D6 ||
                    b == DEVICE_D7 ||
                    b == DEVICE_D8 ||
                    b == DEVICE_R1);

  if (!result) {
    result = m_sdriveHandler.isValidDevice(b);
  }
  
  return result;
}

boolean SIOChannel::isValidAuxData() {
  return true;
}

byte SIOChannel::checksum(byte* chunk, int length) {
  int chkSum = 0;
  for(int i=0; i < length; i++) {
    chkSum = ((chkSum+chunk[i])>>8) + ((chkSum+chunk[i])&0xff);
  }
  return (byte)chkSum;
}

byte SIOChannel::processCommand() {
  int deviceId = 1;
  byte nextCmdPinState = STATE_WAIT_CMD_END;
  
  switch (m_cmdFrame.command) {
    case CMD_READ:
      cmdGetSector(deviceId);
      break;
    case CMD_WRITE:
    case CMD_PUT:
      cmdPutSector(deviceId);
      nextCmdPinState = STATE_READ_DATAFRAME;
      m_startTimeoutInterval = millis();
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
    default:
      m_sdriveHandler.processCommand(&m_cmdFrame, m_stream);
      break;
  }
  
  return nextCmdPinState;
}

void SIOChannel::cmdGetSector(int deviceId) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  // write data frame + checksum
  SectorDataInfo *p = m_driveAccess->readSectorFunc(deviceId, getCommandSector(), (byte*)&m_sectorBuffer);
  if (p != NULL && !p->error) {
    // send complete 
    delay(DELAY_T5);
    m_stream->write(COMPLETE);
  } else {
    // send error
    delay(DELAY_T5);
    m_stream->write(ERR);
  }

  m_stream->flush();

  delayMicroseconds(700);

  if (p != NULL) {
    byte *b = (byte*)&m_sectorBuffer;
    // write data
    for (int i=0; i < p->length; i++) {
      m_stream->write(*b);
      b++;
    }
    // write checksum
    m_stream->write(checksum((byte*)&m_sectorBuffer, p->length));
  } else {
    // write empty data + checksum
    for (int i=0; i < SD_SECTOR_SIZE + 1; i++) {
      m_stream->write((byte)0x00);
    }
  }

  m_stream->flush();
}

void SIOChannel::cmdPutSector(int deviceId) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  DriveStatus *status = m_driveAccess->deviceStatusFunc(deviceId);
  m_putBytesRemaining = status->sectorSize + 1;
  m_putSectorBufferPtr = m_sectorBuffer;
}
  
void SIOChannel::doPutSector() {
  int sectorSize = m_putSectorBufferPtr - m_sectorBuffer - 1;

  // calculate checksum
  byte chksum = checksum(m_sectorBuffer, sectorSize);

  // if checksum is good...
  if (m_sectorBuffer[sectorSize] == chksum) {
    // send ACK
    delay(DELAY_T4);
    m_stream->write(ACK);

    // write sector to disk image
    delay(DELAY_T5);
    if (m_driveAccess->writeSectorFunc(1, getCommandSector(), m_sectorBuffer, sectorSize)) {
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

    LOG_MSG("Data frame checksum error: ");
    LOG_MSG(chksum, HEX);
    LOG_MSG(" vs. ");
    LOG_MSG_CR(m_sectorBuffer[sectorSize], HEX);
  }

  // change state
  m_cmdPinState = STATE_WAIT_CMD_START;
}

void SIOChannel::cmdGetStatus(int deviceId) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  // send complete 
  delay(DELAY_T5);
  m_stream->write(COMPLETE);

  // get device status
  DriveStatus* driveStatus = m_driveAccess->deviceStatusFunc(deviceId);

  // calculate checksum
  int frameLength = sizeof(driveStatus->statusFrame);
  byte chksum = checksum((byte*)&driveStatus->statusFrame, frameLength);

  // send status to bus
  byte* b = (byte*)&driveStatus->statusFrame;
  for (int i=0; i < frameLength; i++) {
    m_stream->write(*b);
    b++;
  }
  m_stream->write(chksum);
}

void SIOChannel::cmdFormat(int deviceId, int density) {
  // send ACK
  delay(DELAY_T2);
  m_stream->write(ACK);

  // perform image format
  if (m_driveAccess->formatFunc(deviceId, density)) {
    // send COMPLETE
    delay(DELAY_T5);
    m_stream->write(COMPLETE);
    
    LOG_MSG("Sending data frame of length ");
    LOG_MSG_CR(SD_SECTOR_SIZE);

    m_stream->write(0xFF);
    m_stream->write(0xFF);
    for (int i=0; i < SD_SECTOR_SIZE - 3; i++) {
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
// we only compile this on DEBUG to save allocating string constants
#ifdef DEBUG
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
      if (!m_sdriveHandler.printCmdName(m_cmdFrame.command)) {
        LOG_MSG("??");
      }
  }
  
  LOG_MSG_CR();
#endif  
}

unsigned long SIOChannel::getCommandSector() {
  return (unsigned long)(m_cmdFrame.aux2 << 8) + (m_cmdFrame.aux1 & 0xff);
}

void SIOChannel::resetCommandFrameBuffer() {
  // reset last command frame info
  memset(&m_cmdFrame, 0, sizeof(m_cmdFrame));
  m_cmdFramePtr = (byte*)&m_cmdFrame;
}

