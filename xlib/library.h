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

#ifndef XLIB_LIBRARY_H_INCLUDED_
#define XLIB_LIBRARY_H_INCLUDED_

#include "module.h"

extern SModule*
lib_Read(const char* filename);

extern bool
lib_Write(SModule* library, const char* filename);

extern SModule*
lib_AddReplace(SModule* library, const char* filename);

extern void
lib_Free(SModule* library);

extern SModule*
lib_DeleteModule(SModule* library, const char* filename);

extern SModule*
lib_Find(SModule* library, const char* filename);

#endif
