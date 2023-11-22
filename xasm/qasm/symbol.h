#pragma once

#include "str.h"
#include "strcoll.h"
#include "types.h"

typedef enum {
    GROUP_TEXT = 0,
    GROUP_BSS = 1
} group_type_t;

#define EGroupType group_type_t


typedef enum {
	SYMBOL_UNKNOWN,
	SYMBOL_INTEGER_CONSTANT,
	SYMBOL_INTEGER_VARIABLE,
	SYMBOL_LABEL,
	SYMBOL_GROUP
} symboltype_t;


typedef struct Symbol {
	string* name;
	symboltype_t type;
	strmap_t* locals;

	union {
		int64_t integer;
		group_type_t group_type;
	} value;

    union {
        int64_t (* integer)(struct Symbol*);
        string* (* string)(struct Symbol*);
    } callback;
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
sym_UpdateVariable(const string* name, int64_t value);

extern SSymbol*
sym_CreateLabel(const string* label);

extern SSymbol*
sym_Find(const string* label);

extern void
sym_Init(void);

extern void
sym_Close(void);

extern int64_t
sym_IntegerValueOf(const string* name);

#define sym_CreateEqu sym_CreateConstant