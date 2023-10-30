#pragma once

#include "str.h"
#include "types.h"

typedef enum {
    GROUP_TEXT = 0,
    GROUP_BSS = 1
} EGroupType;


typedef struct Symbol {
	string* name;
} SSymbol;


extern SSymbol*
sym_CreateGroup(string* name, EGroupType value);

extern SSymbol*
sym_CreateConstant(string* name, int64_t value);

extern SSymbol*
sym_CreateVariable(string* name, int64_t value);

extern SSymbol*
sym_CreateLabel(string* label);
