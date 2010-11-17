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

#ifndef	INCLUDE_ASMOTOR_H
#define	INCLUDE_ASMOTOR_H

#include "types.h"

#if defined(__GNUC_STDC_INLINE__)
#	define INLINE static inline
#elif defined(_MSC_VER)
#	define INLINE static __inline
#else
#	define INLINE static
#endif

#define	ASMOTOR

#define	ASMOTOR_VERSION	"0.1.0"
#define	LINK_VERSION	"0.1.0"
#define	LIB_VERSION		"0.1.0"

extern int32_t imuldiv(int32_t a, int32_t b, int32_t c);

extern int32_t fmul(int32_t a, int32_t b);
extern int32_t fdiv(int32_t a, int32_t b);
extern int32_t fsin(int32_t a);
extern int32_t fcos(int32_t a);
extern int32_t ftan(int32_t a);
extern int32_t fasin(int32_t a);
extern int32_t facos(int32_t a);
extern int32_t fatan(int32_t a);
extern int32_t fatan2(int32_t a, int32_t b);


#if defined(__VBCC__) || defined(__GNUC__)
extern char* _strdup(const char* pszString);
extern char* _strupr(char* pszString);
extern char* _strlwr(char* pszString);
extern int _strnicmp(const char* pszString1, const char* pszString2, int nCount);
extern int _stricmp(const char* pszString1, const char* pszString2);
#endif

#if defined(_MSC_VER) || defined(__VBCC__) || defined(__GNUC__)
#	define	internalerror(s)	fprintf( stderr, "Internal error at "__FILE__"(%d): %s\n", __LINE__, s),exit(EXIT_FAILURE)
#else
#	define	internalerror(s)	fprintf( stderr, "Internal error at "__FILE__"(%d): %s\n", __LINE__, s),exit(EXIT_FAILURE),return NULL
#endif


#endif  /* INCLUDE_ASMOTOR_H */
