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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "asmotor.h"
#include "types.h"
#include "expr.h"
#include "project.h"
#include "fstack.h"
#include "lexer.h"
#include "parse.h"
#include "symbol.h"
#include "section.h"
#include "globlex.h"
#include "options.h"

extern BOOL parse_TargetSpecific(void);
extern SExpression* parse_TargetFunction(void);




/*	Private routines*/

static void parse_VerifyPointers(SExpression* left, SExpression* right)
{
	if(left != NULL && right != NULL )
		return;

	prj_Fail(ERROR_INVALID_EXPRESSION);
}

static BOOL parse_isWhiteSpace(char s)
{
	return s == ' ' || s == '\t' || s == '\0' || s == '\n';
}

static BOOL parse_isToken(char* s, char* token)
{
	int len = (int)strlen(token);
	return _strnicmp(s, token, len) == 0 && parse_isWhiteSpace(s[len]);
}

static BOOL parse_isRept(char* s)
{
	return parse_isToken(s, "REPT");
}

static BOOL parse_isEndr(char* s)
{
	return parse_isToken(s, "ENDR");
}

BOOL parse_isIf(char* s)
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

BOOL parse_isElse(char* s)
{
	return parse_isToken(s, "ELSE");
}

BOOL parse_isEndc(char* s)
{
	return parse_isToken(s, "ENDC");
}

BOOL parse_isMacro(char* s)
{
	return parse_isToken(s, "MACRO");
}

BOOL parse_isEndm(char* s)
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

static BOOL parse_CopyRept(char** newmacro, ULONG* size)
{
	char* src = g_pFileContext->pLexBuffer->pBuffer;
	int len = parse_GetReptLength(src);

	if(len == 0)
		return FALSE;

	*size = len;

	if((*newmacro = (char*)malloc(len + 1)) != NULL)
	{
		SLONG	i;

		(*newmacro)[len]=0;
		for(i=0; i<len; i+=1)
		{
			(*newmacro)[i] = src[i];
		}
	}
	else
	{
		internalerror("Out of memory!");
	}

	lex_SkipBytes(len+4);
	return TRUE;
}

BOOL parse_IfSkipToElse(void)
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
			return TRUE;
		}
		else if(_strnicmp(token, "ELSE", 4) == 0)
		{
			lex_SkipBytes((int)(token + 4 - src));
			return TRUE;
		}
		else
		{
			s = parse_SkipToLine(s);
		}
	}

	return FALSE;
}

BOOL parse_IfSkipToEndc(void)
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
			return TRUE;
		}
		else
		{
			s = parse_SkipToLine(s);
		}
	}

	return 0;
}

BOOL	parse_CopyMacro(char** dest, ULONG* size)
{
	char* src = g_pFileContext->pLexBuffer->pBuffer;
	int len = parse_GetMacroLength(src);

	*size = len;

	if((*dest = (char*)malloc(len + 1)) != NULL)
	{
		SLONG	i;

		(*dest)[len]=0;
		for(i=0; i<len; i+=1)
		{
			(*dest)[i] = src[i];
		}
	}
	else
	{
		internalerror("Out of memory!");
	}

	lex_SkipBytes(len+4);
	return TRUE;
}




static ULONG parse_ColonCount(void)
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

BOOL parse_IsDot(SLexBookmark* pBookmark)
{
	if(pBookmark)
		lex_Bookmark(pBookmark);

	if(g_CurrentToken.ID.Token == '.')
	{
		parse_GetToken();
		return TRUE;
	}

	if(g_CurrentToken.ID.Token == T_ID && g_CurrentToken.Value.aString[0] == '.')
	{
		lex_RewindBytes(strlen(g_CurrentToken.Value.aString) - 1);
		parse_GetToken();
		return TRUE;
	}

	return FALSE;
}

BOOL parse_ExpectChar(char ch)
{
	if(g_CurrentToken.ID.TargetToken == ch)
	{
		parse_GetToken();
		return TRUE;
	}
	else
	{
		prj_Error(ERROR_CHAR_EXPECTED, ch);
		return FALSE;
	}
}

BOOL parse_ExpectComma(void)
{
	return parse_ExpectChar(',');
}



/*
 *	Expression parser
 */

void	parse_FreeExpression(SExpression* expr)
{
	if(expr)
	{
		parse_FreeExpression(expr->pLeft);
		parse_FreeExpression(expr->pRight);
		free(expr);
	}
}


SExpression* parse_DuplicateExpr(SExpression* expr)
{
	SExpression* r;
	if((r = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		*r = *expr;
		if(r->pLeft != NULL)
			r->pLeft = parse_DuplicateExpr(r->pLeft);
		if(r->pRight != NULL)
			r->pRight = parse_DuplicateExpr(r->pRight);
	}

	return r;
}


SExpression* parse_CreateABSExpr(SExpression* right)
{
	SExpression* pSign = parse_CreateSHRExpr(parse_DuplicateExpr(right), parse_CreateConstExpr(31));
	return parse_CreateSUBExpr(parse_CreateXORExpr(right, parse_DuplicateExpr(pSign)), pSign);
}


SExpression* parse_CreateBITExpr(SExpression* right)
{
	SExpression* expr;

	parse_VerifyPointers(right, right);

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		int b = 0;
		ULONG v = right->Value.Value;

		if((right->Flags & EXPRF_isCONSTANT) != 0
		&& ((v & -(SLONG)v) != v))
		{
			prj_Error(ERROR_EXPR_TWO_POWER);
			return NULL;
		}

		if(v != 0)
		{
			while(v != 1)
			{
				v >>= 1;
				++b;
			}
		}

		expr->pRight = right;
		expr->pLeft = NULL;
		expr->Value.Value = b;
		expr->Flags = right->Flags;
		expr->Type = EXPR_OPERATOR;
		expr->Operator = T_OP_BIT;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

SExpression* parse_CreatePCRelExpr(SExpression* in, int nAdjust)
{
	SExpression* expr = (SExpression*)malloc(sizeof(SExpression));

	if(expr == NULL)
		internalerror("Out of memory!");

	expr->Value.Value = 0;
	expr->Type = EXPR_PCREL;
	expr->Flags = EXPRF_isRELOC;
	expr->pLeft = parse_CreateConstExpr(nAdjust);
	expr->pRight = in;
	return expr;
}


SExpression* parse_CreatePCExpr()
{
	SExpression* expr;

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		char sym[MAXSYMNAMELENGTH + 20];
		SSymbol* pSym;

		sprintf(sym, "$%s%lu", pCurrentSection->Name, pCurrentSection->PC);
		pSym = sym_AddLabel(sym);

		if(pSym->Flags & SYMF_CONSTANT)
		{
			expr->Value.Value = pSym->Value.Value;
			expr->Type = EXPR_CONSTANT;
			expr->Flags = EXPRF_isCONSTANT | EXPRF_isRELOC;
			expr->pLeft = NULL;
			expr->pRight = NULL;
		}
		else
		{
			expr->pRight = NULL;
			expr->pLeft = NULL;
			expr->Value.pSymbol = pSym;
			expr->Flags = EXPRF_isRELOC;
			expr->Type = EXPR_SYMBOL;
		}

		return expr;
	}

	internalerror("Out of memory!");
	return NULL;
}

SExpression* parse_CreateConstExpr(SLONG value)
{
	SExpression* expr;

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		expr->Value.Value = value;
		expr->Type = EXPR_CONSTANT;
		expr->Flags = EXPRF_isCONSTANT | EXPRF_isRELOC;
		expr->pLeft = NULL;
		expr->pRight = NULL;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
	}
	return NULL;
}

static	SExpression* parse_MergeExpressions(SExpression* left, SExpression* right)
{
	SExpression* expr;

	parse_VerifyPointers(left, right);

	if((expr=(SExpression*)malloc(sizeof(SExpression)))!=NULL)
	{
		expr->Value.Value=0;
		expr->Type=0;
		expr->Flags=(left->Flags)&(right->Flags);
		expr->pLeft=left;
		expr->pRight=right;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

#define CREATEEXPRDIV(NAME,OP)															\
static SExpression* parse_Create ## NAME ## Expr(SExpression* left, SExpression* right)	\
{																						\
	SLONG val;																			\
	parse_VerifyPointers(left, right);													\
	if(right->Value.Value != 0)															\
	{																					\
		val = left->Value.Value OP right->Value.Value;									\
		left = parse_MergeExpressions(left, right);										\
		left->Type = EXPR_OPERATOR;														\
		left->Operator = T_OP_ ## NAME;													\
		left->Value.Value = val;														\
		return left;																	\
	}																					\
	else																				\
	{																					\
		prj_Fail(ERROR_ZERO_DIVIDE);													\
		return NULL;																	\
	}																					\
}

CREATEEXPRDIV(DIV, /)
CREATEEXPRDIV(MOD, %)

static SExpression* parse_CreateLOGICNOTExpr(SExpression* right)
{
	SExpression* expr;

	parse_VerifyPointers(right, right);

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		expr->pRight = right;
		expr->pLeft = NULL;
		expr->Value.Value = !right->Value.Value;
		expr->Flags = right->Flags;
		expr->Type = EXPR_OPERATOR;
		expr->Operator = T_OP_LOGICNOT;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

#define CREATEEXPR(NAME,OP) \
SExpression* parse_Create ## NAME ## Expr(SExpression* left, SExpression* right)	\
{																					\
	SLONG val;																		\
	parse_VerifyPointers(left, right);												\
	val = left->Value.Value OP right->Value.Value;									\
	left = parse_MergeExpressions(left, right);										\
	left->Type = EXPR_OPERATOR;														\
	left->Operator = T_OP_ ## NAME;													\
	left->Value.Value = val;														\
	return left;																	\
}

CREATEEXPR(SUB, -)
CREATEEXPR(ADD, +)
CREATEEXPR(XOR, ^)
CREATEEXPR(OR,  |)
CREATEEXPR(AND, &)
CREATEEXPR(SHL, <<)
CREATEEXPR(SHR, >>)
CREATEEXPR(MUL, *)
CREATEEXPR(LOGICOR, ||)
CREATEEXPR(LOGICAND, &&)
CREATEEXPR(LOGICGE, >=)
CREATEEXPR(LOGICGT, >)
CREATEEXPR(LOGICLE, <=)
CREATEEXPR(LOGICLT, <)
CREATEEXPR(LOGICEQU, ==)
CREATEEXPR(LOGICNE, !=)

#define CREATELIMIT(NAME,OP)												\
	static SExpression* parse_Create ## NAME ## Expr(SExpression* expr, SExpression* bound)	\
{																			\
	SLONG val;																\
	parse_VerifyPointers(expr, bound);										\
	val = expr->Value.Value;												\
	if((expr->Flags & EXPRF_isCONSTANT) && (bound->Flags & EXPRF_isCONSTANT))	\
	{																		\
		if(expr->Value.Value OP bound->Value.Value)							\
		{																	\
			parse_FreeExpression(expr);										\
			parse_FreeExpression(bound);									\
			return NULL;													\
		}																	\
	}																		\
	expr = parse_MergeExpressions(expr, bound);								\
	expr->Type = EXPR_OPERATOR;												\
	expr->Operator = T_FUNC_ ## NAME;										\
	expr->Value.Value = val;												\
	return expr;															\
}

CREATELIMIT(LOWLIMIT,<)
CREATELIMIT(HIGHLIMIT,>)

SExpression* parse_CheckRange(SExpression* expr, SLONG low, SLONG high)
{
	SExpression* low_expr;
	SExpression* high_expr;

	low_expr = parse_CreateConstExpr(low);
	high_expr = parse_CreateConstExpr(high);

	expr = parse_CreateLOWLIMITExpr(expr, low_expr);
	if(expr != NULL)
		return parse_CreateHIGHLIMITExpr(expr, high_expr);

	parse_FreeExpression(high_expr);
	return NULL;
}

static SExpression* parse_CreateFDIVExpr(SExpression* left, SExpression* right)
{
	SLONG val;

	parse_VerifyPointers(left, right);

	if(right->Value.Value != 0)
	{
		val = fdiv(left->Value.Value, right->Value.Value);

		left = parse_MergeExpressions(left, right);
		left->Type = EXPR_OPERATOR;
		left->Operator = T_FUNC_FDIV;
		left->Value.Value = val;
		left->Flags &= ~EXPRF_isRELOC;
		return left;
	}
	else
	{
		prj_Fail(ERROR_ZERO_DIVIDE);
		return NULL;
	}
}

static SExpression* parse_CreateFMULExpr(SExpression* left, SExpression* right)
{
	SLONG val;

	parse_VerifyPointers(left, right);

	val = fmul(left->Value.Value, right->Value.Value);

	left = parse_MergeExpressions(left, right);
	left->Type = EXPR_OPERATOR;
	left->Operator = T_FUNC_FMUL;
	left->Value.Value = val;
	left->Flags &= ~EXPRF_isRELOC;
	return left;
}

static SExpression* parse_CreateATAN2Expr(SExpression* left, SExpression* right)
{
	SLONG val;

	parse_VerifyPointers(left, right);

	val = fatan2(left->Value.Value, right->Value.Value);

	left = parse_MergeExpressions(left, right);
	left->Type = EXPR_OPERATOR;
	left->Operator = T_FUNC_ATAN2;
	left->Value.Value = val;
	left->Flags &= ~EXPRF_isRELOC;
	return left;
}

#define CREATETRANSEXPR(NAME,FUNC)										\
static SExpression* parse_Create ## NAME ## Expr(SExpression* right)	\
{																		\
	SExpression* expr;													\
	parse_VerifyPointers(right, right);									\
	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)		\
	{																	\
		expr->pRight = right;											\
		expr->pLeft = NULL;												\
		expr->Value.Value = FUNC(right->Value.Value);					\
		expr->Flags = right->Flags & ~EXPRF_isRELOC;					\
		expr->Type = EXPR_OPERATOR;										\
		expr->Operator = T_FUNC_ ## NAME;								\
		return expr;													\
	}																	\
	else																\
	{																	\
		internalerror("Out of memory!");								\
		return NULL;													\
	}																	\
}

CREATETRANSEXPR(SIN,fsin)
CREATETRANSEXPR(COS,fcos)
CREATETRANSEXPR(TAN,ftan)
CREATETRANSEXPR(ASIN,fasin)
CREATETRANSEXPR(ACOS,facos)
CREATETRANSEXPR(ATAN,fatan)

static SExpression* parse_CreateBANKExpr(char* s)
{
	SExpression* expr;

	if(!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
	{
		expr->pRight = NULL;
		expr->pLeft = NULL;
		expr->Value.pSymbol = sym_FindSymbol(s);
		expr->Value.pSymbol->Flags |= SYMF_REFERENCED;
		expr->Flags = EXPRF_isRELOC;
		expr->Type = EXPR_OPERATOR;
		expr->Operator = T_FUNC_BANK;
		return expr;
	}
	else
	{
		internalerror("Out of memory!");
		return NULL;
	}
}

static SExpression* parse_CreateSymbolExpr(char* s)
{
	SSymbol* sym;
	SExpression* expr;

	sym = sym_FindSymbol(s);

	if(sym->Flags & SYMF_EXPR)
	{
		sym->Flags |= SYMF_REFERENCED;
		if(sym->Flags & SYMF_CONSTANT)
		{
			return parse_CreateConstExpr(sym_GetConstant(s));
		}
		else if((expr = (SExpression*)malloc(sizeof(SExpression))) != NULL)
		{
			expr->pRight = NULL;
			expr->pLeft = NULL;
			expr->Value.pSymbol = sym;
			expr->Flags = EXPRF_isRELOC;
			expr->Type = EXPR_SYMBOL;
			return expr;
		}
		else
		{
			internalerror("Out of memory!");
			return NULL;
		}
	}
	else
	{
		prj_Fail(ERROR_SYMBOL_IN_EXPR);
		return NULL;
	}
}

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
				SLONG val = 0;
				int i;
				for(i = 0; i < len; ++i)
				{
					val = val << 8;
					val |= (UBYTE)g_CurrentToken.Value.aString[i];
				}
				parse_GetToken();
				return parse_CreateConstExpr(val);
			}
			return NULL;
		}
		case T_NUMBER:
		{
			SLONG val = g_CurrentToken.Value.nInteger;
			parse_GetToken();
			return parse_CreateConstExpr(val);
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

				parse_FreeExpression(expr);
			}

			lex_Goto(&bookmark);
			return NULL;
			break;
		}
		case T_ID:
		{
			if(strcmp(g_CurrentToken.Value.aString, "@") != 0)
			{
				SExpression* expr = parse_CreateSymbolExpr(g_CurrentToken.Value.aString);
				parse_GetToken();
				return expr;
			}
			// fall through to @
		}
		case T_OP_MUL:
		case '@':
		{
			SExpression* expr = parse_CreatePCExpr();
			parse_GetToken();
			return expr;
		}
		default:
		{
			if(g_CurrentToken.TokenLength > 0 && g_CurrentToken.ID.Token >= T_FIRST_TOKEN)
			{
				SExpression* expr = parse_CreateSymbolExpr(g_CurrentToken.Value.aString);
				parse_GetToken();
				return expr;
			}
			return NULL;
		}

	}
}

static char* parse_StringExpression(void);
static char* parse_StringExpressionRaw_Pri0(void);

SLONG parse_StringCompare(char* s)
{
	SLONG r = 0;
	char* t;

	parse_GetToken();
	if((t = parse_StringExpression()) != NULL)
	{
		r = strcmp(s, t);
		free(t);
	}
	free(s);

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
								r = parse_CreateConstExpr(strcmp(s, t));

							free(t);
						}
					}

					free(s);
					return r;
				}
				case T_FUNC_LENGTH:
				{
					SExpression* r = parse_CreateConstExpr((SLONG)strlen(s));
					free(s);
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
								SLONG val = -1;

								if((p = strstr(s, needle)) != NULL)
									val = (SLONG)(p - s);

								r = parse_CreateConstExpr(val);
							}
							free(needle);
						}
					}
					free(s);
					return r;
				}
				case T_OP_LOGICEQU:
				{
					SLONG v = parse_StringCompare(s);
					return parse_CreateConstExpr(v == 0 ? TRUE : FALSE);
				}
				case T_OP_LOGICNE:
				{
					SLONG v = parse_StringCompare(s);
					return parse_CreateConstExpr(v != 0 ? TRUE : FALSE);
				}
				case T_OP_LOGICGE:
				{
					SLONG v = parse_StringCompare(s);
					return parse_CreateConstExpr(v >= 0 ? TRUE : FALSE);
				}
				case T_OP_LOGICGT:
				{
					SLONG v = parse_StringCompare(s);
					return parse_CreateConstExpr(v > 0 ? TRUE : FALSE);
				}
				case T_OP_LOGICLE:
				{
					SLONG v = parse_StringCompare(s);
					return parse_CreateConstExpr(v <= 0 ? TRUE : FALSE);
				}
				case T_OP_LOGICLT:
				{
					SLONG v = parse_StringCompare(s);
					return parse_CreateConstExpr(v < 0 ? TRUE : FALSE);
				}
				default:
					break;
			}
		}
	}

	free(s);
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
			return parse_TwoArgFunc(parse_CreateATAN2Expr);
		case T_FUNC_SIN:
			return parse_SingleArgFunc(parse_CreateSINExpr);
		case T_FUNC_COS:
			return parse_SingleArgFunc(parse_CreateCOSExpr);
		case T_FUNC_TAN:
			return parse_SingleArgFunc(parse_CreateTANExpr);
		case T_FUNC_ASIN:
			return parse_SingleArgFunc(parse_CreateASINExpr);
		case T_FUNC_ACOS:
			return parse_SingleArgFunc(parse_CreateACOSExpr);
		case T_FUNC_ATAN:
			return parse_SingleArgFunc(parse_CreateATANExpr);
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

			t1 = parse_CreateConstExpr(sym_isDefined(g_CurrentToken.Value.aString));
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

			t1 = parse_CreateBANKExpr(g_CurrentToken.Value.aString);
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
			return parse_CreateSUBExpr(parse_CreateConstExpr(0), parse_ExprPri6());
		}
		case T_OP_NOT:
		{
			parse_GetToken();
			return parse_CreateXORExpr(parse_CreateConstExpr(0xFFFFFFFF), parse_ExprPri6());
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
				t1 = parse_CreateSHLExpr(t1, parse_ExprPri6());
				break;
			}
			case T_OP_SHR:
			{
				parse_GetToken();
				t1 = parse_CreateSHRExpr(t1, parse_ExprPri6());
				break;
			}
			case T_FUNC_FMUL:
			{
				parse_GetToken();
				t1 = parse_CreateFMULExpr(t1, parse_ExprPri6());
				break;
			}
			case T_OP_MUL:
			{
				parse_GetToken();
				t1 = parse_CreateMULExpr(t1, parse_ExprPri6());
				break;
			}
			case T_FUNC_FDIV:
			{
				parse_GetToken();
				t1 = parse_CreateFDIVExpr(t1, parse_ExprPri6());
				break;
			}
			case T_OP_DIV:
			{
				parse_GetToken();
				t1 = parse_CreateDIVExpr(t1, parse_ExprPri6());
				break;
			}
			case T_OP_MOD:
			{
				parse_GetToken();
				t1 = parse_CreateMODExpr(t1, parse_ExprPri6());
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
				t1 = parse_CreateXORExpr(t1, parse_ExprPri5());
				break;
			}
			case T_OP_OR:
			{
				parse_GetToken();
				t1 = parse_CreateORExpr(t1, parse_ExprPri5());
				break;
			}
			case T_OP_AND:
			{
				parse_GetToken();
				t1 = parse_CreateANDExpr(t1, parse_ExprPri5());
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
				t1 = parse_CreateADDExpr(t1, parse_ExprPri4());
				break;
			}
			case T_OP_SUB:
			{
				parse_GetToken();
				t1 = parse_CreateSUBExpr(t1, parse_ExprPri4());
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
				t1 = parse_CreateLOGICEQUExpr(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICGT:
			{
				parse_GetToken();
				t1 = parse_CreateLOGICGTExpr(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICLT:
			{
				parse_GetToken();
				t1 = parse_CreateLOGICLTExpr(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICGE:
			{
				parse_GetToken();
				t1 = parse_CreateLOGICGEExpr(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICLE:
			{
				parse_GetToken();
				t1 = parse_CreateLOGICLEExpr(t1, parse_ExprPri3());
				break;
			}
			case T_OP_LOGICNE:
			{
				parse_GetToken();
				t1 = parse_CreateLOGICNEExpr(t1, parse_ExprPri3());
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
			return parse_CreateLOGICNOTExpr(parse_ExprPri1());
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
				t1 = parse_CreateLOGICORExpr(t1, parse_ExprPri1());
				break;
			}
			case T_OP_LOGICAND:
			{
				parse_GetToken();
				t1 = parse_CreateLOGICANDExpr(t1, parse_ExprPri1());
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

SLONG parse_ConstantExpression(void)
{
	SExpression* expr;

	if((expr = parse_Expression()) != NULL)
	{
		if(expr->Flags & EXPRF_isCONSTANT)
		{
			SLONG r = expr->Value.Value;
			parse_FreeExpression(expr);
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
			char* r;

			if((r = _strdup(g_CurrentToken.Value.aString)) != NULL)
			{
				parse_GetToken();
				return r;
			}

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
			free(r);
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
				SLONG start;
				SLONG count;
				SLONG len = strlen(t);

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
					char* r;
					if((r = malloc(count + 1)) != NULL)
					{
						if(start + count >= len)
							count = len - start;

						strncpy(r, t + start, count);
						r[count] = 0;
						free(t);
						t = r;
					}
					else
						internalerror("Out of memory!");
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

		if((r = malloc(strlen(t1) + strlen(t2) + 1)) != NULL)
		{
			strcpy(r, t1);
			strcat(r, t2);
		}
		free(t2);
		free(t1);

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

static void parse_RS(char* pszName, SLONG size, int coloncount)
{
	sym_AddSET(pszName, sym_GetConstant("__RS"));
	sym_AddSET("__RS", sym_GetConstant("__RS") + size);

	if(coloncount == 2)
		sym_Export(pszName);
}


static void parse_RS_Skip(SLONG size)
{
	sym_AddSET("__RS", sym_GetConstant("__RS") + size);
}


static BOOL parse_Symbol(void)
{
	if(g_CurrentToken.ID.Token==T_LABEL)
	{
		ULONG coloncount;
		char name[MAXSYMNAMELENGTH+1];

		strcpy(name, g_CurrentToken.Value.aString);
		parse_GetToken();

		coloncount = parse_ColonCount();

		switch(g_CurrentToken.ID.Token)
		{
			case T_POP_RB:
			{
				parse_GetToken();
				parse_RS(name, parse_ConstantExpression(), coloncount);

				return TRUE;
			}
			case T_POP_RW:
			{
				parse_GetToken();
				parse_RS(name, parse_ConstantExpression() * 2, coloncount);

				return TRUE;
			}
			case T_POP_RL:
			{
				parse_GetToken();
				parse_RS(name, parse_ConstantExpression() * 4, coloncount);

				return TRUE;
			}
			case T_POP_EQU:
			{
				parse_GetToken();
				sym_AddEQU(name, parse_ConstantExpression());
				if(coloncount == 2)
				{
					sym_Export(name);
				}
				return TRUE;
				break;
			}
			case T_POP_SET:
			{
				parse_GetToken();
				sym_AddSET(name, parse_ConstantExpression());
				if(coloncount == 2)
				{
					sym_Export(name);
				}
				return TRUE;
				break;
			}
			case T_POP_EQUS:
			{
				char* r;

				parse_GetToken();
				if((r = parse_StringExpression()) != NULL)
				{
					sym_AddEQUS(name, r);
					free(r);
					if(coloncount == 2)
					{
						sym_Export(name);
					}
					return TRUE;
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
					{
						sym_AddGROUP(name, GROUP_TEXT);
						break;
					}
					case T_GROUP_BSS:
					{
						sym_AddGROUP(name, GROUP_BSS);
						break;
					}
					default:
					{
						prj_Error(ERROR_EXPECT_TEXT_BSS);
						return FALSE;
					}
				}
				if(coloncount == 2)
				{
					sym_Export(name);
				}
				return TRUE;
				break;
			}
			case T_POP_MACRO:
			{
				ULONG reptsize;
				char* reptblock;
				SLONG lineno = g_pFileContext->LineNumber;
				char* pszfile = g_pFileContext->pName;

				if(parse_CopyMacro(&reptblock, &reptsize))
				{
					sym_AddMACRO(name, reptblock, reptsize);
					parse_GetToken();
					return TRUE;
				}
				prj_Fail(ERROR_NEED_ENDM, pszfile, lineno);
				return FALSE;
				break;
			}
			default:
			{
				sym_AddLabel(name);
				if(coloncount == 2)
				{
					sym_Export(name);
				}
				return TRUE;
				break;
			}
		}
	}

	return FALSE;
}

static BOOL parse_Import(char* pszSym)
{
	return sym_Import(pszSym) != NULL;
}

static BOOL parse_Export(char* pszSym)
{
	return sym_Export(pszSym) != NULL;
}

static BOOL parse_Global(char* pszSym)
{
	return sym_Global(pszSym) != NULL;
}

static BOOL parse_SymbolOp(BOOL (*pOp)(char* pszSymbol))
{
	BOOL r = FALSE;

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
				r = TRUE;
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

static SLONG parse_ExpectBankFixed(void)
{
	SLONG bank;

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

static BOOL parse_PseudoOp(void)
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
			}
			parse_GetToken();
			return TRUE;
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
			return TRUE;
		}
		case T_POP_SECTION:
		{
			SLONG org;
			char* name;
			char r[MAXSYMNAMELENGTH+1];
			SSymbol* sym;

			parse_GetToken();
			if((name = parse_StringExpression()) == NULL)
				return TRUE;

			strcpy(r, name);
			free(name);

			if(!parse_ExpectChar(','))
				return sect_SwitchTo_NAMEONLY(r);

			if(g_CurrentToken.ID.Token != T_ID)
			{
				prj_Error(ERROR_EXPECT_IDENTIFIER);
				return FALSE;
			}

			sym = sym_FindSymbol(g_CurrentToken.Value.aString);
			if(sym->Type != SYM_GROUP)
			{
				prj_Error(ERROR_IDENTIFIER_GROUP);
				return TRUE;
			}
			parse_GetToken();

			if(g_pConfiguration->bSupportBanks && g_CurrentToken.ID.Token == ',')
			{
				SLONG bank;
				parse_GetToken();
				bank = parse_ExpectBankFixed();

				if(bank == -1)
					return TRUE;

				return sect_SwitchTo_BANK(r, sym, bank);
			}
			else if(g_CurrentToken.ID.Token != '[')
			{
				return sect_SwitchTo(r, sym);
			}
			parse_GetToken();

			org = parse_ConstantExpression();
			if(!parse_ExpectChar(']'))
				return TRUE;

			if(g_pConfiguration->bSupportBanks && g_CurrentToken.ID.Token == ',')
			{
				SLONG bank;
				parse_GetToken();
				bank = parse_ExpectBankFixed();

				if(bank == -1)
					return TRUE;

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
				free(r);
				return TRUE;
			}

			return FALSE;
		}
		case T_POP_PRINTV:
		{
			parse_GetToken();
			printf("$%lX", parse_ConstantExpression());
			return TRUE;
			break;
		}
		case T_POP_PRINTF:
		{
			SLONG i;

			parse_GetToken();
			i = parse_ConstantExpression();
			if(i < 0)
			{
				printf("-");
				i = -i;
			}
			printf("%ld.%05ld", i >> 16, imuldiv(i & 0xFFFF, 100000, 65536));

			return TRUE;
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
			BOOL r;

			g_bDontExpandStrings = TRUE;
			r = parse_SymbolOp(sym_Purge);
			g_bDontExpandStrings = FALSE;

			return r;
		}
		case T_POP_RSRESET:
		{
			parse_GetToken();
			sym_AddSET("__RS", 0);
			return TRUE;
			break;
		}
		case T_POP_RSSET:
		{
			SLONG val;

			parse_GetToken();
			val = parse_ConstantExpression();
			sym_AddSET("__RS", val);
			return TRUE;
		}
		case T_POP_RB:
		{
			parse_RS_Skip(parse_ConstantExpression());
			return TRUE;
		}
		case T_POP_RW:
		{
			parse_RS_Skip(parse_ConstantExpression() * 2);
			return TRUE;
		}
		case T_POP_RL:
		{
			parse_RS_Skip(parse_ConstantExpression() * 4);
			return TRUE;
		}
		case T_POP_FAIL:
		{
			char* r;

			parse_GetToken();
			if((r = parse_StringExpression()) != NULL)
			{
				prj_Fail(WARN_USER_GENERIC, r);
				return TRUE;
			}
			else
			{
				internalerror("String expression is NULL");
				return FALSE;
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
				return TRUE;
			}
			else
			{
				internalerror("String expression is NULL");
				return FALSE;
			}
			break;
		}
		case T_POP_EVEN:
		{
			parse_GetToken();
			sect_Align(2);
			return TRUE;
		}
		case T_POP_CNOP:
		{
			SLONG offset;
			SLONG align;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset < 0)
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return TRUE;
			}

			if(!parse_ExpectComma())
				return TRUE;

			align = parse_ConstantExpression();
			if(align < 0)
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return TRUE;
			}
			sect_Align(align);
			sect_SkipBytes(offset);
			return TRUE;
			break;
		}
		case T_POP_DSB:
		{
			SLONG offset;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset >= 0)
			{
				sect_SkipBytes(offset);
				return TRUE;
			}
			else
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return TRUE;
			}
			break;
		}
		case T_POP_DSW:
		{
			SLONG offset;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset >= 0)
			{
				sect_SkipBytes(offset * 2);
				return TRUE;
			}
			else
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return TRUE;
			}
			break;
		}
		case T_POP_DSL:
		{
			SLONG offset;

			parse_GetToken();
			offset = parse_ConstantExpression();
			if(offset >= 0)
			{
				sect_SkipBytes(offset * 4);
				return TRUE;
			}
			else
			{
				prj_Error(ERROR_EXPR_POSITIVE);
				return TRUE;
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
						sect_OutputAbsByte(*s++);
				}
				else if((expr = parse_Expression()) != NULL)
				{
					expr = parse_CheckRange(expr, -128, 255);
					if(expr)
						sect_OutputExprByte(expr);
					else
						prj_Error(ERROR_EXPRESSION_N_BIT, 8);
				}
				else
					prj_Error(ERROR_INVALID_EXPRESSION);
			} while(g_CurrentToken.ID.Token == ',');

			return TRUE;
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
					expr = parse_CheckRange(expr, -32768, 65535);
					if(expr)
						sect_OutputExprWord(expr);
					else
						prj_Error(ERROR_EXPRESSION_N_BIT, 16);
				}
				else
					prj_Error(ERROR_INVALID_EXPRESSION);
			} while(g_CurrentToken.ID.Token == ',');

			return TRUE;
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

			return TRUE;
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

				r = malloc(pEnd - pStart + 1);
				memcpy(r, pStart, pEnd - pStart);
				r[pEnd - pStart] = 0;
				lex_Goto(&mark);
				lex_SkipBytes((ULONG)(pEnd - mark.Buffer.pBuffer));
				parse_GetToken();
			}
			if(r != NULL)
			{
				fstk_RunInclude(r);
				free(r);
				return TRUE;
			}
			else
			{
				prj_Error(ERROR_EXPR_STRING);
				return FALSE;
			}
			break;
		}
		case T_POP_INCBIN:
		{
			char* r;

			parse_GetToken();
			if((r = parse_StringExpression()) != NULL)
			{
				sect_OutputBinaryFile(r);
				free(r);
				return TRUE;
			}
			return FALSE;
			break;
		}
		case T_POP_REPT:
		{
			ULONG reptsize;
			SLONG reptcount;
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
					free(reptblock);
				}
				else
				{
					free(reptblock);
				}
				return TRUE;
			}
			else
			{
				prj_Fail(ERROR_NEED_ENDR);
				return FALSE;
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
				if(expr->Flags&EXPRF_isCONSTANT)
				{
					fstk_ShiftMacroArgs(expr->Value.Value);
					parse_FreeExpression(expr);
					return TRUE;
				}
				else
				{
					prj_Fail(ERROR_EXPR_CONST);
					return FALSE;
				}
			}
			else
			{
				fstk_ShiftMacroArgs(1);
				return TRUE;
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

						free(s1);
						free(s2);
						return TRUE;
					}
					else
						prj_Error(ERROR_EXPR_STRING);
				}
				free(s1);
			}
			else
				prj_Error(ERROR_EXPR_STRING);

			return FALSE;
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

						free(s1);
						free(s2);
						return TRUE;
					}
					else
						prj_Error(ERROR_EXPR_STRING);
				}
				free(s1);
			}
			else
				prj_Error(ERROR_EXPR_STRING);

			return FALSE;
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
				return TRUE;
			}
			prj_Error(ERROR_EXPECT_IDENTIFIER);
			return FALSE;
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
				return TRUE;
			}
			prj_Error(ERROR_EXPECT_IDENTIFIER);
			return FALSE;
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
			return TRUE;
		}
		case T_POP_IFEQ:
		{
			parse_GetToken();

			if(!(parse_ConstantExpression() == 0))
			{
				/* will continue parsing just after ELSE or just at ENDC keyword */
				parse_IfSkipToElse();
			}
			return TRUE;
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
			return TRUE;
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
			return TRUE;
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
			return TRUE;
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
			return TRUE;
			break;
		}
		case T_POP_ELSE:
		{
			/* will continue parsing just at ENDC keyword */
			parse_IfSkipToEndc();
			parse_GetToken();
			return TRUE;
			break;
		}
		case T_POP_ENDC:
		{
			parse_GetToken();
			return TRUE;
			break;
		}
		case T_POP_PUSHO:
		{
			opt_Push();
			parse_GetToken();
			return TRUE;
		}
		case T_POP_POPO:
		{
			opt_Pop();
			parse_GetToken();
			return TRUE;
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
			return TRUE;
		}
		default:
		{
			return FALSE;
		}
	}
}

BOOL parse_Misc(void)
{
	switch(g_CurrentToken.ID.Token)
	{
		case T_ID:
		{
			if(sym_isMacro(g_CurrentToken.Value.aString))
			{
				char* s;

				if((s = _strdup(g_CurrentToken.Value.aString)) != NULL)
				{
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
								return FALSE;
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
							return FALSE;
						}
					}
					lex_SetState(LEX_STATE_NORMAL);
					fstk_RunMacro(s);
					free(s);
					return TRUE;
				}
				else
				{
					internalerror("Out of memory");
					return FALSE;
				}
			}
			else
			{
				prj_Error(ERROR_INSTR_UNKNOWN, g_CurrentToken.Value.aString);
				return FALSE;
			}
			break;
		}
		default:
		{
			return FALSE;
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

BOOL parse_Do(void)
{
	BOOL r = TRUE;

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
				return TRUE;
			}
			else
			{
				prj_Error(ERROR_SYNTAX);
				r = FALSE;
			}
		}
	}

	return r;
}
