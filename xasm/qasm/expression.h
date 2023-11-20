#pragma once

#include "str.h"
#include "types.h"
#include "util.h"

#include "tokens.h"


typedef enum {
	EXPR_STRING,
    EXPR_SYMBOL,
    EXPR_PARENS,
	EXPR_CONSTANT,
	EXPR_OPERATION
} exprtype_t;

typedef struct Expression {
    struct Expression* left;
    struct Expression* right;
	exprtype_t type;
	token_t operation;
	bool isConstant;
    union {
        long double floating;
        int64_t integer;
        struct Symbol* symbol;
		string* string;
    } value;
} SExpression;


extern void
expr_Free(SExpression* expr);

extern SExpression*
expr_Const(int64_t value);

extern SExpression*
expr_String(const string* str);

extern SExpression*
expr_CheckRange(SExpression* expression, int64_t low, int64_t high);

extern SExpression*
expr_BooleanNot(SExpression* expr);

extern SExpression*
expr_BooleanOr(SExpression* left, SExpression* right);

extern SExpression*
expr_BooleanAnd(SExpression* left, SExpression* right);

extern SExpression*
expr_Or(SExpression* left, SExpression* right);

extern SExpression*
expr_And(SExpression* left, SExpression* right);

extern SExpression*
expr_Xor(SExpression* left, SExpression* right);

extern SExpression*
expr_Add(SExpression* left, SExpression* right);

extern SExpression*
expr_Sub(SExpression* left, SExpression* right);

extern SExpression*
expr_Mul(SExpression* left, SExpression* right);

extern SExpression*
expr_Div(SExpression* left, SExpression* right);

extern SExpression*
expr_Mod(SExpression* left, SExpression* right);

extern SExpression*
expr_Asl(SExpression* left, SExpression* right);

extern SExpression*
expr_Asr(SExpression* left, SExpression* right);

extern SExpression*
expr_PcRelative(SExpression* expr, int adjustment);

extern SExpression*
expr_Pc(void);

extern SExpression*
expr_Equal(SExpression* left, SExpression* right);

extern SExpression*
expr_NotEqual(SExpression* left, SExpression* right);

extern SExpression*
expr_GreaterThan(SExpression* left, SExpression* right);

extern SExpression*
expr_LessThan(SExpression* left, SExpression* right);

extern SExpression*
expr_GreaterEqual(SExpression* left, SExpression* right);

extern SExpression*
expr_LessEqual(SExpression* left, SExpression* right);

extern SExpression*
expr_SymbolByName(const string* name);

INLINE bool
expr_IsConstant(const SExpression* expression) {
    return expression != NULL && expression->isConstant;
}
