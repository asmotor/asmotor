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

#include "xasm.h"

extern void locopt_Update(void);
extern BOOL locopt_Parse(char*);
extern void locopt_Open(void);

sOptions* pOptions;

static	void	opt_Update(void)
{
	lex_FloatRemoveAll(BinaryConstID);

	lex_FloatAddRange(BinaryConstID, '%', '%', 1);
    lex_FloatAddRangeAndBeyond(BinaryConstID, pOptions->BinaryChar[0], pOptions->BinaryChar[0], 2);
    lex_FloatAddRangeAndBeyond(BinaryConstID, pOptions->BinaryChar[1], pOptions->BinaryChar[1], 2);

	locopt_Update();
}

void	opt_Push(void)
{
	sOptions* nopt;

	if((nopt = (sOptions*)malloc(sizeof(sOptions))) != NULL)
	{
		*nopt = *pOptions;
		list_Insert(pOptions, nopt);
	}
	else
	{
		internalerror("Out of memory");
	}
}

void	opt_Pop(void)
{
	if(!list_isLast(pOptions))
	{
		sOptions* nopt = pOptions;

		list_Remove(pOptions, pOptions);
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
			if(pOptions->nTotalDisabledWarnings < MAXDISABLEDWARNINGS
			&& 1 == sscanf(&s[1], "%d", &w))
			{
				pOptions->aDisabledWarnings[pOptions->nTotalDisabledWarnings++] = (UWORD)w;
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
					pOptions->Endian = ASM_BIG_ENDIAN;
					break;
				case 'l':
					pOptions->Endian = ASM_LITTLE_ENDIAN;
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
				pOptions->BinaryChar[0] = s[1];
				pOptions->BinaryChar[1] = s[2];
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
					pOptions->UninitChar = -1;
				}
				else
				{
					int	result = sscanf(&s[1], "%x", &pOptions->UninitChar);
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

void	opt_Open(void)
{
	if((pOptions = (sOptions*)malloc(sizeof(sOptions))) != NULL)
	{
		memset(pOptions, 0, sizeof(sOptions));
		pOptions->Flags = 0;
		pOptions->Endian = ASM_DEFAULT_ENDIAN;
		pOptions->BinaryChar[0] = '0';
		pOptions->BinaryChar[1] = '1';
		pOptions->UninitChar = -1;
		pOptions->nTotalDisabledWarnings = 0;
		locopt_Open();
		opt_Update();
	}
	else
	{
		internalerror("Out of memory");
	}
}

void	opt_Close(void)
{
	while(pOptions != NULL)
	{
		sOptions* t = pOptions;
		list_Remove(pOptions, pOptions);
		free(t);
	}
}