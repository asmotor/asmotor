#pragma once

#include "types.h"

#include "expression.h"
#include "tokens.h"


extern int64_t
parse_ConstantExpression(size_t maxStringConstLength);

extern SExpression*
parse_Expression(size_t maxStringConstLength);

