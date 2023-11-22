#pragma once

#include "types.h"

#include "expression.h"


extern void
parse_GetToken(void);

extern bool
parse_ExpectChar(char ch);

extern SSymbol*
parse_Symbol(void);
