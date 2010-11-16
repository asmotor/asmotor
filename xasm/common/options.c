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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "asmotor.h"
#include "xasm.h"
#include "options.h"
#include "lexer.h"
#include "globlex.h"
#include "project.h"
#include "fstack.h"

extern void locopt_Update(void);
extern bool_t locopt_Parse(char*);
extern void locopt_Open(void);

SOptions* g_pOptions;

static void opt_Update(void)
{
	lex_FloatRemoveAll(BinaryConstID);

	lex_FloatAddRange(BinaryConstID, '%', '%', 1);
    lex_FloatAddRangeAndBeyond(BinaryConstID, g_pOptions->BinaryChar[0], g_pOptions->BinaryChar[0], 2);
    lex_FloatAddRangeAndBeyond(BinaryConstID, g_pOptions->BinaryChar[1], g_pOptions->BinaryChar[1], 2);

	locopt_Update();
}

static SOptions* opt_Alloc(void)
{
	SOptions* nopt;

	if((nopt = (SOptions*)malloc(sizeof(SOptions))) != NULL)
	{
		memset(nopt, 0, sizeof(SOptions));
		nopt->pMachine = locopt_Alloc();
		return nopt;
	}

	internalerror("Out of memory");
	return NULL;
}

static void opt_Copy(SOptions* pDest, SOptions* pSrc)
{
	struct MachineOptions* p = pDest->pMachine;

	*pDest = *pSrc;
	pDest->pMachine = p;

	locopt_Copy(pDest->pMachine, pSrc->pMachine);
}

void opt_Push(void)
{
	SOptions* nopt = opt_Alloc();
	opt_Copy(nopt, g_pOptions);

	list_Insert(g_pOptions, nopt);
}

void	opt_Pop(void)
{
	if(!list_isLast(g_pOptions))
	{
		SOptions* nopt = g_pOptions;

		list_Remove(g_pOptions, g_pOptions);
		free(nopt);
		opt_Update();
	}
	else
	{
		prj_Warn(WARN_OPTION_POP);
	}
}

void	opt_Parse(char* s)
{
	switch(s[0])
	{
		case 'w':
		{
			int w;
			if(g_pOptions->nTotalDisabledWarnings < MAXDISABLEDWARNINGS
			&& 1 == sscanf(&s[1], "%d", &w))
			{
				g_pOptions->aDisabledWarnings[g_pOptions->nTotalDisabledWarnings++] = (uint16_t)w;
			}
			else
				prj_Warn(WARN_OPTION, s);
			break;
		}
		case 'i':
		{
			fstk_AddIncludePath(&s[1]);
			break;
		}
		case 'e':
		{
			switch(s[1])
			{
				case 'b':
					g_pOptions->Endian = ASM_BIG_ENDIAN;
					break;
				case 'l':
					g_pOptions->Endian = ASM_LITTLE_ENDIAN;
					break;
				default:
					prj_Warn(WARN_OPTION, s);
					break;
			}
			break;
		}
		case 'm':
		{
			locopt_Parse(&s[1]);
			break;
		}
		case 'b':
		{
			if(strlen(&s[1]) == 2)
			{
				g_pOptions->BinaryChar[0] = s[1];
				g_pOptions->BinaryChar[1] = s[2];
			}
			else
			{
				prj_Warn(WARN_OPTION, s);
			}
			break;
		}
		case 'z':
		{
			if(strlen(&s[1]) <= 2)
			{
				if(strcmp(&s[1], "?") == 0)
				{
					g_pOptions->UninitChar = -1;
				}
				else
				{
					unsigned int nUninitChar;
					int	result = sscanf(&s[1], "%x", &nUninitChar);
					g_pOptions->UninitChar = (int)nUninitChar;
					if(result == EOF || result != 1)
					{
						prj_Warn(WARN_OPTION, s);
					}
				}
			}
			else
			{
				prj_Warn(WARN_OPTION, s);
			}
			break;
		}
		default:
		{
			prj_Warn(WARN_OPTION, s);
			break;
		}
	}
	opt_Update();
}

void opt_Open(void)
{
	g_pOptions = opt_Alloc();

	g_pOptions->Flags = 0;
	g_pOptions->Endian = g_pConfiguration->eDefaultEndianness;
	g_pOptions->BinaryChar[0] = '0';
	g_pOptions->BinaryChar[1] = '1';
	g_pOptions->UninitChar = -1;
	g_pOptions->nTotalDisabledWarnings = 0;
	locopt_Open();
	opt_Update();
}

void opt_Close(void)
{
	while(g_pOptions != NULL)
	{
		SOptions* t = g_pOptions;
		list_Remove(g_pOptions, g_pOptions);
		free(t);
	}
}
