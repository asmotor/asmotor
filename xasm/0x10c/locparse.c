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
#include "parse.h"
#include "project.h"
#include "section.h"
#include "lexer.h"
#include "localasm.h"
#include "options.h"
#include "locopt.h"

static SExpression* parse_CheckSU16(SExpression* pExpr) {
	pExpr = expr_CheckRange(pExpr, -32768, 65535);
	if (pExpr == NULL)
		prj_Error(ERROR_OPERAND_RANGE);

	return expr_And(pExpr, expr_Const(0xFFFF));
}

static SExpression* parse_CheckU16(SExpression* pExpr) {
	pExpr = expr_CheckRange(pExpr, 0, 65535);
	if (pExpr == NULL)
		prj_Error(ERROR_OPERAND_RANGE);
	return pExpr;
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
	EAddrMode eMode;
	SExpression* pAddress;
} SAddrMode;

static SExpression* parse_ExpressionNoReservedIdentifiers() {
	SExpression* pExpr;

	opt_Push();
	g_pOptions->bAllowReservedIdentifierLabels = false;
	pExpr = parse_Expression(4);
	opt_Pop();

	return pExpr;
}

static bool_t parse_IndirectComponent(int* pRegister, SExpression** ppAddress) {
	if (g_CurrentToken.ID.TargetToken >= T_REG_A && g_CurrentToken.ID.TargetToken <= T_REG_J) {
		*pRegister = g_CurrentToken.ID.TargetToken - T_REG_A;
		*ppAddress = NULL;
		parse_GetToken();
		return true;
	} else {
		SExpression* pExpr = parse_ExpressionNoReservedIdentifiers();

		if (pExpr != NULL) {
			*pRegister = -1;
			*ppAddress = pExpr;
			return true;
		}
	}
	return false;
}

static bool_t parse_IndirectAddressing(SAddrMode* pMode, uint32_t nAllowedModes) {
	if (g_CurrentToken.ID.TargetToken == '[') {
		int nRegister = -1;
		SExpression* pAddress = NULL;

		parse_GetToken();

		if (!parse_IndirectComponent(&nRegister, &pAddress))
			return false;

		while (g_CurrentToken.ID.TargetToken == T_OP_ADD || g_CurrentToken.ID.TargetToken == T_OP_SUB) {
			if (g_CurrentToken.ID.TargetToken == T_OP_ADD) {
				int nRegister2 = -1;
				SExpression* pAddress2 = NULL;

				parse_GetToken();
				if (!parse_IndirectComponent(&nRegister2, &pAddress2))
					return false;

				if (nRegister2 >= 0) {
					if (nRegister == -1)
						nRegister = nRegister2;
					else
						prj_Error(MERROR_ADDRMODE_ONE_REGISTER);
				} else if (pAddress2 != NULL) {
					pAddress = pAddress == NULL ? pAddress2 : expr_Add(pAddress, pAddress2);
				} else {
					prj_Error(MERROR_ILLEGAL_ADDRMODE);
				}
			} else if (g_CurrentToken.ID.TargetToken == T_OP_SUB) {
				int nRegister2 = -1;
				SExpression* pAddress2 = NULL;

				parse_GetToken();
				if (!parse_IndirectComponent(&nRegister2, &pAddress2))
					return false;

				if (nRegister2 >= 0) {
					prj_Error(MERROR_ADDRMODE_SUBTRACT_REGISTER);
				} else if (pAddress2 != NULL) {
					pAddress = pAddress == NULL ? pAddress2 : expr_Sub(pAddress, pAddress2);
				} else {
					prj_Error(MERROR_ILLEGAL_ADDRMODE);
				}
			}
		}

		parse_ExpectChar(']');

		if (nRegister != -1 && pAddress == NULL) {
			EAddrMode mode = ADDR_A_IND + nRegister;
			if (nAllowedModes & (1u << mode)) {
				pMode->eMode = mode;
				pMode->pAddress = NULL;
				return true;
			}
		}

		if (nRegister != -1 && pAddress != NULL) {
			EAddrMode mode = ADDR_A_OFFSET_IND + nRegister;
			if (nAllowedModes & (1u << mode)) {
				pMode->eMode = mode;
				pMode->pAddress = parse_CheckSU16(pAddress);
				return true;
			}
		}

		if (nRegister == -1 && pAddress != NULL) {
			EAddrMode mode = ADDR_ADDRESS_IND;
			if (nAllowedModes & (1u << mode)) {
				pMode->eMode = mode;
				pMode->pAddress = parse_CheckU16(pAddress);
				return true;
			}
		}
	}

	return false;
}

static void parse_OptimizeAddressingMode(SAddrMode* pMode) {
	if (g_pOptions->pMachine->bOptimize) {
		/* Optimize literals <= 0x1F */
		if (pMode->eMode == ADDR_LITERAL && (pMode->pAddress->nFlags & EXPRF_CONSTANT)) {
			uint16_t v = (uint16_t) pMode->pAddress->Value.Value;
			if (v <= 0x1F) {
				pMode->eMode = ADDR_LITERAL_00 + v;
				expr_Free(pMode->pAddress);
				pMode->pAddress = NULL;
			}
		}

		/* Optimize [reg+0] to [reg] */
		if (pMode->eMode >= ADDR_A_OFFSET_IND && pMode->eMode <= ADDR_J_OFFSET_IND
			&& (pMode->pAddress->nFlags & EXPRF_CONSTANT) && pMode->pAddress->Value.Value == 0) {
			pMode->eMode = pMode->eMode - ADDR_A_OFFSET_IND + ADDR_A_IND;
			expr_Free(pMode->pAddress);
			pMode->pAddress = NULL;
		}
	}
}

static bool_t parse_AddressingMode(SAddrMode* pMode, uint32_t nAllowedModes) {
	switch (g_CurrentToken.ID.TargetToken) {
		case T_REG_A:
		case T_REG_B:
		case T_REG_C:
		case T_REG_X:
		case T_REG_Y:
		case T_REG_Z:
		case T_REG_I:
		case T_REG_J: {
			EAddrMode mode = ADDR_A + (g_CurrentToken.ID.TargetToken - T_REG_A);
			parse_GetToken();

			if (nAllowedModes & (1u << mode)) {
				pMode->eMode = mode;
				pMode->pAddress = NULL;

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
			EAddrMode eMode = ADDR_POP + (g_CurrentToken.ID.TargetToken - T_REG_POP);
			parse_GetToken();

			if (nAllowedModes & (1u << eMode)) {
				pMode->eMode = eMode;
				pMode->pAddress = NULL;

				return true;
			}
			break;
		}
		default: {
			if (parse_IndirectAddressing(pMode, nAllowedModes)) {
				return true;
			} else {
				SExpression* pAddress = parse_Expression(2);
				if (pAddress != NULL && (nAllowedModes & ADDRF_LITERAL)) {
					pMode->eMode = ADDR_LITERAL;
					pMode->pAddress = pAddress;
					parse_OptimizeAddressingMode(pMode);
					return true;
				}
			}
			break;
		}
	}

	return false;
}

typedef bool_t
(* ParserFunc)(SAddrMode* pMode1, SAddrMode* pMode2, uint32_t nData);

typedef struct {
	uint32_t nData;
	ParserFunc fpParser;
	uint32_t nAllowedModes1;
	uint32_t nAllowedModes2;
} SParser;

static bool_t parse_Basic(SAddrMode* pMode1, SAddrMode* pMode2, uint32_t nData) {
	sect_OutputConst16((uint16_t) ((pMode2->eMode << 10u) | (pMode1->eMode << 4u) | nData));
	if (pMode1->pAddress != NULL)
		sect_OutputExpr16(pMode1->pAddress);
	if (pMode2->pAddress != NULL)
		sect_OutputExpr16(pMode2->pAddress);

	return true;
}

static bool_t parse_ADD_SUB(SAddrMode* pMode1, SAddrMode* pMode2, uint32_t nData, ParserFunc negatedParser) {
	/* Optimize FUNC dest,-$1F */
	if (g_pOptions->pMachine->bOptimize) {
		if (pMode2->eMode == ADDR_LITERAL && (pMode2->pAddress->nFlags & EXPRF_CONSTANT)
			&& ((uint32_t) pMode2->pAddress->Value.Value & 0xFFFFu) >= 0xFFE1u) {

			int v = (uint32_t) -pMode2->pAddress->Value.Value & 0x1Fu;
			expr_Free(pMode2->pAddress);
			pMode2->eMode = ADDR_LITERAL_00 + v;
			return negatedParser(pMode1, pMode2, nData);
		}
	}

	sect_OutputConst16((uint16_t) ((pMode2->eMode << 10u) | (pMode1->eMode << 4u) | nData));
	if (pMode1->pAddress != NULL)
		sect_OutputExpr16(pMode1->pAddress);
	if (pMode2->pAddress != NULL)
		sect_OutputExpr16(pMode2->pAddress);

	return true;
}

static bool_t parse_SUB(SAddrMode* pMode1, SAddrMode* pMode2, uint32_t nData);

static bool_t parse_ADD(SAddrMode* pMode1, SAddrMode* pMode2, uint32_t nData) {
	assert(nData >= 0);
	return parse_ADD_SUB(pMode1, pMode2, 0x2, parse_SUB);
}

static bool_t parse_SUB(SAddrMode* pMode1, SAddrMode* pMode2, uint32_t nData) {
	assert(nData >= 0);
	return parse_ADD_SUB(pMode1, pMode2, 0x3, parse_ADD);
}

static bool_t parse_JSR(SAddrMode* pMode1, SAddrMode* pMode2, uint32_t nData) {
	assert(pMode2 == NULL);

	sect_OutputConst16((uint16_t) ((nData << 4u) | (pMode1->eMode << 10u)));
	if (pMode1->pAddress != NULL)
		sect_OutputExpr16(pMode1->pAddress);

	return true;
}

static SParser g_Parsers[T_0X10C_XOR - T_0X10C_ADD + 1] = {{0x2, parse_ADD,   ADDRF_ALL, ADDRF_ALL},    // T_0X10C_ADD
														   {0x9, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_AND
														   {0xA, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_BOR
														   {0x5, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_DIV
														   {0xF, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFB
														   {0xC, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFE
														   {0xE, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFG
														   {0xD, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_IFN
														   {0x1, parse_JSR,   ADDRF_ALL, 0},            // T_0X10C_JSR
														   {0x6, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_MOD
														   {0x4, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_MUL
														   {0x1, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SET
														   {0x7, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SHL
														   {0x8, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SHR
														   {0x3, parse_SUB,   ADDRF_ALL, ADDRF_ALL},    // T_0X10C_SUB
														   {0xB, parse_Basic, ADDRF_ALL, ADDRF_ALL},    // T_0X10C_XOR
};

bool_t parse_IntegerInstruction(void) {
	if (T_0X10C_ADD <= g_CurrentToken.ID.TargetToken && g_CurrentToken.ID.TargetToken <= T_0X10C_XOR) {
		ETargetToken nToken = (ETargetToken) g_CurrentToken.ID.TargetToken;
		SParser* pParser = &g_Parsers[nToken - T_0X10C_ADD];

		parse_GetToken();

		if (pParser->nAllowedModes2 != 0) {
			/* Two operands */
			SAddrMode addrMode1;
			SAddrMode addrMode2;

			if (!parse_AddressingMode(&addrMode1, pParser->nAllowedModes1))
				return prj_Error(MERROR_ILLEGAL_ADDRMODE);

			if (!parse_ExpectComma())
				return false;

			if (!parse_AddressingMode(&addrMode2, pParser->nAllowedModes2))
				return prj_Error(MERROR_ILLEGAL_ADDRMODE);

			return pParser->fpParser(&addrMode1, &addrMode2, pParser->nData);
		} else {
			/* One operand */
			SAddrMode addrMode1;

			if (!parse_AddressingMode(&addrMode1, pParser->nAllowedModes1))
				return prj_Error(MERROR_ILLEGAL_ADDRMODE);

			return pParser->fpParser(&addrMode1, NULL, pParser->nData);
		}
	}

	return false;
}

SExpression* parse_TargetFunction(void) {
	switch (g_CurrentToken.ID.TargetToken) {
		default:
			return NULL;
	}
}

bool_t parse_TargetSpecific(void) {
	if (parse_IntegerInstruction())
		return true;

	return false;
}
