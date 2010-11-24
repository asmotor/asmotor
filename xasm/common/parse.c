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

#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "asmotor.h"
#include "types.h"
#include "mem.h"
#include "expr.h"
#include "project.h"
#include "fstack.h"
#include "lexer.h"
#include "parse.h"
#include "symbol.h"
#include "section.h"
#include "globlex.h"
#include "options.h"

extern bool_t parse_TargetSpecific(void);
extern SExpression* parse_TargetFunction(void);




/*	Private routines*/

static bool_t parse_isWhiteSpace(char s)
{
	return s == ' ' || s == '\t' || s == '\0' || s == '\n';
}

static bool_t parse_isToken(char* s, char* token)
{
	int len = (int)strlen(token);
	return _strnicmp(s, token, len) == 0 && parse_isWhiteSpace(s[len]);
}

static bool_t parse_isRept(char* s)
{
	return parse_isToken(s, "REPT");
}

static bool_t parse_isEndr(char* s)
{
	return parse_isToken(s, "ENDR");
}

bool_t parse_isIf(char* s)
{
	return
		parse_isToken(s, "IF")
	||	parse_isToken(s, "IFC")
	||	parse_isToken(s, "IFD")
	||	parse_isToken(s, "IFNC")
	||	parse_isToken(s, "IFND")
	||	parse_isToken(s, "IFNE")
	||	parse_isToken(s, "IFEQ")
	||	parse_isToken(s, "IFGT")
	||	parse_isToken(s, "IFGE")
	||	parse_isToken(s, "IFLT")
	||	parse_isToken(s, "IFLE");
}

bool_t parse_isElse(char* s)
{
	return parse_isToken(s, "ELSE");
}

bool_t parse_isEndc(char* s)
{
	return parse_isToken(s, "ENDC");
}

bool_t parse_isMacro(char* s)
{
	return parse_isToken(s, "MACRO");
}

bool_t parse_isEndm(char* s)
{
	return parse_isToken(s, "ENDM");
}

static char* parse_SkipToLine(char* s)
{
	while(*s != 0)
	{
		if(*s++ == '\n')
			return s;
	}

	return NULL;
}

static char* parse_GetLineToken(char* s)
{
	if(s == NULL)
		return NULL;

	if(parse_isRept(s)
	|| parse_isEndr(s)
	|| parse_isIf(s)
	|| parse_isElse(s)
	|| parse_isEndc(s)
	|| parse_isMacro(s)
	|| parse_isEndm(s))
	{
		return s;
	}

	while(!parse_isWhiteSpace(*s))
	{
		if(*s++ == ':')
			break;
	}
	while(parse_isWhiteSpace(*s))
		++s;

	return s;
}

static int parse_GetIfLength(char* s);


static int parse_GetReptLength(char* s)
{
	char* start = s;
	char* token;

	s = parse_SkipToLine(s);
	while(s != NULL && (token = parse_GetLineToken(s)) != NULL)
	{
		if(parse_isRept(token))
		{
			token += 4;
			s = token + parse_GetReptLength(token);
			s = parse_SkipToLine(s);
		}
		else if(parse_isEndr(token))
		{
			return (int)(token - start);
		}
		else
		{
			s = parse_SkipToLine(s);
		}
	}

	return 0;
}

static int parse_GetMacroLength(char* s)
{
	char* start = s;
	char* token;

	s = parse_SkipToLine(s);
	while((token = parse_GetLineToken(s)) != NULL)
	{
		if(parse_isRept(token))
		{
			token += 4;	// 4 = strlen("REPT")
			s = token + parse_GetReptLength(token) + 4; // 4 = strlen("ENDR")
			s = parse_SkipToLine(s);
		}
		else if(parse_isIf(token))
		{
			while(!parse_isWhiteSpace(*token))
				++token;
			s = token + parse_GetIfLength(token) + 4;	// 4 = strlen("ENDC")
			s = parse_SkipToLine(s);
		}
		else if(parse_isMacro(token))
		{
			token += 5;
			s = token + parse_GetMacroLength(token) + 4;
			s = parse_SkipToLine(s);
		}
		else if(parse_isEndm(token))
		{
			return (int)(token - start);
		}
		else
		{
			s = parse_SkipToLine(s);
		}
	}

	return 0;
}

static int parse_GetIfLength(char* s)
{
	char* start = s;
	char* token;

	s = parse_SkipToLine(s);
	while((token = parse_GetLineToken(s)) != NULL)
	{
		if(parse_isRept(token))
		{
			token += 4;	// 4 = strlen("REPT")
			s = token + parse_GetMacroLength(token) + 4; // 4 = strlen("ENDR")
			s = parse_SkipToLine(s);
		}
		else if(parse_isMacro(token))
		{
			token += 5;	// 5 = strlen("MACRO")
			s = token + parse_GetMacroLength(token) + 4; // 4 = strlen("ENDM")
			s = parse_SkipToLine(s);
		}
		else if(parse_isIf(token))
		{
			while(!parse_isWhiteSpace(*token))
				++token;
			s = token + parse_GetIfLength(token) + 4;	// 4 = strlen("ENDC")
			s = parse_SkipToLine(s);
		}
		else if(_strnicmp(token, "ENDC", 4) == 0)
		{
			return (int)(token - start);
		}
		else
		{
			s = parse_SkipToLine(s);
		}
	}

	return 0;
}

static bool_t parse_CopyRept(char** newmacro, uint32_t* size)
{
	char* src = g_pFileContext->pLexBuffer->pBuffer;
	int len = parse_GetReptLength(src);
	int32_t	i;

	if(len == 0)
		return false;

	*size = len;

	*newmacro = (char*)mem_Alloc(len + 1);
	(*newmacro)[len]=0;
	for(i=0; i<len; i+=1)
	{
		(*newmacro)[i] = src[i];
	}

	lex_SkipBytes(len + 4);
	return true;
}

bool_t parse_IfSkipToElse(void)
{
	char* src = g_pFileContext->pLexBuffer->pBuffer;
	char* s = src;
	char* token;

	s = parse_SkipToLine(s);
	while((token = parse_GetLineToken(s)) != NULL)
	{
		if(parse_isRept(token))
		{
			token += 4;
			s = token + parse_GetMacroLength(token) + 4; // 4 = strlen("ENDR")
			s = parse_SkipToLine(s);
		}
		else if(parse_isMacro(token))
		{
			token += 5;
			s = token + parse_GetMacroLength(token) + 4; // 4 = strlen("ENDM")
			s = parse_SkipToLine(s);
		}
		else if(parse_isIf(token))
		{
			while(!parse_isWhiteSpace(*token))
				++token;
			s = token + parse_GetIfLength(token) + 4; // 4 = strlen("ENDC");
			s = parse_SkipToLine(s);
		}
		else if(_strnicmp(token, "ENDC", 4) == 0)
		{
			lex_SkipBytes((int)(token - src));
			g_pFileContext->LineNumber++;
			return true;
		}
		else if(_strnicmp(token, "ELSE", 4) == 0)
		{
			lex_SkipBytes((int)(token + 4 - src));
			g_pFileContext->LineNumber++;
			return true;
		}
		else
		{
			s = parse_SkipToLine(s);
		}
	}

	return false;
}

bool_t parse_IfSkipToEndc(void)
{
	char* src = g_pFileContext->pLexBuffer->pBuffer;
	char* s = src;
	char* token;

	s = parse_SkipToLine(s);
	while((token = parse_GetLineToken(s)) != NULL)
	{
		if(parse_isIf(token))
		{
			while(!parse_isWhiteSpace(*token))
				++token;
			s = token + parse_GetIfLength(token);
			s = parse_SkipToLine(s);
		}
		else if(_strnicmp(token, "ENDC", 4) == 0)
		{
			lex_SkipBytes((int)(token - src));
			return true;
		}
		else
		{
			s = parse_SkipToLine(s);
		}
	}

	return 0;
}

bool_t	parse_CopyMacro(char** dest, uint32_t* size)
{
	char* src = g_pFileContext->pLexBuffer->pBuffer;
	int len = parse_GetMacroLength(src);
	int32_t	i;

	*size = len;

	*dest = (char*)mem_Alloc(len + 1);
	(*dest)[len] = 0;
	for(i = 0; i < len; ++i)
	{
		(*dest)[i] = src[i];
	}

	lex_SkipBytes(len + 4);
	return true;
}




static uint32_t parse_ColonCount(void)
{
	if(g_CurrentToken.ID.Token == ':')
	{
		parse_GetToken();
		if(g_CurrentToken.ID.Token == ':')
		{
			parse_GetToken();
			return 2;
		}
	}
	return 1;
}

bool_t parse_IsDot(SLexBookmark* pBookmark)
{
	if(pBookmark)
		lex_Bookmark(pBookmark);

	if(g_CurrentToken.ID.Token == '.')
	{
		parse_GetToken();
		return true;
	}

	if(g_CurrentToken.ID.Token == T_ID && g_CurrentToken.Value.aString[0] == '.')
	{
		lex_RewindBytes(strlen(g_CurrentToken.Value.aString) - 1);
		parse_GetToken();
		return true;
	}

	return false;
}

bool_t parse_ExpectChar(char ch)
{
	if(g_CurrentToken.ID.TargetToken == ch)
	{
		parse_GetToken();
		return true;
	}
	else
	{
		prj_Error(ERROR_CHAR_EXPECTED, ch);
		return false;
	}
}



/*
 *	Expression parser
 */

static SExpression* parse_ExprPri0(void);

static SExpression* parse_ExprPri9(void)
{
	switch(g_CurrentToken.ID.Token)
	{
		case T_STRING:
		{
			int len = (int)strlen(g_CurrentToken.Value.aString);
			if(len <= 4)
			{
				int32_t val = 0;
				int i;
				for(i = 0; i < len; ++i)
				{
					val = val << 8;
					val |= (uint8_t)g_CurrentToken.Value.aString[i];
				}
				parse_GetToken();
				return expr_Const(val);
			}
			return NULL;
		}
		case T_NUMBER:
		{
			int32_t val = g_CurrentToken.Value.nInteger;
			parse_GetToken();
			return expr_Const(val);
			break;
		}
		case '(':
		{
			SExpression* expr;
			SLexBookmark bookmark;

			lex_Bookmark(&bookmark);

			parse_GetToken();
			expr = parse_ExprPri0();
			if(expr != NULL)
			{
				if(g_CurrentToken.ID.Token == ')')
				{
					parse_GetToken();
					return expr;
				}

				expr_Free(expr);
			}

			lex_Goto(&bookmark);
			return NULL;
			break;
		}
		case T_ID:
		{
			if(strcmp(g_CurrentToken.Value.aString, "@") != 0)
			{
				SExpression* expr = expr_Symbol(g_CurrentToken.Value.aString);
				parse_GetToken();
				return expr;
			}
			// fall through to @
		}
		case T_OP_MUL:
		case '@':
		{
			SExpression* expr = expr_Pc();
			parse_GetToken();
			return expr;
		}
		default:
		{
			if(g_CurrentToken.TokenLength > 0 && g_CurrentToken.ID.Token >= T_FIRST_TOKEN)
			{
				SExpression* expr = expr_Symbol(g_CurrentToken.Value.aString);
				parse_GetToken();
				return expr;
			}
			return NULL;
		}

	}
}

static char* parse_StringExpression(void);
static char* parse_StringExpressionRaw_Pri0(void);

int32_t parse_StringCompare(char* s)
{
	int32_t r = 0;
	char* t;

	parse_GetToken();
	if((t = parse_StringExpression()) != NULL)
	{
		r = strcmp(s, t);
		mem_Free(t);
	}
	mem_Free(s);

	return r;
}

static SExpression* parse_ExprPri8(void)
{
	SLexBookmark bm;
	char* s;

	lex_Bookmark(&bm);
	if((s = parse_StringExpressionRaw_Pri0()) != NULL)
	{
		if(parse_IsDot(NULL))
		{
			switch(g_CurrentToken.ID.Token)
			{
				case T_FUNC_COMPARETO:
				{
					SExpression* r = NULL;
					char* t;

					parse_GetToken();

					if(parse_ExpectChar('('))
					{
						if((t = parse_StringExpression()) != NULL)
						{
							if(parse_ExpectChar(')'))
								r = expr_Const(strcmp(s, t));

							mem_Free(t);
						}
					}

					mem_Free(s);
					return r;
				}
				case T_FUNC_LENGTH:
				{
					SExpression* r = expr_Const((int32_t)strlen(s));
					mem_Free(s);
					parse_GetToken();

					return r;
				}
				case T_FUNC_INDEXOF:
				{
					SExpression* r = NULL;
					parse_GetToken();

					if(parse_ExpectChar('('))
					{
						char* needle;
						if((needle = parse_StringExpression()) != NULL)
						{
							if(parse_ExpectChar(')'))
							{
								char* p;
								int32_t val = -1;

								if((p = strstr(s, needle)) != NULL)
									val = (int32_t)(p - s);

								r = expr_Const(val);
							}
							mem_Free(needle);
						}
					}
					mem_Free(s);
					return r;
				}
				case T_OP_LOGICEQU:
				{
					int32_t v = parse_StringCompare(s);
					return expr_Const(v == 0 ? true : false);
				}
				case T_OP_LOGICNE:
				{
					int32_t v = parse_StringCompare(s);
					return expr_Const(v != 0 ? true : false);
				}
				case T_OP_LOGICGE:
				{
					int32_t v = parse_StringCompare(s);
					return expr_Const(v >= 0 ? true : false);
				}
				case T_OP_LOGICGT:
				{
					int32_t v = parse_StringCompare(s);
					return expr_Const(v > 0 ? true : false);
				}
				case T_OP_LOGICLE:
				{
					int32_t v = parse_StringCompare(s);
					return expr_Const(v <= 0 ? true : false);
				}
				case T_OP_LOGICLT:
				{
					int32_t v = parse_StringCompare(s);
					return expr_Const(v < 0 ? true : false);
				}
				default:
					break;
			}
		}
	}

	mem_Free(s);
	lex_Goto(&bm);
	return parse_ExprPri9();
}

static SExpression* parse_TwoArgFunc(SExpression*(*pFunc)(SExpression*,SExpression*))
{
	SExpression* t1;
	SExpression* t2;

	parse_GetToken();
	if(!parse_ExpectChar('('))
		return NULL;

	t1 = parse_ExprPri0();

	if(!parse_ExpectChar(','))
		return NULL;

	t2 = parse_ExprPri0();

	if(!parse_ExpectChar(')'))
		return NULL;

	return pFunc(t1, t2);
}

static SExpression* parse_SingleArgFunc(SExpression*(*pFunc)(SExpression*))
{
	SExpression* t1;

	parse_GetToken();

	if(!parse_ExpectChar('('))
		return NULL;

	t1 = parse_ExprPri0();

	if(!parse_ExpectChar(')'))
		return NULL;

	return pFunc(t1);
}


static SExpression* parse_ExprPri7(void)
{
	switch(g_CurrentToken.ID.Token)
	{
		case T_FUNC_ATAN2:
			return parse_TwoArgFunc(expr_Atan2);
		case T_FUNC_SIN:
			return parse_SingleArgFunc(expr_Sin);
		case T_FUNC_COS:
			return parse_SingleArgFunc(expr_Cos);
		case T_FUNC_TAN:
			return parse_SingleArgFunc(expr_Tan);
		case T_FUNC_ASIN:
			return parse_SingleArgFunc(expr_Asin);
		case T_FUNC_ACOS:
			return parse_SingleArgFunc(expr_Acos);
		case T_FUNC_ATAN:
			return parse_SingleArgFunc(expr_Atan);
		case T_FUNC_DEF:
		{
			SExpression* t1;

			parse_GetToken();

			if(!parse_ExpectChar('('))
				return NULL;

			if(g_CurrentToken.ID.Token != T_ID)
			{
				prj_Fail(ERROR_DEF_SYMBOL);
				return NULL;
			}

			t1 = expr_Const(sym_isDefined(g_CurrentToken.Value.aString));
			parse_GetToken();

			if(!parse_ExpectChar(')'))
				return NULL;

			return t1;
		}
		case T_FUNC_BANK:
		{
			SExpression* t1;

			if(!g_pConfiguration->bSupportBanks)
				internalerror("Banks not supported");

			parse_GetToken();

			if(!parse_ExpectChar('('))
				return NULL;

			if(g_CurrentToken.ID.Token != T_ID)
			{
				prj_Fail(ERROR_BANK_SYMBOL);
				return NULL;
			}

			t1 = expr_Bank(g_CurrentToken.Value.aString);
			parse_GetToken();

			if(!parse_ExpectChar(')'))
				return NULL;

			return t1;
		}
		default:
		{
			SExpression* expr;
			if((expr = parse_TargetFunction()) != NULL)
				return expr;

			return parse_ExprPri8();
			break;
		}
	}
}

static	SExpression* parse_ExprPri6(void)
{
	switch(g_CurrentToken.ID.Token)
	{
		case T_OP_SUB:
		{
			parse_GetToken();
			return expr_Sub(expr_Const(0), parse_ExprPri6());
		}
		case T_OP_NOT:
		{
			parse_GetToken();
			return expr_Xor(expr_Const(0xFFFFFFFF), parse_ExprPri6());
		}
		case T_OP_ADD:
		{
			parse_GetToken();
			return parse_ExprPri6();
		}
		default:
		{
			return parse_ExprPri7();
		}
	}

}

static	SExpression* parse_ExprPri5(void)
{
	SExpression* t1 = parse_ExprPri6();

	while(g_CurrentToken.ID.Token == T_OP_SHL
	   || g_CurrentToken.ID.Token == T_OP_SHR
	   || g_CurrentToken.ID.Token == T_OP_MUL
	   || g_CurrentToken.ID.Token == T_OP_DIV
	   || g_CurrentToken.ID.Token == T_FUNC_FMUL
	   || g_CurrentToken.ID.Token == T_FUNC_FDIV
	   || g_CurrentToken.ID.Token == T_OP_MOD)
	{
		switch(g_CurrentToken.ID.Token)
		{
			case T_OP_SHL:
			{
				parse_GetToken();
				t1 = expr_Shl(t1, parse_ExprPri6());
				break;
			}
			case T_OP_SHR:
			{
				parse_GetToken();
				t1 = expr_Shr(t1, parse_ExprPri6());
				break;
			}
			case T_FUNC_FMUL:
			{
				parse_GetToken();
				t1 = expr_Fmul(t1, parse_ExprPri6());
				break;
			}
			case T_OP_MUL:
			{
				parse_GetToken();
				t1 = expr_Mul(t1, parse_ExprPri6());
				break;
			}
			case T_FUNC_FDIV:
			{
				parse_GetToken();
				t1 = expr_Fdiv(t1, parse_ExprPri6());
				break;
			}
			case T_OP_DIV:
			{
				parse_GetToken();
				t1 = expr_Div(t1, parse_ExprPri6());
				break;
			}
			case T_OP_MOD:
			{
				parse_GetToken();
				t1 = expr_Mod(t1, parse_ExprPri6());
				break;
			}
			default:
				break;
		}
	}

	return t1;
}

static	SExpression* parse_ExprPri4(void)
{
	SExpression* t1 = parse_ExprPri5();

	while(g_CurrentToken.ID.Token == T_OP_XOR
	   || g_CurrentToken.ID.Token == T_OP_OR
	   || g_CurrentToken.ID.Token == T_OP_AND)
	{
		switch(g_CurrentToken.ID.Token)
		{
			case T_OP_XOR:
			{
				parse_GetToken();
				t1 = expr_Xor(t1, parse_ExprPri5());
				break;
			}
			case T_OP_OR:
			{
				parse_GetToken();
				t1 = expr_Or(t1, parse_ExprPri5());
				break;
			}
			case T_OP_AND:
			{
				parse_GetToken();
				t1 = expr_And(t1, parse_ExprPri5());
				break;
			}
			default:
				break;
		}
	}

	return t1;
}

static SExpression* parse_ExprPri3(void)
{
	SExpression* t1 = parse_ExprPri4();

	while(g_CurrentToken.ID.Token == T_OP_ADD
	   || g_CurrentToken.ID.Token == T_OP_SUB)
	{
		switch(g_CurrentToken.ID.Token)
		{
			case T_OP_ADD:
			{
				parse_GetToken();
				t1 = expr_Add(t1, parse_ExprPri4());
				break;
			}
			case T_OP_SUB:
			{
				parse_GetToken();
				t1 = expr_Sub(t1, parse_ExprPri4());
				break;
			}
			default:
				 break;
		}
	}
	return t1;
}


static	SExpression* parse_ExprPri2(void)
{
	SExpression* t1;

	t1 = parse_ExprPri3();
	while(g_CurrentToken.ID.Token == T_OP_LOGICEQU
	   || g_CurrentToken.ID.Token == T_OP_LOGICGT
	   || g_CurrentToken.ID.Token == T_OP_LOGICLT
	   || g_CurrentToken.ID.Token == T_OP_LOGICGE
	   || g_CurrentToken.ID.Token == T_OP_LOGICLE
	   || g_CurrentToken.ID.Token == T_OP_LOGICNE )
	{
		switch(g_CurrentToken.ID.Token)
		{
			case T_OP_LOGICEQU:
			{
				parse_GetToken();
				t1 = expr_Equal(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICGT:
			{
				parse_GetToken();
				t1 = expr_GreaterThan(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICLT:
			{
				parse_GetToken();
				t1 = expr_LessThan(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICGE:
			{
				parse_GetToken();
				t1 = expr_GreaterEqual(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICLE:
			{
				parse_GetToken();
				t1 = expr_LessEqual(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICNE:
			{
				parse_GetToken();
				t1 = expr_NotEqual(t1, parse_ExprPri3());
				break;
			}
			default:
				break;
		}
	}

	return t1;
}

static	SExpression* parse_ExprPri1(void)
{
	switch(g_CurrentToken.ID.Token)
	{
		case T_OP_OR:
		case T_OP_LOGICNOT:
		{
			parse_GetToken();
			return expr_BooleanNot(parse_ExprPri1());
		}
		default:
		{
			return parse_ExprPri2();
		}
	}

}

static	SExpression* parse_ExprPri0(void)
{
	SExpression* t1 = parse_ExprPri1();

	while(g_CurrentToken.ID.Token == T_OP_LOGICOR
	   || g_CurrentToken.ID.Token == T_OP_LOGICAND)
	{
		switch(g_CurrentToken.ID.Token)
		{
			case T_OP_LOGICOR:
			{
				parse_GetToken();
				t1 = expr_BooleanOr(t1, parse_ExprPri1());
				break;
			}
			case T_OP_LOGICAND:
			{
				parse_GetToken();
				t1 = expr_BooleanAnd(t1, parse_ExprPri1());
				break;
			}
			default:
				break;
		}
	}

	return t1;
}

SExpression* parse_Expression(void)
{
	return parse_ExprPri0();
}

int32_t parse_ConstantExpression(void)
{
	SExpression* expr;

	if((expr = parse_Expression()) != NULL)
	{
		if(expr_IsConstant(expr))
		{
			int32_t r = expr->Value.Value;
			expr_Free(expr);
			return r;
		}

		prj_Error(ERROR_EXPR_CONST);
	}
	else
		prj_Error(ERROR_INVALID_EXPRESSION);

	return 0;
}

static char* parse_StringExpressionRaw_Pri0(void);

static char* parse_StringExpressionRaw_Pri2(void)
{
	SLexBookmark bm;
	lex_Bookmark(&bm);

	switch(g_CurrentToken.ID.Token)
	{
		case T_STRING:
		{
			char* r = mem_Alloc(strlen(g_CurrentToken.Value.aString) + 1);
			strcpy(r, g_CurrentToken.Value.aString);
			parse_GetToken();
			return r;

			internalerror("Out of memory");
			break;
		}
		case '(':
		{
			char *r;

			parse_GetToken();
			r = parse_StringExpressionRaw_Pri0();
			if(r != NULL)
			{
				if(parse_ExpectChar(')'))
					return r;
			}

			lex_Goto(&bm);
			mem_Free(r);
			return NULL;
		}
		default:
		  break;
	}
	return NULL;
}

static char* parse_StringExpressionRaw_Pri1(void)
{
	SLexBookmark bm;
	char* t = parse_StringExpressionRaw_Pri2();

	while(parse_IsDot(&bm))
	{
		switch(g_CurrentToken.ID.Token)
		{
			case T_FUNC_SLICE:
			{
				int32_t start;
				int32_t count;
				int32_t len = strlen(t);

				parse_GetToken();

				if(!parse_ExpectChar('('))
					return NULL;

				start = parse_ConstantExpression();
				if(start < 0)
				{
					start = len + start;
					if(start < 0)
						start = 0;
				}

				if(g_CurrentToken.ID.Token == ',')
				{
					parse_GetToken();
					count = parse_ConstantExpression();
				}
				else
					count = len - start;

				if(parse_ExpectChar(')'))
				{
					char* r = mem_Alloc(count + 1);
					
					if(start + count >= len)
						count = len - start;

					strncpy(r, t + start, count);
					r[count] = 0;
					
					mem_Free(t);
					t = r;
				}
				break;
			}
			case T_FUNC_TOUPPER:
			{
				parse_GetToken();
				if(!parse_ExpectChar('('))
					return NULL;

				if(parse_ExpectChar(')'))
					_strupr(t);

				break;
			}
			case T_FUNC_TOLOWER:
			{
				parse_GetToken();
				if(!parse_ExpectChar('('))
					return NULL;

				if(parse_ExpectChar(')'))
					_strlwr(t);

				break;
			}
			default:
			{
				lex_Goto(&bm);
				return t;
			}
		}
	}

	return t;
}

static char* parse_StringExpressionRaw_Pri0(void)
{
	char* t1 = parse_StringExpressionRaw_Pri1();

	while(g_CurrentToken.ID.Token == T_OP_ADD)
	{
		char* r = NULL;
		char* t2;

		parse_GetToken();

		if((t2 = parse_StringExpressionRaw_Pri1()) == NULL)
			return NULL;

		r = mem_Alloc(strlen(t1) + strlen(t2) + 1);
		strcpy(r, t1);
		strcat(r, t2);

		mem_Free(t2);
		mem_Free(t1);

		return r;
	}

	return t1;
}

static char* parse_StringExpression(void)
{
	char* s = parse_StringExpressionRaw_Pri0();

	if(s == NULL)
		prj_Error(ERROR_EXPR_STRING);

	return s;
}

static void parse_RS(char* pszName, int32_t size, int coloncount)
{
	sym_AddSET(pszName, sym_GetConstant("__RS"));
	sym_AddSET("__RS", sym_GetConstant("__RS") + size);

	if(coloncount == 2)
		sym_Export(pszName);
}


static void parse_RS_Skip(int32_t size)
{
	sym_AddSET("__RS", sym_GetConstant("__RS") + size);
}


static bool_t parse_Symbol(void)
{
	bool_t r = false;
	
	if(g_CurrentToken.ID.Token == T_LABEL)
	{
		string* pName = str_Create(g_CurrentToken.Value.aString);
		uint32_t coloncount;

		parse_GetToken();
		coloncount = parse_ColonCount();

		switch(g_CurrentToken.ID.Token)
		{
			default:
				sym_CreateLabel(pName);
				if(coloncount == 2)
				{
					sym_Export(str_String(pName));
				}
				r = true;
				break;
			case T_POP_RB:
				parse_GetToken();
				parse_RS(str_String(pName), parse_ConstantExpression(), coloncount);
				r = true;
				break;
			case T_POP_RW:
				parse_GetToken();
				parse_RS(str_String(pName), parse_ConstantExpression() * 2, coloncount);
				r = true;
				break;
			case T_POP_RL:
				parse_GetToken();
				parse_RS(str_String(pName), parse_ConstantExpression() * 4, coloncount);
				r = true;
				break;
			case T_POP_EQU:
				parse_GetToken();
				sym_AddEQU(str_String(pName), parse_ConstantExpression());
				if(coloncount == 2)
				{
					sym_Export(str_String(pName));
				}
				r = true;
				break;
			case T_POP_SET:
				parse_GetToken();
				sym_AddSET(str_String(pName), parse_ConstantExpression());
				if(coloncount == 2)
				{
					sym_Export(str_String(pName));
				}
				r = true;
				break;
			case T_POP_EQUS:
			{
				char* pExpr;

				parse_GetToken();
				if((pExpr = parse_StringExpression()) != NULL)
				{
					sym_AddEQUS(str_String(pName), pExpr);
					mem_Free(pExpr);
					if(coloncount == 2)
					{
						sym_Export(str_String(pName));
					}
					r = true;
				}
				else
				{
					internalerror("String expression is NULL");
				}
				break;
			}
			case T_POP_GROUP:
			{
				parse_GetToken();
				switch(g_CurrentToken.ID.Token)
				{
					case T_GROUP_TEXT:
						sym_AddGROUP(str_String(pName), GROUP_TEXT);
						r = true;
						break;
					case T_GROUP_BSS:
						sym_AddGROUP(str_String(pName), GROUP_BSS);
						r = true;
						break;
					default:
						prj_Error(ERROR_EXPECT_TEXT_BSS);
						r = false;
						break;
				}
				if(coloncount == 2)
				{
					sym_Export(str_String(pName));
				}
				break;
			}
			case T_POP_MACRO:
			{
				uint32_t reptsize;
				char* reptblock;
				int32_t lineno = g_pFileContext->LineNumber;
				char* pszfile = str_String(g_pFileContext->pName);

				if(parse_CopyMacro(&reptblock, &reptsize))
				{
					sym_AddMACRO(str_String(pName), reptblock, reptsize);
					parse_GetToken();
					r = true;
				}
				else
					prj_Fail(ERROR_NEED_ENDM, pszfile, lineno);
				break;
			}
		}
		str_Free(pName);
	}

	return r;
}

static bool_t parse_Import(char* pszSym)
{
	return sym_Import(pszSym) != NULL;
}

static bool_t parse_Export(char* pszSym)
{
	return sym_Export(pszSym) != NULL;
}

static bool_t parse_Global(char* pszSym)
{
	return sym_Global(pszSym) != NULL;
}

static bool_t parse_SymbolOp(bool_t (*pOp)(char* pszSymbol))
{
	bool_t r = false;

	parse_GetToken();
	if(g_CurrentToken.ID.Token == T_ID)
	{
		pOp(g_CurrentToken.Value.aString);
		parse_GetToken();
		while(g_CurrentToken.ID.Token == ',')
		{
			parse_GetToken();
			if(g_CurrentToken.ID.Token == T_ID)
			{
				pOp(g_CurrentToken.Value.aString);
				r = true;
				parse_GetToken();
			}
			else
				prj_Error(ERROR_EXPECT_IDENTIFIER);
		}
	}
	else
		prj_Error(ERROR_EXPECT_IDENTIFIER);

	return r;
}

static int32_t parse_ExpectBankFixed(void)
{
	int32_t bank;

	if(!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	if(g_CurrentToken.ID.Token != T_FUNC_BANK)
	{
		prj_Error(ERROR_EXPECT_BANK);
		return -1;
	}

	parse_GetToken();
	if(!parse_ExpectChar('['))
		return -1;

	parse_GetToken();
	bank = parse_ConstantExpression();
	if(!parse_ExpectChar(']'))
		return -1;

	return bank;
}

static bool_t parse_PseudoOp(void)
{
	switch(g_CurrentToken.ID.Token)
	{
		case T_POP_REXIT:
		{
			if(g_pFileContext->Type != CONTEXT_REPT)
			{
				prj_Warn(WARN_REXIT_OUTSIDE_REPT);
			}
			else
			{
				g_pFileContext->BlockInfo.Rept.RemainingRuns = 0;
				fstk_RunNextBuffer();
				g_pFileContext->LineNumber++;
			}
			parse_GetToken();
			return true;
		}
		case T_POP_MEXIT:
		{
			if(g_pFileContext->Type != CONTEXT_MACRO)
			{
				prj_Warn(WARN_MEXIT_OUTSIDE_MACRO);
			}
			else
			{
				fstk_RunNextBuffer();
			}
			parse_GetToken();
			return true;
		}
		case T_POP_SECTION:
		{
			int32_t org;
			char* name;
			char r[MAXSYMNAMELENGTH+1];
			SSymbol* sym;

			parse_GetToken();
			if((name = parse_StringExpression()) == NULL)
				return true;

			strcpy(r, name);
			mem_Free(name);

			if(!parse_ExpectChar(','))
				return sect_SwitchTo_NAMEONLY(r);

			if(g_CurrentToken.ID.Token != T_ID)
			{
				prj_Error(ERROR_EXPECT_IDENTIFIER);
				return false;
			}

			sym = sym_FindSymbol(g_CurrentToken.Value.aString);
			if(sym->Type != SYM_GROUP)
			{
				prj_Error(ERROR_IDENTIFIER_GROUP);
				return true;
			}
			parse_GetToken();

			if(g_pConfiguration->bSupportBanks && g_CurrentToken.ID.Token == ',')
			{
				int32_t bank;
				parse_GetToken();
				bank = parse_ExpectBankFixed();

				if(bank == -1)
					return true;

				return sect_SwitchTo_BANK(r, sym, bank);
			}
			else if(g_CurrentToken.ID.Token != '[')
			{
				return sect_SwitchTo(r, sym);
			}
			parse_GetToken();

			org = parse_ConstantExpression();
			if(!parse_ExpectChar(']'))
				return true;

			if(g_pConfiguration->bSupportBanks && g_CurrentToken.ID.Token == ',')
			{
				int32_t bank;
				parse_GetToken();
				bank = parse_ExpectBankFixed();

				if(bank == -1)
					return true;

				return sect_SwitchTo_ORG_BANK(r, sym, org, bank);
			}

			return sect_SwitchTo_ORG(r, sym, org);
		}
		case T_POP_PRINTT:
		{
			char* r;

			parse_GetToken();
			if((r = parse_StringExpression()) != NULL)
			{
				printf("%s", r);
				mem_Free(r);
				return true;
			}

			return false;
		}
		case T_POP_PRINTV:
		{
			parse_GetToken();
			printf("$%X", parse_ConstantExpression());
			return true;
			break;
		}
		case T_POP_PRINTF:
		{
			int32_t i;

			parse_GetToken();
			i = parse_ConstantExpression();
			if(i < 0)
			{
				printf("-");
				i = -i;
			}
			printf("%d.%05d", i >> 16, imuldiv(i & 0xFFFF, 100000, 65536));

			return true;
			break;
		}
		case T_POP_IMPORT:
		{
			return parse_SymbolOp(parse_Import);
		}
		case T_POP_EXPORT:
		{
			return parse_SymbolOp(parse_Export);
		}
		case T_POP_GLOBAL:
		{
			return parse_SymbolOp(parse_Global);
		}
		case T_POP_PURGE:
		{
			bool_t r;

			g_bDontExpandStrings = true;
			r = parse_SymbolOp(sym_Purge);
			g_bDontExpandStrings = false;

			return r;
		}
		case T_POP_RSRESET:
		{
			parse_GetToken();
			sym_AddSET("__RS", 0);
			return true;
			break;
		}
		case T_POP_RSSET:
		{
			int32_t val;

			parse_GetToken();
			val = parse_ConstantExpression();
			sym_AddSET("__RS", val);
			return true;
		}
		case T_POP_RB:
		{
			parse_RS_Skip(parse_ConstantExpression());
			return true;
		}
		case T_POP_RW:
		{
			parse_RS_Skip(parse_ConstantExpression() * 2);
			return true;
		}
		case T_POP_RL:
		{
			parse_RS_Skip(parse_ConstantExpression() * 4);
			return true;
		}
		case T_POP_FAIL:
		{
			char* r;

			parse_GetToken();
			if((r = parse_StringExpression()) != NULL)
			{
				prj_Fail(WARN_USER_GENERIC, r);
				return true;
			}
			else
			{
				internalerror("String expression is NULL");
				return false;
			}
			break;
		}
		case T_POP_WARN:
		{
			char* r;

			parse_GetToken();
			if((r = parse_StringExpression()) != NULL)
			{
				prj_Warn(WARN_USER_GENERIC, r);
				return true;
			}
			else
			{
				internalerror("String expression is NULL");
				return false;
			}
			break;
		}
		case T_POP_EVEN:
		{
			parse_GetToken();
			sect_Align(2);
			return true;
		}
		case T_POP_CNOP:
		{
			int32_t offset;
			int32_t align;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset < 0)
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return true;
			}

			if(!parse_ExpectComma())
				return true;

			align = parse_ConstantExpression();
			if(align < 0)
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return true;
			}
			sect_Align(align);
			sect_SkipBytes(offset);
			return true;
			break;
		}
		case T_POP_DSB:
		{
			int32_t offset;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset >= 0)
			{
				sect_SkipBytes(offset);
				return true;
			}
			else
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return true;
			}
			break;
		}
		case T_POP_DSW:
		{
			int32_t offset;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset >= 0)
			{
				sect_SkipBytes(offset * 2);
				return true;
			}
			else
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return true;
			}
			break;
		}
		case T_POP_DSL:
		{
			int32_t offset;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset >= 0)
			{
				sect_SkipBytes(offset * 4);
				return true;
			}
			else
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return true;
			}
			break;
		}
		case T_POP_DB:
		{
			SExpression* expr;
			char* s;

			do
			{
				parse_GetToken();
				if((s = parse_StringExpressionRaw_Pri0()) != NULL)
				{
					while(*s)
						sect_OutputConst8(*s++);
				}
				else if((expr = parse_Expression()) != NULL)
				{
					expr = expr_CheckRange(expr, -128, 255);
					if(expr)
						sect_OutputExpr8(expr);
					else
						prj_Error(ERROR_EXPRESSION_N_BIT, 8);
				}
				else
					prj_Error(ERROR_INVALID_EXPRESSION);
			} while(g_CurrentToken.ID.Token == ',');

			return true;
			break;
		}
		case T_POP_DW:
		{
			SExpression* expr;

			do
			{
				parse_GetToken();
				expr = parse_Expression();
				if(expr)
				{
					expr = expr_CheckRange(expr, -32768, 65535);
					if(expr)
						sect_OutputExpr16(expr);
					else
						prj_Error(ERROR_EXPRESSION_N_BIT, 16);
				}
				else
					prj_Error(ERROR_INVALID_EXPRESSION);
			} while(g_CurrentToken.ID.Token == ',');

			return true;
			break;
		}
		case T_POP_DL:
		{
			SExpression* expr;

			do
			{
				parse_GetToken();
				expr = parse_Expression();
				if(expr)
					sect_OutputExprLong(expr);
				else
					prj_Error(ERROR_INVALID_EXPRESSION);
			} while(g_CurrentToken.ID.Token == ',');

			return true;
			break;
		}
		case T_POP_INCLUDE:
		{
			SLexBookmark mark;
			char* r;

			lex_Bookmark(&mark);
			parse_GetToken();
			if((r = parse_StringExpressionRaw_Pri0()) == NULL)
			{
				char* pStart = mark.Buffer.pBuffer;
				char* pEnd;

				while(*pStart && (*pStart == ' ' || *pStart == '\t'))
					++pStart;

				pEnd = pStart;
				while(*pEnd && !isspace(*pEnd))
					++pEnd;

				r = mem_Alloc(pEnd - pStart + 1);
				memcpy(r, pStart, pEnd - pStart);
				r[pEnd - pStart] = 0;
				lex_Goto(&mark);
				lex_SkipBytes((uint32_t)(pEnd - mark.Buffer.pBuffer));
				parse_GetToken();
			}
			
			if(r != NULL)
			{
				string* pFile = str_Create(r);
				fstk_RunInclude(pFile);
				str_Free(pFile);
				mem_Free(r);
				return true;
			}
			else
			{
				prj_Error(ERROR_EXPR_STRING);
				return false;
			}
			break;
		}
		case T_POP_INCBIN:
		{
			char* r;

			parse_GetToken();
			if((r = parse_StringExpression()) != NULL)
			{
				string* pFile = str_Create(r);
				sect_OutputBinaryFile(pFile);
				mem_Free(r);
				str_Free(pFile);
				return true;
			}
			return false;
			break;
		}
		case T_POP_REPT:
		{
			uint32_t reptsize;
			int32_t reptcount;
			char* reptblock;

			parse_GetToken();
			reptcount = parse_ConstantExpression();
			if(parse_CopyRept(&reptblock, &reptsize))
			{
				if(reptcount > 0)
				{
					fstk_RunRept(reptblock, reptsize, reptcount);
				}
				else if(reptcount < 0)
				{
					prj_Error(ERROR_EXPR_POSITIVE);
					mem_Free(reptblock);
				}
				else
				{
					mem_Free(reptblock);
				}
				return true;
			}
			else
			{
				prj_Fail(ERROR_NEED_ENDR);
				return false;
			}
			break;
		}
		case T_POP_SHIFT:
		{
			SExpression* expr;

			parse_GetToken();
			expr = parse_Expression();
			if(expr)
			{
				if(expr_IsConstant(expr))
				{
					fstk_ShiftMacroArgs(expr->Value.Value);
					expr_Free(expr);
					return true;
				}
				else
				{
					prj_Fail(ERROR_EXPR_CONST);
					return false;
				}
			}
			else
			{
				fstk_ShiftMacroArgs(1);
				return true;
			}
			break;
		}
		case T_POP_IFC:
		{
			char* s1;

			parse_GetToken();
			s1 = parse_StringExpression();
			if(s1 != NULL)
			{
				if(parse_ExpectComma())
				{
					char* s2;

					s2 = parse_StringExpression();
					if(s2 != NULL)
					{
						if(strcmp(s1, s2) != 0)
							parse_IfSkipToElse();

						mem_Free(s1);
						mem_Free(s2);
						return true;
					}
					else
						prj_Error(ERROR_EXPR_STRING);
				}
				mem_Free(s1);
			}
			else
				prj_Error(ERROR_EXPR_STRING);

			return false;
			break;
		}
		case	T_POP_IFNC:
		{
			char* s1;

			parse_GetToken();
			s1 = parse_StringExpression();
			if(s1 != NULL)
			{
				if(parse_ExpectComma())
				{
					char* s2;

					s2 = parse_StringExpression();
					if(s2 != NULL)
					{
						if(strcmp(s1, s2) == 0)
							parse_IfSkipToElse();

						mem_Free(s1);
						mem_Free(s2);
						return true;
					}
					else
						prj_Error(ERROR_EXPR_STRING);
				}
				mem_Free(s1);
			}
			else
				prj_Error(ERROR_EXPR_STRING);

			return false;
			break;
		}
		case T_POP_IFD:
		{
			parse_GetToken();

			if(g_CurrentToken.ID.Token == T_ID)
			{
				if(sym_isDefined(g_CurrentToken.Value.aString))
				{
					parse_GetToken();
				}
				else
				{
					parse_GetToken();
					/* will continue parsing just after ELSE or just at ENDC keyword */
					parse_IfSkipToElse();
				}
				return true;
			}
			prj_Error(ERROR_EXPECT_IDENTIFIER);
			return false;
			break;
		}
		case T_POP_IFND:
		{
			parse_GetToken();

			if(g_CurrentToken.ID.Token == T_ID)
			{
				if(!sym_isDefined(g_CurrentToken.Value.aString))
				{
					parse_GetToken();
				}
				else
				{
					parse_GetToken();
					/* will continue parsing just after ELSE or just at ENDC keyword */
					parse_IfSkipToElse();
				}
				return true;
			}
			prj_Error(ERROR_EXPECT_IDENTIFIER);
			return false;
		}
		case T_POP_IF:
		{
			parse_GetToken();

			if(parse_ConstantExpression() == 0)
			{
				/* will continue parsing just after ELSE or just at ENDC keyword */
				parse_IfSkipToElse();
				parse_GetToken();
			}
			return true;
		}
		case T_POP_IFEQ:
		{
			parse_GetToken();

			if(!(parse_ConstantExpression() == 0))
			{
				/* will continue parsing just after ELSE or just at ENDC keyword */
				parse_IfSkipToElse();
			}
			return true;
			break;
		}
		case T_POP_IFGT:
		{
			parse_GetToken();

			if(!(parse_ConstantExpression() > 0))
			{
				/* will continue parsing just after ELSE or just at ENDC keyword */
				parse_IfSkipToElse();
			}
			return true;
			break;
		}
		case T_POP_IFGE:
		{
			parse_GetToken();

			if(!(parse_ConstantExpression() >= 0))
			{
				/* will continue parsing just after ELSE or just at ENDC keyword */
				parse_IfSkipToElse();
			}
			return true;
			break;
		}
		case T_POP_IFLT:
		{
			parse_GetToken();

			if(!(parse_ConstantExpression() < 0))
			{
				/* will continue parsing just after ELSE or just at ENDC keyword */
				parse_IfSkipToElse();
			}
			return true;
			break;
		}
		case T_POP_IFLE:
		{
			parse_GetToken();

			if(!(parse_ConstantExpression() <= 0))
			{
				/* will continue parsing just after ELSE or just at ENDC keyword */
				parse_IfSkipToElse();
			}
			return true;
			break;
		}
		case T_POP_ELSE:
		{
			/* will continue parsing just at ENDC keyword */
			parse_IfSkipToEndc();
			parse_GetToken();
			return true;
			break;
		}
		case T_POP_ENDC:
		{
			parse_GetToken();
			return true;
			break;
		}
		case T_POP_PUSHO:
		{
			opt_Push();
			parse_GetToken();
			return true;
		}
		case T_POP_POPO:
		{
			opt_Pop();
			parse_GetToken();
			return true;
		}
		case T_POP_OPT:
		{
			lex_SetState(LEX_STATE_MACROARGS);
			parse_GetToken();
			if(g_CurrentToken.ID.Token == T_STRING)
			{
				opt_Parse(g_CurrentToken.Value.aString);
				parse_GetToken();
				while(g_CurrentToken.ID.Token == ',')
				{
					parse_GetToken();
					opt_Parse(g_CurrentToken.Value.aString);
					parse_GetToken();
				}
			}
			lex_SetState(LEX_STATE_NORMAL);
			return true;
		}
		default:
		{
			return false;
		}
	}
}

bool_t parse_Misc(void)
{
	switch(g_CurrentToken.ID.Token)
	{
		case T_ID:
		{
			if(sym_isMacro(g_CurrentToken.Value.aString))
			{
				string* s = str_Create(g_CurrentToken.Value.aString);

				lex_SetState(LEX_STATE_MACROARG0);
				parse_GetToken();
				while(g_CurrentToken.ID.Token != '\n')
				{
					if(g_CurrentToken.ID.Token == T_STRING)
					{
						fstk_AddMacroArg(g_CurrentToken.Value.aString);
						parse_GetToken();
						if(g_CurrentToken.ID.Token == ',')
						{
							parse_GetToken();
						}
						else if(g_CurrentToken.ID.Token != '\n')
						{
							prj_Error(ERROR_CHAR_EXPECTED, ',');
							lex_SetState(LEX_STATE_NORMAL);
							parse_GetToken();
							return false;
						}
					}
					else if(g_CurrentToken.ID.Token == T_MACROARG0)
					{
						fstk_SetMacroArg0(g_CurrentToken.Value.aString);
						parse_GetToken();
					}
					else
					{
						internalerror("Must be T_STRING");
						return false;
					}
				}
				lex_SetState(LEX_STATE_NORMAL);
				fstk_RunMacro(s);
				str_Free(s);
				return true;
			}
			else
			{
				prj_Error(ERROR_INSTR_UNKNOWN, g_CurrentToken.Value.aString);
				return false;
			}
			break;
		}
		default:
		{
			return false;
		}
	}
}

/*	Public routines*/

void parse_GetToken(void)
{
	if(lex_GetNextToken())
	{
		return;
	}

	prj_Fail(ERROR_END_OF_FILE);
}

bool_t parse_Do(void)
{
	bool_t r = true;

	lex_GetNextToken();

	while(g_CurrentToken.ID.Token && r)
	{
		if(!parse_TargetSpecific()
		&& !parse_Symbol()
		&& !parse_PseudoOp()
		&& !parse_Misc())
		{
			if(g_CurrentToken.ID.Token == '\n')
			{
				lex_GetNextToken();
				g_pFileContext->LineNumber += 1;
				g_nTotalLines += 1;
			}
			else if(g_CurrentToken.ID.Token == T_POP_END)
			{
				return true;
			}
			else
			{
				prj_Error(ERROR_SYNTAX);
				r = false;
			}
		}
	}

	return r;
}
