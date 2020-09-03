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

#include <stdlib.h>
#include <string.h>

#include "asmotor.h"

#include "xasm.h"
#include "expression.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "errors.h"
#include "section.h"

#include "m68k_errors.h"
#include "m68k_options.h"
#include "m68k_parse.h"
#include "m68k_tokens.h"

static uint32_t
enable020Modes(uint32_t modes) {
    uint32_t modes020 = modes & (AM_IMM_WHEN20 | AM_PCDISP_WHEN20 | AM_PCXDISP_WHEN20);
    if (modes020 != 0) {
        if (modes020 & AM_IMM_WHEN20)
            modes |= AM_IMM;
        if (modes020 & AM_PCDISP_WHEN20)
            modes |= AM_PCDISP;
        if (modes020 & AM_PCXDISP_WHEN20)
            modes |= AM_PCXDISP;
    }

    return modes;
}

static bool
getBitfield(SAddressingMode* mode) {
    if (parse_ExpectChar('{')) {
        mode->hasBitfield = true;

        if (lex_Current.token >= T_68K_REG_D0 && lex_Current.token <= T_68K_REG_D7) {
            mode->bitfieldOffsetRegister = lex_Current.token - T_68K_REG_D0;
            mode->bitfieldOffsetExpression = NULL;
            parse_GetToken();
        } else {
            mode->bitfieldOffsetExpression = parse_Expression(4);
            if (mode->bitfieldOffsetExpression == NULL) {
                err_Error(ERROR_OPERAND);
                return false;
            }
            mode->bitfieldOffsetRegister = -1;
        }

        if (!parse_ExpectChar(':'))
            return false;

        if (lex_Current.token >= T_68K_REG_D0 && lex_Current.token <= T_68K_REG_D7) {
            mode->bitfieldWidthRegister = lex_Current.token - T_68K_REG_D0;
            mode->bitfieldWidthExpression = NULL;
            parse_GetToken();
        } else {
            mode->bitfieldWidthExpression = parse_Expression(4);
            if (mode->bitfieldWidthExpression == NULL) {
                err_Error(ERROR_OPERAND);
                return true;
            }
            mode->bitfieldWidthRegister = -1;
        }

        return parse_ExpectChar('}');
    }

    return false;
}

bool
m68k_OutputExtensionWords(SAddressingMode* mode) {
    switch (mode->mode) {
        case AM_IMM: {
            switch (mode->immediateSize) {
                case SIZE_BYTE: {
                    mode->immediateInteger = m68k_ExpressionCheck8Bit(mode->immediateInteger);
                    if (mode->immediateInteger) {
                        sect_OutputExpr16(mode->immediateInteger);
                        return true;
                    }
                    return false;
                }
                default:
                case SIZE_WORD: {
                    mode->immediateInteger = m68k_ExpressionCheck16Bit(mode->immediateInteger);
                    if (mode->immediateInteger) {
                        sect_OutputExpr16(mode->immediateInteger);
                        return true;
                    }
                    return false;
                }
                case SIZE_LONG: {
                    if (mode->immediateInteger) {
                        sect_OutputExpr32(mode->immediateInteger);
                        return true;
                    }
                    return false;
                }
                case SIZE_SINGLE: {
                    sect_OutputFloat32(mode->immediateFloat);
                    return true;
                }
                case SIZE_DOUBLE: {
                    sect_OutputFloat64(mode->immediateFloat);
                    return true;
                }
                case SIZE_EXTENDED: {
                    err_Error(ERROR_EXTENDED_PRECISION_UNSUPPORTED);
                    return true;
                }
            }
        }
        case AM_WORD: {
            if (mode->outer.displacement) {
                sect_OutputExpr16(mode->outer.displacement);
                return true;
            }
            internalerror("no word");
            break;
        }
        case AM_LONG: {
            if (mode->outer.displacement) {
                sect_OutputExpr32(mode->outer.displacement);
                return true;
            }
            internalerror("no long");
            break;
        }
        case AM_PCDISP: {
            if (mode->outer.displacement)
                mode->outer.displacement = expr_PcRelative(mode->outer.displacement, 0);
        }
        // fall through
        case AM_ADISP: {
            if (mode->outer.displacement) {
                if (mode->outer.displacementSize == SIZE_WORD || mode->outer.displacementSize == SIZE_DEFAULT) {
                    sect_OutputExpr16(mode->outer.displacement);
                    return true;
                }
                err_Error(MERROR_DISP_SIZE);
                return false;
            }
            internalerror("no displacement word");
            break;
        }
        case AM_PCXDISP: {
            if (mode->outer.displacement)
                mode->outer.displacement = expr_PcRelative(mode->outer.displacement, 0);
        }
        // fall through
        case AM_AXDISP: {
            uint16_t ins = (uint16_t) (mode->outer.indexRegister << 12u);
            if (mode->outer.indexSize == SIZE_LONG)
                ins |= 0x0800;

            SExpression* expr;
            if (mode->outer.displacement != NULL)
                expr = m68k_ExpressionCheck8Bit(mode->outer.displacement);
            else
                expr = expr_Const(0);

            expr = expr_And(expr, expr_Const(0xFF));
            if (expr != NULL) {
                expr = expr_Or(expr, expr_Const(ins));
                if (mode->outer.indexScale != NULL) {
                    expr = expr_Or(expr, expr_Asl(mode->outer.indexScale, expr_Const(9)));
                }
                sect_OutputExpr16(expr);
                return true;
            }
            return false;
        }

        case AM_DREG:
        case AM_AREG:
        case AM_FPUREG:
        case AM_AIND:
        case AM_AINC:
        case AM_ADEC:
            return true;

        case AM_PCXDISP020: {
            if (mode->outer.displacement)
                mode->outer.displacement = expr_PcRelative(mode->outer.displacement, 2);
        }
        // fall through
        case AM_AXDISP020: {
            uint16_t ins = 0x0100;
            SExpression* expr;

            if (mode->outer.baseRegister == REG_NONE) {
                ins |= 0x0080;
            }

            if (mode->outer.indexRegister == REG_NONE) {
                ins |= 0x0040;
            } else {
                ins |= mode->outer.indexRegister << 12;
                if (mode->outer.indexSize == SIZE_LONG)
                    ins |= 0x0800;
            }

            if (mode->outer.displacement != NULL) {
                m68k_OptimizeDisplacement(&mode->outer);
                switch (mode->outer.displacementSize) {
                    case SIZE_WORD:
                        ins |= 0x0020;
                        break;
                    case SIZE_LONG:
                        ins |= 0x0030;
                        break;
                    default:
                        internalerror("unknown BD size");
                        return false;
                }
            } else {
                ins |= 0x0010;
            }

            expr = expr_Const(ins);
            if (expr != NULL) {
                if (mode->outer.indexScale != NULL) {
                    expr = expr_Or(expr, expr_Asl(mode->outer.indexScale, expr_Const(9)));
                }
                sect_OutputExpr16(expr);

                if (mode->outer.displacement != NULL) {
                    switch (mode->outer.displacementSize) {
                        default:
                        case SIZE_WORD:
                            sect_OutputExpr16(mode->outer.displacement);
                            break;
                        case SIZE_LONG:
                            sect_OutputExpr32(mode->outer.displacement);
                            break;
                    }
                }

                return true;
            }
            return false;
        }

        case AM_PREINDPCXD020: {
            if (mode->inner.displacement)
                mode->inner.displacement = expr_PcRelative(mode->inner.displacement, 2);
        }
        // fall through
        case AM_PREINDAXD020: {
            uint16_t ins = 0x0100;
            SExpression* expr;

            if (mode->inner.baseRegister == REG_NONE) {
                ins |= 0x0080;
            }

            if (mode->inner.indexRegister == REG_NONE) {
                ins |= 0x0040;
            } else {
                ins |= mode->inner.indexRegister << 12;
                if (mode->inner.indexSize == SIZE_LONG)
                    ins |= 0x0800;
            }

            if (mode->inner.displacement != NULL) {
                m68k_OptimizeDisplacement(&mode->inner);
                switch (mode->inner.displacementSize) {
                    case SIZE_WORD:
                        ins |= 0x0020;
                        break;
                    case SIZE_LONG:
                        ins |= 0x0030;
                        break;
                    default:
                        internalerror("unknown BD size");
                        return false;
                }
            } else {
                ins |= 0x0010;
            }

            if (mode->outer.displacement != NULL) {
                m68k_OptimizeDisplacement(&mode->outer);
                switch (mode->outer.displacementSize) {
                    case SIZE_WORD:
                        ins |= 0x0002;
                        break;
                    case SIZE_LONG:
                        ins |= 0x0003;
                        break;
                    default:
                        internalerror("unknown OD size");
                        return false;
                }
            } else {
                ins |= 0x0001;
            }

            expr = expr_Const(ins);
            if (expr != NULL) {
                if (mode->inner.indexScale != NULL) {
                    expr = expr_Or(expr, expr_Asl(mode->inner.indexScale, expr_Const(9)));
                }
                sect_OutputExpr16(expr);

                if (mode->inner.displacement != NULL) {
                    switch (mode->inner.displacementSize) {
                        default:
                        case SIZE_WORD:
                            sect_OutputExpr16(mode->inner.displacement);
                            break;
                        case SIZE_LONG:
                            sect_OutputExpr32(mode->inner.displacement);
                            break;
                    }
                }

                if (mode->outer.displacement != NULL) {
                    switch (mode->outer.displacementSize) {
                        default:
                        case SIZE_WORD:
                            sect_OutputExpr16(mode->outer.displacement);
                            break;
                        case SIZE_LONG:
                            sect_OutputExpr32(mode->outer.displacement);
                            break;
                    }
                }

                return true;
            }
            return false;
        }

        case AM_POSTINDPCXD020: {
            if (mode->inner.displacement)
                mode->inner.displacement = expr_PcRelative(mode->inner.displacement, 2);
        }
        // fall through
        case AM_POSTINDAXD020: {
            uint16_t ins = 0x0100;
            SExpression* expr;

            if (mode->inner.baseRegister == REG_NONE) {
                ins |= 0x0080;
            }

            if (mode->outer.indexRegister == REG_NONE) {
                ins |= 0x0040;
            } else {
                ins |= mode->outer.indexRegister << 12;
                if (mode->outer.indexSize == SIZE_LONG)
                    ins |= 0x0800;
            }

            if (mode->inner.displacement != NULL) {
                m68k_OptimizeDisplacement(&mode->inner);
                switch (mode->inner.displacementSize) {
                    case SIZE_WORD:
                        ins |= 0x0020;
                        break;
                    case SIZE_LONG:
                        ins |= 0x0030;
                        break;
                    default:
                        internalerror("unknown BD size");
                        return false;
                }
            } else {
                ins |= 0x0010;
            }

            if (mode->outer.displacement != NULL) {
                m68k_OptimizeDisplacement(&mode->outer);
                switch (mode->outer.displacementSize) {
                    case SIZE_WORD:
                        ins |= 0x0006;
                        break;
                    case SIZE_LONG:
                        ins |= 0x0007;
                        break;
                    default:
                        internalerror("unknown OD size");
                        return false;
                }
            } else {
                ins |= 0x0005;
            }

            expr = expr_Const(ins);
            if (expr != NULL) {
                if (mode->outer.indexScale != NULL) {
                    expr = expr_Or(expr, expr_Asl(mode->outer.indexScale, expr_Const(9)));
                }
                sect_OutputExpr16(expr);

                if (mode->inner.displacement != NULL) {
                    switch (mode->inner.displacementSize) {
                        default:
                        case SIZE_WORD:
                            sect_OutputExpr16(mode->inner.displacement);
                            break;
                        case SIZE_LONG:
                            sect_OutputExpr32(mode->inner.displacement);
                            break;
                    }
                }

                if (mode->outer.displacement != NULL) {
                    switch (mode->outer.displacementSize) {
                        default:
                        case SIZE_WORD:
                            sect_OutputExpr16(mode->outer.displacement);
                            break;
                        case SIZE_LONG:
                            sect_OutputExpr32(mode->outer.displacement);
                            break;
                    }
                }

                return true;
            }
            return false;
        }

        default:
            internalerror("unsupported addressing mode");
            return false;

    }

    return false;
}

uint16_t
m68k_GetEffectiveAddressField(SAddressingMode* mode) {
    switch (mode->mode) {
        case AM_DREG:
            return mode->directRegister;
        case AM_AREG:
            return 0x1 << 3 | mode->directRegister;
        case AM_AIND:
            return 0x2 << 3 | (mode->outer.baseRegister & 7u);
        case AM_AINC:
            return 0x3 << 3 | (mode->outer.baseRegister & 7u);
        case AM_ADEC:
            return 0x4 << 3 | (mode->outer.baseRegister & 7u);
        case AM_ADISP:
            return 0x5 << 3 | (mode->outer.baseRegister & 7u);
        case AM_AXDISP:
            return 0x6 << 3 | (mode->outer.baseRegister & 7u);
        case AM_WORD:
            return 0x7 << 3 | 0x0;
        case AM_LONG:
            return 0x7 << 3 | 0x1;
        case AM_IMM:
            return 0x7 << 3 | 0x4;
        case AM_PCDISP:
            return 0x7 << 3 | 0x2;
        case AM_PCXDISP:
            return 0x7 << 3 | 0x3;
        case AM_AXDISP020:
            if (mode->outer.baseRegister != REG_NONE)
                return 0x6 << 3 | (mode->outer.baseRegister & 7u);
            else
                return 0x6 << 3 | 0;
        case AM_PCXDISP020:
        case AM_PREINDPCXD020:
        case AM_POSTINDPCXD020:
            return 0x7 << 3 | 0x3;
        case AM_PREINDAXD020:
        case AM_POSTINDAXD020:
            if (mode->inner.baseRegister != REG_NONE)
                return (uint16_t) ((0x6 << 3) | (mode->inner.baseRegister & 7u));
            else
                return 0x6 << 3 | 0;
        case AM_FPUREG:
            return 0;
        default:
            internalerror("Unknown addressing mode");
    }

    return 0;
}

uint16_t 
get68080BankBits(SAddressingMode* addr) {
    if ((addr->mode & (AM_DREG | AM_AREG)) != 0) {
        return addr->directRegisterBank;
    } else if ((addr->mode & (AM_AXDISP | AM_AXDISP020)) != 0) {
        return (addr->outer.baseBank << 1) | (addr->outer.indexBank);
    } else {
        return 0;
    }
}


static bool
thirdOperandAllowed(SInstruction* instruction, SAddressingMode* dest) {
    return (opt_Current->machineOptions->cpu & CPUF_68080)
    &&     instruction->allowThirdOperand
    &&     (instruction->allowedDestModes & dest->mode & (AM_DREG | AM_AREG | AM_FPUREG));
}

static bool
parseDataRegister(uint16_t* outRegister) {
    if (lex_Current.token >= T_68K_REG_D0 && lex_Current.token <= T_68K_REG_D31) {
        *outRegister = (uint16_t) (lex_Current.token - T_68K_REG_D0);
        parse_GetToken();
        return true;
    }

    return false;
}

static bool
parseAddressRegister(uint16_t* outRegister) {
    if (lex_Current.token >= T_68K_REG_A0 && lex_Current.token <= T_68K_REG_A15) {
        *outRegister = (uint16_t) (lex_Current.token - T_68K_REG_A0);
        parse_GetToken();
        return true;
    }

    return false;
}

static bool
parseFpuRegister(uint16_t* outRegister) {
    if (lex_Current.token >= T_FPUREG_0 && lex_Current.token <= T_FPUREG_7) {
        *outRegister = (uint16_t) (lex_Current.token - T_FPUREG_0);
        parse_GetToken();
        return true;
    }

    return false;
}


static bool 
parseThirdOperand(SInstruction* instruction, SAddressingMode* dest, uint16_t* thirdReg) {
    if (thirdOperandAllowed(instruction, dest)) {
        if (lex_Current.token == ',') {
            parse_GetToken();
            switch (dest->mode) {
                case AM_DREG: return parseDataRegister(thirdReg);
                case AM_AREG: return parseAddressRegister(thirdReg);
                case AM_FPUREG: return parseFpuRegister(thirdReg);
                default: return false;
            }
            return false;
        }
    }
    return true;
}

bool
m68k_ParseOpCore(SInstruction* instruction, ESize inssz, SAddressingMode* src, SAddressingMode* dest) {
    EAddrMode allowedSrc;
    EAddrMode allowedDest;

    allowedSrc = instruction->allowedSourceModes;
    if (opt_Current->machineOptions->cpu >= CPUF_68020)
        allowedSrc = enable020Modes(allowedSrc);

    if ((allowedSrc & src->mode) == 0 && !(allowedSrc == 0 && src->mode == AM_EMPTY)) {
        err_Error(ERROR_SOURCE_OPERAND);
        return true;
    }

    allowedDest = instruction->allowedDestModes;
    if (opt_Current->machineOptions->cpu >= CPUF_68020)
        allowedDest = enable020Modes(allowedDest);

    if ((allowedDest & dest->mode) == 0 && !(allowedDest == 0 && dest->mode == AM_EMPTY)) {
        err_Error(ERROR_DEST_OPERAND);
        return true;
    }

    uint16_t thirdReg = 0;
    if (!parseThirdOperand(instruction, dest, &thirdReg)) {
        err_Error(MERROR_THIRD_OPERAND_WRONG_TYPE);
        return false;
    }

    uint16_t srcBanks = get68080BankBits(src);
    uint16_t destBanks = get68080BankBits(dest);
    bool prefix68080 = srcBanks != 0 || destBanks != 0 || thirdReg != 0;
    uint32_t prefixOffset = sect_CurrentSize();

    if (prefix68080) {
        sect_OutputConst16(0);
    }

    bool result = instruction->handler(inssz, src, dest, instruction->data);

    if (prefix68080) {
        thirdReg ^= (dest->directRegisterBank << 3) | dest->directRegister;
        uint16_t lowThird = (thirdReg & 0x03) << 4;
        uint16_t highThird = (thirdReg & 0x1C) << 7;
        uint16_t size = (sect_CurrentSize() - prefixOffset - 4) >> 1;
        sect_OutputConst16At(0x7100 | highThird | (size << 6) | lowThird | (srcBanks << 2) | destBanks, prefixOffset);
    }

    return result;
}

static bool
check68080ModesAllowed(SAddressingMode* addr) {
    if ((opt_Current->machineOptions->cpu & CPUF_68080) == 0) {
        return (addr->directRegisterBank == 0) && (addr->outer.baseBank == 0) && (addr->outer.indexBank == 0);
    } else {
        switch (addr->mode) {
            case AM_AXDISP:
            case AM_AXDISP020:
                return (addr->outer.baseBank <= 1) && (addr->outer.indexBank <= 1);
            case AM_DREG:
            case AM_AREG:
                return true;
            default:
                return (addr->directRegisterBank == 0) && (addr->outer.baseBank == 0) && (addr->outer.indexBank == 0);
        }
    }
}

bool
m68k_ParseCommonCpuFpu(SInstruction* instruction, bool allowFloat) {
    ESize insSz;
    SAddressingMode src;
    SAddressingMode dest;

    if (instruction->allowedSizes == SIZE_DEFAULT) {
        if (m68k_GetSizeSpecifier(SIZE_DEFAULT) != SIZE_DEFAULT) {
            err_Warn(MERROR_IGNORING_SIZE);
            parse_GetToken();
        }
        insSz = SIZE_DEFAULT;
    } else
        insSz = m68k_GetSizeSpecifier(instruction->defaultSize);

    src.mode = AM_EMPTY;
    dest.mode = AM_EMPTY;

    if (instruction->allowedSourceModes != 0 && instruction->allowedSourceModes != AM_EMPTY) {
        if (m68k_GetAddressingMode(&src, allowFloat)) {
            if (instruction->allowedSourceModes & AM_BITFIELD) {
                if (!getBitfield(&src)) {
                    err_Error(MERROR_EXPECT_BITFIELD);
                    return false;
                }
            }

            if (src.mode == AM_IMM)
                src.immediateSize = insSz;

            if (!check68080ModesAllowed(&src)) {
                err_Error(MERROR_NOT_68080_MODE);
                return true;
            }
        } else {
            if ((instruction->allowedSourceModes & AM_EMPTY) == 0)
                return true;
        }
    }

    if (instruction->allowedDestModes != 0) {
        if (lex_Current.token == ',') {
            parse_GetToken();
            if (!m68k_GetAddressingMode(&dest, allowFloat))
                return false;

            if (instruction->allowedDestModes & AM_BITFIELD) {
                if (!getBitfield(&dest)) {
                    err_Error(MERROR_EXPECT_BITFIELD);
                    return false;
                }
            }

            if (!check68080ModesAllowed(&dest)) {
                err_Error(MERROR_NOT_68080_MODE);
                return true;
            }
        } 
    }

    if ((instruction->allowedSizes & insSz) == 0 && instruction->allowedSizes != 0 && instruction->defaultSize != 0) {
        err_Error(MERROR_INSTRUCTION_SIZE);
    }

    return m68k_ParseOpCore(instruction, insSz, &src, &dest);

}

SExpression*
m68k_ParseFunction(void) {
    switch (lex_Current.token) {
        case T_68K_REGMASK: {
            parse_GetToken();
            if (!parse_ExpectChar('('))
                return NULL;
            uint32_t regs = m68k_ParseRegisterList();
            if (regs == REGLIST_FAIL)
                return NULL;
            if (!parse_ExpectChar(')'))
                return NULL;
            return expr_Const(regs);
        }
        default:
            return NULL;
    }
}

bool
m68k_ParseInstruction(void) {
    if (m68k_IntegerInstruction())
        return true;
    else if (m68k_ParseFpuInstruction())
        return true;
    else if (m68k_ParseDirective())
        return false;

    return false;
}
