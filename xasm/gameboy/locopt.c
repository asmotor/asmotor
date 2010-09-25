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

#include "../common/xasm.h"

void locopt_Open(void)
{
	pOptions->Machine.GameboyChar[0]='0';
	pOptions->Machine.GameboyChar[1]='1';
	pOptions->Machine.GameboyChar[2]='2';
	pOptions->Machine.GameboyChar[3]='3';
}

void locopt_Update(void)
{
	lex_FloatRemoveAll(GameboyConstID);
	lex_FloatAddRange(GameboyConstID, '`', '`', 1);
    lex_FloatAddRangeAndBeyond(GameboyConstID, pOptions->Machine.GameboyChar[0], pOptions->Machine.GameboyChar[0], 2);
    lex_FloatAddRangeAndBeyond(GameboyConstID, pOptions->Machine.GameboyChar[1], pOptions->Machine.GameboyChar[1], 2);
    lex_FloatAddRangeAndBeyond(GameboyConstID, pOptions->Machine.GameboyChar[2], pOptions->Machine.GameboyChar[2], 2);
    lex_FloatAddRangeAndBeyond(GameboyConstID, pOptions->Machine.GameboyChar[3], pOptions->Machine.GameboyChar[3], 2);
}

BOOL locopt_Parse(char* s)
{
	if(s == NULL || strlen(s) == 0)
		return FALSE;

	switch(s[0])
	{
		case 'g':
			if(strlen(&s[1])==4)
			{
				pOptions->Machine.GameboyChar[0]=s[1];
				pOptions->Machine.GameboyChar[1]=s[2];
				pOptions->Machine.GameboyChar[2]=s[3];
				pOptions->Machine.GameboyChar[3]=s[4];
				return TRUE;
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
	printf("\t-mg<ASCI>\tChange the four characters used for Gameboy graphics\n"
			"\t\t\tconstants (default is 0123)\n");
}
