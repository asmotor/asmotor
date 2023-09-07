#pragma once

#include "types.h"

typedef struct Expression {
    struct Expression* left;
    struct Expression* right;
    union {
        long double floating;
        int32_t integer;
        struct Symbol* symbol;
    } value;
} SExpression;


extern SExpression*
expr_Const(int32_t value);

extern SExpression*
expr_CheckRange(SExpression* expression, int32_t low, int32_t high);

extern void
sect_OutputExpr8(struct Expression* expr);

extern void
sect_OutputExpr16(struct Expression* expr);

extern void
sect_OutputExpr32(struct Expression* expr);

extern SExpression*
expr_And(SExpression* left, SExpression* right);

extern SExpression*
expr_Or(SExpression* left, SExpression* right);

extern SExpression*
expr_Asl(SExpression* left, SExpression* right);

extern SExpression*
expr_Asr(SExpression* left, SExpression* right);

extern SExpression*
expr_PcRelative(SExpression* expr, int adjustment);

