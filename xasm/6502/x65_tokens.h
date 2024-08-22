/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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

	/* 65C02 */
	T_65C02_BRA,
	T_65C02_DEA,
	T_65C02_INA,
	T_65C02_PHX,
	T_65C02_PHY,
	T_65C02_PLX,
	T_65C02_PLY,
	T_65C02_STZ,
	T_65C02_TRB,
	T_65C02_TSB,

	/* WDC */
	T_65C02_STP,
	T_65C02_WAI,

	/* Rockwell + WDC */
	T_65C02_BBR,
	T_65C02_BBR0,
	T_65C02_BBR1,
	T_65C02_BBR2,
	T_65C02_BBR3,
	T_65C02_BBR4,
	T_65C02_BBR5,
	T_65C02_BBR6,
	T_65C02_BBR7,
	T_65C02_BBS,
	T_65C02_BBS0,
	T_65C02_BBS1,
	T_65C02_BBS2,
	T_65C02_BBS3,
	T_65C02_BBS4,
	T_65C02_BBS5,
	T_65C02_BBS6,
	T_65C02_BBS7,
	T_65C02_RMB,
	T_65C02_RMB0,
	T_65C02_RMB1,
	T_65C02_RMB2,
	T_65C02_RMB3,
	T_65C02_RMB4,
	T_65C02_RMB5,
	T_65C02_RMB6,
	T_65C02_RMB7,
	T_65C02_SMB,
	T_65C02_SMB0,
	T_65C02_SMB1,
	T_65C02_SMB2,
	T_65C02_SMB3,
	T_65C02_SMB4,
	T_65C02_SMB5,
	T_65C02_SMB6,
	T_65C02_SMB7,

	/* 65816 */
	T_65816_BRL,
	T_65816_COP,
	T_65816_JML,
	T_65816_JSL,
	T_65816_MVN,
	T_65816_MVP,
	T_65816_PEA,
	T_65816_PEI,
	T_65816_PER,
	T_65816_PHB,
	T_65816_PHD,
	T_65816_PHK,
	T_65816_PLB,
	T_65816_PLD,
	T_65816_REP,
	T_65816_RTL,
	T_65816_SEP,
	T_65816_TCD,
	T_65816_TCS,
	T_65816_TDC,
	T_65816_TSC,
	T_65816_TXY,
	T_65816_TYX,
	T_65816_WDM,
	T_65816_XBA,
	T_65816_XCE,

	/* 4510/45GS02 */
	T_4510_ASR,
	T_4510_ASW,
	T_4510_CLE,
	T_4510_CPZ,
	T_4510_DEW,
	T_4510_DEZ,
	T_4510_INW,
	T_4510_INZ,
	T_4510_LBCC,
	T_4510_LBCS,
	T_4510_LBEQ,
	T_4510_LBMI,
	T_4510_LBNE,
	T_4510_LBPL,
	T_4510_LBRA,
	T_4510_LBSR,
	T_4510_LBVS,
	T_4510_LDZ,
	T_4510_MAP,
	T_4510_NEG,
	T_4510_PHW,
	T_4510_PHZ,
	T_4510_PLZ,
	T_4510_ROW,
	T_4510_SEE,
	T_4510_TAB,
	T_4510_TAZ,
	T_4510_TBA,
	T_4510_TSY,
	T_4510_TYS,
	T_4510_TZA,

	T_45GS02_ASLQ,
	T_45GS02_ORAQ,

    /* Registers */
    T_6502_REG_A,
    T_6502_REG_X,
    T_6502_REG_Y,
    T_65816_REG_S,
	T_4510_REG_Z,

	/* Directives */
	T_65816_BITS,

} ETargetToken;

extern SLexConstantsWord*
x65_GetUndocumentedInstructions(int n);

extern SLexConstantsWord*
x65_Get4510Instructions(void);

extern void
x65_DefineTokens(void);

#endif
