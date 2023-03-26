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

#ifndef XASM_6809_TOKENS_H_INCLUDED_
#define XASM_6809_TOKENS_H_INCLUDED_

#include "lexer_constants.h"

typedef enum {
    T_6809_ABX = 6000,
    T_6809_ADCA,
    T_6809_ADCB,
    T_6809_ADDA,
    T_6809_ADDB,
    T_6809_ADDD,
    T_6809_ANDA,
    T_6809_ANDB,
    T_6809_ANDCC,
    T_6809_ASL,
    T_6809_ASLA,
    T_6809_ASLB,
    T_6809_ASR,
    T_6809_ASRA,
    T_6809_ASRB,

	T_6809_BRA,
	T_6809_BRN,
	T_6809_BHI,
	T_6809_BLS,
	T_6809_BHS,
	T_6809_BLO,
	T_6809_BNE,
	T_6809_BEQ,
	T_6809_BVC,
	T_6809_BVS,
	T_6809_BPL,
	T_6809_BMI,
	T_6809_BGE,
	T_6809_BLT,
	T_6809_BGT,
	T_6809_BLE,
	T_6809_BSR,

	T_6809_LBRA,
	T_6809_LBRN,
	T_6809_LBHI,
	T_6809_LBLS,
	T_6809_LBHS,
	T_6809_LBLO,
	T_6809_LBNE,
	T_6809_LBEQ,
	T_6809_LBVC,
	T_6809_LBVS,
	T_6809_LBPL,
	T_6809_LBMI,
	T_6809_LBGE,
	T_6809_LBLT,
	T_6809_LBGT,
	T_6809_LBLE,
	T_6809_LBSR,

	T_6809_BITA,
	T_6809_BITB,

	T_6809_CLR,
	T_6809_CLRA,
	T_6809_CLRB,

	T_6809_CMPA,
	T_6809_CMPB,
	T_6809_CMPD,
	T_6809_CMPX,
	T_6809_CMPY,
	T_6809_CMPU,
	T_6809_CMPS,

	T_6809_COM,
	T_6809_COMA,
	T_6809_COMB,
	T_6809_CWAI,
	T_6809_DAA,
	T_6809_DEC,
	T_6809_DECA,
	T_6809_DECB,
	T_6809_EORA,
	T_6809_EORB,
	T_6809_EXG,
	T_6809_INC,
	T_6809_INCA,
	T_6809_INCB,
	T_6809_JMP,
	T_6809_JSR,

	T_6809_LDA,
	T_6809_LDB,
	T_6809_LDD,
	T_6809_LDX,
	T_6809_LDY,
	T_6809_LDU,
	T_6809_LDS,

	T_6809_LEAX,
	T_6809_LEAY,
	T_6809_LEAU,
	T_6809_LEAS,

	T_6809_LSR,
	T_6809_MUL,
	T_6809_NEG,
	T_6809_NEGA,
	T_6809_NEGB,
	T_6809_ORA,
	T_6809_ORB,
	T_6809_ORCC,

	T_6809_PSHS,
	T_6809_PSHU,
	T_6809_PULS,
	T_6809_PULU,

	T_6809_ROL,
	T_6809_ROLA,
	T_6809_ROLB,
	T_6809_ROR,
	T_6809_RORA,
	T_6809_RORB,

	T_6809_RTI,
	T_6809_RTS,

    T_6809_SBCA,
    T_6809_SBCB,

	T_6809_SEX,

	T_6809_STA,
	T_6809_STB,
	T_6809_STD,
	T_6809_STX,
	T_6809_STY,
	T_6809_STU,
	T_6809_STS,

	T_6809_SUBA,
	T_6809_SUBB,
	T_6809_SUBD,

    T_6809_SWI,
    T_6809_SWI2,
    T_6809_SWI3,
    T_6809_SYNC,

	T_6809_TFR,

	T_6809_TST,
	T_6809_TSTA,
	T_6809_TSTB,

    T_6809_NOP,

    /* Registers */

    T_6809_REG_A,
    T_6809_REG_B,
    T_6809_REG_D,
    T_6809_REG_PCR,
    T_6809_REG_CCR,
    T_6809_REG_DPR,

	// These four must be in order
    T_6809_REG_X,
    T_6809_REG_Y,
   	T_6809_REG_U,
    T_6809_REG_S,

	// Directive
	T_6809_SETDP

} ETargetToken;

extern SLexConstantsWord*
m6809_GetUndocumentedInstructions(int n);

extern void
m6809_DefineTokens(void);

#endif
