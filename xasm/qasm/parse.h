#pragma once

#include "types.h"

#include "expression.h"


extern void
parse_GetToken(void);

extern SExpression*
parse_Expression(size_t maxStringConstLength);

extern bool
parse_ExpectChar(char ch);

