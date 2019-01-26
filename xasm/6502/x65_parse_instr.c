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

#include "expression.h"
#include "lexer.h"
#include "parse.h"
#include "errors.h"

#include "x65_errors.h"
#include "x65_parse.h"
#include "x65_tokens.h"

typedef struct Parser {
    uint8_t baseOpcode;
    uint32_t allowedModes;
    bool
    (* handler)(uint8_t baseOpcode, SAddressingMode* addrMode);
} SParser;

static bool
parse_Standard_All(uint8_t baseOpcode, SAddressingMode* addrMode) {
    switch (addrMode->mode) {
        case MODE_IND_X:
            sect_OutputConst8(baseOpcode | (uint8_t) (0 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_IMM:
            sect_OutputConst8(baseOpcode | (uint8_t) (2 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_IND_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (4 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (6 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        case MODE_ZP:
            sect_OutputConst8(baseOpcode | (uint8_t) (1 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS:
            sect_OutputConst8(baseOpcode | (uint8_t) (3 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        case MODE_ZP_X:
        case MODE_ZP_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (5 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS_X:
            sect_OutputConst8(baseOpcode | (uint8_t) (7 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        default:
            err_Fail(MERROR_ILLEGAL_ADDRMODE);
            return true;
    }

}

static bool
parse_Standard_AbsY7(uint8_t baseOpcode, SAddressingMode* addrMode) {
    switch (addrMode->mode) {
        case MODE_IND_X:
            sect_OutputConst8(baseOpcode | (uint8_t) (0 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_IMM:
            sect_OutputConst8(baseOpcode | (uint8_t) (2 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_IND_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (4 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (7 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        case MODE_ZP:
            sect_OutputConst8(baseOpcode | (uint8_t) (1 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS:
            sect_OutputConst8(baseOpcode | (uint8_t) (3 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        case MODE_ZP_X:
        case MODE_ZP_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (5 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        default:
            err_Fail(MERROR_ILLEGAL_ADDRMODE);
            return true;
    }
}

static bool
parse_Standard_Imm0(uint8_t baseOpcode, SAddressingMode* addrMode) {
    switch (addrMode->mode) {
        case MODE_IMM:
            sect_OutputConst8(baseOpcode | (uint8_t) (0 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ZP:
            sect_OutputConst8(baseOpcode | (uint8_t) (1 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS:
            sect_OutputConst8(baseOpcode | (uint8_t) (3 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        case MODE_ZP_X:
        case MODE_ZP_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (5 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS_X:
        case MODE_ABS_Y:
            sect_OutputConst8(baseOpcode | (uint8_t) (7 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        default:
            err_Fail(MERROR_ILLEGAL_ADDRMODE);
            return true;
    }
}

static bool
parse_Standard_Rotate(uint8_t baseOpcode, SAddressingMode* addrMode) {
    switch (addrMode->mode) {
        case MODE_A:
            sect_OutputConst8(baseOpcode | (uint8_t) (2 << 2));
            return true;
        case MODE_ZP:
            sect_OutputConst8(baseOpcode | (uint8_t) (1 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS:
            sect_OutputConst8(baseOpcode | (uint8_t) (3 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        case MODE_ZP_X:
            sect_OutputConst8(baseOpcode | (uint8_t) (5 << 2));
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ABS_X:
            sect_OutputConst8(baseOpcode | (uint8_t) (7 << 2));
            sect_OutputExpr16(addrMode->expr);
            return true;
        default:
            err_Fail(MERROR_ILLEGAL_ADDRMODE);
            return true;
    }
}

static bool
parse_Branch(uint8_t baseOpcode, SAddressingMode* addrMode) {
    SExpression* pExpr;

    sect_OutputConst8(baseOpcode);
    pExpr = expr_PcRelative(addrMode->expr, -1);
    pExpr = expr_CheckRange(pExpr, -128, 127);
    if (pExpr == NULL) {
        err_Error(ERROR_OPERAND_RANGE);
        return true;
    } else {
        sect_OutputExpr8(pExpr);
    }

    return true;
}

static bool
parse_Implied(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);
    return true;
}

static bool
parse_JMP(uint8_t baseOpcode, SAddressingMode* addrMode) {
    if (addrMode->mode == MODE_IND)
        baseOpcode += 0x20;

    sect_OutputConst8(baseOpcode);
    sect_OutputExpr16(addrMode->expr);
    return true;
}

static bool
parse_BRK(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);
    if (addrMode->mode == MODE_IMM)
        sect_OutputExpr8(addrMode->expr);
    return true;
}

static bool
parse_DOP(uint8_t baseOpcode, SAddressingMode* addrMode) {
    switch (addrMode->mode) {
        case MODE_IMM:
            sect_OutputConst8(0x80);
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ZP:
            sect_OutputConst8(0x04);
            sect_OutputExpr8(addrMode->expr);
            return true;
        case MODE_ZP_X:
            sect_OutputConst8(0x14);
            sect_OutputExpr8(addrMode->expr);
            return true;
        default:
            return false;
    }
}

static SParser g_instructionHandlers[T_6502U_XAS - T_6502_ADC + 1] = {
    { 0x61, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* ADC */
    { 0x21, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* AND */
    { 0x02, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* ASL */
    { 0x20, MODE_ZP | MODE_ABS, parse_Standard_All },	/* BIT */
    { 0x10, MODE_ABS, parse_Branch },	/* BPL */
    { 0x30, MODE_ABS, parse_Branch },	/* BMI */
    { 0x50, MODE_ABS, parse_Branch },	/* BVC */
    { 0x70, MODE_ABS, parse_Branch },	/* BVS */
    { 0x90, MODE_ABS, parse_Branch },	/* BCC */
    { 0xB0, MODE_ABS, parse_Branch },	/* BCS */
    { 0xD0, MODE_ABS, parse_Branch },	/* BNE */
    { 0xF0, MODE_ABS, parse_Branch },	/* BEQ */
    { 0x00, MODE_NONE | MODE_IMM, parse_BRK },	/* BRK */
    { 0xC1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* CMP */
    { 0xE0, MODE_IMM | MODE_ZP | MODE_ABS, parse_Standard_Imm0 },	/* CPX */
    { 0xC0, MODE_IMM | MODE_ZP | MODE_ABS, parse_Standard_Imm0 },	/* CPY */
    { 0xC2, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, parse_Standard_All },	/* DEC */
    { 0x41, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* EOR */
    { 0x18, 0, parse_Implied },	/* CLC */
    { 0x38, 0, parse_Implied },	/* SEC */
    { 0x58, 0, parse_Implied },	/* CLI */
    { 0x78, 0, parse_Implied },	/* SEI */
    { 0xB8, 0, parse_Implied },	/* CLV */
    { 0xD8, 0, parse_Implied },	/* CLD */
    { 0xF8, 0, parse_Implied },	/* SED */
    { 0xE2, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, parse_Standard_All },	/* INC */
    { 0x4C, MODE_ABS | MODE_IND, parse_JMP },	/* JMP */
    { 0x20, MODE_ABS, parse_JMP },	/* JSR */
    { 0xA1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* LDA */
    { 0xA2, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_Y | MODE_ABS_Y, parse_Standard_Imm0 },	/* LDX */
    { 0xA0, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, parse_Standard_Imm0 },	/* LDY */
    { 0x42, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* LSR */
    { 0xEA, 0, parse_Implied },	/* NOP */
    { 0x01, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* ORA */
    { 0xAA, 0, parse_Implied },	/* TAX */
    { 0x8A, 0, parse_Implied },	/* TXA */
    { 0xCA, 0, parse_Implied },	/* DEX */
    { 0xE8, 0, parse_Implied },	/* INX */
    { 0xA8, 0, parse_Implied },	/* TAY */
    { 0x98, 0, parse_Implied },	/* TYA */
    { 0x88, 0, parse_Implied },	/* DEY */
    { 0xC8, 0, parse_Implied },	/* INY */
    { 0x22, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* ROL */
    { 0x62, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, parse_Standard_Rotate },	/* ROR */
    { 0x40, 0, parse_Implied },	/* RTI */
    { 0x60, 0, parse_Implied },	/* RTS */
    { 0xE1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* SBC */
    { 0x81, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* STA */
    { 0x9A, 0, parse_Implied },	/* TXS */
    { 0xBA, 0, parse_Implied },	/* TSX */
    { 0x48, 0, parse_Implied },	/* PHA */
    { 0x68, 0, parse_Implied },	/* PLA */
    { 0x08, 0, parse_Implied },	/* PHP */
    { 0x28, 0, parse_Implied },	/* PLP */
    { 0x82, MODE_ZP | MODE_ABS | MODE_ZP_Y, parse_Standard_Imm0 },	/* STX */
    { 0x80, MODE_ZP | MODE_ABS | MODE_ZP_X, parse_Standard_Imm0 },	/* STY */

    /* Undocumented instructions */

    { 0x0B, MODE_IMM, parse_Standard_Imm0 },	/* AAC */
    { 0x83, MODE_ZP | MODE_ZP_Y | MODE_IND_X | MODE_ABS, parse_Standard_All },	/* AAX */
    { 0x6B, MODE_IMM, parse_Standard_Imm0 },	/* ARR */
    { 0x4B, MODE_IMM, parse_Standard_Imm0 },	/* ASR */
    { 0xAB, MODE_IMM, parse_Standard_Imm0 },	/* ATX */
    { 0x83, MODE_ABS_Y | MODE_IND_Y, parse_Standard_AbsY7 },	/* AXA */
    { 0xCB, MODE_IMM, parse_Standard_Imm0 },	/* AXS */
    { 0xC3, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* DCP */
    { 0x00, MODE_ZP | MODE_ZP_X | MODE_IMM, parse_DOP },	/* DOP */
    { 0xE3, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* ISC */
    { 0x02, 0, parse_Implied },	/* KIL */
    { 0xA3, MODE_ABS_Y, parse_Standard_All },	/* LAR */
    { 0xA3, MODE_ZP | MODE_ZP_Y | MODE_ABS | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_AbsY7 },	/* LAX */
    { 0x43, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* RLA */
    { 0x63, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* RRA */
    { 0x03, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* SLO */
    { 0x43, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, parse_Standard_All },	/* SRE */
    { 0x82, MODE_ABS_Y, parse_Standard_AbsY7 },	/* SXA */
    { 0x80, MODE_ABS_X, parse_Standard_All },	/* SYA */
    { 0x00, MODE_ABS | MODE_ABS_X, parse_Standard_All },	/* TOP */
    { 0x8B, MODE_IMM, parse_Standard_Imm0 },	/* XAA */
    { 0x83, MODE_ABS_Y, parse_Standard_All },	/* XAS */
};

bool
parse_IntegerInstruction(void) {
    if (T_6502_ADC <= lex_Current.token && lex_Current.token <= T_6502U_XAS) {
        SAddressingMode addrMode;
        ETargetToken token = (ETargetToken) lex_Current.token;
        SParser* handler = &g_instructionHandlers[token - T_6502_ADC];

        parse_GetToken();
        if (parse_AddressingMode(&addrMode, handler->allowedModes))
            return handler->handler(handler->baseOpcode, &addrMode);
        else
            err_Error(MERROR_ILLEGAL_ADDRMODE);
    }

    return false;
}
