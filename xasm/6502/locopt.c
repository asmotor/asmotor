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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

#include "xasm.h"
#include "options.h"
#include "project.h"

#include "localasm.h"
#include "locerror.h"
#include "locopt.h"
#include "loccpu.h"
#include "tokens.h"

static int s_nPreviousInstructionSet = 0;

void locopt_Copy(struct MachineOptions* pDest, struct MachineOptions* pSrc)
{
	*pDest = *pSrc;
}

struct MachineOptions* locopt_Alloc(void)
{
	return mem_Alloc(sizeof(SMachineOptions));
}

void locopt_Open(void)
{
	opt_Current->machineOptions->nUndocumented = 0;
}

void locopt_Update(void)
{
	int nNewSet = opt_Current->machineOptions->nUndocumented;
	if(s_nPreviousInstructionSet != nNewSet)
	{
		SLexConstantsWord* pPrev = loclexer_GetUndocumentedInstructions(s_nPreviousInstructionSet);
		SLexConstantsWord* pNew = loclexer_GetUndocumentedInstructions(nNewSet);
		if(pPrev)
			lex_ConstantsUndefineWords(pPrev);
		if(pNew)
			lex_ConstantsDefineWords(pNew);
			
		s_nPreviousInstructionSet = nNewSet;
	}
}

bool locopt_Parse(char* s)
{
	if(s == NULL || strlen(s) == 0)
		return false;

	switch(s[0])
	{
		case 'u':
			if(strlen(&s[0]) >= 2)
			{
				int n = atoi(&s[1]);
				if(n >= 0 && n <= 3)
				{
					opt_Current->machineOptions->nUndocumented = n;
					return true;
				}
				prj_Error(ERROR_MACHINE_OPTION_UNDOCUMENTED_RANGE);
				return false;
			}
			break;
		default:
			break;
	}

	prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
	return false;
}

void locopt_PrintOptions(void)
{
	printf("    -mu<x>  Enable undocumented opcodes, name set x (0, 1 or 2)\n");
}
