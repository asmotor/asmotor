/*  Copyright 2008 Carsten Sørensen

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

#ifndef	LIBRARY_H
#define	LIBRARY_H

#include "libwrap.h"

extern SLibrary* lib_Read(char* filename);
extern bool_t lib_Write(SLibrary* lib, char* filename);
extern SLibrary* lib_AddReplace(SLibrary* lib, char* filename);
extern void lib_Free(SLibrary* lib);
extern SLibrary* lib_DeleteModule(SLibrary* lib, char* filename);
extern SLibrary* lib_Find(SLibrary* lib, char* filename);

#endif
