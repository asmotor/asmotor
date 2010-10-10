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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "xasm.h"
#include "fstack.h"
#include "options.h"

extern char* loc_GetError(int n);

static char* g_pszWarning[]=
{
	"Cannot PURGE non-existant symbol",
	"Error in option %s, ignored",
	"Cannot pop options from an empty stack",
	"%s",
	"SHIFT used outside MACRO, ignored",
	"MEXIT used outside MACRO, ignored",
	"REXIT used outside REPT, ignored",
	"Error in machine option %s",
};

static char* g_pszError[]=
{
	"%c expected",
	"Expression must be %d bit",
	"Invalid expression",
	"Invalid source operand",
	"Invalid destination operand",
	"Invalid first operand",
	"Invalid second operand",
	"Invalid operand",
	"Expression expected",
	"Operand out of range",
	"Cannot modify symbol",
	"Label before SECTION",
	"Cannot export symbol",
	"SECTION cannot contain initialised data",
	"Cannot import already defined symbol",
	"Available SECTION space exhausted",
	"No SECTION defined",
	"Expression is neither constant nor relocatable",
	"Expression must be a power of two",
	"Expression must be constant",
	"Expression must be relocatable",
	"Invalid string expression",
	"Bad expression",
	"BANK expected",
	"TEXT or BSS expected",
	"Identifier must be a GROUP symbol",
	"Identifier expected",
	"Expression must be positive",
	"Syntax error",
	"Unknown instruction \"%s\"",
	"When writing binary file only PC relative addressing must be used, or first section must be ORG fixed.\n",
	"Section \"%s\" cannot be placed at $%X",
	"Symbol must be constant",
	"Symbol must be EQUS",
	"SECTION already exist in a different GROUP",
	"Read error",
	"File not found",
	"SECTION already exists but it's not ORG fixed to the same address",
	"SECTION already exists but it's not BANK fixed to the same bank",
	"SECTION already exists but it's not ORG/BANK fixed to the same address/bank",
	"SECTION does not exist",
	"Divide by zero",
	"Symbol cannot be used in an expression",
	"DEF() needs a symbol",
	"BANK() needs a symbol",
	"Unterminated MACRO block (started at %s, line %d)",
	"Unterminated REPT block",
	"Unexpected end of file reached",
	"Unterminated string",
	"Malformed identifier",
	"Maximum number of include paths reached",
	"MACRO doesn't exist",
	"Symbol %s is undefined",
	"Object file does not support expression",
};

static char* geterror(int n)
{
	if(n >= 1000)
		return loc_GetError(n);

	if(n < sizeof(g_pszWarning) / sizeof(char*))
		return g_pszWarning[n];
	else
		return g_pszError[n - 100];
}

static void	prj_Common(char severity, int n, va_list args)
{
	char* s = geterror(n);

	printf("%c%04d ", severity, n);
	fstk_Dump();
	
	vprintf(s, args);
	printf("\n");
}



void prj_Warn(int n, ...)
{
	va_list	args;

	int i = 0;
	for(i = 0; i < g_pOptions->nTotalDisabledWarnings; ++i)
	{
		if(g_pOptions->aDisabledWarnings[i] == n)
			return;
	}

	va_start(args, n);
	prj_Common('W', n, args);
	va_end(args);

	++g_nTotalWarnings;
}

void prj_Error(int n, ...)
{
	va_list	args;

	va_start(args, n);
	prj_Common('E', n, args);
	va_end(args);

	++g_nTotalErrors;
}

void prj_Fail(int n, ...)
{
	va_list	args;

	va_start(args, n);
	prj_Common('F', n, args);
	va_end(args);

	printf("Bailing out.\n");
	exit(EXIT_FAILURE);
}
