/*
 * drive_access.cpp
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
#include "drive_access.h"

DriveAccess::DriveAccess(DriveStatus*(*a)(int), SectorDataInfo*(*b)(int,unsigned long,byte*), boolean(*c)(int,unsigned long,byte*,unsigned long), boolean(*d)(int,int)) {
  deviceStatusFunc = a;
  readSectorFunc = b;
  writeSectorFunc = c;
  formatFunc = d;
}

