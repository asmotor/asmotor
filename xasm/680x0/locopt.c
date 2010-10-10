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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xasm.h"
#include "project.h"
#include "options.h"
#include "locopt.h"

#include "loccpu.h"

void locopt_Copy(SMachineOptions* pDest, SMachineOptions* pSrc)
{
	*pDest = *pSrc;
}

SMachineOptions* locopt_Alloc(void)
{
	return malloc(sizeof(SMachineOptions));
}

void locopt_Open(void)
{
	g_pOptions->pMachine->nCpu = CPUF_68000;
}

void locopt_Update(void)
{
}

BOOL locopt_Parse(char* s)
{
	if(s == NULL || strlen(s) == 0)
		return FALSE;

	switch(s[0])
	{
		case 'c':
			if(strlen(&s[1]) == 1)
			{
				switch(s[1] - '0')
				{
					case 0:
						g_pOptions->pMachine->nCpu = CPUF_68000;
						return TRUE;
					case 1:
						g_pOptions->pMachine->nCpu = CPUF_68010;
						return TRUE;
					case 2:
						g_pOptions->pMachine->nCpu = CPUF_68020;
						return TRUE;
					case 3:
						g_pOptions->pMachine->nCpu = CPUF_68030;
						return TRUE;
					case 4:
						g_pOptions->pMachine->nCpu = CPUF_68040;
						return TRUE;
					case 6:
						g_pOptions->pMachine->nCpu = CPUF_68060;
						return TRUE;
					default:
						prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
						return FALSE;
				}
			}
			else
			{
				prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
				return FALSE;
			}
			break;
		default:
			prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
			return FALSE;
	}
}

void locopt_PrintOptions(void)
{
	printf("    -mc<X>  Enable CPU 680X0\n");
}
