#include <ctype.h>

#include "errors.h"
#include "symbol.h"


static strmap_t* s_symbols = NULL;


static void
freeSymbol(intptr_t userData, intptr_t element) {
	SSymbol* sym = (SSymbol*) element;

	if (sym->locals != NULL)
		strmap_Free(sym->locals);
	str_Free(sym->name);
	mem_Free(sym);
}


static strmap_t*
allocateSymbolMap(void) {
	return strmap_Create(freeSymbol);
}


static SSymbol*
findOrAllocateSymbol(const string* name) {
	SSymbol* sym;
	if (!strmap_Value(s_symbols, name, (intptr_t*) &sym)) {
		sym = (SSymbol*) mem_Alloc(sizeof(SSymbol));
		sym->name = str_Copy(name);
		sym->type = SYMBOL_UNKNOWN;
		sym->locals = NULL;
		sym->value.integer = 0;
		sym->callback.integer = NULL;
		strmap_Insert(s_symbols, name, (intptr_t) sym);
	}

	return sym;
}


static SSymbol*
defineSymbolOfType(const string* name, symboltype_t type) {
	SSymbol* sym;
	if (!strmap_Value(s_symbols, name, (intptr_t*) &sym)) {
		sym = (SSymbol*) mem_Alloc(sizeof(SSymbol));
		sym->name = str_Copy(name);
		sym->type = type;
		sym->locals = NULL;
		sym->value.integer = 0;
		sym->callback.integer = NULL;
		strmap_Insert(s_symbols, name, (intptr_t) sym);
	}

	err_Error(ERROR_SYMBOL_EXISTS, str_String(name));
	return NULL;
}


extern void
sym_Init(void) {
	s_symbols = allocateSymbolMap();
}


extern void
sym_Close(void) {
	strmap_Free(s_symbols);
}


extern bool
sym_IsLabelStartChar(char ch) {
	return ch == '.' || ch == '_' || isalpha(ch);
}


extern bool
sym_IsLabelChar(char ch) {
	return ch == '_' || isalnum(ch);
}


extern SSymbol*
sym_CreateGroup(const string* name, EGroupType value) {
	internalerror("sym_CreateGroup not implemented");
}


extern SSymbol*
sym_CreateConstant(const string* name, int64_t value) {
	SSymbol* sym = defineSymbolOfType(name, SYMBOL_INTEGER_CONSTANT);
	if (sym != NULL)
		sym->value.integer = value;

	return sym;
}


extern SSymbol*
sym_CreateVariable(const string* name, int64_t value) {
	SSymbol* sym = defineSymbolOfType(name, SYMBOL_INTEGER_CONSTANT);
	if (sym != NULL)
		sym->value.integer = value;

	return sym;
}


extern SSymbol*
sym_UpdateVariable(const string* name, int64_t value) {
	SSymbol* sym;
	if (strmap_Value(s_symbols, name, (intptr_t*) &sym)) {
		if (sym->type == SYMBOL_INTEGER_VARIABLE) {
			sym->value.integer = value;
			return sym;
		}
		err_Error(ERROR_NOT_A_VARIABLE, str_String(name));
	}

	return sym;
}


extern SSymbol*
sym_CreateLabel(const string* label) {
	internalerror("sym_CreateLabel not implemented");
}


extern SSymbol*
sym_Find(const string* label) {
	return findOrAllocateSymbol(label);
}


extern int64_t
sym_IntegerValueOf(const string* name) {
	SSymbol* sym;
	if (strmap_Value(s_symbols, name, (intptr_t*) &sym)) {
		if (sym != NULL && (sym->type == SYMBOL_INTEGER_CONSTANT || sym->type == SYMBOL_INTEGER_VARIABLE)) {
			return sym->callback.integer(sym);
		}
	}

	err_Error(ERROR_SYMBOL_EXISTS, str_String(name));
	return 0;
}

