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

#if !defined(INTEGER_INSTRUCTIONS_SCHIP_)
#define INTEGER_INSTRUCTIONS_SCHIP_

#include <assert.h>

#define	MODE_REG	0x01
#define MODE_IMM	0x02
#define MODE_I		0x04
#define MODE_DT		0x08
#define MODE_ST		0x10
#define MODE_I_IND	0x20
#define MODE_IMM_V0	0x40
#define MODE_RPL	0x80

typedef struct
{
	uint8_t			nMode;
	uint8_t			nRegister;
	SExpression*	pExpr;
} SAddressMode;

static int parse_GetRegister(void)
{
	if(g_CurrentToken.Token >= T_CHIP_REG_V0
	&& g_CurrentToken.Token <= T_CHIP_REG_V15)
	{
		int r = g_CurrentToken.Token - T_CHIP_REG_V0;
		parse_GetToken();

		return r;
	}
	
	return -1;
}


static bool parse_AddressMode(SAddressMode* pMode)
{
	if((pMode->nRegister = (uint8_t)parse_GetRegister()) != 0xFF)
	{
		pMode->nMode = MODE_REG;
		if(g_CurrentToken.Token != T_OP_ADD)
			return true;

		parse_GetToken();
		if((pMode->pExpr = parse_ExpressionU12()) != NULL)
		{
			pMode->nMode = MODE_IMM_V0;
			return true;
		}
	}
	else if(g_CurrentToken.Token == T_CHIP_REG_I)
	{
		parse_GetToken();
		pMode->nMode = MODE_I;
		return true;
	}
	else if(g_CurrentToken.Token == T_CHIP_REG_RPL)
	{
		parse_GetToken();
		pMode->nMode = MODE_RPL;
		return true;
	}
	else if(g_CurrentToken.Token == T_CHIP_REG_DT)
	{
		parse_GetToken();
		pMode->nMode = MODE_DT;
		return true;
	}
	else if(g_CurrentToken.Token == T_CHIP_REG_ST)
	{
		parse_GetToken();
		pMode->nMode = MODE_ST;
		return true;
	}
	else if(g_CurrentToken.Token == T_CHIP_REG_I_IND)
	{
		parse_GetToken();
		pMode->nMode = MODE_I_IND;
		return true;
	}
	else if((pMode->pExpr = parse_Expression(1)) != NULL)
	{
		pMode->nMode = MODE_IMM;
		return true;
	}
	return false;
}

static bool parse_ModeReg(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode2 == NULL);
	assert(pMode3 == NULL);

	sect_OutputConst16(nOpcode | (pMode1->nRegister << 8));
	return true;
}

static bool parse_ModeRegReg(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode3 == NULL);

	sect_OutputConst16(nOpcode | (pMode1->nRegister << 8) | (pMode2->nRegister << 4));
	return true;
}


static bool parse_ModeImm12(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode2 == NULL);
	assert(pMode3 == NULL);

	sect_OutputExpr16(
		expr_Or(
			expr_Const(nOpcode),
			expr_CheckRange(pMode1->pExpr, 0, 4095)
		)
	);
	return true;
}


static bool parse_DRW(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	sect_OutputExpr16(
		expr_Or(
			expr_Const(nOpcode | (pMode1->nRegister << 8) | (pMode2->nRegister << 4)),
			expr_And(
				expr_CheckRange(pMode3->pExpr, 0, 16),
				expr_Const(15)
			)
		)
	);
	return true;
}


static bool parse_SCRD(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode1 == NULL);
	assert(pMode2 == NULL);

	sect_OutputExpr16(
		expr_Or(
			expr_Const(nOpcode),
			expr_CheckRange(pMode3->pExpr, 0, 15)
		)
	);
	return true;
}


static bool parse_ModeRegImm(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode3 == NULL);

	sect_OutputExpr16(
		expr_Or(
			expr_Const(nOpcode | (pMode1->nRegister << 8)),
			expr_And(
				expr_CheckRange(pMode2->pExpr, -128, 255),
				expr_Const(255)
			)
		)
	);
	return true;
}


static bool parse_LD(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(nOpcode >= 0);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_IMM)
		return parse_ModeRegImm(pMode1, pMode2, pMode3, 0x6000);
	
	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_REG)
		return parse_ModeRegReg(pMode1, pMode2, pMode3, 0x8000);

	if(pMode1->nMode == MODE_I && pMode2->nMode == MODE_IMM)
		return parse_ModeImm12(pMode2, NULL, NULL, 0xA000);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_DT)
		return parse_ModeReg(pMode1, NULL, NULL, 0xF007);

	if(pMode1->nMode == MODE_DT && pMode2->nMode == MODE_REG)
		return parse_ModeReg(pMode2, NULL, NULL, 0xF015);

	if(pMode1->nMode == MODE_ST && pMode2->nMode == MODE_REG)
		return parse_ModeReg(pMode2, NULL, NULL, 0xF018);

	return prj_Error(ERROR_OPERAND);
}


static bool parse_LDM(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode3 == NULL);
	assert(nOpcode >= 0);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_I_IND)
		return parse_ModeReg(pMode1, NULL, NULL, 0xF065);

	if(pMode1->nMode == MODE_I_IND && pMode2->nMode == MODE_REG)
		return parse_ModeReg(pMode2, NULL, NULL, 0xF055);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_RPL)
		return parse_ModeReg(pMode1, NULL, NULL, 0xF085);

	if(pMode1->nMode == MODE_RPL && pMode2->nMode == MODE_REG)
		return parse_ModeReg(pMode2, NULL, NULL, 0xF075);

	return prj_Error(ERROR_OPERAND);
}

static bool parse_ADD(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode3 == NULL);
	assert(nOpcode >= 0);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_REG)
		return parse_ModeRegReg(pMode1, pMode2, NULL, 0x8004);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_IMM)
		return parse_ModeRegImm(pMode1, pMode2, NULL, 0x7000);

	if(pMode1->nMode == MODE_I && pMode2->nMode == MODE_REG)
		return parse_ModeReg(pMode2, NULL, NULL, 0xF01E);

	return prj_Error(ERROR_OPERAND);
}

static bool parse_Skips(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode3 == NULL);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_IMM)
		return parse_ModeRegImm(pMode1, pMode2, NULL, nOpcode);

	if(pMode1->nMode == MODE_REG && pMode2->nMode == MODE_REG)
	{
		nOpcode = (uint16_t)(nOpcode == 0x3000 ? 0x5000 : 0x9000);
		return parse_ModeRegReg(pMode1, pMode2, NULL, nOpcode);
	}

	return prj_Error(ERROR_OPERAND);
}

static bool parse_JP(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode2 == NULL);
	assert(pMode3 == NULL);
	assert(nOpcode >= 0);

	if(pMode1->nMode == MODE_IMM)
		return parse_ModeImm12(pMode1, NULL, NULL, 0x1000);

	if(pMode1->nMode == MODE_IMM_V0)
		return parse_ModeImm12(pMode1, NULL, NULL, 0xB000);

	return prj_Error(ERROR_OPERAND);
}


static bool parse_ModeNone(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode)
{
	assert(pMode1 == NULL);
	assert(pMode2 == NULL);
	assert(pMode3 == NULL);

	sect_OutputConst16(nOpcode);
	return true;
}


typedef bool (*fpParser_t)(SAddressMode* pMode1, SAddressMode* pMode2, SAddressMode* pMode3, uint16_t nOpcode);

typedef struct
{
	uint8_t		nMode1;
	uint8_t		nMode2;
	uint8_t		nMode3;
	uint16_t	nOpcode;
	fpParser_t	fpParser;
} SInstruction;

SInstruction g_Parsers[T_CHIP_INSTR_LAST - T_CHIP_INSTR_FIRST + 1] =
{
	{ MODE_REG, 0, 0, 0xF033, parse_ModeReg },	// BCD
	{ MODE_REG, 0, 0, 0xF029, parse_ModeReg },	// LDF
	{ MODE_REG, 0, 0, 0xF030, parse_ModeReg },	// LDF10
	{ MODE_REG, 0, 0, 0x800E, parse_ModeReg },	// SHL
	{ MODE_REG, 0, 0, 0xE0A1, parse_ModeReg },	// SKNP
	{ MODE_REG, 0, 0, 0xE09E, parse_ModeReg },	// SKP
	{ MODE_REG, 0, 0, 0x8006, parse_ModeReg },	// SHR
	{ MODE_REG, 0, 0, 0xF00A, parse_ModeReg },	// WKP

	{ MODE_REG, MODE_REG, 0, 0x8002, parse_ModeRegReg },	// AND
	{ MODE_REG, MODE_REG, 0, 0x8001, parse_ModeRegReg },	// OR
	{ MODE_REG, MODE_REG, 0, 0x8005, parse_ModeRegReg },	// SUB
	{ MODE_REG, MODE_REG, 0, 0x8007, parse_ModeRegReg },	// SUBN
	{ MODE_REG, MODE_REG, 0, 0x8003, parse_ModeRegReg },	// XOR

	{ MODE_REG, MODE_REG, MODE_IMM, 0xD000, parse_DRW },	// DRW

	{ MODE_REG | MODE_I | MODE_DT | MODE_ST, MODE_REG | MODE_IMM | MODE_DT, 0, 0, parse_LD},	// LD

	{ MODE_REG | MODE_I_IND | MODE_RPL, MODE_REG | MODE_I_IND | MODE_RPL, 0, 0, parse_LDM},	// LDM

	{ MODE_REG | MODE_I, MODE_REG | MODE_IMM, 0, 0, parse_ADD},	// ADD

	{ MODE_REG, MODE_REG | MODE_IMM, 0, 0x3000, parse_Skips},	// SE
	{ MODE_REG, MODE_REG | MODE_IMM, 0, 0x4000, parse_Skips},	// SNE

	{ MODE_REG, MODE_IMM, 0, 0xC000, parse_ModeRegImm},	// RND

	{ MODE_IMM, 0, 0, 0x00C0, parse_SCRD},	// SCRD

	{ MODE_IMM | MODE_IMM_V0, 0, 0, 0x1000, parse_JP},	// JP

	{ MODE_IMM, 0, 0, 0x2000, parse_ModeImm12},	// CALL

	{ 0, 0, 0, 0x00E0, parse_ModeNone},	// CLS
	{ 0, 0, 0, 0x00FD, parse_ModeNone},	// EXIT
	{ 0, 0, 0, 0x00FE, parse_ModeNone},	// LO
	{ 0, 0, 0, 0x00FF, parse_ModeNone},	// HI
	{ 0, 0, 0, 0x00EE, parse_ModeNone},	// RET
	{ 0, 0, 0, 0x00FB, parse_ModeNone},	// SCRR
	{ 0, 0, 0, 0x00FC, parse_ModeNone},	// SCRL
};


bool parse_IntegerInstruction(void)
{
	if(T_CHIP_INSTR_FIRST <= g_CurrentToken.Token && g_CurrentToken.Token <= T_CHIP_INSTR_LAST)
	{
		bool r;
		SInstruction* pInstr = &g_Parsers[g_CurrentToken.Token - T_CHIP_INSTR_FIRST];
		SAddressMode mode1 = {0, 0, NULL};
		SAddressMode mode2 = {0, 0, NULL};
		SAddressMode mode3 = {0, 0, NULL};

		parse_GetToken();

		if(pInstr->nMode1 != 0 && parse_AddressMode(&mode1))
		{
			if((pInstr->nMode1 & mode1.nMode) == 0)
				return prj_Error(ERROR_OPERAND);

			if(pInstr->nMode2 != 0 && parse_ExpectComma() && parse_AddressMode(&mode2))
			{
				if((pInstr->nMode2 & mode2.nMode) == 0)
					return prj_Error(ERROR_OPERAND);

				if(pInstr->nMode3 != 0 && parse_ExpectComma() && parse_AddressMode(&mode3))
				{
					if((pInstr->nMode3 & mode3.nMode) == 0)
						return prj_Error(ERROR_OPERAND);
				}
			}
		}

		r = pInstr->fpParser(&mode1, &mode2, &mode3, pInstr->nOpcode);

		return r;
	}

	return false;
}


#endif
