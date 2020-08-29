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
#include "parse_expression.h"

#include "m68k_errors.h"
#include "m68k_options.h"
#include "m68k_parse.h"
#include "m68k_tokens.h"

#define FPU_INS 0xF200u

#define AM_FPU_SOURCE_020 (AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_FPUREG)
#define AM_FPU_SOURCE (AM_FPU_SOURCE_020 | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP)


static uint16_t
getSourceSpecifier(ESize sz, SAddressingMode* addr) {
    if (addr->mode == AM_FPUREG) {
        return (addr->directRegister & 7) << 10;
    } else {
        switch(sz) {
            case SIZE_LONG:		        return 0x4000 | (0 << 10);
            case SIZE_SINGLE:	        return 0x4000 | (1 << 10);
            case SIZE_EXTENDED:         return 0x4000 | (2 << 10);
            case SIZE_PACKED:	        return 0x4000 | (3 << 10);
            case SIZE_WORD:		        return 0x4000 | (4 << 10);
            case SIZE_DOUBLE:	        return 0x4000 | (5 << 10);
            case SIZE_BYTE:		        return 0x4000 | (6 << 10);
            case SIZE_PACKED_DYNAMIC:	return 0x4000 | (7 << 10);
            default: internalerror("unknown size");
        }
    }
}

static uint16_t
getEffectiveAddressField(SAddressingMode* src) {
    if (src->mode == AM_FPUREG) {
        return 0;
    } else {
        return m68k_GetEffectiveAddressField(src);
    }
}

static bool
genericInstruction(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if (dest->mode != AM_FPUREG) {
        err_Error(MERROR_FPU_REGISTER_EXPECTED);
        return true;
    }

    sect_OutputConst16(FPU_INS | getEffectiveAddressField(src));
    sect_OutputConst16(getSourceSpecifier(sz, src) | ((dest->directRegister & 7) << 7) | opmode);
    return m68k_OutputExtensionWords(src);
}


static bool
possiblyUnaryInstruction(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if ((dest == NULL || dest->mode == AM_EMPTY) && src->mode == AM_FPUREG) {
        dest = src;
    }

    return genericInstruction(sz, src, dest, opmode);
}


static bool
handleFBcc(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if (sz == SIZE_DEFAULT) {
        sz = src->mode == AM_WORD ? SIZE_WORD : SIZE_LONG;
    }

    opmode |= 0x0080 | FPU_INS;

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


static bool
handleFDBcc(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    SExpression* expr = expr_CheckRange(expr_PcRelative(dest->outer.displacement, 0), -32768, 32767);

    if (expr != NULL) {
        sect_OutputConst16(FPU_INS | 0x0048 | (src->directRegister & 7));
        sect_OutputConst16(opmode);
        sect_OutputExpr16(expr);
        return true;
    }

    err_Error(ERROR_OPERAND_RANGE);
    return true;
}


static bool
handleMOVE(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if (dest->mode == AM_FPUREG) {
        return genericInstruction(sz, src, dest, 0x0000);
    } else if (src->mode == AM_FPUREG) {
        SExpression* kFactor;
        if (sz == SIZE_PACKED) {
            if (!parse_ExpectChar('{'))
                return false;

            if (lex_Current.token >= T_68K_REG_D0 && lex_Current.token <= T_68K_REG_D7) {
                sz = SIZE_PACKED_DYNAMIC;
                kFactor = expr_Const((lex_Current.token - T_68K_REG_D0) << 4);
                parse_GetToken();
            } else {
                kFactor = expr_CheckRange(parse_Expression(1), -64, 63);
            }

            if (kFactor == NULL) {
                err_Error(ERROR_INVALID_EXPRESSION);
                return false;
            }

            if (!parse_ExpectChar('}'))
                return false;
        } else {
            kFactor = expr_Const(0);
        }

        sect_OutputConst16(FPU_INS | getEffectiveAddressField(dest));
        sect_OutputExpr16(expr_Or(expr_Const(0x6000 | getSourceSpecifier(sz, src)), kFactor));
        return m68k_OutputExtensionWords(dest);
    }

    err_Error(ERROR_OPERAND);
    return false;
}


static bool
handleFMOVECR(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    SExpression* expr = 
        expr_Or(
            expr_Const(((dest->directRegister & 7) << 7) | 0x5C00),
            expr_CheckRange(src->immediateInteger, 0, 127));

    sect_OutputConst16(FPU_INS);
    sect_OutputExpr16(expr);

    return true;
}


static bool
getFpuRegister(uint16_t* outRegister) {
    if (lex_Current.token >= T_FPUREG_0 && lex_Current.token <= T_FPUREG_7) {
        *outRegister = (uint16_t) (lex_Current.token - T_FPUREG_0);
        parse_GetToken();
        return true;
    }

    return false;
}

static bool
getRegisterRange(uint16_t* outStart, uint16_t* outEnd) {
    if (getFpuRegister(outStart)) {
        if (lex_Current.token == T_OP_SUBTRACT) {
            parse_GetToken();
            if (!getFpuRegister(outEnd))
                return 0;
            return true;
        }
        *outEnd = *outStart;
        return true;
    }
    return false;
}


static bool
parseRegisterList(uint16_t* result) {
    uint16_t r;
    uint16_t start;
    uint16_t end;

    if (lex_Current.token == '#') {
        int32_t expr;
        parse_GetToken();
        expr = parse_ConstantExpression();
        if (expr >= 0 && expr <= 255) {
            *result = expr;
            return true;
        }
        return false;
    }

    r = 0;

    while (getRegisterRange(&start, &end)) {
        if (start > end) {
            err_Error(ERROR_OPERAND);
            return false;
        }

        while (start <= end)
            r |= 1 << start++;

        if (lex_Current.token != T_OP_DIVIDE) {
            *result = r;
            return true;
        }

        parse_GetToken();
    }

    return false;
}


static uint8_t
reverseBits(uint8_t bits) {
    uint8_t r = 0;
    int i;

    for (i = 0; i < 8; ++i)
        r |= (bits & 1 << i) ? 1 << (7 - i) : 0;

    return r;
}


static uint16_t
reverseRegisterList(uint16_t opmode) {
    if ((opmode & (3 << 11)) == (2 << 11)) {
        return (opmode & 0xFF00) | reverseBits((uint8_t)opmode);
    }

    return opmode;
}


static bool
handleFMOVEMMemoryDest(SAddressingMode* dest, uint16_t opmode) {
    if (parse_ExpectChar(',')) {
        uint32_t allowedModes = (AM_AIND | AM_ADEC | AM_ADISP | AM_AXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_WORD | AM_LONG);
        if (m68k_GetAddressingMode(dest, false) && ((dest->mode & allowedModes) != 0)) {
            if (dest->mode != AM_ADEC) {
                opmode |= 2 << 11;
            }
            opmode = reverseRegisterList(opmode);

            uint16_t firstWord = getEffectiveAddressField(dest) | FPU_INS;
            sect_OutputConst16(firstWord);
            sect_OutputConst16(opmode);
            m68k_OutputExtensionWords(dest);
            return true;
        } else {
            err_Error(ERROR_DEST_OPERAND);
        }
    }
    return false;
}


static bool
handleFMOVEMMemorySrc(SAddressingMode* src, uint16_t opmode) {
    uint32_t allowedModes = (AM_AIND | AM_AINC | AM_ADISP | AM_AXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_WORD | AM_LONG);
    if ((src->mode & allowedModes) != 0) {
        opmode = reverseRegisterList(opmode);
        uint16_t firstWord = getEffectiveAddressField(src) | FPU_INS;
        sect_OutputConst16(firstWord);
        sect_OutputConst16(opmode);
        m68k_OutputExtensionWords(src);
        return true;
    } else {
        err_Error(ERROR_SOURCE_OPERAND);
    }
    return false;
}


static bool
parseControlRegisterList(uint16_t* result) {
    uint16_t word = 0;

    for (;;) {
        switch (lex_Current.token) {
            case T_FPU_FPCR:
                word |= 1 << 12;
                break;
            case T_FPU_FPSR:
                word |= 1 << 11;
                break;
            case T_FPU_FPIAR:
                word |= 1 << 10;
                break;
            default:
                return false;
        }

        parse_GetToken();

        if (lex_Current.token != T_OP_DIVIDE) {
            *result = word;
            return true;
        }

        parse_GetToken();
    }
}


static bool
handleFMOVEMControl(SAddressingMode* addr, uint16_t opmode, uint16_t regList, ESize sz) {
    bool oneRegisterSelected = (regList & -regList) == regList;
    uint16_t firstWord = FPU_INS | getEffectiveAddressField(addr);

    opmode |= 0x8000 | regList;

    if ((sz != SIZE_LONG) && (sz != SIZE_DEFAULT))
        return err_Error(MERROR_INSTRUCTION_SIZE);

    if (addr->mode == AM_DREG && !oneRegisterSelected) {
        // if source is data register, only one destination register may be specified
        return err_Error(ERROR_OPERAND);
    }

    if (addr->mode == AM_AREG && regList != (1 << 10)) {
        // if source is address register, destination register must be FPIAR
        return err_Error(ERROR_OPERAND);
    }

    sect_OutputConst16(firstWord);
    sect_OutputConst16(opmode);
    m68k_OutputExtensionWords(addr);

    return true;
}


static bool
handleFMOVEM(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    uint16_t regList;

    if (lex_Current.token >= T_68K_REG_D0 && lex_Current.token <= T_68K_REG_D7) {
        // FMOVEM Dn,<ea>
        int reg = lex_Current.token - T_68K_REG_D0;
        opmode = 0xE000 | (reg << 4) | (1 << 11);
        parse_GetToken();

        if ((sz != SIZE_EXTENDED) && (sz != SIZE_DEFAULT))
            return err_Error(MERROR_INSTRUCTION_SIZE);

        return handleFMOVEMMemoryDest(dest, opmode);
    } else if (parseRegisterList(&regList)) {
        // FMOVEM register_list,<ea>
        opmode = 0xE000 | regList | (0 << 11);

        if ((sz != SIZE_EXTENDED) && (sz != SIZE_DEFAULT))
            return err_Error(MERROR_INSTRUCTION_SIZE);

        return handleFMOVEMMemoryDest(dest, opmode);
    } else if (parseControlRegisterList(&regList)) {
        // FMOVEM control_register_list,<ea>
        if (parse_ExpectChar(',') && m68k_GetAddressingMode(dest, false)) {
            return handleFMOVEMControl(dest, 1 << 13, regList, sz);
        }
    } else if (m68k_GetAddressingMode(src, false)) {
        if (parse_ExpectChar(',')) {
            if (lex_Current.token >= T_68K_REG_D0 && lex_Current.token <= T_68K_REG_D7) {
                // FMOVEM <ea>,Dn
                int reg = lex_Current.token - T_68K_REG_D0;
                opmode = 0xC000 | (reg << 4) | (3 << 11);
                parse_GetToken();

                if ((sz != SIZE_EXTENDED) && (sz != SIZE_DEFAULT))
                    return err_Error(MERROR_INSTRUCTION_SIZE);

                return handleFMOVEMMemorySrc(dest, opmode);
            } else if (parseRegisterList(&regList)) {
                // FMOVEM <ea>,register_list
                opmode = 0xC000 | regList | (2 << 11);

                if ((sz != SIZE_EXTENDED) && (sz != SIZE_DEFAULT))
                    return err_Error(MERROR_INSTRUCTION_SIZE);

                return handleFMOVEMMemorySrc(src, opmode);
            } else if (parseControlRegisterList(&regList)) {
                // FMOVEM <ea>,control_register_list
                return handleFMOVEMControl(src, 0 << 13, regList, sz);
            }
        }
    }

    return false;
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
        0x000F,
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
    {   // FCMP
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0038,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FCOS
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x001D,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FCOSH
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0019,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FDBF
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0000,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBEQ
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0001,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDOGT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0002,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDOGE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0003,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDOLT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0004,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDOLE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0005,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FBOGL
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0006,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBOR
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0007,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBUN
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0008,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBUEQ
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0009,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBUGT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x000A,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBUGE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x000B,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBULT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x000C,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBULE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x000D,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBNE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x000E,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x000F,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBSF
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0010,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBSEQ
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0011,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBGT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0012,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBGE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0013,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBLT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0014,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBLE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0015,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBGL
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0016,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBGLE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0017,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBNGLE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0018,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBNGL
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0019,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBNLE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x001A,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBNLT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x001B,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBNGE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x001C,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBNGT
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x001D,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBSNE
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x001E,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDBST
        FPUF_6888X | FPUF_68040,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x001F,
        AM_DREG,
        AM_WORD | AM_LONG,
        handleFDBcc
    },
    {   // FDIV
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0020,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FSDIV
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0060,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FDDIV
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0064,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FETOX
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0010,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FETOXM1
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0008,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FGETEXP
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x001E,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FGETMAN
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x001F,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FINT
        FPUF_6888X | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0001,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FINTRZ
        FPUF_6888X | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0003,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FLOG10
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0015,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FLOG2
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0016,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FLOGN
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0014,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FLOGNP1
        0,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0006,
        AM_FPU_SOURCE,
        AM_FPUREG,
        possiblyUnaryInstruction
    },
    {   // FMOD
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0021,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FMOVE
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0000,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_WORD | AM_LONG,
        handleMOVE
    },
    {   // FSMOVE
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0040,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FDMOVE
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0044,
        AM_FPU_SOURCE,
        AM_FPUREG,
        genericInstruction
    },
    {   // FMOVECR
        FPUF_6888X,
        SIZE_EXTENDED, SIZE_EXTENDED,
        0x0044,
        AM_IMM,
        AM_FPUREG,
        handleFMOVECR
    },
    {	// FMOVEM
        CPUF_ALL,
        SIZE_EXTENDED | SIZE_LONG | SIZE_DEFAULT, SIZE_DEFAULT,
        0x0000,
        AM_NONE, 
        AM_NONE,
        handleFMOVEM
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
