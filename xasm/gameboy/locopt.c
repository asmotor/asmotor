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

#include <string.h>
#include <stdlib.h>

#include "xasm.h"
#include "mem.h"
#include "options.h"
#include "lexer.h"
#include "locopt.h"
#include "globlex.h"
#include "project.h"

uint32_t GameboyConstID;

SMachineOptions* locopt_Alloc(void)
{
	return mem_Alloc(sizeof(SMachineOptions));
}

void locopt_Copy(SMachineOptions* pDest, SMachineOptions* pSrc)
{
	*pDest = *pSrc;
}

void locopt_Open(void)
{
	g_pOptions->pMachine->GameboyChar[0]='0';
	g_pOptions->pMachine->GameboyChar[1]='1';
	g_pOptions->pMachine->GameboyChar[2]='2';
	g_pOptions->pMachine->GameboyChar[3]='3';
}

void locopt_Update(void)
{
	lex_FloatRemoveAll(GameboyConstID);
	lex_FloatAddRange(GameboyConstID, '`', '`', 1);
    lex_FloatAddRangeAndBeyond(GameboyConstID, g_pOptions->pMachine->GameboyChar[0], g_pOptions->pMachine->GameboyChar[0], 2);
    lex_FloatAddRangeAndBeyond(GameboyConstID, g_pOptions->pMachine->GameboyChar[1], g_pOptions->pMachine->GameboyChar[1], 2);
    lex_FloatAddRangeAndBeyond(GameboyConstID, g_pOptions->pMachine->GameboyChar[2], g_pOptions->pMachine->GameboyChar[2], 2);
    lex_FloatAddRangeAndBeyond(GameboyConstID, g_pOptions->pMachine->GameboyChar[3], g_pOptions->pMachine->GameboyChar[3], 2);
}

bool_t locopt_Parse(char* s)
{
	if(s == NULL || strlen(s) == 0)
		return false;

	switch(s[0])
	{
		case 'g':
			if(strlen(&s[1])==4)
			{
				g_pOptions->pMachine->GameboyChar[0]=s[1];
				g_pOptions->pMachine->GameboyChar[1]=s[2];
				g_pOptions->pMachine->GameboyChar[2]=s[3];
				g_pOptions->pMachine->GameboyChar[3]=s[4];
				return true;
			}
			else
			{
				prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
				return false;
			}
			break;
		default:
			prj_Warn(WARN_MACHINE_UNKNOWN_OPTION, s);
			return false;
	}
}

void locopt_PrintOptions(void)
{
	printf("\t-mg<ASCI>\tChange the four characters used for Gameboy graphics\n"
			"\t\t\tconstants (default is 0123)\n");
}
