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

#ifndef XLINK_COMMODORE_H_INCLUDED_
#define XLINK_COMMODORE_H_INCLUDED_

extern void
commodore_WritePrg(const char* outputFilename, const char* entry, uint32_t baseAddress);

extern void
commodore_SetupUnbankedCommodore128(void);

extern void
commodore_SetupCommodore64(void);

extern void
commodore_SetupCommodore264(void);

extern void
commodore_SetupCommodore128FunctionROM(void);

extern void
commodore_SetupCommodore128FunctionROMLow(void);

extern void
commodore_SetupCommodore128FunctionROMHigh(void);

#endif
