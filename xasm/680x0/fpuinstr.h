/*  Copyright 2008-2014 Carsten Sørensen

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

#if !defined(FPU_INSTRUCTIONS_M68K_)
#define FPU_INSTRUCTIONS_M68K_

static SInstruction s_FpuInstructions[] =
{
    {   // FABS
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_FPUREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ 0,
        parse_FABS
    }
};

bool_t parse_FpuInstruction(void)
{
	if(g_CurrentToken.ID.TargetToken < T_FPU_FIRST
	|| g_CurrentToken.ID.TargetToken > T_FPU_LAST)
	{
		return false;
	}
	
	return false;
}

#endif

