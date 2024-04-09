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

#ifndef XASM_V_TOKENS_H_INCLUDED_
#define XASM_V_TOKENS_H_INCLUDED_

typedef	enum {
	T_V_32I_ADD = 6000,
	T_V_32I_ADDI,
	T_V_32I_AND,
	T_V_32I_ANDI,
	T_V_32I_AUIPC,
	T_V_32I_BEQ,
	T_V_32I_BGE,
	T_V_32I_BGEU,
	T_V_32I_BLT,
	T_V_32I_BLTU,
	T_V_32I_BNE,
	T_V_32I_FENCE,
	T_V_32I_JAL,
	T_V_32I_JALR,
	T_V_32I_LB,
	T_V_32I_LBU,
	T_V_32I_LH,
	T_V_32I_LHU,
	T_V_32I_LUI,
	T_V_32I_LW,
	T_V_32I_OR,
	T_V_32I_ORI,
	T_V_32I_SB,
	T_V_32I_SH,
	T_V_32I_SLL,
	T_V_32I_SLLI,
	T_V_32I_SLT,
	T_V_32I_SLTI,
	T_V_32I_SLTIU,
	T_V_32I_SLTU,
	T_V_32I_SRA,
	T_V_32I_SRAI,
	T_V_32I_SRL,
	T_V_32I_SRLI,
	T_V_32I_SUB,
	T_V_32I_SW,
	T_V_32I_XOR,
	T_V_32I_XORI,

	/* Pseudo instructions */
	T_V_32I_J,		/* JAL x0,offset */
	T_V_32I_JR,		/* JALR x0,0(rs) */
	T_V_32I_RET,	/* JALR x0,0(x1) */

	T_V_32I_BEQZ,	/* BEQ rs,x0,offset */
	T_V_32I_BNEZ,	/* BNE rs,x0,offset */
	T_V_32I_BLEZ,	/* BGE x0,rs,offset */
	T_V_32I_BGEZ,	/* BGE rs,x0,offset */
	T_V_32I_BLTZ,	/* BLT rs,x0,offset */
	T_V_32I_BGTZ,	/* BLT x0,rs,offset */
	T_V_32I_BGT,	/* BLT rt,rs,offset */
	T_V_32I_BLE,	/* BGE rt,rs,offset */
	T_V_32I_BGTU,	/* BLTU rt,rs,offset */
	T_V_32I_BLEU,	/* BGEU rt,rs,offset */

	T_V_32I_MV,		/* ADDI rd,rs,0 */
	T_V_32I_NEG,	/* SUB rd,x0,rs */
	T_V_32I_NOT,	/* XORI rd,rs,-1 */
	T_V_32I_NOP,	/* ADDI x0,x0,0 */
	T_V_32I_SEQZ,	/* SLTUI rd,rs,1 */
	T_V_32I_SNEZ,	/* SLTU rd,x0,rs */
	T_V_32I_SLTZ,	/* SLT rd,rs,x0 */
	T_V_32I_SGTZ,	/* SLT rd,x0,rs */

	T_V_32I_LA,
	T_V_32I_LLA,
	T_V_32I_LI,
	T_V_32I_CALL,
	T_V_32I_TAIL,

	T_V_LAST = T_V_32I_TAIL,

	/* Registers */
	
	T_V_REG_X0,
	T_V_REG_X1,
	T_V_REG_X2,
	T_V_REG_X3,
	T_V_REG_X4,
	T_V_REG_X5,
	T_V_REG_X6,
	T_V_REG_X7,
	T_V_REG_X8,
	T_V_REG_X9,
	T_V_REG_X10,
	T_V_REG_X11,
	T_V_REG_X12,
	T_V_REG_X13,
	T_V_REG_X14,
	T_V_REG_X15,
	T_V_REG_X16,
	T_V_REG_X17,
	T_V_REG_X18,
	T_V_REG_X19,
	T_V_REG_X20,
	T_V_REG_X21,
	T_V_REG_X22,
	T_V_REG_X23,
	T_V_REG_X24,
	T_V_REG_X25,
	T_V_REG_X26,
	T_V_REG_X27,
	T_V_REG_X28,
	T_V_REG_X29,
	T_V_REG_X30,
	T_V_REG_X31
} ETargetToken;

extern void 
v_DefineTokens(void);

#endif
