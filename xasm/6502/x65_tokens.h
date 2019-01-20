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

#ifndef XASM_6502_TOKENS_H_INCLUDED_
#define XASM_6502_TOKENS_H_INCLUDED_

#include "lexer_constants.h"

typedef enum {
    T_6502_ADC = 6000,
    T_6502_AND,
    T_6502_ASL,
    T_6502_BIT,

    T_6502_BPL,
    T_6502_BMI,
    T_6502_BVC,
    T_6502_BVS,
    T_6502_BCC,
    T_6502_BCS,
    T_6502_BNE,
    T_6502_BEQ,

    T_6502_BRK,
    T_6502_CMP,
    T_6502_CPX,
    T_6502_CPY,
    T_6502_DEC,
    T_6502_EOR,

    T_6502_CLC,
    T_6502_SEC,
    T_6502_CLI,
    T_6502_SEI,
    T_6502_CLV,
    T_6502_CLD,
    T_6502_SED,

    T_6502_INC,
    T_6502_JMP,
    T_6502_JSR,
    T_6502_LDA,
    T_6502_LDX,
    T_6502_LDY,
    T_6502_LSR,
    T_6502_NOP,
    T_6502_ORA,

    T_6502_TAX,
    T_6502_TXA,
    T_6502_DEX,
    T_6502_INX,
    T_6502_TAY,
    T_6502_TYA,
    T_6502_DEY,
    T_6502_INY,

    T_6502_ROL,
    T_6502_ROR,
    T_6502_RTI,
    T_6502_RTS,
    T_6502_SBC,
    T_6502_STA,

    T_6502_TXS,
    T_6502_TSX,
    T_6502_PHA,
    T_6502_PLA,
    T_6502_PHP,
    T_6502_PLP,

    T_6502_STX,
    T_6502_STY,

    /* Undocumented instructions */

    T_6502U_AAC,
    T_6502U_AAX,
    T_6502U_ARR,
    T_6502U_ASR,
    T_6502U_ATX,
    T_6502U_AXA,
    T_6502U_AXS,
    T_6502U_DCP,
    T_6502U_DOP,
    T_6502U_ISC,
    T_6502U_KIL,
    T_6502U_LAR,
    T_6502U_LAX,
    T_6502U_RLA,
    T_6502U_RRA,
    T_6502U_SLO,
    T_6502U_SRE,
    T_6502U_SXA,
    T_6502U_SYA,
    T_6502U_TOP,
    T_6502U_XAA,
    T_6502U_XAS,

    /* Registers */

    T_6502_REG_A,
    T_6502_REG_X,
    T_6502_REG_Y,

} ETargetToken;

extern SLexConstantsWord*
loclexer_GetUndocumentedInstructions(int n);

#endif
