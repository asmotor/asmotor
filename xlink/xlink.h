/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

    This file is part of ASMotor.

    ASMotor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ASMotor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ASMotor.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef XLINK_XLINK_H_INCLUDED_
#define XLINK_XLINK_H_INCLUDED_

#include "util.h"

#define FILE_FORMAT_NONE				0x0000
#define FILE_FORMAT_BINARY				0x0001
#define FILE_FORMAT_GAME_BOY			0x0002
#define FILE_FORMAT_AMIGA_EXECUTABLE	0x0004
#define FILE_FORMAT_AMIGA_LINK_OBJECT	0x0008
#define FILE_FORMAT_CBM_PRG				0x0010
#define FILE_FORMAT_MEGA_DRIVE			0x0020
#define FILE_FORMAT_MASTER_SYSTEM		0x0040
#define FILE_FORMAT_HC800_KERNEL		0x0080
#define FILE_FORMAT_HC800				0x0100
#define FILE_FORMAT_PGZ					0x0200
#define FILE_FORMAT_COCO_BIN			0x0400

typedef uint32_t FileFormat;

extern FileFormat g_allowedFormats;
extern uint16_t g_cbmBaseAddress;

NORETURN (extern void error(const char* fmt, ...));

#endif
