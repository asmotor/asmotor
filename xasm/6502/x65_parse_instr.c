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
handleStandardAll(uint8_t baseOpcode, SAddressingMode* addrMode) {
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
handleStandardAbsY7(uint8_t baseOpcode, SAddressingMode* addrMode) {
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
handleStandardImm0(uint8_t baseOpcode, SAddressingMode* addrMode) {
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
handleStandardRotate(uint8_t baseOpcode, SAddressingMode* addrMode) {
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
handleBranch(uint8_t baseOpcode, SAddressingMode* addrMode) {
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
handleImplied(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);
    return true;
}

static bool
handleJMP(uint8_t baseOpcode, SAddressingMode* addrMode) {
    if (addrMode->mode == MODE_IND)
        baseOpcode += 0x20;

    sect_OutputConst8(baseOpcode);
    sect_OutputExpr16(addrMode->expr);
    return true;
}

static bool
handleBRK(uint8_t baseOpcode, SAddressingMode* addrMode) {
    sect_OutputConst8(baseOpcode);
    if (addrMode->mode == MODE_IMM)
        sect_OutputExpr8(addrMode->expr);
    return true;
}

static bool
handleDOP(uint8_t baseOpcode, SAddressingMode* addrMode) {
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
    { 0x61, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* ADC */
    { 0x21, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* AND */
    { 0x02, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* ASL */
    { 0x20, MODE_ZP | MODE_ABS, handleStandardAll },	/* BIT */
    { 0x10, MODE_ABS, handleBranch },	/* BPL */
    { 0x30, MODE_ABS, handleBranch },	/* BMI */
    { 0x50, MODE_ABS, handleBranch },	/* BVC */
    { 0x70, MODE_ABS, handleBranch },	/* BVS */
    { 0x90, MODE_ABS, handleBranch },	/* BCC */
    { 0xB0, MODE_ABS, handleBranch },	/* BCS */
    { 0xD0, MODE_ABS, handleBranch },	/* BNE */
    { 0xF0, MODE_ABS, handleBranch },	/* BEQ */
    { 0x00, MODE_NONE | MODE_IMM, handleBRK },	/* BRK */
    { 0xC1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* CMP */
    { 0xE0, MODE_IMM | MODE_ZP | MODE_ABS, handleStandardImm0 },	/* CPX */
    { 0xC0, MODE_IMM | MODE_ZP | MODE_ABS, handleStandardImm0 },	/* CPY */
    { 0xC2, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, handleStandardAll },	/* DEC */
    { 0x41, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* EOR */
    { 0x18, 0, handleImplied },	/* CLC */
    { 0x38, 0, handleImplied },	/* SEC */
    { 0x58, 0, handleImplied },	/* CLI */
    { 0x78, 0, handleImplied },	/* SEI */
    { 0xB8, 0, handleImplied },	/* CLV */
    { 0xD8, 0, handleImplied },	/* CLD */
    { 0xF8, 0, handleImplied },	/* SED */
    { 0xE2, MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, handleStandardAll },	/* INC */
    { 0x4C, MODE_ABS | MODE_IND, handleJMP },	/* JMP */
    { 0x20, MODE_ABS, handleJMP },	/* JSR */
    { 0xA1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* LDA */
    { 0xA2, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_Y | MODE_ABS_Y, handleStandardImm0 },	/* LDX */
    { 0xA0, MODE_IMM | MODE_ZP | MODE_ABS | MODE_ZP_X | MODE_ABS_X, handleStandardImm0 },	/* LDY */
    { 0x42, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* LSR */
    { 0xEA, 0, handleImplied },	/* NOP */
    { 0x01, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* ORA */
    { 0xAA, 0, handleImplied },	/* TAX */
    { 0x8A, 0, handleImplied },	/* TXA */
    { 0xCA, 0, handleImplied },	/* DEX */
    { 0xE8, 0, handleImplied },	/* INX */
    { 0xA8, 0, handleImplied },	/* TAY */
    { 0x98, 0, handleImplied },	/* TYA */
    { 0x88, 0, handleImplied },	/* DEY */
    { 0xC8, 0, handleImplied },	/* INY */
    { 0x22, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* ROL */
    { 0x62, MODE_A | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X, handleStandardRotate },	/* ROR */
    { 0x40, 0, handleImplied },	/* RTI */
    { 0x60, 0, handleImplied },	/* RTS */
    { 0xE1, MODE_IMM | MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* SBC */
    { 0x81, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* STA */
    { 0x9A, 0, handleImplied },	/* TXS */
    { 0xBA, 0, handleImplied },	/* TSX */
    { 0x48, 0, handleImplied },	/* PHA */
    { 0x68, 0, handleImplied },	/* PLA */
    { 0x08, 0, handleImplied },	/* PHP */
    { 0x28, 0, handleImplied },	/* PLP */
    { 0x82, MODE_ZP | MODE_ABS | MODE_ZP_Y, handleStandardImm0 },	/* STX */
    { 0x80, MODE_ZP | MODE_ABS | MODE_ZP_X, handleStandardImm0 },	/* STY */

    /* Undocumented instructions */

    { 0x0B, MODE_IMM, handleStandardImm0 },	/* AAC */
    { 0x83, MODE_ZP | MODE_ZP_Y | MODE_IND_X | MODE_ABS, handleStandardAll },	/* AAX */
    { 0x6B, MODE_IMM, handleStandardImm0 },	/* ARR */
    { 0x4B, MODE_IMM, handleStandardImm0 },	/* ASR */
    { 0xAB, MODE_IMM, handleStandardImm0 },	/* ATX */
    { 0x83, MODE_ABS_Y | MODE_IND_Y, handleStandardAbsY7 },	/* AXA */
    { 0xCB, MODE_IMM, handleStandardImm0 },	/* AXS */
    { 0xC3, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* DCP */
    { 0x00, MODE_ZP | MODE_ZP_X | MODE_IMM, handleDOP },	/* DOP */
    { 0xE3, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* ISC */
    { 0x02, 0, handleImplied },	/* KIL */
    { 0xA3, MODE_ABS_Y, handleStandardAll },	/* LAR */
    { 0xA3, MODE_ZP | MODE_ZP_Y | MODE_ABS | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAbsY7 },	/* LAX */
    { 0x43, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* RLA */
    { 0x63, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* RRA */
    { 0x03, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* SLO */
    { 0x43, MODE_ZP | MODE_ZP_X | MODE_ABS | MODE_ABS_X | MODE_ABS_Y | MODE_IND_X | MODE_IND_Y, handleStandardAll },	/* SRE */
    { 0x82, MODE_ABS_Y, handleStandardAbsY7 },	/* SXA */
    { 0x80, MODE_ABS_X, handleStandardAll },	/* SYA */
    { 0x00, MODE_ABS | MODE_ABS_X, handleStandardAll },	/* TOP */
    { 0x8B, MODE_IMM, handleStandardImm0 },	/* XAA */
    { 0x83, MODE_ABS_Y, handleStandardAll },	/* XAS */
};

bool
x65_ParseIntegerInstruction(void) {
    if (T_6502_ADC <= lex_Current.token && lex_Current.token <= T_6502U_XAS) {
        SAddressingMode addrMode;
        ETargetToken token = (ETargetToken) lex_Current.token;
        SParser* handler = &g_instructionHandlers[token - T_6502_ADC];

        parse_GetToken();
        if (x65_ParseAddressingMode(&addrMode, handler->allowedModes))
            return handler->handler(handler->baseOpcode, &addrMode);
        else
            err_Error(MERROR_ILLEGAL_ADDRMODE);
    }

    return false;
}
