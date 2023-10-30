#pragma once

#include "types.h"

#include "expression.h"


extern void
parse_GetToken(void);

extern int64_t
parse_ConstantExpression(size_t maxStringConstLength);

extern SExpression*
parse_Expression(size_t maxStringConstLength);

extern bool
parse_ExpectChar(char ch);
