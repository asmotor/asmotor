/*  Copyright 2008-2017 Carsten Elton Sorensen

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

#ifndef	XLINK_H_INCLUDED_
#define	XLINK_H_INCLUDED_

#define	MAXSYMNAMELENGTH	256

#include "types.h"
#include "object.h"
#include "image.h"
#include "commodore.h"
#include "mapfile.h"
#include "symbol.h"
#include "patch.h"
#include "section.h"
#include "group.h"
#include "assign.h"
#include "smart.h"
#include "amiga.h"
#include "mem.h"
#include "section.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#if defined(__clang__) || defined(__llvm__) || defined(__GNUC__)
#   define NO_RETURN __attribute__ ((noreturn))
#else
#   define NO_RETURN
#endif

extern void Error(char* fmt, ...) NO_RETURN;

#endif
