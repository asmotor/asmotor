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

#ifndef	INCLUDE_XASM_H
#define	INCLUDE_XASM_H

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#if defined(__VBCC__)
#define __CLOCK_T 1
typedef unsigned long clock_t;
#include <time.h>
#include "../../amitime.h"
#define clock time_GetMicroSeconds
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000
#else
#include <time.h>
#endif

#define	MAXSYMNAMELENGTH		256
#define	MAXSTRINGSYMBOLSIZE		256
#define	ASM_CRLF				10
#define	ASM_TAB					9

#define	MAXTOKENLENGTH			256

#include "../../asmotor.h"
#include "../../types.h"

#include "lists.h"
#include "project.h"
#include "options.h"
#include "tokens.h"
#include "lexer.h"
#include "expr.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"
#include "fstack.h"
#include "parse.h"
#include "globlex.h"
#include "object.h"
#include "binobj.h"
#include "amigaobj.h"

#include "localasm.h"

#if defined(_MSC_VER) || defined(__VBCC__) || defined(__GNUC__)
#	define	internalerror(s)	fprintf( stderr, "Internal error at "__FILE__"(%d): %s\n", __LINE__, s),exit(EXIT_FAILURE)
#else
#	define	internalerror(s)	fprintf( stderr, "Internal error at "__FILE__"(%d): %s\n", __LINE__, s),exit(EXIT_FAILURE),return(NULL)
#endif


#if	defined(__GNUC__) && !defined(__DJGPP__)
extern	void	strupr(char* s);
extern	void	strlwr(char* s);
#endif

extern ULONG g_nTotalLines;
extern ULONG g_nTotalErrors;
extern ULONG g_nTotalWarnings;


#endif	/*INCLUDE_XASM_H*/