/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

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

#include "z80_section.h"

extern void
z80_AssignSection(SSection* section) {
	if (section->flags & SECTF_BANKFIXED) {
		section->imagePosition += section->bank * 16384;
		section->cpuOrigin += section->bank == 0 ? 0 : 16384;
		section->flags |= SECTF_LOADFIXED;
	}
}
