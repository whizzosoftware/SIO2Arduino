/*
 * sdrive.cpp - SIO handler for processing SDrive commands.
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

#import "sdrive.h"
#import "config.h"

SDriveHandler::SDriveHandler() {
}

void SDriveHandler::setDriveControl(DriveControl* driveControl) {
  m_driveControl = driveControl;
}

boolean SDriveHandler::isValidCommand(byte cmd) {
  return (cmd == CMD_SDRIVE_IDENT ||
          cmd == CMD_SDRIVE_INIT ||
          cmd == CMD_SDRIVE_CHROOT ||
          cmd == CMD_SDRIVE_SWAP_VDN ||
          cmd == CMD_SDRIVE_GETPARAMS ||
          cmd == CMD_SDRIVE_GET_ENTRIES ||
          cmd == CMD_SDRIVE_CHDIR ||
          cmd == CMD_SDRIVE_GET20 ||
          cmd == CMD_SDRIVE_MOUNT_D0 ||
          cmd == CMD_SDRIVE_MOUNT_D1 ||
          cmd == CMD_SDRIVE_MOUNT_D2 ||
          cmd == CMD_SDRIVE_MOUNT_D3 ||
          cmd == CMD_SDRIVE_MOUNT_D4);
}

boolean SDriveHandler::isValidDevice(byte device) {
  return (device == DEVICE_SDRIVE);
}

void SDriveHandler::processCommand(CommandFrame* cmdFrame, Stream* stream) {
  switch (cmdFrame->command) {
    case CMD_SDRIVE_IDENT:
      cmdIdent(stream);
      break;
    case CMD_SDRIVE_INIT:
      cmdInit(stream);
      break;
    case CMD_SDRIVE_CHROOT:
      cmdChroot(stream);
      break;
    case CMD_SDRIVE_SWAP_VDN:
      cmdSwapVdn(stream);
      break;
    case CMD_SDRIVE_GETPARAMS:
      cmdGetParams(stream);
      break;
    case CMD_SDRIVE_GET_ENTRIES:
      cmdGetEntries(cmdFrame->aux1, stream);
      break;
    case CMD_SDRIVE_CHDIR:
      cmdChdir(stream);
      break;
    case CMD_SDRIVE_GET20:
      cmdGet20(stream);
      break;
    case CMD_SDRIVE_MOUNT_D0:
      cmdMountDrive(0, cmdFrame->aux2 * 256 + cmdFrame->aux1, stream);
      break;
    case CMD_SDRIVE_MOUNT_D1:
      cmdMountDrive(1, cmdFrame->aux2 * 256 + cmdFrame->aux1, stream);
      break;
    case CMD_SDRIVE_MOUNT_D2:
      cmdMountDrive(2, cmdFrame->aux2 * 256 + cmdFrame->aux1, stream);
      break;
    case CMD_SDRIVE_MOUNT_D3:
      cmdMountDrive(3, cmdFrame->aux2 * 256 + cmdFrame->aux1, stream);
      break;
    case CMD_SDRIVE_MOUNT_D4:
      cmdMountDrive(4, cmdFrame->aux2 * 256 + cmdFrame->aux1, stream);
      break;
  }
}

void SDriveHandler::cmdIdent(Stream *stream) {
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  stream->write("SDrive01");
  stream->write(0xB0);
  stream->flush();
}

void SDriveHandler::cmdInit(Stream *stream) {
  // NO-OP
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  stream->flush();
}

void SDriveHandler::cmdChroot(Stream *stream) {
  // NO-OP
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  stream->flush();
}

void SDriveHandler::cmdSwapVdn(Stream *stream) {
  // NO-OP
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  stream->flush();
}

void SDriveHandler::cmdGetParams(Stream *stream) {
  // NO-OP
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  stream->write(0x06);
  stream->write((byte)0x00);
  stream->write(0x06);
  stream->flush();
}

void SDriveHandler::cmdGetEntries(byte n, Stream *stream) {
  // NO-OP
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  for (int i=0; i < n * 12; i++) {
    stream->write((byte)0x00);
  }
  stream->write((byte)0x00);
  stream->flush();
}

void SDriveHandler::cmdChdir(Stream* stream) {
  // NO-OP
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  for (int i=0; i < 14; i++) {
    stream->write((byte)0x00);
  }
  stream->write((byte)0x00);
  stream->flush();
}

void SDriveHandler::cmdGet20(Stream *stream) {
  delay(DELAY_T2);
  stream->write(ACK);
  delay(DELAY_T5);
  stream->write(COMPLETE);
  byte chkSum = 0;
  FileEntry fileEntries[20];

  m_driveControl->getFileList(0, 20, fileEntries);

  for (int i1=0; i1 < 20; i1++) {
    FileEntry entry = fileEntries[i1];
    for (int i2=0; i2 < 11; i2++) {
      char c = entry.name[i2];
      stream->write(c);
      chkSum = ((chkSum+c)>>8) + ((chkSum+c)&0xff);
    }
    stream->write((byte)0x00);
  }

  stream->write((byte)0x00);
  stream->write(chkSum);
  stream->flush();
}

void SDriveHandler::cmdMountDrive(byte driveNum, byte index, Stream* stream) {
  delay(DELAY_T2);
  stream->write(ACK);

  m_driveControl->mountFile(1, index, 20);

  delay(DELAY_T5);
  stream->write(COMPLETE);
  stream->flush();
}

boolean SDriveHandler::printCmdName(byte cmd) {
// we only compile this on DEBUG to save allocating string constants
#ifdef DEBUG
  switch (cmd) {
    case CMD_SDRIVE_IDENT:
      LOG_MSG("SDRIVE IDENT");
      break;
    case CMD_SDRIVE_INIT:
      LOG_MSG("SDRIVE INIT");
      break;
    case CMD_SDRIVE_CHROOT:
      LOG_MSG("SDRIVE CHROOT");
      break;
    case CMD_SDRIVE_SWAP_VDN:
      LOG_MSG("SDRIVE SWAP VDN");
      break;
    case CMD_SDRIVE_GETPARAMS:
      LOG_MSG("SDRIVE GETPARAMS");
      break;
    case CMD_SDRIVE_GET_ENTRIES:
      LOG_MSG("SDRIVE GET ENTRIES");
      break;
    case CMD_SDRIVE_CHDIR:
      LOG_MSG("SDRIVE CHDIR");
      break;
    case CMD_SDRIVE_GET20:
      LOG_MSG("SDRIVE GET20");
      break;
    case CMD_SDRIVE_MOUNT_D0:
      LOG_MSG("SDRIVE MOUNTvD0");
      break;
    case CMD_SDRIVE_MOUNT_D1:
      LOG_MSG("SDRIVE MOUNTvD1");
      break;
    case CMD_SDRIVE_MOUNT_D2:
      LOG_MSG("SDRIVE MOUNTvD2");
      break;
    case CMD_SDRIVE_MOUNT_D3:
      LOG_MSG("SDRIVE MOUNTvD3");
      break;
    case CMD_SDRIVE_MOUNT_D4:
      LOG_MSG("SDRIVE MOUNTvD4");
      break;
    default:
      return false;
  }
#endif

  return true;
}
