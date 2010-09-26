/*  Copyright 2008 Carsten Sørensen

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

/* GB Z80 instruction groups

 n3 = 3-bit
 n  = 8-bit
 nn = 16-bit

 */

#include "../common/xasm.h"

//	Register r group

enum
{
	REG_B=0,
	REG_C,
	REG_D,
	REG_E,
	REG_H,
	REG_L,
	REG_HL_IND,
	REG_A
};

//	Register rr group

enum
{
	REG_BC_IND=0,
	REG_DE_IND,
	REG_HL_INDINC,
	REG_HL_INDDEC,
};

//	Register ss group

enum
{
	REG_BC=0,
	REG_DE,
	REG_HL,
	REG_SP
};

//	Register tt group

//#define REG_BC	0
//#define REG_DE	1
//#define REG_HL	2
#define	REG_AF	3

//	Conditioncode cc group

enum
{
	CC_NZ=0,
	CC_Z,
	CC_NC,
	CC_C
};

static	BOOL	parse_GetOptional_AComma(void)
{
	if(g_CurrentToken.ID.TargetToken==T_MODE_A)
	{
		parse_GetToken();
		if(g_CurrentToken.ID.TargetToken==',')
		{
			parse_GetToken();
			return TRUE;
		}
		else
		{
			prj_Error(ERROR_CHAR_EXPECTED, ',');
			return FALSE;
		}

	}
	else
	{
		return TRUE;
	}
}

static	SLONG	parse_ConditionCode(void)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case	T_CC_NZ:
		{
			parse_GetToken();
			return CC_NZ;
			break;
		}
		case	T_CC_Z:
		{
			parse_GetToken();
			return CC_Z;
			break;
		}
		case	T_CC_NC:
		{
			parse_GetToken();
			return CC_NC;
			break;
		}
		case	T_MODE_C:
		{
			parse_GetToken();
			return CC_C;
			break;
		}
		default:
		{
			return -1;
			break;
		}
	}
}

static	SLONG	parse_Register_SS(void)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case	T_MODE_BC:
		{
			parse_GetToken();
			return REG_BC;
			break;
		}
		case	T_MODE_DE:
		{
			parse_GetToken();
			return REG_DE;
			break;
		}
		case	T_MODE_HL:
		{
			parse_GetToken();
			return REG_HL;
			break;
		}
		case	T_MODE_SP:
		{
			parse_GetToken();
			return REG_SP;
			break;
		}
		default:
		{
			return -1;
			break;
		}
	}
}

static	SLONG	parse_Register_RR(void)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case	T_MODE_BC_IND:
		{
			parse_GetToken();
			return REG_BC_IND;
			break;
		}
		case	T_MODE_DE_IND:
		{
			parse_GetToken();
			return REG_DE_IND;
			break;
		}
		case	T_MODE_HL_INDINC:
		{
			parse_GetToken();
			return REG_HL_INDINC;
			break;
		}
		case	T_MODE_HL_INDDEC:
		{
			parse_GetToken();
			return REG_HL_INDDEC;
			break;
		}
		default:
		{
			return -1;
			break;
		}
	}
}

static	SLONG	parse_Register_TT(void)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case	T_MODE_BC:
		{
			parse_GetToken();
			return REG_BC;
			break;
		}
		case	T_MODE_DE:
		{
			parse_GetToken();
			return REG_DE;
			break;
		}
		case	T_MODE_HL:
		{
			parse_GetToken();
			return REG_HL;
			break;
		}
		case	T_MODE_AF:
		{
			parse_GetToken();
			return REG_AF;
			break;
		}
		default:
		{
			return -1;
			break;
		}
	}
}

static SLONG parse_Register_8bit(void)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case	T_MODE_A:
		{
			parse_GetToken();
			return REG_A;
			break;
		}
		case	T_MODE_B:
		{
			parse_GetToken();
			return REG_B;
			break;
		}
		case	T_MODE_C:
		{
			parse_GetToken();
			return REG_C;
			break;
		}
		case	T_MODE_D:
		{
			parse_GetToken();
			return REG_D;
			break;
		}
		case	T_MODE_E:
		{
			parse_GetToken();
			return REG_E;
			break;
		}
		case	T_MODE_H:
		{
			parse_GetToken();
			return REG_H;
			break;
		}
		case	T_MODE_L:
		{
			parse_GetToken();
			return REG_L;
			break;
		}
		case	T_MODE_HL_IND:
		{
			parse_GetToken();
			return REG_HL_IND;
			break;
		}
		default:
		{
			return -1;
			break;
		}
	}
}

static BOOL parse_ExpectRegisterA(void)
{
	SLONG r = parse_Register_8bit();
	if(r == REG_A)
		return TRUE;

	prj_Error(MERROR_EXPECT_A);
	return FALSE;
}

static SExpression* parse_ExpressionPCRel(void)
{
	SExpression* pExpr = parse_Expression();

	if(pExpr != NULL)
	{
		pExpr = parse_CreatePCRelExpr(pExpr, -1);
		pExpr = parse_CheckRange(pExpr, -128, 127);
		if(pExpr != NULL)
			return pExpr;

		prj_Error(ERROR_EXPRESSION_N_BIT, 8);
		return NULL;
	}

	prj_Error(ERROR_INVALID_EXPRESSION);
	return NULL;
}

static SExpression* parse_Expression16bit(void)
{
	SExpression* expr;

	if((expr=parse_Expression())!=NULL)
	{
		if((expr=parse_CheckRange(expr,-32768,65535))==NULL)
		{
			prj_Error(ERROR_EXPRESSION_N_BIT, 16);
			return NULL;
		}
		else
		{
			return expr;
		}
	}
	else
	{
		prj_Error(ERROR_INVALID_EXPRESSION);
		return NULL;
	}
}

static	SExpression* parse_Expression8bit(void)
{
	SExpression* expr;

	if((expr=parse_Expression())!=NULL)
	{
		if((expr=parse_CheckRange(expr,-128,255))==NULL)
		{
			prj_Error(ERROR_EXPRESSION_N_BIT, 8);
			return NULL;
		}
		else
		{
			return expr;
		}
	}
	else
	{
		prj_Error(ERROR_INVALID_EXPRESSION);
		return NULL;
	}
}

static	SExpression* parse_Expression3bit(void)
{
	SExpression* expr;

	if((expr=parse_Expression())!=NULL)
	{
		if((expr=parse_CheckRange(expr,0,7))==NULL)
		{
			prj_Error(ERROR_EXPRESSION_N_BIT, 3);
			return NULL;
		}
		else
		{
			return expr;
		}
	}
	else
	{
		prj_Error(ERROR_INVALID_EXPRESSION);
		return NULL;
	}
}

static	void	parse_GetArithmeticOpcodes(SLONG* oc1, SLONG* oc2)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case	T_Z80_ADC:
		{
			//	ADC A,n : 0xCE
			//	ADC A,r	: 0x88|r
			*oc1=0xCE;
			*oc2=0x88;
			break;
		}
		case	T_Z80_AND:
		{
			//	AND A,n	: 0xE6
			//	AND A,r	: 0xA0|r
			*oc1=0xE6;
			*oc2=0xA0;
			break;
		}
		case	T_Z80_CP:
		{
			//	CP  A,n	: 0xFE
			//	CP  A,r	: 0xB8|r
			*oc1=0xFE;
			*oc2=0xB8;
			break;
		}
		case	T_Z80_OR:
		{
			//	OR  A,n	: 0xF6
			//	OR  A,r	: 0xB0|r
			*oc1=0xF6;
			*oc2=0xB0;
			break;
		}
		case	T_Z80_SBC:
		{
			//	SBC A,n	: 0xDE
			//	SBC A,r	: 0x98|r
			*oc1=0xDE;
			*oc2=0x98;
			break;
		}
		case	T_Z80_SUB:
		{
			//	SUB A,n	: 0xD6
			//	SUB A,r	: 0x90|r
			*oc1=0xD6;
			*oc2=0x90;
			break;
		}
		case	T_Z80_XOR:
		{
			//	XOR A,n	: 0xEE
			//	XOR A,r	: 0xA8|r
			*oc1=0xEE;
			*oc2=0xA8;
			break;
		}
	}
}

BOOL	parse_TargetSpecific(void)
{
	switch(g_CurrentToken.ID.TargetToken)
	{
		case	T_Z80_NOP:
		{
			//	NOP : 0x00
			parse_GetToken();
			sect_OutputAbsByte(0x00);
			return TRUE;
			break;
		}
		case	T_Z80_CCF:
		{
			//	CCF : 0x3F
			parse_GetToken();
			sect_OutputAbsByte(0x3F);
			return TRUE;
			break;
		}
		case	T_Z80_CPL:
		{
			//	CPL : 0x2F
			parse_GetToken();
			sect_OutputAbsByte(0x2F);
			return TRUE;
			break;
		}
		case	T_Z80_DAA:
		{
			//	DAA	: 0x27
			parse_GetToken();
			sect_OutputAbsByte(0x27);
			return TRUE;
			break;
		}
		case	T_Z80_DI:
		{
			//	DI : 0xF3
			parse_GetToken();
			sect_OutputAbsByte(0xF3);
			return TRUE;
			break;
		}
		case	T_Z80_EI:
		{
			//	EI : 0xFB
			parse_GetToken();
			sect_OutputAbsByte(0xFB);
			return TRUE;
			break;
		}
		case	T_Z80_HALT:
		{
			//	HALT : 0x76
			parse_GetToken();
			sect_OutputAbsByte(0x76);
			return TRUE;
			break;
		}
		case	T_Z80_RET:
		{
			//	RET    : 0xC9
			//	RET cc : 0xC0|(cc<<3)
			SLONG	cc;

			parse_GetToken();
			if((cc=parse_ConditionCode())!=-1)
			{
				sect_OutputAbsByte((UBYTE)(0xC0|(cc<<3)));
			}
			else
			{
				sect_OutputAbsByte(0xC9);
			}
			return TRUE;
			break;
		}
		case	T_Z80_RETI:
		{
			//	RETI : 0xD9
			parse_GetToken();
			sect_OutputAbsByte(0xD9);
			return TRUE;
			break;
		}
		case	T_Z80_RLA:
		{
			//	RLA : 0x17
			parse_GetToken();
			sect_OutputAbsByte(0x17);
			return TRUE;
			break;
		}
		case	T_Z80_RLCA:
		{
			//	RLCA : 0x07
			parse_GetToken();
			sect_OutputAbsByte(0x07);
			return TRUE;
			break;
		}
		case	T_Z80_RRA:
		{
			//	RRA : 0x1F
			parse_GetToken();
			sect_OutputAbsByte(0x1F);
			return TRUE;
			break;
		}
		case	T_Z80_RRCA:
		{
			//	RRCA : 0x0F
			parse_GetToken();
			sect_OutputAbsByte(0x0F);
			return TRUE;
			break;
		}
		case	T_Z80_SCF:
		{
			//	SCF : 0x37
			parse_GetToken();
			sect_OutputAbsByte(0x37);
			return TRUE;
			break;
		}
		case	T_Z80_STOP:
		{
			//	STOP : 0x10
			parse_GetToken();
			sect_OutputAbsByte(0x10);
			return TRUE;
			break;
		}
		case	T_Z80_ADC:
		case	T_Z80_AND:
		case	T_Z80_CP:
		case	T_Z80_OR:
		case	T_Z80_SBC:
		case	T_Z80_SUB:
		case	T_Z80_XOR:
		{
			//	ADC A,n : 0xCE
			//	ADC A,r	: 0x88|r
			//	AND A,n	: 0xE6
			//	AND A,r	: 0xA0|r
			//	CP  A,n	: 0xFE
			//	CP  A,r	: 0xB8|r
			//	OR  A,n	: 0xF6
			//	OR  A,r	: 0xB0|r
			//	SBC A,n	: 0xDE
			//	SBC A,r	: 0x98|r
			//	SUB A,n	: 0xD6
			//	SUB A,r	: 0x90|r
			//	XOR A,n	: 0xEE
			//	XOR A,r	: 0xA8|r
			SLONG	opcode1,
					opcode2;

			parse_GetArithmeticOpcodes(&opcode1, &opcode2);

			parse_GetToken();
			if(parse_GetOptional_AComma())
			{
				SLONG	reg;

				if((reg=parse_Register_8bit())==-1)
				{
					//	ADC A,n : 0xCE

					SExpression* expr;

					if((expr=parse_Expression8bit())!=NULL)
					{
						sect_OutputAbsByte((UBYTE)(opcode1));
						sect_OutputExprByte(expr);
					}
					return TRUE;
				}
				else
				{
					//	ADC A,r	: 0x88|r
					sect_OutputAbsByte((UBYTE)(opcode2|reg));
					return TRUE;
				}
			}
			return FALSE;
			break;
		}
		case	T_Z80_ADD:
		{
			parse_GetToken();
			if(g_CurrentToken.ID.TargetToken==T_MODE_HL)
			{
				//	ADD HL,ss : 0x09|(ss<<4)
				SLONG	reg;

				parse_GetToken();
				if(parse_ExpectComma())
				{
					if((reg=parse_Register_SS())!=-1)
					{
						sect_OutputAbsByte((UBYTE)(0x09|(reg<<4)));
						return TRUE;
					}
					else
					{
						prj_Error(ERROR_SOURCE_OPERAND);
						return FALSE;
					}
				}
				else
				{
					return FALSE;
				}
			}
			else if(g_CurrentToken.ID.TargetToken==T_MODE_SP)
			{
				//	ADD SP,n  : 0xE8
				parse_GetToken();
				if(parse_ExpectComma())
				{
					SExpression* expr;

					if((expr=parse_Expression8bit())!=NULL)
					{
						sect_OutputAbsByte(0xE8);
						sect_OutputExprByte(expr);
					}
					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
			else
			{
				//	ADD A,n : 0xC6
				//	ADD A,r : 0x80|r
				//	ADD n   : 0xC6
				//	ADD r   : 0x80|r

				SLONG	reg;

				if(g_CurrentToken.ID.TargetToken==T_MODE_A)
				{
					parse_GetToken();
					if(g_CurrentToken.ID.TargetToken!=',')
					{
						//	ADD A
						//	special case
						sect_OutputAbsByte(0x80|REG_A);
						return TRUE;
					}
					parse_GetToken();
				}

				if((reg=parse_Register_8bit())!=-1)
				{
					//	ADD A,r : 0x80|r
					sect_OutputAbsByte((UBYTE)(0x80|reg));
					return TRUE;
				}
				else
				{
					//	ADD A,n : 0xC6
					SExpression* expr;

					if((expr=parse_Expression8bit())!=NULL)
					{
						sect_OutputAbsByte(0xC6);
						sect_OutputExprByte(expr);
					}
					return TRUE;
				}
			}
			break;
		}
		case	T_Z80_BIT:
		case	T_Z80_RES:
		case	T_POP_SET:
		{
			//	BIT n3,r : 0xCB 0x40|(n3<<3)|r
			//	RES n3,r : 0xCB 0x80|(n3<<3)|r
			//	SET n3,r : 0xCB 0xC0|(n8<<3)|r
			SLONG	opcode;
			SExpression* expr;

			if(g_CurrentToken.ID.TargetToken==T_Z80_BIT)
			{
				opcode=0x40;
			}
			else if(g_CurrentToken.ID.TargetToken==T_Z80_RES)
			{
				opcode=0x80;
			}
			else
			{
				opcode=0xC0;
			}

			parse_GetToken();

			if((expr=parse_Expression3bit())!=NULL)
			{
				SLONG	reg;
				parse_ExpectComma();
				if((reg=parse_Register_8bit())!=-1)
				{
					SExpression* expr2;

					expr2=parse_CreateConstExpr(opcode|reg);
					sect_OutputExprByte(parse_CreateORExpr(expr,expr2));
					return TRUE;
				}
				else
				{
					prj_Error(ERROR_OPERAND);
				}
			}
			return FALSE;
			break;
		}
		case	T_Z80_CALL:
		{
			SLONG	cc;

			parse_GetToken();
			if((cc=parse_ConditionCode())!=-1)
			{
				//	CALL cc,nn : 0xC4|(cc<<3)
				if(parse_ExpectComma())
				{
					SExpression* expr;

					if((expr=parse_Expression16bit())!=NULL)
					{
						sect_OutputAbsByte((UBYTE)(0xC4|(cc<<3)));
						sect_OutputExprWord(expr);
						return TRUE;
					}
				}
				return FALSE;
			}
			else
			{
				//	CALL nn    : 0xCD
				SExpression* expr;

				if((expr=parse_Expression16bit())!=NULL)
				{
					sect_OutputAbsByte(0xCD);
					sect_OutputExprWord(expr);
					return TRUE;
				}
				return FALSE;
			}
			break;
		}
		case	T_Z80_DEC:
		{
			SLONG	reg;

			parse_GetToken();

			if((reg=parse_Register_8bit())!=-1)
			{
				//	DEC r : 0x05|(r<<3)
				sect_OutputAbsByte((UBYTE)(0x05|(reg<<3)));
				return TRUE;
			}
			else if((reg=parse_Register_SS())!=-1)
			{
				//	DEC ss : 0x0B|(ss<<4)
				sect_OutputAbsByte((UBYTE)(0x0B|(reg<<4)));
				return TRUE;
			}
			else
			{
				return FALSE;
			}
			break;
		}
		case	T_Z80_INC:
		{
			SLONG	reg;

			parse_GetToken();

			if((reg=parse_Register_8bit())!=-1)
			{
				//	INC r : 0x04|(r<<3)
				sect_OutputAbsByte((UBYTE)(0x04|(reg<<3)));
				return TRUE;
			}
			else if((reg=parse_Register_SS())!=-1)
			{
				//	INC ss : 0x03|(ss<<4)
				sect_OutputAbsByte((UBYTE)(0x03|(reg<<4)));
				return TRUE;
			}
			else
			{
				return FALSE;
			}
			break;
		}
		case	T_Z80_EX:
		{
			//	EX HL,(SP) : 0xE3

			SLONG	r1,
					r2;

			parse_GetToken();
			r1=g_CurrentToken.ID.TargetToken;
			parse_GetToken();
			if(parse_ExpectComma())
			{
				r2=g_CurrentToken.ID.TargetToken;
				parse_GetToken();

				if( (r1==T_MODE_HL && r2==T_MODE_SP_IND)
				||	(r2==T_MODE_HL && r1==T_MODE_SP_IND) )
				{
					sect_OutputAbsByte(0xE3);
					return TRUE;
				}

				prj_Error(ERROR_OPERAND);
				return TRUE;
			}
			return FALSE;
			break;
		}
		case	T_Z80_JP:
		{
			SLONG	cc;

			parse_GetToken();

			if(g_CurrentToken.ID.TargetToken==T_MODE_HL_IND)
			{
				//	JP (HL)  : 0xE9
				parse_GetToken();
				sect_OutputAbsByte(0xE9);
				return TRUE;
			}
			else if((cc=parse_ConditionCode())!=-1)
			{
				//	JP cc,nn : 0xC2|(cc<<3)
				if(parse_ExpectComma())
				{
					SExpression* expr;

					if((expr=parse_Expression16bit())!=NULL)
					{
						sect_OutputAbsByte((UBYTE)(0xC2|(cc<<3)));
						sect_OutputExprWord(expr);
						return TRUE;
					}
				}
				return FALSE;
			}
			else
			{
				//	JP nn    : 0xC3|(cc<<3)
				SExpression* expr;

				if((expr=parse_Expression16bit())!=NULL)
				{
					sect_OutputAbsByte(0xC3);
					sect_OutputExprWord(expr);
					return TRUE;
				}
				return FALSE;
			}
			break;
		}
		case	T_Z80_RLC:
		case	T_Z80_RRC:
		case	T_Z80_RL:
		case	T_Z80_RR:
		case	T_Z80_SLA:
		case	T_Z80_SRA:
		case	T_Z80_SWAP:
		case	T_Z80_SRL:
		{
			//	RLC  r : 0xCB 0x00|r
			//	RRC  r : 0xCB 0x08|r
			//	RL   r : 0xCB 0x10|r
			//	RR   r : 0xCB 0x18|r
			//	SLA  r : 0xCB 0x20|r
			//	SRA  r : 0xCB 0x28|r
			//	SWAP r : 0xCB 0x30|r
			//	SRL  r : 0xCB 0x38|r
			SLONG	opcode,
					reg;

			switch(g_CurrentToken.ID.TargetToken)
			{
				case	T_Z80_RLC:
				{
					opcode=0x00;
					break;
				}
				case	T_Z80_RRC:
				{
					opcode=0x08;
					break;
				}
				case	T_Z80_RL:
				{
					opcode=0x10;
					break;
				}
				case	T_Z80_RR:
				{
					opcode=0x18;
					break;
				}
				case	T_Z80_SLA:
				{
					opcode=0x20;
					break;
				}
				case	T_Z80_SRA:
				{
					opcode=0x28;
					break;
				}
				case	T_Z80_SWAP:
				{
					opcode=0x30;
					break;
				}
				case	T_Z80_SRL:
				{
					opcode=0x38;
					break;
				}
				default:
				{
					internalerror("unknown opcode");
					opcode = 0;
					break;
				}
			}

			parse_GetToken();
			if((reg=parse_Register_8bit())!=-1)
			{
				sect_OutputAbsByte(0xCB);
				sect_OutputAbsByte((UBYTE)(opcode|reg));
				return TRUE;
			}
			else
			{
				prj_Error(ERROR_OPERAND);
				return FALSE;
			}

			break;
		}
		case	T_Z80_POP:
		case	T_Z80_PUSH:
		{
			//	POP  tt : 0xC1|(tt<<4)
			//	PUSH tt : 0xC5|(tt<<4)
			SLONG	opcode,
					reg;

			if(g_CurrentToken.ID.TargetToken==T_Z80_POP)
			{
				opcode=0xC1;
			}
			else
			{
				opcode=0xC5;
			}

			parse_GetToken();

			if((reg=parse_Register_TT())!=-1)
			{
				sect_OutputAbsByte((UBYTE)(opcode|(reg<<4)));
				return TRUE;
			}
			else
			{
				prj_Error(ERROR_OPERAND);
				return FALSE;
			}
		}
		case	T_Z80_RST:
		{
			//	RST n : 0xC7|n
			SLONG	val;

			parse_GetToken();
			val=parse_ConstantExpression();
			if(val==(val&0x38))
			{
				sect_OutputAbsByte((UBYTE)(0xC7|val));
			}
			else
			{
				prj_Error(ERROR_OPERAND);
			}
			return TRUE;
			break;
		}
		case	T_Z80_LDH:
		{
			SLONG	reg;

			parse_GetToken();
			if((reg = parse_Register_8bit()) != -1)
			{
				if(reg == REG_A)
				{
					if(parse_ExpectComma() && parse_ExpectChar('['))
					{
						SExpression* expr;

						if((expr = parse_Expression16bit()) != NULL)
						{
							if(parse_ExpectChar(']'))
							{
								SLONG val = expr->Value.Value & 0xFFFF;

								if(expr->Flags & EXPRF_isCONSTANT)
								{
									if((val & 0xFF00) == 0xFF00
									|| (val & 0xFF00) == 0x0000)
									{
										//	LD A,($FF00+n) : 0xF0

										sect_OutputAbsByte(0xF0);
										sect_OutputAbsByte((UBYTE)(val & 0xFF));
										return TRUE;
									}
									else
									{
										prj_Error(ERROR_OPERAND_RANGE);
										return FALSE;
									}
								}
								else if(expr->Flags & EXPRF_isRELOC)
								{
									expr = parse_CreateANDExpr(parse_CreateConstExpr(0xFF), expr);
									sect_OutputAbsByte(0xF0);
									sect_OutputExprByte(expr);
									return TRUE;
								}
							}
						}
					}
				}
				return FALSE;
			}
			else
			{
				if(parse_ExpectChar('['))
				{
					SExpression* expr;

					if((expr = parse_Expression16bit()) != NULL)
					{
						if(parse_ExpectChar(']') && parse_ExpectComma() && parse_ExpectRegisterA())
						{
							if(expr->Flags & EXPRF_isCONSTANT)
							{
								SLONG val = expr->Value.Value & 0xFFFF;

								if((val & 0xFF00) == 0xFF00
								|| (val & 0xFF00) == 0x0000)
								{
									//	LD ($FF00+n),A : 0xE0
									sect_OutputAbsByte(0xE0);
									sect_OutputAbsByte((UBYTE)(val & 0xFF));
									return TRUE;
								}
								else
								{
									prj_Error(ERROR_OPERAND_RANGE);
									return FALSE;
								}
							}
							else if(expr->Flags&EXPRF_isRELOC)
							{
								expr=parse_CreateANDExpr(parse_CreateConstExpr(0xFF), expr);
								sect_OutputAbsByte(0xE0);
								sect_OutputExprByte(expr);
								return TRUE;
							}
						}
					}
				}
			}
			return FALSE;
			break;
		}
		case	T_Z80_LD:
		{
			SLONG	reg;

			parse_GetToken();
			if((reg=parse_Register_8bit())!=-1)
			{
				if(parse_ExpectComma())
				{
					SLONG	reg2;

					if((reg2=parse_Register_8bit())!=-1)
					{
						//	LD r,r' 	   : 0x40|(r<<3)|r' // NOTE: LD (HL),(HL) not allowed
						if(reg==REG_HL_IND && reg2==REG_HL_IND)
						{
							prj_Error(ERROR_OPERAND);
						}
						else
						{
							sect_OutputAbsByte((UBYTE)(0x40|(reg<<3)|reg2));
						}
						return TRUE;
					}
					else if(reg==REG_A)
					{
						if((reg2=parse_Register_RR())!=-1)
						{
							//	LD A,(rr)	   : 0x0A|(rr<<4)
							sect_OutputAbsByte((UBYTE)(0x0A|(reg2<<4)));
							return TRUE;
						}
						else if(g_CurrentToken.ID.TargetToken==T_MODE_C_IND)
						{
							//	LD A,($FF00+C) : 0xF2
							parse_GetToken();
							sect_OutputAbsByte(0xF2);
							return TRUE;
						}
						else
						{
							if(g_CurrentToken.ID.TargetToken=='[')
							{
								SExpression* expr;

								parse_GetToken();

								if((expr=parse_Expression16bit())!=NULL)
								{
									SLONG	val;

									val=expr->Value.Value&0xFFFF;

									if( expr->Flags&EXPRF_isCONSTANT
									&&  (val&0xFF00)==0xFF00 )
									{
										if(g_CurrentToken.ID.TargetToken==']')
										{
											//	LD A,($FF00+n) : 0xF0

											sect_OutputAbsByte(0xF0);
											sect_OutputAbsByte((UBYTE)(val&0xFF));
											parse_GetToken();
											return TRUE;
										}
										else
										{
											prj_Error(ERROR_CHAR_EXPECTED, ']');
											return FALSE;
										}
									}
									else
									{
										if(g_CurrentToken.ID.TargetToken==']')
										{
											//	LD A,(nn)	   : 0xFA

											sect_OutputAbsByte(0xFA);
											sect_OutputExprWord(expr);
											parse_GetToken();
											return TRUE;
										}
										else
										{
											prj_Error(ERROR_CHAR_EXPECTED, ']');
											return FALSE;
										}
									}
								}
								return FALSE;
							}
							else
							{
								//	LD r,n		   : 0x06|(r<<3)
								SExpression* expr;

								if((expr=parse_Expression8bit())!=NULL)
								{
									sect_OutputAbsByte(0x06|(REG_A<<3));
									sect_OutputExprByte(expr);
									return TRUE;
								}
								return FALSE;
							}
						}

					}
					else
					{
						//	LD r,n		   : 0x06|(r<<3)
						SExpression* expr;

						if((expr=parse_Expression8bit())!=NULL)
						{
							sect_OutputAbsByte((UBYTE)(0x06|(reg<<3)));
							sect_OutputExprByte(expr);
							return TRUE;
						}
						return FALSE;
					}
				}
			}
			else if((reg=parse_Register_RR())!=-1)
			{
				if(parse_ExpectComma())
				{
					if(g_CurrentToken.ID.TargetToken==T_MODE_A)
					{
						//	LD (rr),A	   : 0x02|(rr<<4)
						parse_GetToken();
						sect_OutputAbsByte((UBYTE)(0x02|(reg<<4)));
						return TRUE;
					}
					else
					{
						prj_Error(MERROR_EXPECT_A);
					}
				}
				return FALSE;
			}
			else if((reg=parse_Register_SS())!=-1)
			{
				if(parse_ExpectComma())
				{
					if( reg==REG_SP
					&&	g_CurrentToken.ID.TargetToken==T_MODE_HL )
					{
						//	LD SP,HL	   : 0xF9
						parse_GetToken();
						sect_OutputAbsByte(0xF9);
						return TRUE;
					}
					else if( reg==REG_HL
						 &&  g_CurrentToken.ID.TargetToken=='[' )
					{
						parse_GetToken();
						if(g_CurrentToken.ID.TargetToken==T_MODE_SP)
						{
							SExpression* expr;
							parse_GetToken();
							if((expr=parse_Expression8bit())!=NULL)
							{
								if(g_CurrentToken.ID.TargetToken==']')
								{
									//	LD HL,(SP+n)   : 0xF8
									parse_GetToken();
									sect_OutputAbsByte(0xF8);
									sect_OutputExprByte(expr);
									return TRUE;
								}
								else
								{
									prj_Error(ERROR_CHAR_EXPECTED, ']');
								}
							}
						}
						else
						{
							prj_Error(MERROR_EXPECT_SP);
						}
					}
					else
					{
						SExpression* expr;

						//	LD ss,nn	   : 0x01|(ss<<4)
						if((expr=parse_Expression16bit())!=NULL)
						{
							sect_OutputAbsByte((UBYTE)(0x01|(reg<<4)));
							sect_OutputExprWord(expr);
							return TRUE;
						}
					}
				}
				return FALSE;
			}
			else if(g_CurrentToken.ID.TargetToken==T_MODE_C_IND)
			{
				parse_GetToken();

				if(parse_ExpectComma())
				{
					if(g_CurrentToken.ID.TargetToken==T_MODE_A)
					{
						//	LD ($FF00+C),A : 0xE2
						parse_GetToken();
						sect_OutputAbsByte(0xE2);
						return TRUE;
					}
					else
					{
						prj_Error(MERROR_EXPECT_A);
					}
				}
				return FALSE;
			}
			else
			{
				if(g_CurrentToken.ID.TargetToken=='[')
				{
					SExpression* expr;

					parse_GetToken();
					if((expr=parse_Expression16bit())!=NULL)
					{
						if(g_CurrentToken.ID.TargetToken==']')
						{
							parse_GetToken();
							if(parse_ExpectComma())
							{
								if(g_CurrentToken.ID.TargetToken==T_MODE_SP)
								{
									//	LD (nn),SP	   : 0x08
									parse_GetToken();
									sect_OutputAbsByte(0x08);
									sect_OutputExprWord(expr);
									return TRUE;
								}
								else if(g_CurrentToken.ID.TargetToken==T_MODE_A)
								{
									SLONG	val;

									parse_GetToken();

									val=expr->Value.Value&0xFFFF;
									if( expr->Flags&EXPRF_isCONSTANT
									&&	(val&0xFF00)==0xFF00 )
									{
										//	LD ($FF00+n),A : 0xE0
										sect_OutputAbsByte(0xE0);
										sect_OutputAbsByte((UBYTE)(val&0xFF));
										return TRUE;
									}
									else
									{
										//	LD (nn),A	   : 0xEA
										sect_OutputAbsByte(0xEA);
										sect_OutputExprWord(expr);
										return TRUE;
									}
								}
								else
								{
									prj_Error(ERROR_OPERAND);
								}
							}
						}
						else
						{
							prj_Error(ERROR_CHAR_EXPECTED, ']');
						}
					}
				}

				return FALSE;
			}

		}
		case	T_Z80_JR:
		{
			SLONG	cc;

			parse_GetToken();

			if((cc = parse_ConditionCode()) != -1)
			{
				if(parse_ExpectComma())
				{
					//	JR cc,n : 0x20|(cc<<3)
					SExpression* expr;

					sect_OutputAbsByte((UBYTE)(0x20|(cc<<3)));
					if((expr=parse_ExpressionPCRel())!=NULL)
					{
						sect_OutputExprByte(expr);
						return TRUE;
					}
				}
				return FALSE;

			}
			else
			{
				//	JR n    : 0x18
				SExpression* expr;

				sect_OutputAbsByte(0x18);
				if((expr=parse_ExpressionPCRel())!=NULL)
				{
					sect_OutputExprByte(expr);
					return TRUE;
				}
				return FALSE;
			}
		}
		default:
		{
			return FALSE;
			break;
		}
	}
}

SExpression* parse_TargetFunction(void)
{
	return NULL;
}
