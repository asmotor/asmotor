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

#include <stdio.h>

#include "asmotor.h"
#include "mem.h"
#include "xasm.h"
#include "expression.h"
#include "symbol.h"
#include "section.h"
#include "tokens.h"
#include "project.h"

#define T_OP_Sub T_OP_SUB
#define T_OP_Add T_OP_ADD
#define T_OP_Xor T_OP_XOR
#define T_OP_Or T_OP_OR
#define T_OP_And T_OP_AND
#define T_OP_Shl T_OP_SHL
#define T_OP_Shr T_OP_SHR
#define T_OP_Mul T_OP_MUL
#define T_OP_Div T_OP_DIV
#define T_OP_Mod T_OP_MOD
#define T_OP_BooleanOr T_OP_LOGICOR
#define T_OP_BooleanAnd T_OP_LOGICAND
#define T_OP_GreaterEqual T_OP_LOGICGE
#define T_OP_GreaterThan T_OP_LOGICGT
#define T_OP_LessEqual T_OP_LOGICLE
#define T_OP_LessThan T_OP_LOGICLT
#define T_OP_Equal T_OP_LOGICEQU
#define T_OP_NotEqual T_OP_LOGICNE

#define T_FUNC_Sin T_FUNC_SIN
#define T_FUNC_Cos T_FUNC_COS
#define T_FUNC_Tan T_FUNC_TAN
#define T_FUNC_Asin T_FUNC_ASIN
#define T_FUNC_Acos T_FUNC_ACOS
#define T_FUNC_Atan T_FUNC_ATAN
#define T_FUNC_Atan2 T_FUNC_ATAN2

#define T_FUNC_LowLimit T_FUNC_LOWLIMIT
#define T_FUNC_HighLimit T_FUNC_HIGHLIMIT

static bool_t expr_VerifyPointers(SExpression* pLeft, SExpression* pRight)
{
    if(pLeft != NULL && pRight != NULL )
        return true;

    prj_Fail(ERROR_INVALID_EXPRESSION);
    expr_Free(pLeft);
    expr_Free(pRight);
    return false;
}

static bool_t expr_VerifyPointer(SExpression* pExpr)
{
    if(pExpr != NULL)
        return true;

    prj_Fail(ERROR_INVALID_EXPRESSION);
    return false;
}

SExpression* expr_Parens(SExpression* pExpr)
{
    SExpression* expr;

    if(!expr_VerifyPointer(pExpr))
        return NULL;

    expr = (SExpression*)mem_Alloc(sizeof(SExpression));
    expr->pRight = pExpr;
    expr->pLeft = NULL;
    expr->Value.Value = pExpr->Value.Value;
    expr->nFlags = pExpr->nFlags;
    expr->eType = EXPR_PARENS;
    expr->eOperator = 0;
    return expr;
}

SExpression* expr_Abs(SExpression* pRight)
{
    SExpression* pSign = expr_Shr(expr_Clone(pRight), expr_Const(31));
    return expr_Sub(expr_Xor(pRight, expr_Clone(pSign)), pSign);
}


SExpression* expr_Bit(SExpression* pRight)
{
    SExpression* expr;
    int b = 0;
    uint32_t v;

    if(!expr_VerifyPointer(pRight))
        return NULL;

    v = pRight->Value.Value;

    if(expr_IsConstant(pRight) && (v & -(int32_t)v) != v)
    {
        prj_Error(ERROR_EXPR_TWO_POWER);
        return NULL;
    }

    b = 0;
    if(v != 0)
    {
        while(v != 1)
        {
            v >>= 1;
            ++b;
        }
    }

    expr = (SExpression*)mem_Alloc(sizeof(SExpression));
    expr->pRight = pRight;
    expr->pLeft = NULL;
    expr->Value.Value = b;
    expr->nFlags = pRight->nFlags;
    expr->eType = EXPR_OPERATOR;
    expr->eOperator = T_OP_BIT;
    return expr;
}

SExpression* expr_PcRelative(SExpression* pExpr, int nAdjust)
{
    if (!expr_VerifyPointer(pExpr))
        return NULL;

    if ((pExpr->nFlags & EXPRF_CONSTANT) && (pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)))
    {
        pExpr->Value.Value -= (pCurrentSection->PC + pCurrentSection->BasePC + pCurrentSection->OrgOffset - nAdjust);
        return pExpr;
    }
    else if (pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED))
    {
        return expr_Add(pExpr, expr_Const(nAdjust - (pCurrentSection->PC + pCurrentSection->BasePC + pCurrentSection->OrgOffset)));
    }
    else
    {
        SExpression* expr = (SExpression*)mem_Alloc(sizeof(SExpression));
        
        expr->Value.Value = 0;
        expr->eType = EXPR_PCREL;
        expr->nFlags = EXPRF_RELOC;
        expr->pLeft = expr_Const(nAdjust);
        expr->pRight = pExpr;
        return expr;
    }
}


SExpression* expr_Pc()
{
    string* pName;
    SExpression* expr = (SExpression*)mem_Alloc(sizeof(SExpression));

    char sym[MAXSYMNAMELENGTH + 20];
    SSymbol* pSym;

    sprintf(sym, "$%s%u", str_String(pCurrentSection->Name), pCurrentSection->PC);
    pName = str_Create(sym);
    pSym = sym_CreateLabel(pName);
    str_Free(pName);

    if(pSym->nFlags & SYMF_CONSTANT)
    {
        expr->Value.Value = pSym->Value.Value;
        expr->eType = EXPR_CONSTANT;
        expr->nFlags = EXPRF_CONSTANT | EXPRF_RELOC;
        expr->pLeft = NULL;
        expr->pRight = NULL;
    }
    else
    {
        expr->pRight = NULL;
        expr->pLeft = NULL;
        expr->Value.pSymbol = pSym;
        expr->nFlags = EXPRF_RELOC;
        expr->eType = EXPR_SYMBOL;
    }

    return expr;
}

void expr_SetConst(SExpression* pExpr, int32_t nValue)
{
    pExpr->pLeft = NULL;
    pExpr->pRight = NULL;
    pExpr->eType = EXPR_CONSTANT;
    pExpr->nFlags = EXPRF_CONSTANT | EXPRF_RELOC;
    pExpr->Value.Value = nValue;
}

SExpression* expr_Const(int32_t nValue)
{
    SExpression* pExpr = (SExpression*)mem_Alloc(sizeof(SExpression));

    expr_SetConst(pExpr, nValue);
    return pExpr;
}

static SExpression* parse_MergeExpressions(SExpression* pLeft, SExpression* pRight)
{
    SExpression* expr;

    if(!expr_VerifyPointers(pLeft, pRight))
        return NULL;

    expr = (SExpression*)mem_Alloc(sizeof(SExpression));

    expr->Value.Value = 0;
    expr->eType = 0;
    expr->nFlags = pLeft->nFlags & pRight->nFlags;
    expr->pLeft = pLeft;
    expr->pRight = pRight;
    return expr;
}

#define CREATEEXPRDIV(NAME,OP)															\
SExpression* expr_ ## NAME(SExpression* left, SExpression* right)	\
{													\
    int32_t val;									\
    if(!expr_VerifyPointers(left, right))			\
        return NULL;								\
    if(right->Value.Value == 0)						\
    {												\
        prj_Fail(ERROR_ZERO_DIVIDE);				\
        return NULL;								\
    }												\
    val = left->Value.Value OP right->Value.Value;	\
    left = parse_MergeExpressions(left, right);		\
    left->eType = EXPR_OPERATOR;						\
    left->eOperator = T_OP_ ## NAME;					\
    left->Value.Value = val;							\
    return left;									\
}

CREATEEXPRDIV(Div, /)
CREATEEXPRDIV(Mod, %)

SExpression* expr_BooleanNot(SExpression* right)
{
    SExpression* expr;

    if(!expr_VerifyPointer(right))
        return NULL;

    expr = (SExpression*)mem_Alloc(sizeof(SExpression));

    expr->pRight = right;
    expr->pLeft = NULL;
    expr->Value.Value = !right->Value.Value;
    expr->nFlags = right->nFlags;
    expr->eType = EXPR_OPERATOR;
    expr->eOperator = T_OP_LOGICNOT;
    return expr;
}

SExpression* expr_Sub(SExpression* left, SExpression* right)
{
    int32_t val;
    if(!expr_VerifyPointers(left, right))
        return NULL;
    val = left->Value.Value - right->Value.Value;
    left = parse_MergeExpressions(left, right);
    left->eType = EXPR_OPERATOR;
    left->eOperator = T_OP_SUB;
    left->Value.Value = val;
    if ((left->nFlags & EXPRF_CONSTANT) == 0 && left->pLeft != NULL && left->pRight != NULL)
    {
        if (left->pLeft->eType == EXPR_SYMBOL && left->pRight->eType == EXPR_SYMBOL)
        {
            if (left->pLeft->Value.pSymbol->pSection == left->pRight->Value.pSymbol->pSection)
            {
                left->nFlags = EXPRF_CONSTANT;
                left->Value.Value = left->pLeft->Value.pSymbol->Value.Value - left->pRight->Value.pSymbol->Value.Value;
            }
        }
    }
    return left;
}

#define CREATEEXPR(NAME,OP) \
SExpression* expr_ ## NAME(SExpression* left, SExpression* right)	\
{													\
    int32_t val;									\
    if(!expr_VerifyPointers(left, right))			\
        return NULL;								\
    val = left->Value.Value OP right->Value.Value;	\
    left = parse_MergeExpressions(left, right);		\
    left->eType = EXPR_OPERATOR;						\
    left->eOperator = T_OP_ ## NAME;					\
    left->Value.Value = val;							\
    return left;									\
}

CREATEEXPR(Add, +)
CREATEEXPR(Xor, ^)
CREATEEXPR(Or,  |)
CREATEEXPR(And, &)
CREATEEXPR(Shl, <<)
CREATEEXPR(Shr, >>)
CREATEEXPR(Mul, *)
CREATEEXPR(BooleanOr, ||)
CREATEEXPR(BooleanAnd, &&)
CREATEEXPR(GreaterEqual, >=)
CREATEEXPR(GreaterThan, >)
CREATEEXPR(LessEqual, <=)
CREATEEXPR(LessThan, <)
CREATEEXPR(Equal, ==)
CREATEEXPR(NotEqual, !=)

#define CREATELIMIT(NAME,OP)	\
SExpression* expr_ ## NAME(SExpression* expr, SExpression* bound)	\
{														\
    int32_t val;										\
    if(!expr_VerifyPointers(expr, bound))				\
        return NULL;									\
    val = expr->Value.Value;								\
    if(expr_IsConstant(expr) && expr_IsConstant(bound))	\
    {													\
        if(expr->Value.Value OP bound->Value.Value)		\
        {												\
            expr_Free(expr);							\
            expr_Free(bound);							\
            return NULL;								\
        }												\
    }													\
    expr = parse_MergeExpressions(expr, bound);			\
    expr->eType = EXPR_OPERATOR;							\
    expr->eOperator = T_FUNC_ ## NAME;					\
    expr->Value.Value = val;								\
    return expr;										\
}

CREATELIMIT(LowLimit,<)
CREATELIMIT(HighLimit,>)

SExpression* expr_CheckRange(SExpression* pExpr, int32_t nLow, int32_t nHigh)
{
    SExpression* pLowExpr = expr_Const(nLow);
    SExpression* pHighExpr = expr_Const(nHigh);

    pExpr = expr_LowLimit(pExpr, pLowExpr);
    if(pExpr != NULL)
        return expr_HighLimit(pExpr, pHighExpr);

    expr_Free(pHighExpr);
    return NULL;
}

SExpression* expr_Fdiv(SExpression* left, SExpression* right)
{
    if(!expr_VerifyPointers(left, right))
        return NULL;

    if(right->Value.Value != 0)
    {
        int32_t val = fdiv(left->Value.Value, right->Value.Value);

        left = parse_MergeExpressions(left, right);
        left->eType = EXPR_OPERATOR;
        left->eOperator = T_FUNC_FDIV;
        left->Value.Value = val;
        left->nFlags &= ~EXPRF_RELOC;
        return left;
    }

    prj_Fail(ERROR_ZERO_DIVIDE);
    return NULL;
}

SExpression* expr_Fmul(SExpression* left, SExpression* right)
{
    int32_t val;

    if(!expr_VerifyPointers(left, right))
        return NULL;

    val = fmul(left->Value.Value, right->Value.Value);

    left = parse_MergeExpressions(left, right);
    left->eType = EXPR_OPERATOR;
    left->eOperator = T_FUNC_FMUL;
    left->Value.Value = val;
    left->nFlags &= ~EXPRF_RELOC;
    return left;
}

SExpression* expr_Atan2(SExpression* left, SExpression* right)
{
    int32_t val;

    if(!expr_VerifyPointers(left, right))
        return NULL;

    val = fatan2(left->Value.Value, right->Value.Value);

    left = parse_MergeExpressions(left, right);
    left->eType = EXPR_OPERATOR;
    left->eOperator = T_FUNC_ATAN2;
    left->Value.Value = val;
    left->nFlags &= ~EXPRF_RELOC;
    return left;
}

#define CREATETRANSEXPR(NAME,FUNC)							\
SExpression* expr_ ## NAME(SExpression* right)				\
{															\
    SExpression* expr;										\
    if(!expr_VerifyPointer(right))							\
        return NULL;										\
    expr = (SExpression*)mem_Alloc(sizeof(SExpression));	\
    expr->pRight = right;									\
    expr->pLeft = NULL;										\
    expr->Value.Value = FUNC(right->Value.Value);			\
    expr->nFlags = right->nFlags & ~EXPRF_RELOC;				\
    expr->eType = EXPR_OPERATOR;								\
    expr->eOperator = T_FUNC_ ## NAME;						\
    return expr;											\
}

CREATETRANSEXPR(Sin,fsin)
CREATETRANSEXPR(Cos,fcos)
CREATETRANSEXPR(Tan,ftan)
CREATETRANSEXPR(Asin,fasin)
CREATETRANSEXPR(Acos,facos)
CREATETRANSEXPR(Atan,fatan)

SExpression* expr_Bank(char* s)
{
    SExpression* expr;
    string* pName;

    if(!g_pConfiguration->bSupportBanks)
        internalerror("Banks not supported");

    pName = str_Create(s);
    
    expr = (SExpression*)mem_Alloc(sizeof(SExpression));

    expr->pRight = NULL;
    expr->pLeft = NULL;
    expr->Value.pSymbol = sym_FindSymbol(pName);
    expr->Value.pSymbol->nFlags |= SYMF_REFERENCED;
    expr->nFlags = EXPRF_RELOC;
    expr->eType = EXPR_OPERATOR;
    expr->eOperator = T_FUNC_BANK;
    
    str_Free(pName);
    
    return expr;
}

SExpression* expr_Symbol(char* s)
{
    string* pName = str_Create(s);
    SSymbol* pSym = sym_FindSymbol(pName);
    str_Free(pName);

    if(pSym->nFlags & SYMF_EXPR)
    {
        pSym->nFlags |= SYMF_REFERENCED;
        
        if(pSym->nFlags & SYMF_CONSTANT)
        {
            return expr_Const(sym_GetValue(pSym));
        }
        else
        {
            SExpression* pExpr = (SExpression*)mem_Alloc(sizeof(SExpression));

            pExpr->pRight = NULL;
            pExpr->pLeft = NULL;
            pExpr->Value.pSymbol = pSym;
            pExpr->nFlags = EXPRF_RELOC;
            pExpr->eType = EXPR_SYMBOL;
            
            return pExpr;
        }
    }

    prj_Fail(ERROR_SYMBOL_IN_EXPR);
    return NULL;
}


void expr_Clear(SExpression* pExpr)
{
    if(pExpr == NULL)
        return;

    expr_Free(pExpr->pLeft);
    pExpr->pLeft = NULL;

    expr_Free(pExpr->pRight);
    pExpr->pRight = NULL;

    pExpr->eType = 0;
    pExpr->nFlags = 0;
}


void expr_Free(SExpression* pExpr)
{
    if(pExpr == NULL)
        return;

    expr_Free(pExpr->pLeft);
    expr_Free(pExpr->pRight);
    mem_Free(pExpr);
}


SExpression* expr_Clone(SExpression* expr)
{
    SExpression* r = (SExpression*)mem_Alloc(sizeof(SExpression));
    
    *r = *expr;
    if(r->pLeft != NULL)
        r->pLeft = expr_Clone(r->pLeft);
    if(r->pRight != NULL)
        r->pRight = expr_Clone(r->pRight);

    return r;
}
