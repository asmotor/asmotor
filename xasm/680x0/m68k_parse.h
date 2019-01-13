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

#ifndef	XASM_M68K_PARSE_H_INCLUDED_
#define	XASM_M68K_PARSE_H_INCLUDED_

#include "expression.h"

#define REGLIST_FAIL 65536

typedef enum
{
	AM_NONE = 0,
	AM_DREG          = 0x00000001,	// Dn
	AM_AREG          = 0x00000002,	// An
	AM_AIND          = 0x00000004,	// (An)
	AM_AINC          = 0x00000008,	// (An)+
	AM_ADEC          = 0x00000010,	// -(An)
	AM_ADISP         = 0x00000020,	// d16(An)
	AM_AXDISP        = 0x00000040,	// d8(An,Xn)
	AM_PCDISP        = 0x00000080,	// d16(PC)
	AM_PCXDISP       = 0x00000100,	// d8(PC,Xn)
	AM_WORD          = 0x00000200,	// (xxx).W
	AM_LONG          = 0x00000400,	// (xxx).L
	AM_IMM           = 0x00000800,	// #xxx
	AM_AXDISP020     = 0x00010000,	// (bd,An,Xn)
	AM_PREINDAXD020  = 0x00020000,	// ([bd,An,Xn],od)
	AM_POSTINDAXD020 = 0x00040000,	// ([bd,An],Xn,od)
	AM_PCXDISP020    = 0x00080000,	// (bd,PC,Xn)
	AM_PREINDPCXD020 = 0x00100000,	// ([bd,PC,Xn],od)
	AM_POSTINDPCXD020= 0x00200000,	// ([bd,PC],Xn,od)
	AM_SYSREG        = 0x01000000,	// CCR
	AM_FPUREG        = 0x02000000,	// FPU
	AM_BITFIELD      = 0x20000000,	// {offset:width}
	AM_EMPTY         = 0x40000000
} EAddrMode;

typedef enum
{
	SIZE_DEFAULT  = 0x00,
	SIZE_BYTE     = 0x01,
	SIZE_WORD     = 0x02,
	SIZE_LONG     = 0x04,
	SIZE_SINGLE   = 0x08,
	SIZE_DOUBLE   = 0x10,
	SIZE_EXTENDED = 0x20,
	SIZE_PACKED   = 0x40,
} ESize;

typedef enum {
	REG_D0 = 0,
	REG_D1 = 1,
	REG_D2 = 2,
	REG_D3 = 3,
	REG_D4 = 4,
	REG_D5 = 5,
	REG_D6 = 6,
	REG_D7 = 7,
	REG_A0 = 8,
	REG_A1 = 9,
	REG_A2 = 10,
	REG_A3 = 11,
	REG_A4 = 12,
	REG_A5 = 13,
	REG_A6 = 14,
	REG_A7 = 15,
	REG_FP0 = 16,
	REG_FP1 = 17,
	REG_FP2 = 18,
	REG_FP3 = 19,
	REG_FP4 = 20,
	REG_FP5 = 21,
	REG_FP6 = 22,
	REG_FP7 = 23,
	REG_PC = 16,
	REG_NONE = 255
} ERegister;

typedef struct
{
	ERegister nBaseReg;

	ERegister nIndexReg;
	ESize eIndexSize;
	SExpression* pIndexScale;

	SExpression* pDisp;
	ESize eDispSize;
} SModeRegs;

typedef struct
{
	EAddrMode	eMode;

	uint16_t	nDirectReg;

	SExpression* pImmediate;
	ESize	eImmSize;

	SModeRegs	Inner;
	SModeRegs	Outer;

	bool		bBitfield;
	int			nBFOffsetReg;
	SExpression* pBFOffsetExpr;
	int			nBFWidthReg;
	SExpression* pBFWidthExpr;

} SAddressingMode;

typedef struct
{
	uint32_t	nSourceModes;
	uint32_t	nDestModes;
} SIntInstruction;

typedef struct
{
	int	nCPU;
	ESize nAllowSize;
	ESize nDefaultSize;
	EAddrMode nAllowSrc;
	EAddrMode nAllowDest;
	EAddrMode nAllowSrc020;
	EAddrMode nAllowDest020;
	bool (*pHandler)(ESize eSize, SAddressingMode* pSrc, SAddressingMode* pDest);
} SInstruction;

extern SExpression* parse_Check8bit(SExpression* pExpr);
extern SExpression* parse_Check16bit(SExpression* pExpr);

extern bool parse_OutputExtWords(SAddressingMode* mode);

extern void parse_OptimizeDisp(SModeRegs* pRegs);
extern uint16_t parse_GetEAField(SAddressingMode* mode);
extern bool parse_GetAddrMode(SAddressingMode* pMode);
extern ESize parse_GetSizeSpec(ESize eDefault);

extern uint32_t parse_RegisterList(void);

extern bool parse_IntegerInstruction(void);
extern bool parse_FpuInstruction(void);
extern bool m68k_Directive(void);

#endif
