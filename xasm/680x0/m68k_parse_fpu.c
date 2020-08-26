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

#define AM_FPU_SOURCE_020 (AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_FPUREG)
#define AM_FPU_SOURCE (AM_FPU_SOURCE_020 | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP)


static uint16_t
getSourceSpecifier(ESize sz) {
    switch(sz) {
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


static bool
genericInstruction(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    uint16_t rm = src->mode == AM_FPUREG ? (uint16_t) 0x0000 : (uint16_t) 0x4000;

    if(dest->mode != AM_FPUREG) {
        err_Error(MERROR_FPU_REGISTER_EXPECTED);
        return true;
    }

    sect_OutputConst16(FPU_INS | (rm ? m68k_GetEffectiveAddressField(src) : 0u));
    sect_OutputConst16(rm | getSourceSpecifier(sz) | (dest->directRegister << 7) | opmode);
    return m68k_OutputExtensionWords(src);
}


static bool
possiblyUnaryInstruction(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if((dest == NULL || dest->mode == AM_EMPTY) && src->mode == AM_FPUREG) {
        dest = src;
    }

    return genericInstruction(sz, src, dest, opmode);
}


static bool
handleFBcc(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if (sz == SIZE_DEFAULT) {
        sz = src->mode == AM_WORD ? SIZE_WORD : SIZE_LONG;
    }

    opmode |= 0xF080 | FPU_INS;

    if (sz == SIZE_WORD) {
        SExpression* expr = expr_CheckRange(expr_PcRelative(src->outer.displacement, 0), -32768, 32767);
        opmode |= 0x0040;

        if (expr != NULL) {
            sect_OutputConst16(opmode);
            sect_OutputExpr16(expr);
            return true;
        }

        err_Error(ERROR_OPERAND_RANGE);
        return true;
    } else /*if (sz == SIZE_LONG)*/ {
        SExpression* expr = expr_PcRelative(src->outer.displacement, 0);
        sect_OutputConst16(opmode);
        sect_OutputExpr32(expr);
        return true;
    }
}


static SInstruction
s_FpuInstructions[] = {
    {   // FABS
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0018,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FSABS
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0058,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FDABS
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x005C,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FACOS
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x001C,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FADD
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0022,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FSADD
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0062,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FDADD
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0066,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FASIN
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x000C,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FATAN
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x000A,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FATANH
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x000D,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FBF
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0000,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBEQ
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0001,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBOGT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0002,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBOGE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0003,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBOLT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0004,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBOLE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0005,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBOGL
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0006,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBOR
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0007,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBUN
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0008,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBUEQ
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0009,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBUGT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x000A,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBUGE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x000B,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBULT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x000C,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBULE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x000D,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBNE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x000E,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x000F,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBSF
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0010,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBSEQ
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0011,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBGT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0012,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBGE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0013,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBLT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0014,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBLE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0015,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBGL
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0016,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBGLE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0017,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBNGLE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0018,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBNGL
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x0019,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBNLE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x001A,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBNLT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x001B,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBNGE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x001C,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBNGT
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x001D,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBSNE
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x001E,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
    {   // FBST
        FPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        0x001F,
        AM_WORD | AM_LONG,
        AM_NONE,
        handleFBcc
    },
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

    return m68k_ParseCommonCpuFpu(instruction, true);
}
