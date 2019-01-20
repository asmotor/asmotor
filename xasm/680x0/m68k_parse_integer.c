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

#include <assert.h>
#include <stdbool.h>

#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "project.h"

#include "m68k_errors.h"
#include "m68k_options.h"
#include "m68k_parse.h"
#include "m68k_tokens.h"

static bool
handleToken(ETargetToken token, ESize size, SAddressingMode* src, SAddressingMode* dest);

static int
getSizeField(ESize sz) {
    switch (sz) {
        case SIZE_BYTE:
            return 0x0;
        case SIZE_WORD:
            return 0x1;
        case SIZE_LONG:
            return 0x2;
        default:
            internalerror("Unknown size");
    }

    return -1;
}

static bool
outputOpcode(uint16_t opcode, SAddressingMode* addrMode) {
    sect_OutputConst16(opcode | parse_GetEAField(addrMode));
    return parse_OutputExtensionWords(addrMode);
}


static bool
outputOpcodeSize(uint16_t opcode, ESize size, SAddressingMode* src) {
    return outputOpcode((uint16_t) (opcode | getSizeField(size) << 6), src);
}

static bool
handleXBCD(uint16_t opcode, ESize size, SAddressingMode* src, SAddressingMode* dest) {
    if (src->mode != dest->mode)
        prj_Error(ERROR_OPERAND);

    if (src->mode == AM_ADEC) {
        opcode |= 0x0008;
        opcode |= (src->outer.baseRegister & 7u);
        opcode |= (dest->outer.baseRegister & 7u) << 9u;
    } else {
        opcode |= src->directRegister;
        opcode |= dest->directRegister << 9;
    }

    opcode |= getSizeField(size) << 6;

    sect_OutputConst16(opcode);

    return true;
}

static bool
handleABCD(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleXBCD(0xC100, size, src, dest);
}

static bool
handleSBCD(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleXBCD(0x8100, sz, src, dest);
}

static bool
handleADDX(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleXBCD(0xD100, sz, src, dest);
}

static bool
handleSUBX(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleXBCD(0x9100, sz, src, dest);
}

static bool
handleQuick(uint16_t ins, ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    src->immediate = expr_CheckRange(src->immediate, 1, 8);
    if (src->immediate == NULL) {
        prj_Error(ERROR_OPERAND_RANGE);
        return true;
    }

    ins |= (uint16_t) (parse_GetEAField(dest) | (getSizeField(sz) << 6));

    SExpression* expr = expr_Const(ins);
    expr = expr_Or(expr, expr_Asl(expr_And(src->immediate, expr_Const(7)), expr_Const(9)));

    sect_OutputExpr16(expr);
    return parse_OutputExtensionWords(dest);
}

static bool
handleADDQ(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleQuick(0x5000, sz, src, dest);
}

static bool
handleSUBQ(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleQuick(0x5100, size, src, dest);
}

static bool
handleArithmeticA(ESize size, SAddressingMode* src, SAddressingMode* dest, ETargetToken quick, uint16_t opcode) {
    if (src->mode == AM_IMM && expr_IsConstant(src->immediate) && src->immediate->value.integer >= 1
        && src->immediate->value.integer <= 8) {
        return handleToken(quick, size, src, dest);
    }

    opcode |= (uint16_t) (dest->directRegister << 9 | (size == SIZE_WORD ? 0x3 : 0x7) << 6);
    return outputOpcode(opcode, src);
}

static bool
handleADDA(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleArithmeticA(size, src, dest, T_68K_ADDQ, 0xD000);
}

static bool
handleSUBA(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleArithmeticA(size, src, dest, T_68K_SUBQ, 0x9000);
}

static bool
handleArithmeticLogicalI(uint16_t opcode, ESize size, SAddressingMode* src, SAddressingMode* dest) {
    opcode |= parse_GetEAField(dest);
    if (size == SIZE_BYTE) {
        opcode |= 0x0 << 6;
        sect_OutputConst16(opcode);
        sect_OutputExpr16(expr_And(parse_ExpressionCheck8Bit(src->immediate), expr_Const(0xFF)));
    } else if (size == SIZE_WORD) {
        opcode |= 0x1 << 6;
        sect_OutputConst16(opcode);
        sect_OutputExpr16(parse_ExpressionCheck16Bit(src->immediate));
    } else {
        opcode |= 0x2 << 6;
        sect_OutputConst16(opcode);
        sect_OutputExpr32(src->immediate);
    }

    return parse_OutputExtensionWords(dest);
}

static bool
handleArithmeticI(ESize size, SAddressingMode* src, SAddressingMode* dest, ETargetToken quick, uint16_t opcode) {
    if (src->mode == AM_IMM && expr_IsConstant(src->immediate) && src->immediate->value.integer >= 1
        && src->immediate->value.integer <= 8) {
        return handleToken(quick, size, src, dest);
    }

    return handleArithmeticLogicalI(opcode, size, src, dest);
}

static bool
handleADDI(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleArithmeticI(size, src, dest, T_68K_ADDQ, 0x0600);
}

static bool
handleSUBI(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleArithmeticI(size, src, dest, T_68K_SUBQ, 0x0400);
}

static bool
handleBitwiseI(ESize size, SAddressingMode* src, SAddressingMode* dest, uint16_t opcode) {
    if (dest->mode == AM_SYSREG) {
        if (dest->directRegister == T_68K_REG_CCR) {
            if (size != SIZE_BYTE) {
                prj_Error(MERROR_INSTRUCTION_SIZE);
                return true;
            }

            sect_OutputConst16(opcode | 0x003C);
            sect_OutputExpr16(expr_And(src->immediate, expr_Const(0xFF)));
            return true;
        } else if (dest->directRegister == T_68K_REG_SR) {
            if (size != SIZE_WORD) {
                prj_Error(MERROR_INSTRUCTION_SIZE);
                return true;
            }

            prj_Warn(MERROR_INSTRUCTION_PRIV);
            sect_OutputConst16(opcode | 0x007C);
            sect_OutputExpr16(src->immediate);
            return true;
        }
        prj_Error(ERROR_DEST_OPERAND);
        return true;
    }

    return handleArithmeticLogicalI(opcode, size, src, dest);
}

static bool
handleANDI(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleBitwiseI(size, src, dest, 0x0200);
}

static bool
handleEORI(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleBitwiseI(size, src, dest, 0x0A00);
}

static bool
handleORI(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleBitwiseI(size, src, dest, 0x0000);
}

static bool
handleArithmeticLogical(uint16_t opcode, ESize size, SAddressingMode* src, SAddressingMode* dest) {
    if (dest->mode == AM_DREG) {
        if (src->mode == AM_AREG && size != SIZE_WORD && size != SIZE_LONG) {
            prj_Error(MERROR_INSTRUCTION_SIZE);
            return true;
        }

        return outputOpcodeSize((uint16_t) (opcode | 0x0000 | dest->directRegister << 9), size, src);
    } else if (src->mode == AM_DREG) {
        return outputOpcodeSize((uint16_t) (opcode | 0x0100 | src->directRegister << 9), size, dest);
    }

    prj_Error(ERROR_OPERAND);
    return true;
}

static bool
handleArithmetic(ESize size, SAddressingMode* src, SAddressingMode* dest, ETargetToken address, ETargetToken immediate, uint16_t opcode) {
    if (dest->mode == AM_AREG)
        return handleToken(address, size, src, dest);

    if (src->mode == AM_IMM)
        return handleToken(immediate, size, src, dest);

    return handleArithmeticLogical(opcode, size, src, dest);
}

static bool
handleADD(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleArithmetic(size, src, dest, T_68K_ADDA, T_68K_ADDI, 0xD000);
}

static bool
handleSUB(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleArithmetic(size, src, dest, T_68K_SUBA, T_68K_SUBI, 0x9000);
}

static bool
handleCMPA(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    uint16_t opcode = (uint16_t) (0xB040 | dest->directRegister << 9 | (size == SIZE_WORD ? 0x3 : 0x7) << 6);
    return outputOpcode(opcode, src);
}

static bool
handleCMPI(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    uint16_t opcode = (uint16_t) (0x0C00 | getSizeField(size) << 6 | parse_GetEAField(dest));
    sect_OutputConst16(opcode);

    if (size == SIZE_BYTE) {
        SExpression* expr = parse_ExpressionCheck8Bit(src->immediate);
        if (expr == NULL) {
            prj_Error(ERROR_OPERAND_RANGE);
            return true;
        }
        sect_OutputExpr16(expr_And(expr, expr_Const(0xFF)));
    } else if (size == SIZE_WORD) {
        SExpression* expr = parse_ExpressionCheck16Bit(src->immediate);
        if (expr == NULL) {
            prj_Error(ERROR_OPERAND_RANGE);
            return true;
        }
        sect_OutputExpr16(expr);
    } else if (size == SIZE_WORD) {
        sect_OutputExpr32(src->immediate);
    }
    return parse_OutputExtensionWords(dest);
}

static bool
handleCMPM(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    uint16_t opcode = (uint16_t) (0xB108 | (dest->outer.baseRegister & 7u) << 9 | (src->outer.baseRegister & 7u) | getSizeField(size) << 6);
    sect_OutputConst16(opcode);
    return true;
}

static bool
handleCMP(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    if (src->mode == AM_AINC && dest->mode == AM_AINC)
        return handleToken(T_68K_CMPM, size, src, dest);

    if (dest->mode == AM_AREG)
        return handleToken(T_68K_CMPA, size, src, dest);

    if (src->mode == AM_IMM)
        return handleToken(T_68K_CMPI, size, src, dest);

    if (dest->mode == AM_DREG)
        return handleArithmeticLogical(0xB000, size, src, dest);

    prj_Fail(ERROR_DEST_OPERAND);
    return false;
}

static bool
handleAND(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    if (src->mode == AM_IMM || dest->mode == AM_SYSREG)
        return handleToken(T_68K_ANDI, size, src, dest);

    return handleArithmeticLogical(0xC000, size, src, dest);
}

static bool
handleCLR(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);
    return outputOpcodeSize(0x4200, size, src);
}

static bool
handleTST(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);

    if (src->mode == AM_AREG && size == SIZE_BYTE) {
        prj_Error(MERROR_INSTRUCTION_SIZE);
        return true;
    }

    return outputOpcodeSize(0x4A00, size, src);
}

static bool
handleShift(uint16_t opcode, uint16_t memoryOpcode, ESize size, SAddressingMode* src, SAddressingMode* dest) {
    if (dest->mode == AM_DREG) {
        opcode |= 0x0000 | getSizeField(size) << 6 | dest->directRegister;
        if (src->mode == AM_IMM) {
            SExpression* expr;
            expr = expr_CheckRange(src->immediate, 1, 8);
            expr = expr_And(expr, expr_Const(7));
            if (expr == NULL) {
                prj_Error(ERROR_OPERAND_RANGE);
                return true;
            }
            expr = expr_Or(expr_Const(opcode), expr_Asl(expr, expr_Const(9)));
            sect_OutputExpr16(expr);
        } else if (src->mode == AM_DREG) {
            opcode |= 0x0020 | src->directRegister << 9;
            sect_OutputConst16(opcode);
        }
        return true;
    }
    if (dest->mode != AM_EMPTY) {
        prj_Error(ERROR_DEST_OPERAND);
        return true;
    }
    if (src->mode & (AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_POSTINDAXD020 | AM_PREINDAXD020)) {
        if (size != SIZE_WORD) {
            prj_Error(MERROR_INSTRUCTION_SIZE);
            return true;
        }
        return outputOpcode(memoryOpcode, src);
    }

    prj_Error(ERROR_SOURCE_OPERAND);
    return true;
}

static bool
handleASL(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE100, 0xE1C0, size, src, dest);
}

static bool
handleASR(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE000, 0xE0C0, size, src, dest);
}

static bool
handleLSL(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE108, 0xE3C0, size, src, dest);
}

static bool
handleLSR(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE008, 0xE2C0, size, src, dest);
}

static bool
handleROL(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE118, 0xE7C0, size, src, dest);
}

static bool
handleROR(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE018, 0xE6C0, size, src, dest);
}

static bool
handleROXL(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE110, 0xE5C0, size, src, dest);
}

static bool
handleROXR(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleShift(0xE010, 0xE4C0, size, src, dest);
}

static bool
handleBcc(uint16_t opcode, ESize size, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);

    opcode = (uint16_t) 0x6000 | (opcode << 8);
    if (size == SIZE_BYTE) {
        SExpression* expr = expr_CheckRange(expr_PcRelative(src->outer.displacement, -2), -128, 127);
        if (expr != NULL) {
            expr = expr_And(expr, expr_Const(0xFF));
            expr = expr_Or(expr, expr_Const(opcode));
            sect_OutputExpr16(expr);
            return true;
        }

        prj_Error(ERROR_OPERAND_RANGE);
        return true;
    } else if (size == SIZE_WORD) {
        SExpression* expr = expr_CheckRange(expr_PcRelative(src->outer.displacement, 0), -32768, 32767);
        if (expr != NULL) {
            sect_OutputConst16(opcode);
            sect_OutputExpr16(expr);
            return true;
        }

        prj_Error(ERROR_OPERAND_RANGE);
        return true;
    } else if (size == SIZE_LONG) {
        SExpression* expr;

        if (opt_Current->machineOptions->cpu < CPUF_68020) {
            prj_Error(MERROR_INSTRUCTION_SIZE);
            return true;
        }

        expr = expr_PcRelative(src->outer.displacement, 0);
        sect_OutputConst16(opcode | 0xFFu);
        sect_OutputExpr32(expr);
        return true;
    }

    return true;
}

static bool
handleBRA(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x0, size, src, dest);
}

static bool
handleBSR(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x1, size, src, dest);
}

static bool
handleBHI(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x2, size, src, dest);
}

static bool
handleBLS(ESize size, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x3, size, src, dest);
}

static bool
handleBCC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x4, sz, src, dest);
}

static bool
handleBCS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x5, sz, src, dest);
}

static bool
handleBNE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x6, sz, src, dest);
}

static bool
handleBEQ(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x7, sz, src, dest);
}

static bool
handleBVC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x8, sz, src, dest);
}

static bool
handleBVS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0x9, sz, src, dest);
}

static bool
handleBPL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0xA, sz, src, dest);
}

static bool
handleBMI(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0xB, sz, src, dest);
}

static bool
handleBGE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0xC, sz, src, dest);
}

static bool
handleBLT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0xD, sz, src, dest);
}

static bool
handleBGT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0xE, sz, src, dest);
}

static bool
handleBLE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleBcc(0xF, sz, src, dest);
}

static bool
handleBitInstruction(uint16_t dataOpcode, uint16_t immediateOpcode, SAddressingMode* src, SAddressingMode* dest) {
    if (src->mode == AM_DREG) {
        dataOpcode |= src->directRegister << 9 | parse_GetEAField(dest);
        sect_OutputConst16(dataOpcode);
        return parse_OutputExtensionWords(dest);
    } else if (src->mode == AM_IMM) {
        SExpression* expr;

        immediateOpcode |= parse_GetEAField(dest);
        sect_OutputConst16(immediateOpcode);

        if (dest->mode == AM_DREG)
            expr = expr_CheckRange(src->immediate, 0, 31);
        else
            expr = expr_CheckRange(src->immediate, 0, 7);

        if (expr != NULL) {
            sect_OutputExpr16(expr);
            return parse_OutputExtensionWords(dest);
        }
        prj_Error(ERROR_OPERAND_RANGE);
    }
    return true;
}

static bool
handleBCHG(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitInstruction(0x0140, 0x0840, src, dest);
}

static bool
handleBCLR(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitInstruction(0x0180, 0x0880, src, dest);
}

static bool
handleBSET(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitInstruction(0x01C0, 0x08C0, src, dest);
}

static bool
handleBTST(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitInstruction(0x0100, 0x0800, src, dest);
}

static bool
handleBitfieldInstruction(uint16_t opcode, uint16_t extension, SAddressingMode* src) {
    SExpression* expr = expr_Const(extension);

    opcode |= parse_GetEAField(src);
    sect_OutputConst16(opcode);

    if (src->bitfieldOffsetRegister != -1) {
        expr = expr_Or(expr, expr_Const(0x0800 | src->bitfieldOffsetRegister << 6));
    } else {
        SExpression* bf = expr_CheckRange(src->bitfieldOffsetExpression, 0, 31);
        if (bf == NULL) {
            prj_Error(ERROR_OPERAND_RANGE);
            return true;
        }
        expr = expr_Or(expr, expr_Asl(bf, expr_Const(6)));
    }

    if (src->bitfieldWidthRegister != -1) {
        expr = expr_Or(expr, expr_Const(0x0020 | src->bitfieldWidthRegister));
    } else {
        SExpression* bf = expr_CheckRange(src->bitfieldWidthExpression, 0, 31);
        if (bf == NULL) {
            prj_Error(ERROR_OPERAND_RANGE);
            return true;
        }
        expr = expr_Or(expr, bf);
    }

    sect_OutputExpr16(expr);
    return parse_OutputExtensionWords(src);
}

static bool
handleSingleOpBitfieldInstruction(uint16_t opcode, SAddressingMode* src) {
    return handleBitfieldInstruction(opcode, 0, src);
}

static bool
handleBFCHG(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);
    return handleSingleOpBitfieldInstruction(0xEAC0, src);
}

static bool
handleBFCLR(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);
    return handleSingleOpBitfieldInstruction(0xECC0, src);
}

static bool
handleBFSET(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);
    return handleSingleOpBitfieldInstruction(0xEEC0, src);
}

static bool
handleBFTST(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);
    return handleSingleOpBitfieldInstruction(0xE8C0, src);
}

static bool
handleBFEXTS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitfieldInstruction(0xEBC0, (uint16_t) (dest->directRegister << 12), src);
}

static bool
handleBFEXTU(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitfieldInstruction(0xE9C0, (uint16_t) (dest->directRegister << 12), src);
}

static bool
handleBFFFO(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitfieldInstruction(0xEDC0, (uint16_t) (dest->directRegister << 12), src);
}

static bool
handleBFINS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleBitfieldInstruction(0xEFC0, (uint16_t) (src->directRegister << 12), dest);
}

static bool
handleBKPT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    SExpression* expr = expr_CheckRange(src->immediate, 0, 7);
    if (expr == NULL) {
        prj_Error(ERROR_OPERAND_RANGE);
        return true;
    }

    expr = expr_Or(expr, expr_Const(0x4848));
    sect_OutputExpr16(expr);
    return true;
}

static bool
handleCALLM(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);

    SExpression* expr = expr_CheckRange(src->immediate, 0, 255);
    if (expr == NULL) {
        prj_Error(ERROR_OPERAND_RANGE);
        return true;
    }

    sect_OutputConst16((uint16_t) 0x06C0 | parse_GetEAField(dest));
    sect_OutputExpr16(expr);
    return parse_OutputExtensionWords(dest);
}

static bool
handleCAS(ESize sz, SAddressingMode* dc, SAddressingMode* du) {
    if (opt_Current->machineOptions->cpu == CPUF_68060 && sz != SIZE_BYTE) {
        prj_Warn(MERROR_MISALIGNED_FAIL_68060);
        return true;
    }

    if (!parse_ExpectComma())
        return false;

    SAddressingMode ea;
    if (!parse_GetAddrMode(&ea))
        return false;

    if ((ea.mode & (AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020)) == 0) {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    uint16_t opcode;

    opcode = (uint16_t) (0x08C0 | (getSizeField(sz) + 1) << 9 | parse_GetEAField(&ea));
    sect_OutputConst16(opcode);

    opcode = (uint16_t) (0x0000 | du->directRegister << 6 | dc->directRegister);
    sect_OutputConst16(opcode);

    return parse_OutputExtensionWords(&ea);
}

static bool
expectDataRegister(uint16_t* outRegister) {
    if (lex_Current.token >= T_68K_REG_D0 && lex_Current.token <= T_68K_REG_D7) {
        *outRegister = (uint16_t) (lex_Current.token - T_68K_REG_D0);
        parse_GetToken();
        return true;
    }

    prj_Error(ERROR_OPERAND);
    return false;
}

static bool
expectIndirectRegister(uint16_t* outRegister) {
    if (lex_Current.token >= T_68K_REG_A0_IND && lex_Current.token <= T_68K_REG_A7_IND) {
        *outRegister = (uint16_t) (lex_Current.token - T_68K_REG_A0_IND + 8);
        parse_GetToken();
        return true;
    }

    if (!parse_ExpectChar('('))
        return false;

    if (!expectDataRegister(outRegister))
        return false;

    if (!parse_ExpectChar(')'))
        return false;

    return true;
}

static bool
handleCAS2(ESize sz, SAddressingMode* unused1, SAddressingMode* unused2) {
    assert(unused1 != NULL);
    assert(unused2 != NULL);

    uint16_t dc1, dc2, du1, du2, rn1, rn2;

    if (!expectDataRegister(&dc1))
        return false;

    if (!parse_ExpectChar(':'))
        return false;

    if (!expectDataRegister(&dc2))
        return false;

    if (!parse_ExpectComma())
        return false;

    if (!expectDataRegister(&du1))
        return false;

    if (!parse_ExpectChar(':'))
        return false;

    if (!expectDataRegister(&du2))
        return false;

    if (!parse_ExpectComma())
        return false;

    if (!expectIndirectRegister(&rn1))
        return false;

    if (!parse_ExpectChar(':'))
        return false;

    if (!expectIndirectRegister(&rn2))
        return false;

    if (opt_Current->machineOptions->cpu == CPUF_68060) {
        prj_Error(MERROR_INSTRUCTION_CPU);
        return true;
    }

    sect_OutputConst16(0x08FC | (uint16_t) (getSizeField(sz) + 1) << 9);
    sect_OutputConst16(rn1 << 12 | du1 << 6 | dc1);
    sect_OutputConst16(rn2 << 12 | du2 << 6 | dc2);

    return true;
}

static bool
handleCHK(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (sz == SIZE_LONG && opt_Current->machineOptions->cpu < CPUF_68020) {
        prj_Error(MERROR_INSTRUCTION_SIZE);
        return true;
    }

    uint16_t opcode = (uint16_t) (0x4000 | dest->directRegister << 9 | (sz == SIZE_WORD ? 0x3 : 0x2) << 7);
    return outputOpcode(opcode, src);
}

static bool
handleCHK2(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (opt_Current->machineOptions->cpu == CPUF_68060) {
        prj_Error(MERROR_INSTRUCTION_CPU);
        return true;
    }

    uint16_t opcode = (uint16_t) (0x00C0 | getSizeField(sz) << 9 | parse_GetEAField(src));
    sect_OutputConst16(opcode);

    opcode = (uint16_t) (0x0800 | dest->directRegister << 12);
    sect_OutputConst16(opcode);

    return parse_OutputExtensionWords(src);
}

static bool
handleCMP2(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (opt_Current->machineOptions->cpu == CPUF_68060) {
        prj_Error(MERROR_INSTRUCTION_CPU);
        return true;
    }

    uint16_t opcode = (uint16_t) (0x00C0 | getSizeField(sz) << 9 | parse_GetEAField(src));
    sect_OutputConst16(opcode);

    opcode = (uint16_t) (dest->directRegister << 12);
    if (dest->mode == AM_AREG)
        opcode |= 0x8000;
    sect_OutputConst16(opcode);
    return true;
}

static bool
handleDBcc(uint16_t code, SAddressingMode* src, SAddressingMode* dest) {
    code = (uint16_t) (0x50C8 | code << 8 | src->directRegister);
    sect_OutputConst16(code);
    sect_OutputExpr16(expr_PcRelative(dest->outer.displacement, 0));
    return true;
}

static bool
handleDBT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x0, src, dest);
}

static bool
handleDBF(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x1, src, dest);
}

static bool
handleDBHI(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x2, src, dest);
}

static bool
handleDBLS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x3, src, dest);
}

static bool
handleDBCC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x4, src, dest);
}

static bool
handleDBCS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x5, src, dest);
}

static bool
handleDBNE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x6, src, dest);
}

static bool
handleDBEQ(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x7, src, dest);
}

static bool
handleDBVC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x8, src, dest);
}

static bool
handleDBVS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0x9, src, dest);
}

static bool
handleDBPL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0xA, src, dest);
}

static bool
handleDBMI(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0xB, src, dest);
}

static bool
handleDBGE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0xC, src, dest);
}

static bool
handleDBLT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0xD, src, dest);
}

static bool
handleDBGT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0xE, src, dest);
}

static bool
handleDBLE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return handleDBcc(0xF, src, dest);
}

static bool
handleDIVxx(bool sign, bool l, ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (l || sz == SIZE_LONG) {
        bool div64;
        uint16_t dq, dr;
        if (lex_Current.token == ':') {
            uint16_t reg;
            parse_GetToken();
            if (!expectDataRegister(&reg))
                return false;

            dq = reg;
            dr = dest->directRegister;
            div64 = !l;
        } else {
            dq = dest->directRegister;
            dr = dq;
            div64 = false;
        }

        if (opt_Current->machineOptions->cpu >= CPUF_68060 && div64) {
            prj_Error(MERROR_INSTRUCTION_CPU);
            return true;
        }

        sect_OutputConst16((uint16_t) (0x4C40 | parse_GetEAField(src)));
        sect_OutputConst16((uint16_t) (sign << 11 | div64 << 10 | dq << 12 | dr));
        return parse_OutputExtensionWords(src);
    } else {
        uint16_t opcode = (uint16_t) (0x80C0 | sign << 8 | dest->directRegister << 9);
        return outputOpcode(opcode, src);
    }
}

static bool
handleDIVS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleDIVxx(true, false, sz, src, dest);
}

static bool
handleDIVSL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleDIVxx(true, true, sz, src, dest);
}

static bool
handleDIVU(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleDIVxx(false, false, sz, src, dest);
}

static bool
handleDIVUL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleDIVxx(false, true, sz, src, dest);
}

static bool
handleEOR(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (src->mode == AM_IMM || dest->mode == AM_SYSREG)
        return handleToken(T_68K_EORI, sz, src, dest);

    uint16_t opcode = (uint16_t) (0xB100 | src->directRegister << 9);
    return outputOpcodeSize(opcode, sz, dest);
}

static bool
handleEXG(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_LONG);

    uint16_t ins;
    uint16_t rx, ry;

    if (src->mode != dest->mode) {
        ins = 0x11 << 3;
        if (src->mode == AM_AREG) {
            rx = (uint16_t) dest->directRegister;
            ry = (uint16_t) src->directRegister;
        } else {
            rx = (uint16_t) src->directRegister;
            ry = (uint16_t) dest->directRegister;
        }
    } else {
        rx = (uint16_t) src->directRegister;
        ry = (uint16_t) dest->directRegister;
        if (src->mode == AM_DREG)
            ins = 0x08 << 3;
        else
            ins = 0x09 << 3;
    }

    sect_OutputConst16(ins | 0xC100 | rx << 9 | ry);
    return true;
}

static bool
handleEXT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);

    uint16_t ins = (uint16_t) (0x4800 | src->directRegister);
    if (sz == SIZE_WORD)
        ins |= 0x2 << 6;
    else
        ins |= 0x3 << 6;

    sect_OutputConst16(ins);
    return true;
}

static bool
handleEXTB(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_LONG);
    assert(dest != NULL);

    sect_OutputConst16((uint16_t) (0x4800 | src->directRegister | 0x7 << 6));
    return true;
}

static bool
handleILLEGAL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(src != NULL);
    assert(dest != NULL);

    sect_OutputConst16(0x4AFC);
    return true;
}

static bool
handleJMP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    return outputOpcode(0x4EC0, src);
}

static bool
handleJSR(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    return outputOpcode(0x4E80, src);
}

static bool
handleLEA(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    return outputOpcode((uint16_t) (0x41C0 | dest->directRegister << 9), src);
}

static bool
handleLINK(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (sz == SIZE_LONG && opt_Current->machineOptions->cpu < CPUF_68020) {
        prj_Error(MERROR_INSTRUCTION_SIZE);
        return true;
    }

    if (sz == SIZE_WORD) {
        sect_OutputConst16((uint16_t) (0x4E50 | src->directRegister));
        sect_OutputExpr16(dest->immediate);
        return true;
    } else /*if(sz == SIZE_LONG)*/ {
        sect_OutputConst16((uint16_t) (0x4808 | src->directRegister));
        sect_OutputExpr32(dest->immediate);
        return true;
    }
}

static bool
handleMOVEfromSYSREG(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (src->directRegister == T_68K_REG_USP) {
        prj_Warn(MERROR_INSTRUCTION_PRIV);

        if (sz != SIZE_LONG) {
            prj_Error(MERROR_INSTRUCTION_SIZE);
            return true;
        }

        if (dest->mode != AM_AREG) {
            prj_Error(ERROR_DEST_OPERAND);
            return true;
        }

        sect_OutputConst16((uint16_t) (0x4E68 | dest->directRegister));
        return true;
    } else {
        EAddrMode allow =
             AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG;

        if (opt_Current->machineOptions->cpu >= CPUF_68020)
            allow |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;

        if ((dest->mode & allow) == 0) {
            prj_Error(ERROR_DEST_OPERAND);
            return true;
        }

        if (src->directRegister == T_68K_REG_CCR) {
            if (opt_Current->machineOptions->cpu < CPUF_68010) {
                prj_Error(MERROR_INSTRUCTION_CPU);
                return true;
            }

            if (sz != SIZE_WORD) {
                prj_Error(MERROR_INSTRUCTION_SIZE);
                return true;
            }

            return outputOpcode(0x42C0, dest);
        } else if (src->directRegister == T_68K_REG_SR) {
            if (opt_Current->machineOptions->cpu >= CPUF_68010)
                prj_Warn(MERROR_INSTRUCTION_PRIV);

            if (sz != SIZE_WORD) {
                prj_Error(MERROR_INSTRUCTION_SIZE);
                return true;
            }

            return outputOpcode(0x40C0, dest);
        }
    }

    return true;
}

static bool
handleMOVEtoSYSREG(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (dest->directRegister == T_68K_REG_USP) {
        prj_Warn(MERROR_INSTRUCTION_PRIV);

        if (sz != SIZE_LONG) {
            prj_Error(MERROR_INSTRUCTION_SIZE);
            return true;
        }

        if (src->mode != AM_AREG) {
            prj_Error(ERROR_SOURCE_OPERAND);
            return true;
        }

        sect_OutputConst16((uint16_t) (0x4E60 | src->directRegister));
        return true;
    } else {
        EAddrMode allow =
            AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP;

        if (opt_Current->machineOptions->cpu >= CPUF_68020)
            allow |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020;

        if ((src->mode & allow) == 0) {
            prj_Error(ERROR_SOURCE_OPERAND);
            return true;
        }

        if (dest->directRegister == T_68K_REG_CCR) {
            if (sz != SIZE_WORD) {
                prj_Error(MERROR_INSTRUCTION_SIZE);
                return true;
            }

            return outputOpcode(0x44C0, src);
        } else if (dest->directRegister == T_68K_REG_SR) {
            if (sz != SIZE_WORD) {
                prj_Error(MERROR_INSTRUCTION_SIZE);
                return true;
            }

            prj_Warn(MERROR_INSTRUCTION_PRIV);

            return outputOpcode(0x46C0, src);
        }
    }

    return true;
}

static bool
handleMOVE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    uint16_t destea;
    uint16_t ins;

    if (src->mode == AM_IMM && dest->mode == AM_DREG && sz == SIZE_LONG && expr_IsConstant(src->immediate)
        && src->immediate->value.integer >= -128 && src->immediate->value.integer < 127) {
        return handleToken(T_68K_MOVEQ, sz, src, dest);
    }

    if (src->mode == AM_SYSREG)
        return handleMOVEfromSYSREG(sz, src, dest);

    if (dest->mode == AM_SYSREG)
        return handleMOVEtoSYSREG(sz, src, dest);

    if (dest->mode == AM_AREG)
        return handleToken(T_68K_MOVEA, sz, src, dest);

    destea = (uint16_t) parse_GetEAField(dest);
    ins = (uint16_t) parse_GetEAField(src);

    destea = (destea >> 3 | destea << 3) & 0x3F;

    ins |= destea << 6;
    if (sz == SIZE_BYTE)
        ins |= 0x1 << 12;
    else if (sz == SIZE_WORD)
        ins |= 0x3 << 12;
    else /*if(sz == SIZE_LONG)*/
        ins |= 0x2 << 12;

    sect_OutputConst16(ins);
    if (!parse_OutputExtensionWords(src))
        return false;
    return parse_OutputExtensionWords(dest);
}

static bool
handleMOVEA(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    uint16_t opcode = (uint16_t) (0x0040 | dest->directRegister << 9 | (sz == SIZE_WORD ? 0x3 : 0x2) << 12);
    return outputOpcode(opcode, src);
}

static bool
handleMOVE16(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);

    if (src->mode == AM_AINC && dest->mode == AM_AINC) {
        sect_OutputConst16((uint16_t) (0xF620 | (src->outer.baseRegister & 7u)));
        sect_OutputConst16((uint16_t) (0x8000 | (dest->outer.baseRegister & 7u) << 12));
        return true;
    }

    uint16_t opmode;
    SExpression* line;
    uint16_t reg;

    if (src->mode == AM_AINC && dest->mode == AM_LONG) {
        opmode = 0x0;
        line = dest->outer.displacement;
        reg = (uint16_t) (src->outer.baseRegister & 7u);
    } else if (src->mode == AM_LONG && dest->mode == AM_AINC) {
        opmode = 0x1;
        line = src->outer.displacement;
        reg = (uint16_t) (dest->outer.baseRegister & 7u);
    } else if (src->mode == AM_AIND && dest->mode == AM_LONG) {
        opmode = 0x2;
        line = dest->outer.displacement;
        reg = (uint16_t) (src->outer.baseRegister & 7u);
    } else if (src->mode == AM_LONG && dest->mode == AM_AIND) {
        opmode = 0x3;
        line = src->outer.displacement;
        reg = (uint16_t) (dest->outer.baseRegister & 7u);
    } else {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    sect_OutputConst16(0xF600 | opmode << 3 | reg);
    sect_OutputExpr32(line);
    return true;
}

static uint16_t
reverseBits(uint16_t bits) {
    uint16_t r = 0;
    int i;

    for (i = 0; i < 16; ++i)
        r |= (bits & 1 << i) ? 1 << (15 - i) : 0;

    return r;
}

static bool
handleMOVEM(ESize sz, SAddressingMode* unused1, SAddressingMode* unused2) {
    assert(unused1 != NULL);
    assert(unused2 != NULL);

    uint16_t direction;
    SAddressingMode mode;

    uint32_t registerMask = parse_RegisterList();
    if (registerMask != REGLIST_FAIL) {
        EAddrMode allowedModes = AM_AIND | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG;

        if (!parse_ExpectComma())
            return false;

        if (!parse_GetAddrMode(&mode))
            return false;

        if (opt_Current->machineOptions->cpu >= CPUF_68020)
            allowedModes |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;

        if ((mode.mode & allowedModes) == 0) {
            prj_Error(ERROR_DEST_OPERAND);
            return true;
        }
        direction = 0;
    } else {
        EAddrMode allowedModes = AM_AIND | AM_AINC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP;

        if (!parse_GetAddrMode(&mode))
            return false;

        if (opt_Current->machineOptions->cpu >= CPUF_68020)
            allowedModes |= AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020;

        if ((mode.mode & allowedModes) == 0) {
            prj_Error(ERROR_SOURCE_OPERAND);
            return true;
        }

        if (!parse_ExpectComma())
            return false;

        registerMask = parse_RegisterList();
        if (registerMask == REGLIST_FAIL)
            return false;

        direction = 1;
    }

    if (registerMask == 0) {
        prj_Warn(MERROR_MOVEM_SKIPPED);
        return true;
    }

    uint16_t opcode = (uint16_t) (0x4880 | direction << 10 | parse_GetEAField(&mode));
    if (sz == SIZE_LONG)
        opcode |= 1 << 6;

    sect_OutputConst16(opcode);
    if (mode.mode == AM_ADEC)
        registerMask = reverseBits((uint16_t) registerMask);
    sect_OutputConst16((uint16_t) registerMask);
    return parse_OutputExtensionWords(&mode);
}

static bool
handleMOVEP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (opt_Current->machineOptions->cpu == CPUF_68060) {
        prj_Error(MERROR_INSTRUCTION_CPU);
        return true;
    }

    if (src->mode == AM_AIND) {
        src->mode = AM_ADISP;
        src->outer.displacement = NULL;
    }

    if (dest->mode == AM_AIND) {
        dest->mode = AM_ADISP;
        dest->outer.displacement = NULL;
    }

    uint16_t dr;
    uint16_t ar;
    uint16_t opmode;
    SExpression* disp;

    if (src->mode == AM_ADISP && dest->mode == AM_DREG) {
        if (sz == SIZE_WORD)
            opmode = 0x4;
        else
            opmode = 0x5;

        dr = (uint16_t) dest->directRegister;
        ar = (uint16_t) (src->outer.baseRegister & 7u);
        disp = src->outer.displacement;
    } else if (src->mode == AM_DREG && dest->mode == AM_ADISP) {
        if (sz == SIZE_WORD)
            opmode = 0x6;
        else
            opmode = 0x7;

        dr = (uint16_t) src->directRegister;
        ar = (uint16_t) (dest->outer.baseRegister & 7u);
        disp = dest->outer.displacement;
    } else {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    sect_OutputConst16(0x0008 | dr << 9 | opmode << 6 | ar);
    if (disp != NULL)
        sect_OutputExpr16(disp);
    else
        sect_OutputConst16(0);

    return true;
}

static bool
handleMOVEQ(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_LONG);

    SExpression* expr = expr_CheckRange(src->immediate, -128, 127);
    if (expr == NULL) {
        prj_Error(ERROR_OPERAND_RANGE);
        return true;
    }

    expr = expr_And(expr, expr_Const(0xFF));
    expr = expr_Or(expr, expr_Const(0x7000 | dest->directRegister << 9));
    sect_OutputExpr16(expr);
    return true;
}

static bool
handleMULx(uint16_t sign, ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (sz == SIZE_LONG && opt_Current->machineOptions->cpu < CPUF_68020) {
        prj_Error(MERROR_INSTRUCTION_CPU);
        return true;
    }

    if (sz == SIZE_LONG) {
        uint16_t dh, dl, mul64;
        if (lex_Current.token == ':') {
            parse_GetToken();
            if (!expectDataRegister(&dl))
                return false;
            dh = dest->directRegister;
            mul64 = 1;
            if (dh == dl)
                prj_Warn(MERROR_UNDEFINED_RESULT);

            if (opt_Current->machineOptions->cpu == CPUF_68060) {
                prj_Error(MERROR_INSTRUCTION_CPU);
                return true;
            }
        } else {
            dl = dest->directRegister;
            dh = 0;
            mul64 = 0;
        }

        sect_OutputConst16((uint16_t) (0x4C00 | parse_GetEAField(src)));
        sect_OutputConst16(0x0000 | sign << 11 | mul64 << 10 | dl << 12 | dh);
        return parse_OutputExtensionWords(src);
    } else {
        return outputOpcode((uint16_t) (0xC0C0 | sign << 8 | dest->directRegister << 9), src);
    }
}

static bool
handleMULS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleMULx(true, sz, src, dest);
}

static bool
handleMULU(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleMULx(false, sz, src, dest);
}

static bool
handleNBCD(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_BYTE);
    assert(dest != NULL);

    return outputOpcode((uint16_t) 0x4800, src);
}

static bool
handleNEG(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);

    return outputOpcodeSize((uint16_t) 0x4400, sz, src);
}

static bool
handleNEGX(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);

    return outputOpcodeSize((uint16_t) 0x4000, sz, src);
}

static bool
handleNOP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(src != NULL);
    assert(dest != NULL);

    sect_OutputConst16(0x4E71);
    return true;
}

static bool
handleNOT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);

    return outputOpcodeSize((uint16_t) 0x4600, sz, src);
}

static bool
handleOR(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    if (src->mode == AM_IMM || dest->mode == AM_SYSREG)
        return handleToken(T_68K_ORI, sz, src, dest);

    return handleArithmeticLogical(0x8000, sz, src, dest);
}

static bool
handlePackUnpack(uint16_t ins, ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);

    if (!parse_ExpectComma())
        return false;

    SAddressingMode adj;
    if (!parse_GetAddrMode(&adj))
        return false;

    if (src->mode != dest->mode || adj.mode != AM_IMM) {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    uint16_t dy, dx;
    uint16_t rm;

    if (src->mode == AM_DREG) {
        dx = (uint16_t) src->directRegister;
        dy = (uint16_t) dest->directRegister;
        rm = 0;
    } else {
        dx = (uint16_t) (src->outer.baseRegister & 7u);
        dy = (uint16_t) (dest->outer.baseRegister & 7u);
        rm = 1;
    }

    sect_OutputConst16(ins | dy << 9 | rm << 3 | dx);
    sect_OutputExpr16(adj.immediate);
    return true;
}

static bool
handlePACK(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handlePackUnpack(0x8140, sz, src, dest);
}

static bool
handleUNPACK(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handlePackUnpack(0x8180, sz, src, dest);
}

static bool
handlePEA(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_LONG);
    assert(dest != NULL);

    return outputOpcode((uint16_t) 0x4840, src);
}

static bool
handleRTD(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    sect_OutputConst16(0x4E74);
    sect_OutputExpr16(src->immediate);
    return true;
}

static bool
handleRTM(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    uint16_t reg;

    if (src->mode == AM_DREG)
        reg = (uint16_t) src->directRegister;
    else /* if(src->eMode == AM_AREG) */
        reg = (uint16_t) (src->directRegister + 8);

    sect_OutputConst16((uint16_t) 0x06C0 | reg);
    return true;
}

static bool
handleRTR(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(src != NULL);
    assert(dest != NULL);

    sect_OutputConst16(0x4E77);
    return true;
}

static bool
handleRTS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(src != NULL);
    assert(dest != NULL);

    sect_OutputConst16(0x4E75);
    return true;
}

static bool
handleScc(uint16_t code, ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_BYTE);
    assert(dest != NULL);

    return outputOpcode((uint16_t) (0x50C0 | code << 8), src);
}

static bool
handleST(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x0, sz, src, dest);
}

static bool
handleSF(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x1, sz, src, dest);
}

static bool
handleSHI(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x2, sz, src, dest);
}

static bool
handleSLS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x3, sz, src, dest);
}

static bool
handleSCC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x4, sz, src, dest);
}

static bool
handleSCS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x5, sz, src, dest);
}

static bool
handleSNE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x6, sz, src, dest);
}

static bool
handleSEQ(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x7, sz, src, dest);
}

static bool
handleSVC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x8, sz, src, dest);
}

static bool
handleSVS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0x9, sz, src, dest);
}

static bool
handleSPL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0xA, sz, src, dest);
}

static bool
handleSMI(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0xB, sz, src, dest);
}

static bool
handleSGE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0xC, sz, src, dest);
}

static bool
handleSLT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0xD, sz, src, dest);
}

static bool
handleSGT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0xE, sz, src, dest);
}

static bool
handleSLE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleScc(0xF, sz, src, dest);
}

static bool
handleSWAP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_WORD);
    assert(dest != NULL);

    sect_OutputConst16((uint16_t) (0x4840 | src->directRegister));
    return true;
}

static bool
handleTAS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_BYTE);
    assert(dest != NULL);

    return outputOpcode(0x4AC0, src);
}

static bool
handleTRAP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    SExpression* expr = expr_CheckRange(src->immediate, 0, 15);
    if (expr == NULL) {
        prj_Error(ERROR_OPERAND_RANGE);
        return true;
    }
    expr = expr_Or(expr, expr_Const(0x4E40));
    sect_OutputExpr16(expr);
    return true;
}

static bool
handleTRAPcc(uint16_t code, ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(dest != NULL);

    uint16_t opmode;

    if (sz == SIZE_DEFAULT && src->mode == AM_EMPTY) {
        opmode = 0x4;
    } else if (sz == SIZE_WORD && src->mode == AM_IMM) {
        opmode = 0x2;
    } else if (sz == SIZE_LONG && src->mode == AM_IMM) {
        opmode = 0x3;
    } else {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    sect_OutputConst16(0x50F8 | opmode | code << 8);
    if (sz == SIZE_WORD)
        sect_OutputExpr16(src->immediate);
    else if (sz == SIZE_LONG)
        sect_OutputExpr32(src->immediate);

    return true;
}

static bool
handleTRAPT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x0, sz, src, dest);
}

static bool
handleTRAPF(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x1, sz, src, dest);
}

static bool
handleTRAPHI(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x2, sz, src, dest);
}

static bool
handleTRAPLS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x3, sz, src, dest);
}

static bool
handleTRAPCC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x4, sz, src, dest);
}

static bool
handleTRAPCS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x5, sz, src, dest);
}

static bool
handleTRAPNE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x6, sz, src, dest);
}

static bool
handleTRAPEQ(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x7, sz, src, dest);
}

static bool
handleTRAPVC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x8, sz, src, dest);
}

static bool
handleTRAPVS(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0x9, sz, src, dest);
}

static bool
handleTRAPPL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0xA, sz, src, dest);
}

static bool
handleTRAPMI(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0xB, sz, src, dest);
}

static bool
handleTRAPGE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0xC, sz, src, dest);
}

static bool
handleTRAPLT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0xD, sz, src, dest);
}

static bool
handleTRAPGT(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0xE, sz, src, dest);
}

static bool
handleTRAPLE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleTRAPcc(0xF, sz, src, dest);
}

static bool
handleTRAPV(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(src != NULL);
    assert(dest != NULL);

    sect_OutputConst16(0x4E76);
    return true;
}

static bool
handleUNLK(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    sect_OutputConst16((uint16_t) (0x4E58 | src->directRegister));
    return true;
}

static bool
handleRESET(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(src != NULL);
    assert(dest != NULL);

    prj_Warn(MERROR_INSTRUCTION_PRIV);
    sect_OutputConst16(0x4E70);
    return true;
}

static bool
handleRTE(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(src != NULL);
    assert(dest != NULL);

    prj_Warn(MERROR_INSTRUCTION_PRIV);
    sect_OutputConst16(0x4E73);
    return true;
}

static bool
handleSTOP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);
    assert(dest != NULL);

    prj_Warn(MERROR_INSTRUCTION_PRIV);
    sect_OutputConst16(0x4E72);
    sect_OutputExpr16(src->immediate);
    return true;
}

static bool
handleCache040(uint16_t ins, uint16_t scope, ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_DEFAULT);

    prj_Warn(MERROR_INSTRUCTION_PRIV);

    uint16_t cache = 0;

    if (src->directRegister == T_68K_REG_DC)
        cache = 0x1;
    else if (src->directRegister == T_68K_REG_IC)
        cache = 0x2;
    else if (src->directRegister == T_68K_REG_BC)
        cache = 0x3;
    else {
        prj_Error(ERROR_DEST_OPERAND);
        return true;
    }

    uint16_t reg;

    if (scope == 3)
        reg = 0;
    else
        reg = (uint16_t) (dest->outer.baseRegister & 7u);

    sect_OutputConst16(ins | scope << 3 | cache << 6 | reg);
    return true;
}

static bool
handleCINVA(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleCache040(0xF400, 0x3, sz, src, dest);
}

static bool
handleCINVL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleCache040(0xF400, 0x1, sz, src, dest);
}

static bool
handleCINVP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleCache040(0xF400, 0x2, sz, src, dest);
}

static bool
handleCPUSHA(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleCache040(0xF420, 0x3, sz, src, dest);
}

static bool
handleCPUSHL(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleCache040(0xF420, 0x1, sz, src, dest);
}

static bool
handleCPUSHP(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    return handleCache040(0xF420, 0x2, sz, src, dest);
}

typedef struct {
    uint16_t cpu;
    uint16_t code;
} SControlRegister;

SControlRegister g_controlRegister[] = {
    {// SFC
        CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x000
    },
    {// DFC
        CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x001
    },
    {// USP
        CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x800
    },
    {// VBR
        CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x801
    },
    {// CACR
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x002
    },
    {// CAAR
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x802
    },
    {// MSP
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x803
    },
    {// ISP
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        0x804
    },
    {// TC
        CPUF_68040 | CPUF_68060,
        0x003
    },
    {// ITT0
        CPUF_68040 | CPUF_68060,
        0x004
    },
    {// ITT1
        CPUF_68040 | CPUF_68060,
        0x005
    },
    {// DTT0
        CPUF_68040 | CPUF_68060,
        0x006
    },
    {// DTT1
        CPUF_68040 | CPUF_68060,
        0x007
    },
    {// MMUSR
        CPUF_68040 | CPUF_68060,
        0x805
    },
    {// URP
        CPUF_68040 | CPUF_68060,
        0x806
    },
    {// SRP
        CPUF_68040 | CPUF_68060,
        0x807
    },
    {// IACR0
        CPUF_68040,
        0x004
    },
    {// IACR1
        CPUF_68040,
        0x005
    },
    {// DACR0
        CPUF_68040,
        0x006
    },
    {// DACR1
        CPUF_68040,
        0x007
    },
};

static bool
handleMOVEC(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    assert(sz == SIZE_LONG);

    prj_Warn(MERROR_INSTRUCTION_PRIV);

    uint16_t dr;
    int control;
    uint16_t reg;

    if (src->mode == AM_SYSREG && (dest->mode == AM_DREG || dest->mode == AM_AREG)) {
        dr = 0;
        control = src->directRegister;
        if (dest->mode == AM_DREG)
            reg = (uint16_t) dest->directRegister;
        else
            reg = (uint16_t) dest->directRegister + 8;
    } else if (dest->mode == AM_SYSREG && (src->mode == AM_DREG || src->mode == AM_AREG)) {
        dr = 1;
        control = dest->directRegister;
        if (src->mode == AM_DREG)
            reg = (uint16_t) src->directRegister;
        else
            reg = (uint16_t) src->directRegister + 8;
    } else {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    if (control < T_68K_REG_SFC || control > T_68K_REG_DACR1) {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    SControlRegister* ctrlRegister = &g_controlRegister[control - T_68K_REG_SFC];
    if ((ctrlRegister->cpu & opt_Current->machineOptions->cpu) == 0) {
        prj_Error(MERROR_INSTRUCTION_CPU);
        return true;
    }

    sect_OutputConst16(0x4E7A | dr);
    sect_OutputConst16(reg << 12 | ctrlRegister->code);
    return true;
}

static bool
handleMOVES(ESize sz, SAddressingMode* src, SAddressingMode* dest) {
    prj_Warn(MERROR_INSTRUCTION_PRIV);

    int allowedModes = AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_PCXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020;
    uint16_t direction;
    SAddressingMode* addrMode;
    SAddressingMode* registerMode;

    if ((src->mode == AM_DREG || src->mode == AM_AREG) && (dest->mode & allowedModes)) {
        addrMode = dest;
        registerMode = src;
        direction = 1;
    } else if ((dest->mode == AM_DREG || dest->mode == AM_AREG) && (src->mode & allowedModes)) {
        addrMode = src;
        registerMode = dest;
        direction = 0;
    } else {
        prj_Error(ERROR_OPERAND);
        return true;
    }

    uint16_t reg = (uint16_t) registerMode->directRegister;
    if (registerMode->mode == AM_AREG)
        reg += 8;

    sect_OutputConst16((uint16_t) (0x0E00 | getSizeField(sz) << 6 | parse_GetEAField(addrMode)));
    sect_OutputConst16(reg << 12 | direction << 11);
    return parse_OutputExtensionWords(addrMode);
}

static SInstruction
g_integerInstructions[] = {
    {	// ABCD
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
        AM_NONE, AM_NONE,
        handleABCD
    },
    {	// ADD
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for adda */ | AM_AREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleADD
    },
    {	// ADDA
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_AREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_NONE,
        handleADDA
    },
    {	// ADDI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleADDI
    },
    {	// ADDQ
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleADDQ
    },
    {	// ADDX
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
        AM_NONE, AM_NONE,
        handleADDX
    },
    {	// AND
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for ANDI */ | AM_SYSREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleAND
    },
    {	// ANDI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleANDI
    },
    {	// ASL
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleASL
    },
    {	// ASR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | (uint32_t)AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleASR
    },
    {	// BCC
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBCC
    },
    {	// BCS
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBCS
    },
    {	// BEQ
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBEQ
    },
    {	// BGE
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBGE
    },
    {	// BGT
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBGT
    },
    {	// BHI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBHI
    },
    {	// BLE
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBLE
    },
    {	// BLS
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBLS
    },
    {	// BLT
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBLT
    },
    {	// BMI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBMI
    },
    {	// BNE
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBNE
    },
    {	// BPL
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBPL
    },
    {	// BVC
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBVC
    },
    {	// BVS
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBVS
    },
    {	// BCHG
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleBCHG
    },
    {	// BCLR
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleBCLR
    },
    {	// BFCHG
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD, AM_NONE,
        AM_NONE, AM_NONE,
        handleBFCHG
    },
    {	// BFCLR
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD, AM_NONE,
        AM_NONE, AM_NONE,
        handleBFCLR
    },
    {	// BFEXTS
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, AM_DREG,
        AM_NONE, AM_NONE,
        handleBFEXTS
    },
    {	// BFEXTU
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, AM_DREG,
        AM_NONE, AM_NONE,
        handleBFEXTU
    },
    {	// BFFFO
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, AM_DREG,
        AM_NONE, AM_NONE,
        handleBFFFO
    },
    {	// BFINS
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD,
        AM_NONE, AM_NONE,
        handleBFINS
    },
    {	// BFSET
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_BITFIELD, AM_NONE,
        AM_NONE, AM_NONE,
        handleBFSET
    },
    {	// BFTST
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020 | AM_BITFIELD, AM_NONE,
        AM_NONE, AM_NONE,
        handleBFTST
    },
    {	// BKPT
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleBKPT
    },
    {	// BRA
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBRA
    },
    {	// BSET
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleBSET
    },
    {	// BSR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_LONG, AM_NONE,
        AM_NONE, AM_NONE,
        handleBSR
    },
    {	// BTST
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_IMM, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleBTST
    },
    {	// CALLM
        CPUF_68020,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_IMM, AM_AIND | AM_ADISP | AM_AXDISP | AM_PCDISP | AM_PCXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020,
        AM_NONE, AM_NONE,
        handleCALLM
    },
    {	// CAS
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG, AM_DREG,
        AM_NONE, AM_NONE,
        handleCAS
    },
    {	// CAS2
        CPUF_68020 | CPUF_68030 | CPUF_68040,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleCAS2
    },
    {	// CHK
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_NONE,
        handleCHK
    },
    {	// CHK2
        CPUF_68020 | CPUF_68030 | CPUF_68040,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_DREG | AM_AREG,
        AM_NONE, AM_NONE,
        handleCHK2
    },
    {	// CINVA
        CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_SYSREG, AM_NONE,
        AM_NONE, AM_NONE,
        handleCINVA
    },
    {	// CINVL
        CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_SYSREG, AM_AIND,
        AM_NONE, AM_NONE,
        handleCINVL
    },
    {	// CINVP
        CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_SYSREG, AM_AIND,
        AM_NONE, AM_NONE,
        handleCINVP
    },
    {	// CLR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleCLR
    },
    {	// CMP
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG /* for CMPA */ | AM_AREG /* for CMPM */ | AM_AINC /* for CMPI */ | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_NONE,
        handleCMP
    },
    {	// CMPA
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_AREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_NONE,
        handleCMPA
    },
    {	// CMPI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020,
        handleCMPI
    },
    {	// CMPM
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_AINC, AM_AINC,
        AM_NONE, AM_NONE,
        handleCMPM
    },
    {	// CMP2
        CPUF_68020 | CPUF_68030 | CPUF_68040,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_DREG | AM_AREG,
        AM_NONE, AM_NONE,
        handleCMP2
    },
    {	// CPUSHA
        CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_SYSREG, AM_NONE,
        AM_NONE, AM_NONE,
        handleCPUSHA
    },
    {	// CPUSHL
        CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_SYSREG, AM_AIND,
        AM_NONE, AM_NONE,
        handleCPUSHL
    },
    {	// CPUSHP
        CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_SYSREG, AM_AIND,
        AM_NONE, AM_NONE,
        handleCPUSHP
    },
    {	// DBCC
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBCC
    },
    {	// DBCS
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBCS
    },
    {	// DBEQ
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBEQ
    },
    {	// DBF
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBF
    },
    {	// DBGE
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBGE
    },
    {	// DBGT
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBGT
    },
    {	// DBHI
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBHI
    },
    {	// DBLE
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBLE
    },
    {	// DBLS
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBLS
    },
    {	// DBLT
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBLT
    },
    {	// DBMI
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBMI
    },
    {	// DBNE
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBNE
    },
    {	// DBPL
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBPL
    },
    {	// DBT
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBT
    },
    {	// DBVC
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBVC
    },
    {	// DBVS
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG, AM_WORD | AM_LONG,
        AM_NONE, AM_NONE,
        handleDBVS
    },
    {	// DIVS
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleDIVS
    },
    {	// DIVSL
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_LONG, SIZE_LONG,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleDIVSL
    },
    {	// DIVU
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleDIVU
    },
    {	// DIVUL
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_LONG, SIZE_LONG,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, AM_DREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleDIVUL
    },
    {	// EOR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleEOR
    },
    {	// EORI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleEORI
    },
    {	// EXG
        CPUF_ALL,
        SIZE_LONG, SIZE_LONG,
        AM_DREG | AM_AREG, AM_DREG | AM_AREG,
        AM_NONE, AM_NONE,
        handleEXG
    },
    {	// EXT
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG, AM_NONE,
        AM_NONE, AM_NONE,
        handleEXT
    },
    {	// EXTB
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_LONG, SIZE_LONG,
        AM_DREG, AM_NONE,
        AM_NONE, AM_NONE,
        handleEXTB
    },
    {	// ILLEGAL
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleILLEGAL
    },
    {	// JMP
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleJMP
    },
    {	// JSR
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleJSR
    },
    {	// LEA
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, AM_AREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleLEA
    },
    {	// LINK
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_AREG, AM_IMM,
        AM_NONE, AM_NONE,
        handleLINK
    },
    {	// LSL
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleLSL
    },
    {	// LSR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleLSR
    },
    {	// MOVE
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_SYSREG | AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for movea */ | AM_AREG /* for move to ccr */ | AM_SYSREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleMOVE
    },
    {	// MOVEA
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_AREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ AM_NONE,
        handleMOVEA
    },
    {	// MOVEC
        CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_LONG, SIZE_LONG,
        AM_DREG | AM_AREG | AM_SYSREG, AM_DREG | AM_AREG | AM_SYSREG,
        AM_NONE, AM_NONE,
        handleMOVEC
    },
    {	// MOVE16
        CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_AIND | AM_AINC | AM_LONG, AM_AIND | AM_AINC | AM_LONG,
        AM_NONE, AM_NONE,
        handleMOVE16
    },
    {	// MOVEM
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleMOVEM
    },
    {	// MOVEP
        CPUF_68000 | CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_ADISP, AM_DREG | AM_AIND | AM_ADISP,
        AM_NONE, AM_NONE,
        handleMOVEP
    },
    {	// MOVEQ
        CPUF_ALL,
        SIZE_LONG, SIZE_LONG,
        AM_IMM, AM_DREG,
        AM_NONE, AM_NONE,
        handleMOVEQ
    },
    {	// MOVES
        CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_LONG | SIZE_WORD | SIZE_BYTE, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_PCXDISP | AM_WORD | AM_LONG, AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_PCXDISP | AM_WORD | AM_LONG,
        AM_PCXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_PCXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleMOVES
    },
    {	// MULS
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_DREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ AM_NONE,
        handleMULS
    },
    {	// MULU
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /* dest */ AM_DREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /* dest */ AM_NONE,
        handleMULU
    },
    {	// NBCD
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleNBCD
    },
    {	// NEG
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleNEG
    },
    {	// NEGX
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleNEGX
    },
    {	// NOP
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleNOP
    },
    {	// NOT
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleNOT
    },
    {	// OR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for ORI */ | AM_SYSREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleOR
    },
    {	// ORI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_SYSREG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleORI
    },
    {	// PACK
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
        AM_NONE, AM_NONE,
        handlePACK
    },
    {	// PEA
        CPUF_ALL,
        SIZE_LONG, SIZE_LONG,
        AM_AIND | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_PCDISP | AM_PCXDISP, AM_NONE,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handlePEA
    },
    {	// RESET
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleRESET
    },
    {	// ROL
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleROL
    },
    {	// ROR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleROR
    },
    {	// ROXL
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleROXL
    },
    {	// ROXR
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM | AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_DREG | AM_EMPTY,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        handleROXR
    },
    {	// RTD
        CPUF_68010 | CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleRTD
    },
    {	// RTE
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleRTE
    },
    {	// RTM
        CPUF_68020,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_AREG, AM_NONE,
        AM_NONE, AM_NONE,
        handleRTM
    },
    {	// RTR
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleRTR
    },
    {	// RTS
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleRTS
    },
    {	// SBCD
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
        AM_NONE, AM_NONE,
        handleSBCD
    },
    {	// SCC
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSCC
    },
    {	// SCS
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSCS
    },
    {	// SEQ
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSEQ
    },
    {	// SF
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSF
    },
    {	// SGE
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSGE
    },
    {	// SGT
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSGT
    },
    {	// SHI
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSHI
    },
    {	// SLE
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSLE
    },
    {	// SLS
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSLS
    },
    {	// SLT
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSLT
    },
    {	// SMI
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSMI
    },
    {	// SNE
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSNE
    },
    {	// SPL
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSPL
    },
    {	// ST
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleST
    },
    {	// SVC
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSVC
    },
    {	// SVS
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleSVS
    },

    {	// STOP
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleSTOP
    },
    {	// SUB
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG /* for adda */ | AM_AREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleSUB
    },
    {	// SUBA
        CPUF_ALL,
        SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_IMM | AM_PCDISP | AM_PCXDISP, /*dest*/ AM_AREG,
        AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, /*dest*/ AM_NONE,
        handleSUBA
    },
    {	// SUBI
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleSUBI
    },
    {	// SUBQ
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_IMM, AM_DREG | AM_AREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG,
        AM_NONE, AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020,
        handleSUBQ
    },
    {	// SUBX
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
        AM_NONE, AM_NONE,
        handleSUBX
    },
    {	// SWAP
        CPUF_ALL,
        SIZE_WORD, SIZE_WORD,
        AM_DREG, AM_NONE,
        AM_NONE, AM_NONE,
        handleSWAP
    },
    {	// TAS
        CPUF_ALL,
        SIZE_BYTE, SIZE_BYTE,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020, AM_NONE,
        AM_NONE, AM_NONE,
        handleTAS
    },
    {	// TRAP
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAP
    },

    {	// TRAPCC
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPCC
    },
    {	// TRAPCS
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPCS
    },
    {	// TRAPEQ
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPEQ
    },
    {	// TRAPF
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPF
    },
    {	// TRAPGE
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPGE
    },
    {	// TRAPGT
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPGT
    },
    {	// TRAPHI
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPHI
    },
    {	// TRAPLE
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPLE
    },
    {	// TRAPLS
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPLS
    },
    {	// TRAPLT
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPLT
    },
    {	// TRAPMI
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPMI
    },
    {	// TRAPNE
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPNE
    },
    {	// TRAPPL
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPPL
    },
    {	// TRAPT
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPT
    },
    {	// TRAPVC
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPVC
    },
    {	// TRAPVS
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_WORD | SIZE_LONG, SIZE_DEFAULT,
        AM_EMPTY | AM_IMM, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPVS
    },
    {	// TRAPV
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_NONE, AM_NONE,
        AM_NONE, AM_NONE,
        handleTRAPV
    },

    {	// TST
        CPUF_ALL,
        SIZE_BYTE | SIZE_WORD | SIZE_LONG, SIZE_WORD,
        AM_DREG | AM_AIND | AM_AINC | AM_ADEC | AM_ADISP | AM_AXDISP | AM_WORD | AM_LONG, AM_NONE,
        AM_IMM | AM_PCDISP | AM_PCXDISP | AM_AXDISP020 | AM_PREINDAXD020 | AM_POSTINDAXD020 | AM_PCXDISP020 | AM_PREINDPCXD020 | AM_POSTINDPCXD020, AM_NONE,
        handleTST
    },
    {	// UNLK
        CPUF_ALL,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_AREG, AM_NONE,
        AM_NONE, AM_NONE,
        handleUNLK
    },
    {	// UNPACK
        CPUF_68020 | CPUF_68030 | CPUF_68040 | CPUF_68060,
        SIZE_DEFAULT, SIZE_DEFAULT,
        AM_DREG | AM_ADEC, AM_DREG | AM_ADEC,
        AM_NONE, AM_NONE,
        handleUNPACK
    },
};

static bool
handleToken(ETargetToken token, ESize size, SAddressingMode* src, SAddressingMode* dest) {
    SInstruction* instruction = &g_integerInstructions[token - T_68K_INTEGER_FIRST];
    return parse_OpCore(instruction, size, src, dest);
}

bool
parse_IntegerInstruction(void) {
    if (lex_Current.token < T_68K_INTEGER_FIRST || lex_Current.token > T_68K_INTEGER_LAST) {
        return false;
    }

    int op = lex_Current.token - T_68K_INTEGER_FIRST;
    parse_GetToken();

    SInstruction* instruction = &g_integerInstructions[op];
    if ((instruction->cpu & opt_Current->machineOptions->cpu) == 0) {
        prj_Error(MERROR_INSTRUCTION_CPU);
        return true;
    }

    return parse_CommonCpuFpu(instruction);
}
