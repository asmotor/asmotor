#pragma once

#include "str.h"
#include "strcoll.h"
#include "types.h"

typedef enum {
    GROUP_TEXT = 0,
    GROUP_BSS = 1
} EGroupType;


typedef enum {
	SYMBOL_UNKNOWN,
	SYMBOL_INTEGER_CONSTANT,
	SYMBOL_INTEGER_VARIABLE,
	SYMBOL_LABEL
} symboltype_t;


typedef struct Symbol {
	string* name;
	symboltype_t type;
	strmap_t* locals;
	union {
		int64_t integer;
	} value;
} SSymbol;


extern bool
sym_IsLabelStartChar(char ch);

extern bool
sym_IsLabelChar(char ch);

extern SSymbol*
sym_CreateGroup(const string* name, EGroupType value);

extern SSymbol*
sym_CreateConstant(const string* name, int64_t value);

extern SSymbol*
sym_CreateVariable(const string* name, int64_t value);

extern SSymbol*
sym_CreateLabel(const string* label);

extern SSymbol*
sym_Find(const string* label);

extern void
sym_Init(void);

extern void
sym_Close(void);
