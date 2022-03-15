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

#include "expression.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "parse_float_expression.h"
#include "errors.h"

#include "m68k_errors.h"
#include "m68k_options.h"
#include "m68k_parse.h"
#include "m68k_tokens.h"

static SExpression*
expressionCheckScaleRange(SExpression* expression) {
    if ((expression = expr_CheckRange(expression, 1, 8)) == NULL) {
        err_Error(MERROR_SCALE_RANGE);
        return NULL;
    }

    return expression;
}

static bool
getDataRegister(uint16_t* outRegister) {
    if (lex_Context->token.id >= T_68K_REG_D0 && lex_Context->token.id <= T_68K_REG_D7) {
        *outRegister = (uint16_t) (lex_Context->token.id - T_68K_REG_D0);
        parse_GetToken();
        return true;
    }

    return false;
}

static bool
getAddressRegister(uint16_t* outRegister) {
    if (lex_Context->token.id >= T_68K_REG_A0 && lex_Context->token.id <= T_68K_REG_A7) {
        *outRegister = (uint16_t) (lex_Context->token.id - T_68K_REG_A0);
        parse_GetToken();
        return true;
    }

    return false;
}

static bool
getRegister(uint16_t* outRegister) {
    if (getDataRegister(outRegister))
        return true;

    if (getAddressRegister(outRegister)) {
        *outRegister += 8;
        return true;
    }

    return false;
}

static bool
getRegisterRange(uint16_t* outStart, uint16_t* outEnd) {
    if (getRegister(outStart)) {
        if (lex_Context->token.id == T_OP_SUBTRACT) {
            parse_GetToken();
            if (!getRegister(outEnd))
                return 0;
            return true;
        }
        *outEnd = *outStart;
        return true;
    }
    return false;
}

static bool
getIndexReg(SModeRegisters* outMode) {
    if (lex_Context->token.id >= T_68K_REG_D0 && lex_Context->token.id <= T_68K_REG_D15) {
        uint8_t reg = lex_Context->token.id - T_68K_REG_D0;
        outMode->indexRegister = REG_D0 + (reg & 7);
        outMode->indexBank = reg >> 3;
    } else if (lex_Context->token.id >= T_68K_REG_A0 && lex_Context->token.id <= T_68K_REG_A15) {
        uint8_t reg = lex_Context->token.id - T_68K_REG_A0;
        outMode->indexRegister = REG_A0 + (reg & 7);
        outMode->indexBank = reg >> 3;
    }

    parse_GetToken();

    outMode->indexSize = m68k_GetSizeSpecifier(SIZE_WORD);

    if (outMode->indexSize != SIZE_WORD && outMode->indexSize != SIZE_LONG) {
        err_Error(MERROR_INDEXREG_SIZE);
        return false;
    }

    if (lex_Context->token.id == T_OP_MULTIPLY) {
        parse_GetToken();
        if ((outMode->indexScale = parse_Expression(1)) == NULL) {
            err_Error(ERROR_EXPECT_EXPR);
            return false;
        }
        outMode->indexScale = expressionCheckScaleRange(outMode->indexScale);
        outMode->indexScale = expr_Bit(outMode->indexScale);
    }

    return true;
}

static bool
singleModePart(SModeRegisters* outMode) {
    // parses xxxx, Ax, PC, Xn.S*scale
    SExpression* expr;

    if (lex_Context->token.id == T_68K_REG_PC) {
        if (outMode->baseRegister != REG_NONE)
            return false;

        outMode->baseRegister = REG_PC;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id >= T_68K_REG_A0 && lex_Context->token.id <= T_68K_REG_A15) {
        ESize sz;

        if (outMode->baseRegister != REG_NONE) {
            if (outMode->indexRegister != REG_NONE) {
                return false;
            }

            return getIndexReg(outMode);
        }

        int addressRegister = lex_Context->token.id - T_68K_REG_A0;
        parse_GetToken();
        sz = m68k_GetSizeSpecifier(SIZE_DEFAULT);
        if (sz == SIZE_WORD) {
            if (outMode->indexRegister == REG_NONE) {
                outMode->indexRegister = REG_A0 + (addressRegister & 7);
                outMode->indexBank = (addressRegister >> 3);
                outMode->indexSize = SIZE_WORD;
                return true;
            }
            return false;
        }

        outMode->baseRegister = REG_A0 + (addressRegister & 7);
        outMode->baseBank = addressRegister >> 3;
        return true;
    }

    if (lex_Context->token.id >= T_68K_REG_D0 && lex_Context->token.id <= T_68K_REG_D15) {
        if (outMode->indexRegister != REG_NONE) {
            return false;
        }

        return getIndexReg(outMode);
    }

    expr = parse_Expression(4);
    if (expr != NULL) {
        if (outMode->displacement != NULL)
            return false;

        outMode->displacement = expr;
        outMode->displacementSize = m68k_GetSizeSpecifier(SIZE_DEFAULT);
        return true;
    }

    return false;
}

static bool
getInnerMode(SAddressingMode* outMode) {
    for (;;) {
        if (!singleModePart(&outMode->inner))
            return false;

        if (lex_Context->token.id == ']') {
            parse_GetToken();
            return true;
        }

        if (lex_Context->token.id == ',') {
            parse_GetToken();
            continue;
        }

        return false;
    }
}

static bool
getOuterPart(SAddressingMode* outMode) {
    if (opt_Current->machineOptions->cpu >= CPUF_68020 && lex_Context->token.id == '[') {
        parse_GetToken();
        return getInnerMode(outMode);
    }

    return singleModePart(&outMode->outer);
}

static bool
getOuterMode(SAddressingMode* outMode) {
    for (;;) {
        if (!getOuterPart(outMode))
            return false;

        if (lex_Context->token.id == ')') {
            parse_GetToken();
            return true;
        }

        if (lex_Context->token.id == ',') {
            parse_GetToken();
            continue;
        }

        return false;
    }

    return false;
}

static void
optimizeFields(SModeRegisters* registers) {
    if (registers->displacement != NULL && expr_IsConstant(registers->displacement) && registers->displacement->value.integer == 0) {
        expr_Free(registers->displacement);
        registers->displacement = NULL;
    }

    if (registers->indexScale != NULL && expr_IsConstant(registers->indexScale) && registers->indexScale->value.integer == 0) {
        expr_Free(registers->indexScale);
        registers->indexScale = NULL;
    }

    if (registers->baseRegister == REG_NONE && registers->indexRegister >= REG_A0 && registers->indexRegister <= REG_A7
        && registers->indexSize == SIZE_LONG) {
        registers->baseRegister = registers->indexRegister;
        registers->indexRegister = REG_NONE;
    }
}

#define I_BASE  0x01
#define I_INDEX 0x02
#define I_DISP  0x04
#define O_BASE  0x08
#define O_INDEX 0x10
#define O_DISP  0x20

static bool
optimizeMode(SAddressingMode* mode) {
    uint32_t inner = 0;

    optimizeFields(&mode->inner);
    optimizeFields(&mode->outer);

    if (mode->inner.baseRegister != REG_NONE)
        inner |= I_BASE;

    if (mode->inner.indexRegister != REG_NONE)
        inner |= I_INDEX;

    if (mode->inner.displacement != NULL)
        inner |= I_DISP;

    if (mode->outer.baseRegister != REG_NONE)
        inner |= O_BASE;

    if (mode->outer.indexRegister != REG_NONE)
        inner |= O_INDEX;

    if (mode->outer.displacement != NULL)
        inner |= O_DISP;

    if ((inner & (I_BASE | I_INDEX | I_DISP)) != 0) {
        if (mode->inner.displacement != NULL && mode->inner.displacementSize == SIZE_BYTE)
            mode->inner.displacementSize = SIZE_WORD;

        if (mode->outer.displacement != NULL && mode->outer.displacementSize == SIZE_BYTE)
            mode->outer.displacementSize = SIZE_WORD;

        if (mode->outer.baseRegister != REG_NONE && mode->outer.indexRegister != REG_NONE)
            return false;

        if (mode->outer.baseRegister != REG_NONE && mode->outer.indexRegister == REG_NONE) {
            mode->outer.indexRegister = mode->outer.baseRegister;
            mode->outer.indexSize = SIZE_LONG;
            mode->outer.baseRegister = REG_NONE;
            mode->outer.indexScale = NULL;
        }

        switch (inner) {
            default:
                return false;
            case I_BASE:
            case          I_INDEX:
            case I_BASE | I_INDEX:
            case                    I_DISP:
            case I_BASE |           I_DISP:
            case          I_INDEX | I_DISP:
            case I_BASE | I_INDEX | I_DISP:
            case                             O_DISP:
            case I_BASE |                    O_DISP:
            case          I_INDEX |          O_DISP:
            case I_BASE | I_INDEX |          O_DISP:
            case                    I_DISP | O_DISP:
            case I_BASE |           I_DISP | O_DISP:
            case          I_INDEX | I_DISP | O_DISP:
            case I_BASE | I_INDEX | I_DISP | O_DISP:
                // ([bd,An,Xn],od)
                if (mode->inner.baseRegister == REG_PC)
                    mode->mode = AM_PREINDPCXD020;
                else
                    mode->mode = AM_PREINDAXD020;
                return true;

                //case I_BASE:
                //case          I_DISP:
                //case I_BASE | I_DISP:
            case                   O_INDEX:
            case I_BASE |          O_INDEX:
            case          I_DISP | O_INDEX:
            case I_BASE | I_DISP | O_INDEX:
                //case                             O_DISP:
                //case I_BASE                    | O_DISP:
                //case          I_DISP           | O_DISP:
                //case I_BASE | I_DISP           | O_DISP:
                //case                 | O_INDEX | O_DISP:
            case I_BASE |          O_INDEX | O_DISP:
            case          I_DISP | O_INDEX | O_DISP:
            case I_BASE | I_DISP | O_INDEX | O_DISP:
                // ([bd,An],Xn,od)
                if (mode->inner.baseRegister == REG_PC)
                    mode->mode = AM_POSTINDPCXD020;
                else
                    mode->mode = AM_POSTINDAXD020;
                return true;
        }
    }

    switch (inner) {
        default:
            return false;
        case O_BASE:
            mode->mode = AM_AIND;
            return true;
        case O_INDEX:
        case O_INDEX | O_DISP:
            mode->mode = AM_AXDISP020;
            return true;
        case O_BASE | O_INDEX:
            mode->mode = AM_AXDISP;
            return true;
        case O_DISP:
            if (mode->outer.displacementSize == SIZE_BYTE) {
                err_Error(MERROR_DISP_SIZE);
                return false;
            }
            if (mode->outer.displacementSize == SIZE_WORD)
                mode->mode = AM_WORD;
            else
                mode->mode = AM_LONG;
            return true;
        case O_BASE | O_DISP:
            if (mode->outer.displacementSize == SIZE_BYTE) {
                err_Error(MERROR_DISP_SIZE);
                return false;
            }

            if (opt_Current->machineOptions->cpu <= CPUF_68010) {
                if (mode->outer.displacementSize == SIZE_DEFAULT)
                    mode->outer.displacementSize = SIZE_WORD;

                if (mode->outer.baseRegister == REG_PC)
                    mode->mode = AM_PCDISP;
                else
                    mode->mode = AM_ADISP;

                return true;
            }

            m68k_OptimizeDisplacement(&mode->outer);
            if (mode->outer.displacementSize == SIZE_WORD) {
                if (mode->outer.baseRegister == REG_PC)
                    mode->mode = AM_PCDISP;
                else
                    mode->mode = AM_ADISP;

                return true;
            }

            if (mode->outer.baseRegister == REG_PC)
                mode->mode = AM_PCXDISP020;
            else
                mode->mode = AM_AXDISP020;

            return true;
        case O_BASE | O_INDEX | O_DISP:
            if (opt_Current->machineOptions->cpu <= CPUF_68010) {
                if (mode->outer.displacementSize == SIZE_DEFAULT)
                    mode->outer.displacementSize = SIZE_BYTE;
            } else {
                if (mode->outer.displacementSize == SIZE_DEFAULT) {
                    if (expr_IsConstant(mode->outer.displacement)) {
                        if (mode->outer.displacement->value.integer >= -128 && mode->outer.displacement->value.integer <= 127)
                            mode->outer.displacementSize = SIZE_BYTE;
                        else if (mode->outer.displacement->value.integer >= -32768
                                 && mode->outer.displacement->value.integer <= 32767)
                            mode->outer.displacementSize = SIZE_WORD;
                        else
                            mode->outer.displacementSize = SIZE_LONG;
                    } else
                        mode->outer.displacementSize = SIZE_BYTE;
                }
            }

            if (mode->outer.displacementSize == SIZE_BYTE) {
                if (mode->outer.baseRegister == REG_PC)
                    mode->mode = AM_PCXDISP;
                else
                    mode->mode = AM_AXDISP;
            } else {
                if (mode->outer.baseRegister == REG_PC)
                    mode->mode = AM_PCXDISP020;
                else
                    mode->mode = AM_AXDISP020;
            }
            return true;
    }
}

uint16_t
m68k_SourceUpdatedRegisters(SAddressingMode* addrMode) {
	if (addrMode->mode & (AM_AINC | AM_ADEC)) {
		return 0x100 << addrMode->directRegister;
	}
	return 0;
}

uint16_t
m68k_DestinationUpdatedRegisters(SAddressingMode* addrMode) {
	if (addrMode->mode == AM_DREG) {
		return 1 << addrMode->directRegister;
	}
	if (addrMode->mode & (AM_AREG | AM_AINC | AM_ADEC)) {
		return 0x100 << addrMode->directRegister;
	}
	return 0;
}

void
m68k_OptimizeDisplacement(SModeRegisters* pRegs) {
    if (pRegs->displacement != NULL && pRegs->displacementSize == SIZE_DEFAULT) {
        if (expr_IsConstant(pRegs->displacement)) {
            if (pRegs->displacement->value.integer >= -32768 && pRegs->displacement->value.integer <= 32767)
                pRegs->displacementSize = SIZE_WORD;
            else
                pRegs->displacementSize = SIZE_LONG;
        } else
            pRegs->displacementSize = SIZE_LONG;
    }
}

bool
m68k_GetAddressingMode(SAddressingMode* addrMode, bool allowFloat) {
    addrMode->immediateInteger = NULL;
    addrMode->immediateFloat = 0;
    addrMode->directRegister = 0;
    addrMode->directRegisterBank = BANK_0;
    addrMode->inner.baseRegister = REG_NONE;
    addrMode->inner.baseBank = BANK_0;
    addrMode->inner.indexRegister = REG_NONE;
    addrMode->inner.indexBank = BANK_0;
    addrMode->inner.indexScale = NULL;
    addrMode->inner.displacement = NULL;
    addrMode->outer.baseRegister = REG_NONE;
    addrMode->outer.baseBank = BANK_0;
    addrMode->outer.indexRegister = REG_NONE;
    addrMode->outer.indexBank = BANK_0;
    addrMode->outer.indexScale = NULL;
    addrMode->outer.displacement = NULL;
    addrMode->hasBitfield = false;

    if (lex_Context->token.id >= T_68K_SYSREG_FIRST && lex_Context->token.id <= T_68K_SYSREG_LAST) {
        addrMode->mode = AM_SYSREG;
        addrMode->directRegister = lex_Context->token.id;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id >= T_68K_REG_D0 && lex_Context->token.id <= T_68K_REG_D31) {
        uint8_t reg = lex_Context->token.id - T_68K_REG_D0;
        addrMode->mode = AM_DREG;
        addrMode->directRegister = reg & 7;
        addrMode->directRegisterBank = reg >> 3;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id >= T_68K_REG_A0 && lex_Context->token.id <= T_68K_REG_A15) {
        uint8_t reg = lex_Context->token.id - T_68K_REG_A0;
        addrMode->mode = AM_AREG;
        addrMode->directRegister = reg & 7;
        addrMode->directRegisterBank = reg >> 3;
        parse_GetToken();
        return true;
    }

    if (allowFloat && lex_Context->token.id >= T_FPUREG_0 && lex_Context->token.id <= T_FPUREG_7) {
        addrMode->mode = AM_FPUREG;
        addrMode->directRegister = lex_Context->token.id - T_FPUREG_0;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id >= T_FPU_CR_FIRST && lex_Context->token.id <= T_FPU_CR_LAST) {
        addrMode->mode = AM_FPUCR;
        addrMode->directRegister = lex_Context->token.id - T_FPU_CR_FIRST;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id >= T_68K_REG_A0_IND && lex_Context->token.id <= T_68K_REG_A15_IND) {
        uint8_t reg = lex_Context->token.id - T_68K_REG_A0_IND;
        addrMode->mode = AM_AIND;
        addrMode->directRegister = reg & 7;
        addrMode->directRegisterBank = reg >> 3;
        addrMode->outer.baseRegister = REG_A0 + (reg & 7);
        addrMode->outer.baseBank = reg >> 3;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id >= T_68K_REG_A0_DEC && lex_Context->token.id <= T_68K_REG_A15_DEC) {
        uint8_t reg = lex_Context->token.id - T_68K_REG_A0_DEC;
        addrMode->mode = AM_ADEC;
        addrMode->directRegister = reg & 7;
        addrMode->directRegisterBank = reg >> 3;
        addrMode->outer.baseRegister = REG_A0 + (reg & 7);
        addrMode->outer.baseBank = reg >> 3;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id >= T_68K_REG_A0_INC && lex_Context->token.id <= T_68K_REG_A15_INC) {
        uint8_t reg = lex_Context->token.id - T_68K_REG_A0_INC;
        addrMode->mode = AM_AINC;
        addrMode->directRegister = reg & 7;
        addrMode->directRegisterBank = reg >> 3;
        addrMode->outer.baseRegister = REG_A0 + (reg & 7);
        addrMode->outer.baseBank = reg >> 3;
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id == '#') {
        parse_GetToken();
        addrMode->mode = AM_IMM;
        addrMode->immediateInteger = parse_Expression(4);
        if (addrMode->immediateInteger == NULL && allowFloat) {
            addrMode->immediateFloat = parse_FloatExpression(4);
        }
        return true;
    }

    addrMode->outer.displacement = parse_Expression(4);
    if (addrMode->outer.displacement != NULL)
        addrMode->outer.displacementSize = m68k_GetSizeSpecifier(SIZE_DEFAULT);

    // parse (xxxx)
    if (lex_Context->token.id == '(') {
        parse_GetToken();

        if (getOuterMode(addrMode)) {
            return optimizeMode(addrMode);
        }
        return false;
    }

    if (addrMode->outer.displacement != NULL) {
        if (lex_Context->token.id == T_68K_REG_PC_IND) {
            addrMode->mode = AM_PCDISP;
            addrMode->outer.baseRegister = REG_PC;
            addrMode->outer.displacementSize = SIZE_WORD;
            parse_GetToken();
            return true;
        } else if (lex_Context->token.id >= T_68K_REG_A0_IND && lex_Context->token.id <= T_68K_REG_A7_IND) {
            if ((addrMode->outer.displacement = m68k_ExpressionCheck16Bit(addrMode->outer.displacement)) != NULL) {
                addrMode->mode = AM_ADISP;
                addrMode->outer.baseRegister = REG_A0 + (lex_Context->token.id - T_68K_REG_A0_IND);
                addrMode->outer.displacementSize = SIZE_WORD;
                parse_GetToken();
                return true;
            }
        }
    }

    m68k_OptimizeDisplacement(&addrMode->outer);

    if (addrMode->outer.displacement != NULL) {
        if (addrMode->outer.displacementSize == SIZE_WORD) {
            addrMode->mode = AM_WORD;
            return true;
        } else if (addrMode->outer.displacementSize == SIZE_LONG) {
            addrMode->mode = AM_LONG;
            return true;
        } else
            err_Error(MERROR_DISP_SIZE);
    }

    return false;
}

uint32_t
m68k_ParseRegisterList(void) {
    uint16_t r;
    uint16_t start;
    uint16_t end;

    if (lex_Context->token.id == '#') {
        int32_t expr;
        parse_GetToken();
        expr = parse_ConstantExpression();
        if (expr >= 0 && expr <= 65535)
            return (uint16_t) expr;

        return REGLIST_FAIL;
    }

    r = 0;

    while (getRegisterRange(&start, &end)) {
        if (start > end) {
            err_Error(ERROR_OPERAND);
            return REGLIST_FAIL;
        }

        while (start <= end)
            r |= 1 << start++;

        if (lex_Context->token.id != T_OP_DIVIDE)
            return r;

        parse_GetToken();
    }

    return REGLIST_FAIL;
}

ESize
m68k_GetSizeSpecifier(ESize defaultSize) {
    if (lex_Context->token.id == T_ID && strlen(lex_Context->token.value.string) == 2) {
        if (_strnicmp(lex_Context->token.value.string, ".b", 2) == 0) {
            parse_GetToken();
            return SIZE_BYTE;
        } else if (_strnicmp(lex_Context->token.value.string, ".w", 2) == 0) {
            parse_GetToken();
            return SIZE_WORD;
        } else if (_strnicmp(lex_Context->token.value.string, ".l", 2) == 0) {
            parse_GetToken();
            return SIZE_LONG;
        } else if (_strnicmp(lex_Context->token.value.string, ".s", 2) == 0) {
            parse_GetToken();
            return SIZE_SINGLE;
        } else if (_strnicmp(lex_Context->token.value.string, ".d", 2) == 0) {
            parse_GetToken();
            return SIZE_DOUBLE;
        } else if (_strnicmp(lex_Context->token.value.string, ".x", 2) == 0) {
            parse_GetToken();
            return SIZE_EXTENDED;
        } else if (_strnicmp(lex_Context->token.value.string, ".p", 2) == 0) {
            parse_GetToken();
            return SIZE_PACKED;
        }
    }

    return defaultSize;
}

SExpression*
m68k_ExpressionCheck16Bit(SExpression* expression) {
    if ((expression = expr_CheckRange(expression, -32768, 65535)) == NULL) {
        err_Error(ERROR_EXPRESSION_N_BIT, 16);
        return NULL;
    }

    return expression;
}

SExpression*
m68k_ExpressionCheck8Bit(SExpression* expression) {
    if ((expression = expr_CheckRange(expression, -128, 255)) == NULL) {
        err_Error(ERROR_EXPRESSION_N_BIT, 8);
        return NULL;
    }

    return expression;
}

