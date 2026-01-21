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

#ifndef XLINK_AMIGA_H_INCLUDED_
#define XLINK_AMIGA_H_INCLUDED_

#include <stdbool.h>

extern void
amiga_WriteExecutable(const char* filename, const char* entry, bool debugInfo);

extern void
amiga_WriteLinkObject(const char* filename, bool debugInfo);

#endif
