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

#ifndef	INCLUDE_TYPES_H
#define	INCLUDE_TYPES_H

#if defined(__VBCC__)
#include <exec/types.h>
#else
typedef	unsigned char	UBYTE;
typedef	unsigned short	UWORD;
typedef	unsigned long	ULONG;
typedef	enum
{
	FALSE=0,
	TRUE=1
}	BOOL;
#endif


typedef	signed char	SBYTE;
typedef	signed short SWORD;
typedef	signed long	SLONG;
typedef signed long long SLLONG;


#define	INVERTBOOL(x)	((x)?FALSE:TRUE)

#ifndef	NULL
#define	NULL	0L
#endif

#endif	/*INCLUDE_TYPES_H*/