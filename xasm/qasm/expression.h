#pragma once

#include "types.h"
#include "util.h"

typedef enum {
    EXPR_PARENS
} EExpressionType;

typedef struct Expression {
    struct Expression* left;
    struct Expression* right;
	EExpressionType type;
	bool isConstant;
    union {
        long double floating;
        int64_t integer;
        struct Symbol* symbol;
    } value;
} SExpression;


extern SExpression*
expr_Const(int64_t value);

extern SExpression*
expr_CheckRange(SExpression* expression, int64_t low, int64_t high);

extern void
sect_OutputExpr8(struct Expression* expr);

extern void
sect_OutputExpr16(struct Expression* expr);

extern void
sect_OutputExpr32(struct Expression* expr);

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

INLINE bool
expr_IsConstant(const SExpression* expression) {
    return expression != NULL && expression->isConstant;
}
