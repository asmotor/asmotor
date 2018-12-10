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

#ifndef UTIL_ASMOTOR_H_INCLUDED_
#define UTIL_ASMOTOR_H_INCLUDED_

#include <stdlib.h>
#include "types.h"

#if defined(__GNUC_STDC_INLINE__)
#	define INLINE static inline
#elif defined(_MSC_VER)
#	define INLINE static __inline
#else
#	define INLINE static
#endif

#define ASMOTOR

#define ASMOTOR_VERSION "0.1.0"
#define LINK_VERSION "0.1.0"
#define LIB_VERSION "0.1.0"

#if defined(__VBCC__) || defined(__GNUC__)
extern char*
_strdup(const char* str);

extern char*
_strupr(char* str);

extern char*
_strlwr(char* str);

extern int
_strnicmp(const char* string1, const char* string2, size_t length);

extern int
_stricmp(const char* string1, const char* string2);
#endif

#if defined(_MSC_VER) || defined(__VBCC__) || defined(__GNUC__)
# define internalerror(s) fprintf( stderr, "Internal error at "__FILE__"(%d): %s\n", __LINE__, s),exit(EXIT_FAILURE)
#else
# define internalerror(s) fprintf( stderr, "Internal error at "__FILE__"(%d): %s\n", __LINE__, s),exit(EXIT_FAILURE),return NULL
#endif

#endif /* UTIL_ASMOTOR_H_INCLUDED_ */
