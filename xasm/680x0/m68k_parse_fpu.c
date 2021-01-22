/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

// 68080 - FTRAPcc


#define TRAPcc(opmode) \
    {   \
        FPUF_6888X | FPUF_68040SP | FPUF_68060SP,    \
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,    \
        opmode, \
        AM_IMM | AM_EMPTY,  \
        AM_EMPTY,   \
        false,  \
        handleFTRAPcc   \
    }

#define FScc(opmode)    \
    {   \
        FPUF_6888X | FPUF_68040,    \
        SIZE_BYTE | SIZE_DEFAULT, SIZE_DEFAULT, \
        opmode, \
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP,  \
        AM_EMPTY,    \
        false,  \
        handleFScc  \
    }

#define FDBcc(opmode)   \
    {   \
        FPUF_6888X | FPUF_68040,    \
        SIZE_DEFAULT, SIZE_DEFAULT, \
        opmode, \
        AM_DREG,    \
        AM_WORD | AM_LONG,  \
        false,  \
        handleFDBcc \
    }

#define FBcc(opmode) \
    {   \
        FPUF_ALL,   \
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,    \
        opmode, \
        AM_WORD | AM_LONG,  \
        AM_NONE,    \
        false,  \
        handleFBcc  \
    }

#define TRANS(opmode) \
    {   \
        FPUF_6888X, \
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED, \
        opmode, \
        AM_FPU_SOURCE,  \
        AM_EMPTY | AM_FPUREG,   \
        false,  \
        possiblyUnaryInstruction    \
    }

static uint16_t
getSourceSpecifier(ESize sz, SAddressingMode* addr) {
    if (addr->mode == AM_FPUREG) {
        return addr->directRegister << 10;
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

    if (src->mode == AM_DREG && sz != SIZE_BYTE && sz != SIZE_WORD && sz != SIZE_LONG && sz != SIZE_SINGLE) {
        err_Error(ERROR_SOURCE_OPERAND);
        return true;
    }

    if (src->mode == AM_FPUREG && sz != SIZE_EXTENDED) {
        err_Error(ERROR_SOURCE_OPERAND);
        return true;
    }

    sect_OutputConst16(FPU_INS | getEffectiveAddressField(src));
    sect_OutputConst16(getSourceSpecifier(sz, src) | (dest->directRegister << 7) | opmode);
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
unaryInstruction(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    dest->mode = AM_FPUREG;
    dest->directRegister = 0;

    return genericInstruction(sz, src, dest, opmode);
}


static bool
handleFBcc(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    SExpression* offset = expr_PcRelative(src->outer.displacement, -2);

    if (sz == SIZE_DEFAULT) {
        if (offset->isConstant && offset->value.integer >= -32768 && offset->value.integer <= 32767) {
            sz = SIZE_WORD;
        } else {
            sz = src->mode == AM_WORD ? SIZE_WORD : SIZE_LONG;
        }
    }

    opmode |= 0x0080 | FPU_INS;

    if (sz == SIZE_WORD) {
        SExpression* expr = expr_CheckRange(offset, -32768, 32767);

        if (expr != NULL) {
            sect_OutputConst16(opmode);
            sect_OutputExpr16(expr);
            return true;
        }

        err_Error(ERROR_OPERAND_RANGE);
        return true;
    } else /*if (sz == SIZE_LONG)*/ {
        opmode |= 0x0040;

        sect_OutputConst16(opmode);
        sect_OutputExpr32(offset);
        return true;
    }
}


static bool
handleFDBcc(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    SExpression* expr = expr_CheckRange(expr_PcRelative(dest->outer.displacement, -4), -32768, 32767);

    if (expr != NULL) {
        sect_OutputConst16(FPU_INS | 0x0048 | src->directRegister);
        sect_OutputConst16(opmode);
        sect_OutputExpr16(expr);
        return true;
    }

    err_Error(ERROR_OPERAND_RANGE);
    return true;
}


static bool
handleFScc(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    sect_OutputConst16(FPU_INS | 0x0040 | getEffectiveAddressField(src));
    sect_OutputConst16(opmode);
    m68k_OutputExtensionWords(src);

    return true;
}


static bool handleMOVEControlRegister(ESize sz, SAddressingMode* ea, uint16_t opmode) {
    if (sz != SIZE_LONG && sz != SIZE_DEFAULT) {
        err_Error(MERROR_INSTRUCTION_SIZE);
        return true;
    }

    sect_OutputConst16(FPU_INS | getEffectiveAddressField(ea));
    sect_OutputConst16(opmode);
    m68k_OutputExtensionWords(ea);

    return true;
}

static bool
handleMOVE(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if (dest->mode == AM_FPUCR) {
        return handleMOVEControlRegister(sz, src, 0x8000 | (0 << 13) | (1 << (dest->directRegister + 10)));
    } else if (src->mode == AM_FPUCR) {
        return handleMOVEControlRegister(sz, dest, 0x8000 | (1 << 13) | (1 << (src->directRegister + 10)));
    } else if (dest->mode == AM_FPUREG) {
        return genericInstruction(sz, src, dest, 0x0000);
    } else if (src->mode == AM_FPUREG) {
        SExpression* kFactor;
        opmode = 0x6000;

        if (sz == SIZE_PACKED) {
            if (!parse_ExpectChar('{'))
                return false;

            if (lex_Context->token.id >= T_68K_REG_D0 && lex_Context->token.id <= T_68K_REG_D7) {
                sz = SIZE_PACKED_DYNAMIC;
                kFactor = expr_Const((lex_Context->token.id - T_68K_REG_D0) << 4);
                opmode |= 0x1000;
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
        sect_OutputExpr16(expr_Or(expr_Const(opmode | (src->directRegister << 7) | getSourceSpecifier(sz, src)), kFactor));
        return m68k_OutputExtensionWords(dest);
    }

    err_Error(ERROR_OPERAND);
    return false;
}


static bool
handleFMOVECR(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    SExpression* expr = 
        expr_Or(
            expr_Const((dest->directRegister << 7) | 0x5C00),
            expr_CheckRange(src->immediateInteger, 0, 127));

    sect_OutputConst16(FPU_INS);
    sect_OutputExpr16(expr);

    return true;
}


static bool
getFpuRegister(uint16_t* outRegister) {
    if (lex_Context->token.id >= T_FPUREG_0 && lex_Context->token.id <= T_FPUREG_7) {
        *outRegister = (uint16_t) (lex_Context->token.id - T_FPUREG_0);
        parse_GetToken();
        return true;
    }

    return false;
}

static bool
getRegisterRange(uint16_t* outStart, uint16_t* outEnd) {
    if (getFpuRegister(outStart)) {
        if (lex_Context->token.id == T_OP_SUBTRACT) {
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

    if (lex_Context->token.id == '#') {
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

        if (lex_Context->token.id != T_OP_DIVIDE) {
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
        switch (lex_Context->token.id) {
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

        if (lex_Context->token.id != T_OP_DIVIDE) {
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

    if (lex_Context->token.id >= T_68K_REG_D0 && lex_Context->token.id <= T_68K_REG_D7) {
        // FMOVEM Dn,<ea>
        int reg = lex_Context->token.id - T_68K_REG_D0;
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
            if (lex_Context->token.id >= T_68K_REG_D0 && lex_Context->token.id <= T_68K_REG_D7) {
                // FMOVEM <ea>,Dn
                int reg = lex_Context->token.id - T_68K_REG_D0;
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


static bool
handleFNOP(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    sect_OutputConst16(FPU_INS | 0x0080);
    sect_OutputConst16(0x0000);
    return true;
}


static bool
handleSingleWordInstruction(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    err_Warn(MERROR_INSTRUCTION_PRIV);
    sect_OutputConst16(FPU_INS | opmode | getEffectiveAddressField(src));
    return true;
}


static bool
handleFSINCOS(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    if (parse_ExpectComma()) {
        if (lex_Context->token.id >= T_FPUREG_0 && lex_Context->token.id <= T_FPUREG_7) {
            uint16_t fps = lex_Context->token.id - T_FPUREG_0;
            uint16_t fpc = dest->directRegister;

            parse_GetToken();

            dest->directRegister = fps;

            return genericInstruction(sz, src, dest, fpc | 0x0030);
        }
    }
    return false;
}


static bool
handleFTRAPcc(ESize sz, SAddressingMode* src, SAddressingMode* dest, uint16_t opmode) {
    uint16_t firstWord = FPU_INS | 0x0078;
    bool outputWord = false;
    bool outputLong = false;

    if (sz == SIZE_DEFAULT && src->mode == AM_EMPTY) {
        firstWord |= 0x0004;
    } else if (src->mode == AM_IMM) {
        if (sz == SIZE_DEFAULT) {
            if (src->immediateInteger->isConstant) {
                if (src->immediateInteger->value.integer >= 0 && src->immediateInteger->value.integer <= 65535) {
                    sz = SIZE_WORD;
                } else {
                    sz = SIZE_LONG;
                }
            }
        }
        if (sz == SIZE_WORD) {
            firstWord |= 0x0002;
            outputWord = true;
        } else if (sz == SIZE_LONG) {
            firstWord |= 0x0003;
            outputLong = true;
        }
    } else {
        err_Error(ERROR_OPERAND);
        return true;
    }

    sect_OutputConst16(firstWord);
    sect_OutputConst16(opmode);

    if (outputWord) {
        sect_OutputExpr16(expr_CheckRange(src->immediateInteger, 0, 65535));
    } else if (outputLong) {
        sect_OutputExpr32(src->immediateInteger);
    }
    
    return true;
}


static SInstruction
s_FpuInstructions[] = {
    {   // FABS
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0018,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },
    {   // FSABS
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0058,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },
    {   // FDABS
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x005C,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },

    TRANS(0x001C),  // FACOS

    {   // FADD
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0022,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FSADD
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0062,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FDADD
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0066,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },

    TRANS(0x000C),  // FASIN
    TRANS(0x000F),  // FATAN
    TRANS(0x000D),  // FATANH

    FBcc(0x0000),    // FBF
    FBcc(0x0001),    // FBEQ
    FBcc(0x0002),    // FBOGT
    FBcc(0x0003),    // FBOGE
    FBcc(0x0004),    // FBOLT
    FBcc(0x0005),    // FBOLE
    FBcc(0x0006),    // FBOGL
    FBcc(0x0007),    // FBOR
    FBcc(0x0008),    // FBUN
    FBcc(0x0009),    // FBUEQ
    FBcc(0x000A),    // FBUGT
    FBcc(0x000B),    // FBUGE
    FBcc(0x000C),    // FBULT
    FBcc(0x000D),    // FBULE
    FBcc(0x000E),    // FBNE
    FBcc(0x000F),    // FBT
    FBcc(0x0010),    // FBSF
    FBcc(0x0011),    // FBSEQ
    FBcc(0x0012),    // FBGT
    FBcc(0x0013),    // FBGE
    FBcc(0x0014),    // FBLT
    FBcc(0x0015),    // FBLE
    FBcc(0x0016),    // FBGL
    FBcc(0x0017),    // FBGLE
    FBcc(0x0018),    // FBNGLE
    FBcc(0x0019),    // FBNGL
    FBcc(0x001A),    // FBNLE
    FBcc(0x001B),    // FBNLT
    FBcc(0x001C),    // FBNGE
    FBcc(0x001D),    // FBNGT
    FBcc(0x001E),    // FBSNE
    FBcc(0x001F),    // FBST

    {   // FCMP
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0038,
        AM_FPU_SOURCE,
        AM_FPUREG,
        false,
        genericInstruction
    },

    TRANS(0x001D),  // FCOS
    TRANS(0x0019),  // FCOSH

    FDBcc(0x0000),   // FDBF
    FDBcc(0x0001),   // FDBEQ
    FDBcc(0x0002),   // FDOGT
    FDBcc(0x0003),   // FDOGE
    FDBcc(0x0004),   // FDOLT
    FDBcc(0x0005),   // FDOLE
    FDBcc(0x0006),   // FBOGL
    FDBcc(0x0007),   // FDBOR
    FDBcc(0x0008),   // FDBUN
    FDBcc(0x0009),   // FDBUEQ
    FDBcc(0x000A),   // FDBUGT
    FDBcc(0x000B),   // FDBUGE
    FDBcc(0x000C),   // FDBULT
    FDBcc(0x000D),   // FDBULE
    FDBcc(0x000E),   // FDBNE
    FDBcc(0x000F),   // FDBT
    FDBcc(0x0010),   // FDBSF
    FDBcc(0x0011),   // FDBSEQ
    FDBcc(0x0012),   // FDBGT
    FDBcc(0x0013),   // FDBGE
    FDBcc(0x0014),   // FDBLT
    FDBcc(0x0015),   // FDBLE
    FDBcc(0x0016),   // FDBGL
    FDBcc(0x0017),   // FDBGLE
    FDBcc(0x0018),   // FDBNGLE
    FDBcc(0x0019),   // FDBNGL
    FDBcc(0x001A),   // FDBNLE
    FDBcc(0x001B),   // FDBNLT
    FDBcc(0x001C),   // FDBNGE
    FDBcc(0x001D),   // FDBNGT
    FDBcc(0x001E),   // FDBSNE
    FDBcc(0x001F),   // FDBST

    {   // FDIV
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0020,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FSDIV
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0060,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FDDIV
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0064,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FETOX
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0010,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FETOXM1
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0008,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FGETEXP
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x001E,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FGETMAN
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x001F,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FINT
        FPUF_6888X | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0001,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FINTRZ
        FPUF_6888X | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0003,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FLOG10
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0015,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FLOG2
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0016,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FLOGN
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0014,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FLOGNP1
        0,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0006,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FMOD
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0021,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FMOVE
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0000,
        AM_FPU_SOURCE | AM_FPUCR,
        AM_FPUCR | AM_FPUREG | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_WORD | AM_LONG,
        false,
        handleMOVE
    },
    {   // FSMOVE
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0040,
        AM_FPU_SOURCE,
        AM_FPUREG,
        false,
        genericInstruction
    },
    {   // FDMOVE
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0044,
        AM_FPU_SOURCE,
        AM_FPUREG,
        false,
        genericInstruction
    },
    {   // FMOVECR
        FPUF_6888X,
        SIZE_EXTENDED, SIZE_EXTENDED,
        0x0044,
        AM_IMM,
        AM_FPUREG,
        false,
        handleFMOVECR
    },
    {	// FMOVEM
        CPUF_ALL,
        SIZE_EXTENDED | SIZE_LONG | SIZE_DEFAULT, SIZE_DEFAULT,
        0x0000,
        AM_NONE, 
        AM_NONE,
        false,
        handleFMOVEM
    },
    {   // FMUL
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0023,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FSMUL
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0063,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FDMUL
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0067,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FNEG
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x001A,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FSNEG
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x005A,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FDNEG
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x005E,
        AM_FPU_SOURCE,
        AM_FPUREG | AM_EMPTY,
        false,
        possiblyUnaryInstruction
    },
    {   // FNOP
        FPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        0x0000,
        AM_NONE,
        AM_NONE,
        false,
        handleFNOP
    },
    {   // FREM
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0025,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FRESTORE
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0140,
        AM_AIND | AM_AINC | AM_ADISP | AM_AXDISP | AM_AXDISP020 | AM_POSTINDAXD020 | AM_PREINDAXD020 | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020,
        AM_NONE,
        false,
        handleSingleWordInstruction
    },
    {   // FSAVE
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0100,
        AM_AIND | AM_ADEC | AM_ADISP | AM_AXDISP | AM_AXDISP020 | AM_POSTINDAXD020 | AM_PREINDAXD020 | AM_WORD | AM_LONG,
        AM_NONE,
        false,
        handleSingleWordInstruction
    },
    {   // FSCALE
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0026,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    FScc(0x0000), // FSF
    FScc(0x0001), // FSEQ
    FScc(0x0002), // FSOGT
    FScc(0x0003), // FSOGE
    FScc(0x0004), // FSOLT
    FScc(0x0005), // FSOLE
    FScc(0x0006), // FSOGL
    FScc(0x0007), // FSOR
    FScc(0x0008), // FSUN
    FScc(0x0009), // FSUEQ
    FScc(0x000A), // FSUGT
    FScc(0x000B), // FSUGE
    FScc(0x000C), // FSULT
    FScc(0x000D), // FSULE
    FScc(0x000E), // FSNE
    FScc(0x000F), // FST
    FScc(0x0010), // FSSF
    FScc(0x0011), // FSSEQ
    FScc(0x0012), // FSGT
    FScc(0x0013), // FSGE
    FScc(0x0014), // FSLT
    FScc(0x0015), // FSLE
    FScc(0x0016), // FSGL
    FScc(0x0017), // FSGLE
    FScc(0x0018), // FSNGLE
    FScc(0x0019), // FSNGL
    FScc(0x001A), // FSNLE
    FScc(0x001B), // FSNLT
    FScc(0x001C), // FSNGE
    FScc(0x001D), // FSNGT
    FScc(0x001E), // FSSNE
    FScc(0x001F), // FSST

    {   // FSGLDIV
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0024,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FSGLMUL
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0027,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },

    TRANS(0x000E),  // FSIN

    {   // FSINCOS
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x000E,
        AM_FPU_SOURCE,
        AM_FPUREG,
        false,
        handleFSINCOS
    },

    TRANS(0x0002),  // FSINH

    {   // FSQRT
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0004,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },
    {   // FSSQRT
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0041,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },
    {   // FDSQRT
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0045,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },
    {   // FSUB
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0028,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FSSUB
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0064,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },
    {   // FDSUB
        FPUF_68040 | FPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x006C,
        AM_FPU_SOURCE,
        AM_FPUREG,
        true,
        genericInstruction
    },

    TRANS(0x000A),  // FTAN
    TRANS(0x0009),  // FTANH

    {   // FTENTOX
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0012,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },

    TRAPcc(0x0000),  // FTRAPF
    TRAPcc(0x0001),  // FTRAPEQ
    TRAPcc(0x0002),  // FTRAPOGT
    TRAPcc(0x0003),  // FTRAPOGE
    TRAPcc(0x0004),  // FTRAPOLT
    TRAPcc(0x0005),  // FTRAPOLE
    TRAPcc(0x0006),  // FTRAPOGL
    TRAPcc(0x0007),  // FTRAPOR
    TRAPcc(0x0008),  // FTRAPUN
    TRAPcc(0x0009),  // FTRAPUEQ
    TRAPcc(0x000A),  // FTRAPUGT
    TRAPcc(0x000B),  // FTRAPUGE
    TRAPcc(0x000C),  // FTRAPULT
    TRAPcc(0x000D),  // FTRAPULE
    TRAPcc(0x000E),  // FTRAPNE
    TRAPcc(0x000F),  // FTRAPT

    {   // FTST
        FPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x003A,
        AM_FPU_SOURCE,
        AM_NONE,
        false,
        unaryInstruction
    },
    {   // FTOTOX
        FPUF_6888X,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG | SIZE_SINGLE | SIZE_DOUBLE | SIZE_EXTENDED | SIZE_PACKED, SIZE_EXTENDED,
        0x0011,
        AM_FPU_SOURCE,
        AM_EMPTY | AM_FPUREG,
        false,
        possiblyUnaryInstruction
    },
};


bool
m68k_ParseFpuInstruction(void) {
    SInstruction* instruction;

    if(lex_Context->token.id < T_FPU_FIRST
    || lex_Context->token.id > T_FPU_LAST)
    {
        return false;
    }

    EToken token = lex_Context->token.id;
    int tokenOp = lex_Context->token.id - T_FPU_FIRST;
    parse_GetToken();

    instruction = &s_FpuInstructions[tokenOp];
    if((instruction->cpu & opt_Current->machineOptions->fpu) == 0)
    {
        err_Error(MERROR_INSTRUCTION_FPU);
        return true;
    }

    return m68k_ParseCommonCpuFpu(instruction, token, true);
}
