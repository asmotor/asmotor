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

#if !defined(INTEGER_INSTRUCTIONS_SCHIP_)
#define INTEGER_INSTRUCTIONS_SCHIP_


static int parse_GetRegister(void)
{
	if(g_CurrentToken.ID.TargetToken >= T_CHIP_REG_V0
	&& g_CurrentToken.ID.TargetToken <= T_CHIP_REG_V15)
	{
		int r = g_CurrentToken.ID.TargetToken - T_CHIP_REG_V0;
		parse_GetToken();

		return r;
	}
	
	return -1;
}


static int parse_ExpectRegister(void)
{
	int r = parse_GetRegister();
	
	if(r != -1)
		return r;
		
	prj_Error(MERROR_REGISTER_EXPECTED);
	return -1;
}


typedef bool_t (*fpParser_t)(void);

fpParser_t g_fpParsers[T_CHIP_ADD - T_CHIP_ADD + 1] =
{
	parse_IntegerInstructionRR,	//	T_CHIP_ADD = 6000,
};


bool_t parse_IntegerInstruction(void)
{
	if(T_CHIP_ADD <= g_CurrentToken.ID.TargetToken && g_CurrentToken.ID.TargetToken <= T_CHIP_RET)
	{
		return g_fpParsers[g_CurrentToken.ID.TargetToken - T_CHIP_ADD]();
	}

	return false;
}


#endif
