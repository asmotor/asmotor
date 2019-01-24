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

/*
Thoughts on the ISA:

1) Destination literal on add, sub, xor etc - leave the operation undefined, this will give you more room for extensions later.
*/

#include <assert.h>

#include "xasm.h"
#include "expression.h"
#include "lexer.h"
#include "options.h"
#include "parse.h"
#include "parse_expression.h"
#include "project.h"
#include "section.h"

#include "0x10c_errors.h"
#include "0x10c_options.h"
#include "0x10c_tokens.h"

static SExpression*
parse_CheckSU16(SExpression* expression) {
    expression = expr_CheckRange(expression, -32768, 65535);
    if (expression == NULL)
        prj_Error(ERROR_OPERAND_RANGE);

    return expr_And(expression, expr_Const(0xFFFF));
}

static SExpression*
parse_CheckU16(SExpression* expression) {
    expression = expr_CheckRange(expression, 0, 65535);
    if (expression == NULL)
        prj_Error(ERROR_OPERAND_RANGE);
    return expression;
}

typedef enum {
    ADDR_A,
    ADDR_B,
    ADDR_C,
    ADDR_X,
    ADDR_Y,
    ADDR_Z,
    ADDR_I,
    ADDR_J,
    ADDR_A_IND,
    ADDR_B_IND,
    ADDR_C_IND,
    ADDR_X_IND,
    ADDR_Y_IND,
    ADDR_Z_IND,
    ADDR_I_IND,
    ADDR_J_IND,
    ADDR_A_OFFSET_IND,
    ADDR_B_OFFSET_IND,
    ADDR_C_OFFSET_IND,
    ADDR_X_OFFSET_IND,
    ADDR_Y_OFFSET_IND,
    ADDR_Z_OFFSET_IND,
    ADDR_I_OFFSET_IND,
    ADDR_J_OFFSET_IND,
    ADDR_POP,
    ADDR_PEEK,
    ADDR_PUSH,
    ADDR_SP,
    ADDR_PC,
    ADDR_O,
    ADDR_ADDRESS_IND,
    ADDR_LITERAL,
    ADDR_LITERAL_00,
} EAddrMode;

#define ADDRF_ALL 0xFFFFFFFFu
#define ADDRF_LITERAL (1UL << (uint32_t)ADDR_LITERAL)

typedef struct {
    EAddrMode mode;
    SExpression* address;
} SAddressingMode;

static SExpression*
expressionNoReservedIdentifiers() {
    opt_Push();
    opt_Current->allowReservedKeywordLabels = false;
    SExpression* expression = parse_Expression(4);
    opt_Pop();

    return expression;
}

static bool
indirectComponent(uint32_t* reg, SExpression** address) {
    if (lex_Current.token >= T_REG_A && lex_Current.token <= T_REG_J) {
        *reg = lex_Current.token - T_REG_A;
        *address = NULL;
        parse_GetToken();
        return true;
    } else {
        SExpression* expression = expressionNoReservedIdentifiers();

        if (expression != NULL) {
            *reg = UINT32_MAX;
            *address = expression;
            return true;
        }
    }
    return false;
}

static bool
indirectAdd(uint32_t* reg, SExpression** address) {
    uint32_t newReg = UINT32_MAX;
    SExpression* newAddress = NULL;
    if (!indirectComponent(&newReg, &newAddress))
        return false;

    if (newReg >= 0) {
        if (*reg == UINT32_MAX)
            *reg = newReg;
        else
            prj_Error(MERROR_ADDRMODE_ONE_REGISTER);
    } else if (newAddress != NULL) {
        *address = *address == NULL ? newAddress : expr_Add(*address, newAddress);
    } else {
        prj_Error(MERROR_ILLEGAL_ADDRMODE);
    }
    return true;
}

static bool indirectSubtract(SExpression** address) {
    uint32_t newReg = UINT32_MAX;
    SExpression* newAddress = NULL;

    parse_GetToken();
    if (!indirectComponent(&newReg, &newAddress))
        return false;

    if (newReg >= 0) {
        prj_Error(MERROR_ADDRMODE_SUBTRACT_REGISTER);
    } else if (newAddress != NULL) {
        *address = address == NULL ? newAddress : expr_Sub(*address, newAddress);
    } else {
        prj_Error(MERROR_ILLEGAL_ADDRMODE);
    }
    return true;
}

static bool
indirectAddressing(SAddressingMode* outMode, uint32_t allowedModes) {
    if (lex_Current.token == '[') {
        uint32_t reg = UINT32_MAX;
        SExpression* address = NULL;

        parse_GetToken();

        if (!indirectComponent(&reg, &address))
            return false;

        while (lex_Current.token == T_OP_ADD || lex_Current.token == T_OP_SUBTRACT) {
            if (lex_Current.token == T_OP_ADD) {
                parse_GetToken();
                if (!indirectAdd(&reg, &address))
                    return false;
            } else if (lex_Current.token == T_OP_SUBTRACT) {
                parse_GetToken();
                if (!indirectSubtract(&address))
                    return false;
            }
        }

        parse_ExpectChar(']');

        if (reg != UINT32_MAX && address == NULL) {
            EAddrMode mode = ADDR_A_IND + reg;
            if (allowedModes & (1u << mode)) {
                outMode->mode = mode;
                outMode->address = NULL;
                return true;
            }
        }

        if (reg != UINT32_MAX && address != NULL) {
            EAddrMode mode = ADDR_A_OFFSET_IND + reg;
            if (allowedModes & (1u << mode)) {
                outMode->mode = mode;
                outMode->address = parse_CheckSU16(address);
                return true;
            }
        }

        if (reg == UINT32_MAX && address != NULL) {
            EAddrMode mode = ADDR_ADDRESS_IND;
            if (allowedModes & (1u << mode)) {
                outMode->mode = mode;
                outMode->address = parse_CheckU16(address);
                return true;
            }
        }
    }

    return false;
}

static void
optimizeAddressingMode(SAddressingMode* addrMode) {
    if (opt_Current->machineOptions->optimize) {
        // Optimize literals <= 0x1F
        if (addrMode->mode == ADDR_LITERAL && expr_IsConstant(addrMode->address)) {
            uint16_t v = (uint16_t) addrMode->address->value.integer;
            if (v <= 0x1F) {
                addrMode->mode = ADDR_LITERAL_00 + v;
                expr_Free(addrMode->address);
                addrMode->address = NULL;
            }
        }

        // Optimize [reg+0] to [reg]
        if (addrMode->mode >= ADDR_A_OFFSET_IND && addrMode->mode <= ADDR_J_OFFSET_IND && expr_IsConstant(addrMode->address)
            && addrMode->address->value.integer == 0) {
            addrMode->mode = addrMode->mode - ADDR_A_OFFSET_IND + ADDR_A_IND;
            expr_Free(addrMode->address);
            addrMode->address = NULL;
        }
    }
}

static bool
addressingMode(SAddressingMode* outMode, uint32_t allowedModes) {
    switch (lex_Current.token) {
        case T_REG_A:
        case T_REG_B:
        case T_REG_C:
        case T_REG_X:
        case T_REG_Y:
        case T_REG_Z:
        case T_REG_I:
        case T_REG_J: {
            EAddrMode mode = ADDR_A + (lex_Current.token - T_REG_A);
            parse_GetToken();

            if (allowedModes & (1u << mode)) {
                outMode->mode = mode;
                outMode->address = NULL;

                return true;
            }
            break;
        }
        case T_REG_POP:
        case T_REG_PEEK:
        case T_REG_PUSH:
        case T_REG_SP:
        case T_REG_PC:
        case T_REG_O: {
            EAddrMode eMode = ADDR_POP + (lex_Current.token - T_REG_POP);
            parse_GetToken();

            if (allowedModes & (1u << eMode)) {
                outMode->mode = eMode;
                outMode->address = NULL;

                return true;
            }
            break;
        }
        default: {
            if (indirectAddressing(outMode, allowedModes)) {
                return true;
            } else {
                SExpression* pAddress = parse_Expression(2);
                if (pAddress != NULL && (allowedModes & ADDRF_LITERAL)) {
                    outMode->mode = ADDR_LITERAL;
                    outMode->address = pAddress;
                    optimizeAddressingMode(outMode);
                    return true;
                }
            }
            break;
        }
    }

    return false;
}

typedef bool
(* ParserFunc)(SAddressingMode* mode1, SAddressingMode* mode2, uint32_t data);

typedef struct {
    uint32_t data;
    ParserFunc parser;
    uint32_t allowedModes1;
    uint32_t allowedModes2;
} SParser;

static bool
genericInstr(SAddressingMode* mode1, SAddressingMode* mode2, uint32_t data) {
    sect_OutputConst16((uint16_t) ((mode2->mode << 10u) | (mode1->mode << 4u) | data));
    if (mode1->address != NULL)
        sect_OutputExpr16(mode1->address);
    if (mode2->address != NULL)
        sect_OutputExpr16(mode2->address);

    return true;
}

static bool
addSubInstr(SAddressingMode* mode1, SAddressingMode* mode2, uint32_t data, ParserFunc negatedParser) {
    // Optimize FUNC dest,-$1F
    if (opt_Current->machineOptions->optimize) {
        if (mode2->mode == ADDR_LITERAL && (expr_IsConstant(mode2->address))
            && ((uint32_t) mode2->address->value.integer & 0xFFFFu) >= 0xFFE1u) {

            int v = (uint32_t) -mode2->address->value.integer & 0x1Fu;
            expr_Free(mode2->address);
            mode2->mode = ADDR_LITERAL_00 + v;
            return negatedParser(mode1, mode2, data);
        }
    }

    sect_OutputConst16((uint16_t) ((mode2->mode << 10u) | (mode1->mode << 4u) | data));
    if (mode1->address != NULL)
        sect_OutputExpr16(mode1->address);
    if (mode2->address != NULL)
        sect_OutputExpr16(mode2->address);

    return true;
}

static bool
subInstr(SAddressingMode* mode1, SAddressingMode* mode2, uint32_t data);

static bool
addInstr(SAddressingMode* mode1, SAddressingMode* mode2, uint32_t data) {
    return addSubInstr(mode1, mode2, 0x2, subInstr);
}

static bool
subInstr(SAddressingMode* mode1, SAddressingMode* mode2, uint32_t data) {
    return addSubInstr(mode1, mode2, 0x3, addInstr);
}

static bool
jsrInstr(SAddressingMode* mode1, SAddressingMode* mode2, uint32_t data) {
    sect_OutputConst16((uint16_t) ((data << 4u) | (mode1->mode << 10u)));
    if (mode1->address != NULL)
        sect_OutputExpr16(mode1->address);

    return true;
}

static SParser g_instructionHandlers[T_0X10C_XOR - T_0X10C_ADD + 1] = {
    {0x2, addInstr,     ADDRF_ALL, ADDRF_ALL},    // T_0X10C_ADD
    {0x9, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_AND
    {0xA, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_BOR
    {0x5, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_DIV
    {0xF, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFB
    {0xC, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFE
    {0xE, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFG
    {0xD, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFN
    {0x1, jsrInstr,     ADDRF_ALL, 0},            // T_0X10C_JSR
    {0x6, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_MOD
    {0x4, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_MUL
    {0x1, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SET
    {0x7, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SHL
    {0x8, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SHR
    {0x3, subInstr,     ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SUB
    {0xB, genericInstr, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_XOR
};

static uint32_t
translateToken(uint32_t token) {
    if (token == T_SYM_SET)
        return T_0X10C_SET;
    else
        return token;
}

bool
parse_IntegerInstruction(void) {
    uint32_t token = translateToken(lex_Current.token);

    if (T_0X10C_ADD <= token && token <= T_0X10C_XOR) {
        ETargetToken nToken = (ETargetToken) token;
        SParser* pParser = &g_instructionHandlers[nToken - T_0X10C_ADD];

        parse_GetToken();

        if (pParser->allowedModes2 != 0) {
            /* Two operands */
            SAddressingMode addrMode1;
            SAddressingMode addrMode2;

            if (!addressingMode(&addrMode1, pParser->allowedModes1))
                return prj_Error(MERROR_ILLEGAL_ADDRMODE);

            if (!parse_ExpectComma())
                return false;

            if (!addressingMode(&addrMode2, pParser->allowedModes2))
                return prj_Error(MERROR_ILLEGAL_ADDRMODE);

            return pParser->parser(&addrMode1, &addrMode2, pParser->data);
        } else {
            /* One operand */
            SAddressingMode addrMode1;

            if (!addressingMode(&addrMode1, pParser->allowedModes1))
                return prj_Error(MERROR_ILLEGAL_ADDRMODE);

            return pParser->parser(&addrMode1, NULL, pParser->data);
        }
    }

    return false;
}

SExpression*
x10c_ParseFunction(void) {
    return NULL;
}

bool
x10c_ParseInstruction(void) {
    return parse_IntegerInstruction();
}
