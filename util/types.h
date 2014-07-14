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

#if defined(__APPLE__)
#	include <stdint.h>
#else
typedef	unsigned char	uint8_t;
typedef	signed char		int8_t;
typedef	unsigned short	uint16_t;
typedef	signed short	int16_t;
typedef	unsigned int	uint32_t;
typedef	signed int		int32_t;
typedef signed long long int64_t;
#endif

#if defined(__cplusplus)
typedef bool bool_t;
#else
typedef	enum
{
	false = 0,
	true = 1
}	bool_t;
#endif

#ifndef	NULL
#	define	NULL	0L
#endif

#endif	/*INCLUDE_TYPES_H*/
