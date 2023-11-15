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
findSymbol(const string* name) {
	SSymbol* sym;
	if (!strmap_Value(s_symbols, name, (intptr_t*) &sym)) {
		sym = (SSymbol*) mem_Alloc(sizeof(SSymbol));
		sym->name = str_Copy(name);
		sym->type = SYMBOL_UNKNOWN;
		sym->locals = NULL;
		sym->value.integer = 0;
		strmap_Insert(s_symbols, name, (intptr_t) sym);
	}

	return sym;
}


static SSymbol*
allocateSymbolOfType(const string* name, symboltype_t type) {
	SSymbol* sym = findSymbol(name);
	if (sym->type == SYMBOL_UNKNOWN) {
		sym->type = type;
		return sym;
	}

	err_Error(ERROR_SYMBOL_EXISTS);
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
	SSymbol* sym = allocateSymbolOfType(name, SYMBOL_INTEGER_CONSTANT);
	if (sym != NULL)
		sym->value.integer = value;

	return sym;
}


extern SSymbol*
sym_CreateVariable(const string* name, int64_t value) {
	internalerror("sym_CreateVariable not implemented");
}


extern SSymbol*
sym_CreateLabel(const string* label) {
	internalerror("sym_CreateLabel not implemented");
}


extern SSymbol*
sym_Find(const string* label) {
	return findSymbol(label);
}

