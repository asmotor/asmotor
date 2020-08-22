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

#include "errors.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"

#include "m68k_errors.h"
#include "m68k_options.h"
#include "m68k_parse.h"
#include "m68k_tokens.h"

#define FPU_INS 0xF200u


static uint16_t parse_GetSourceSpecifier(ESize sz)
{
    switch(sz)
    {
        case SIZE_LONG:		return 0 << 10;
        case SIZE_SINGLE:	return 1 << 10;
        case SIZE_EXTENDED: return 2 << 10;
        case SIZE_PACKED:	return 3 << 10;
        case SIZE_WORD:		return 4 << 10;
        case SIZE_DOUBLE:	return 5 << 10;
        case SIZE_BYTE:		return 6 << 10;
        default:			internalerror("unknown size");
    }
}


static bool parse_FpuGeneric(ESize sz, uint16_t opmode, SAddressingMode* src, SAddressingMode* dest)
{
    uint16_t rm = src->mode == AM_FPUREG ? (uint16_t) 0x0000 : (uint16_t) 0x4000;

    if(dest->mode != AM_FPUREG)
    {
        err_Error(MERROR_FPU_REGISTER_EXPECTED);
        return true;
    }

    sect_OutputConst16(FPU_INS | (rm ? m68k_GetEffectiveAddressField(src) : 0u));
    sect_OutputConst16(rm | parse_GetSourceSpecifier(sz) | (dest->directRegister << 7) | opmode);
    return m68k_OutputExtensionWords(src);
}


static bool parse_FABS(ESize sz, SAddressingMode* src, SAddressingMode* dest)
{
    if(dest == NULL && src->mode == AM_FPUREG)
    {
        return parse_FABS(sz, src, src);
    }

    return parse_FpuGeneric(sz, 0x18, src, dest);
}


static SInstruction s_FpuInstructions[] =
{
    {   // FABS
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_FPUREG | AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_FPUREG, /*dest*/ AM_NONE,
        parse_FABS
    }
};


bool
m68k_ParseFpuInstruction(void) {
    int tokenOp;
    SInstruction* instruction;

    if(lex_Current.token < T_FPU_FIRST
    || lex_Current.token > T_FPU_LAST)
    {
        return false;
    }

    tokenOp = lex_Current.token - T_FPU_FIRST;
    parse_GetToken();

    instruction = &s_FpuInstructions[tokenOp];
    if((instruction->cpu & opt_Current->machineOptions->fpu) == 0)
    {
        err_Error(MERROR_INSTRUCTION_FPU);
        return true;
    }

    return m68k_ParseCommonCpuFpu(instruction);
}
