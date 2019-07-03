/*  Copyright 2008-2017 Carsten Elton Sorensen

    This file is part of ASMotor.

    ASMotor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ASMotor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ASMotor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "asmotor.h"
#include "mem.h"
#include "xasm.h"
#include "symbol.h"
#include "filestack.h"
#include "errors.h"
#include "section.h"

#define SET_TYPE_AND_FLAGS(symbol, t) ((symbol)->type=t,(symbol)->flags=((symbol)->flags&SYMF_EXPORT)|g_defaultSymbolFlags[t])

static uint32_t g_defaultSymbolFlags[] = {
    SYMF_RELOC | SYMF_EXPORTABLE | SYMF_EXPRESSION,     // SYM_LABEL
    SYMF_CONSTANT | SYMF_EXPORTABLE | SYMF_EXPRESSION,  // SYM_EQU
    SYMF_CONSTANT | SYMF_EXPRESSION | SYMF_MODIFIABLE,  // SYM_SET
    SYMF_HAS_DATA,                                      // SYM_EQUS
    SYMF_HAS_DATA,                                      // SYM_MACRO
    SYMF_EXPRESSION | SYMF_RELOC,                       // SYM_IMPORT
    SYMF_EXPORT,                                        // SYM_GROUP
    SYMF_EXPRESSION | SYMF_MODIFIABLE | SYMF_RELOC,     // SYM_GLOBAL
    SYMF_MODIFIABLE | SYMF_EXPRESSION | SYMF_EXPORTABLE // SYM_UNDEFINED
};

static SSymbol* g_currentScope;

SSymbol* sym_hashedSymbols[SYMBOL_HASH_SIZE];


// Symbol value callbacks

static int32_t
callback__NARG(SSymbol* symbol) {
    return fstk_GetMacroArgumentCount();
}

static int32_t
callback__LINE(SSymbol* symbol) {
    return fstk_GetMostCurrentStackEntry()->lineNumber;
}

static string* getDateString() {
    time_t currentTime = time(NULL);

    char dateString[16];
    size_t stringLength = strftime(dateString, sizeof(dateString), "%Y-%m-%d", localtime(&currentTime));

    return str_CreateLength(dateString, stringLength);
}

static string* getTimeString() {
    time_t currentTime = time(NULL);

    char timeString[16];
    size_t stringLength = strftime(timeString, sizeof(timeString), "%X", localtime(&currentTime));

    return str_CreateLength(timeString, stringLength);
}

static string* getAmigaDateString() {
    time_t currentTime = time(NULL);
    struct tm* localTime = localtime(&currentTime);

    char dateString[16];
    size_t stringLength = sprintf(dateString, "%d.%d.%d", localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900);

    return str_CreateLength(dateString, stringLength);
}

static string*
callback__DATE(SSymbol* symbol) {
    str_Free(symbol->value.macro);
    symbol->value.macro = getDateString();

    return str_Copy(symbol->value.macro);
}

static string*
callback__TIME(SSymbol* symbol) {
    str_Free(symbol->value.macro);
    symbol->value.macro = getTimeString();

    return str_Copy(symbol->value.macro);
}

static string*
callback__AMIGADATE(SSymbol* symbol) {
    str_Free(symbol->value.macro);
    symbol->value.macro = getAmigaDateString();

    return str_Copy(symbol->value.macro);
}

static void
createEquCallback(const char* name, int32_t (*callback)(struct Symbol*)) {
    string* nameString = str_Create(name);
    SSymbol* symbol = sym_CreateEqu(nameString, 0);
    symbol->callback.integer = callback;
    str_Free(nameString);
}

static void
createEqusCallback(const char* name, string* (*callback)(struct Symbol*)) {
    string* nameString = str_Create(name);
    SSymbol* symbol = sym_CreateEqus(nameString, NULL);
    symbol->callback.string = callback;
    str_Free(nameString);
}


// Private functions

static uint32_t
hash(const string* name) {
    uint32_t result = 0;
    size_t length = str_Length(name);

    for (size_t i = 0; i < length; ++i) {
        result += str_CharAt(name, i);
        result += result << 10U;
        result ^= result >> 6U;
    }

    result += result << 3U;
    result ^= result >> 11U;
    result += result << 15U;

    return result & (SYMBOL_HASH_SIZE - 1);
}

static SSymbol*
getSymbol(const string* name, const SSymbol* scope) {
    for (SSymbol* symbol = sym_hashedSymbols[hash(name)]; symbol; symbol = list_GetNext(symbol)) {
        if (symbol->scope == scope && str_Equal(symbol->name, name))
            return symbol;
    }

    return NULL;
}

static SSymbol*
createSymbol(const string* name, SSymbol* scope) {
    SSymbol* newSymbol = (SSymbol*) mem_Alloc(sizeof(SSymbol));
    memset(newSymbol, 0, sizeof(SSymbol));

    SET_TYPE_AND_FLAGS(newSymbol, SYM_UNDEFINED);
    newSymbol->name = str_Copy(name);
    newSymbol->scope = scope;

    SSymbol** hashTableEntry = &sym_hashedSymbols[hash(name)];
    list_Insert(*hashTableEntry, newSymbol);

    return newSymbol;
}

static bool
isLocalName(const string *name) {
    return str_CharAt(name, 0) == '$' || str_CharAt(name, 0) == '.' || str_CharAt(name, -1) == '$';
}

static SSymbol*
assumedScopeOf(const string* name) {
    return isLocalName(name) ? g_currentScope : NULL;
}

static SSymbol*
findOrCreateSymbol(const string* name) {
    SSymbol* scope = assumedScopeOf(name);
    SSymbol* symbol = getSymbol(name, scope);

    if (symbol == NULL) {
        symbol = createSymbol(name, scope);
    }

    return symbol;
}

static bool
canBeType(const SSymbol* symbol, ESymbolType type) {
    return symbol->type == type || symbol->type == SYM_UNDEFINED;
}

static bool
isModifiable(const SSymbol* symbol) {
    return symbol->flags & SYMF_MODIFIABLE ? true : false;
}

/* ----------------------------------------------------------------------- */


int32_t
sym_GetValue(SSymbol* symbol) {
    assert (symbol->type != SYM_EQUS);

    if (symbol->callback.integer)
        return symbol->callback.integer(symbol);

    return symbol->value.integer;
}

int32_t
sym_GetValueByName(string* name) {
    SSymbol* symbol = findOrCreateSymbol(name);

    if (symbol->flags & SYMF_CONSTANT)
        return sym_GetValue(symbol);

    err_Error(ERROR_SYMBOL_CONSTANT);
    return 0;
}

SSymbol*
sym_GetSymbol(string* name) {
    return findOrCreateSymbol(name);
}

static SSymbol*
createSymbolOfType(string* name, ESymbolType type) {
    SSymbol* symbol = findOrCreateSymbol(name);

    if (isModifiable(symbol) && canBeType(symbol, type)) {
        SET_TYPE_AND_FLAGS(symbol, type);
        return symbol;
    }

    err_Error(ERROR_MODIFY_SYMBOL);
    return NULL;
}

SSymbol*
sym_CreateGroup(string* name, EGroupType value) {
    SSymbol* symbol = createSymbolOfType(name, SYM_GROUP);
    if (symbol != NULL) {
        symbol->value.groupType = value;
    }
    return symbol;
}

SSymbol*
sym_CreateEqus(string* name, string* value) {
    SSymbol* symbol = createSymbolOfType(name, SYM_EQUS);
    if (symbol != NULL) {
        symbol->value.macro = str_Copy(value);
    }
    return symbol;
}

SSymbol*
sym_CreateMacro(string* name, char* macroData, size_t macroSize) {
    SSymbol* symbol = createSymbolOfType(name, SYM_MACRO);
    if (symbol != NULL) {
        symbol->value.macro = str_CreateLength(macroData, macroSize);
    }
    return symbol;
}

SSymbol*
sym_CreateEqu(string* name, int32_t value) {
    SSymbol* symbol = findOrCreateSymbol(name);

    if (isModifiable(symbol) && (canBeType(symbol, SYM_EQU) || symbol->type == SYM_GLOBAL)) {
        if (symbol->type == SYM_GLOBAL)
            symbol->flags |= SYMF_EXPORT;

        SET_TYPE_AND_FLAGS(symbol, SYM_EQU);
        symbol->value.integer = value;

        return symbol;
    }

    err_Error(ERROR_MODIFY_SYMBOL);
    return NULL;
}

SSymbol*
sym_CreateSet(string* name, int32_t value) {
    SSymbol* symbol = createSymbolOfType(name, SYM_SET);
    if (symbol != NULL) {
        symbol->value.integer = value;
    }
    return symbol;
}

SSymbol*
sym_CreateLabel(string* name) {
    SSymbol* symbol = findOrCreateSymbol(name);

    if (isModifiable(symbol) && (canBeType(symbol, SYM_LABEL) || symbol->type == SYM_GLOBAL)) {
        if (symbol->type == SYM_GLOBAL)
            symbol->flags |= SYMF_EXPORT;

        if (sect_Current) {
            if (!isLocalName(name))
                g_currentScope = symbol;

            if ((sect_Current->flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)) == 0) {
                SET_TYPE_AND_FLAGS(symbol, SYM_LABEL);
                symbol->section = sect_Current;
                symbol->value.integer = sect_Current->cpuProgramCounter;
            } else {
                SET_TYPE_AND_FLAGS(symbol, SYM_EQU);
                symbol->value.integer = sect_Current->cpuProgramCounter + sect_Current->cpuAdjust
                                      + sect_Current->cpuOrigin;
            }
            return symbol;
        } else {
            err_Error(ERROR_LABEL_SECTION);
        }
    } else {
        err_Error(ERROR_MODIFY_SYMBOL);
    }

    return NULL;
}

string*
sym_GetSymbolValueAsStringByName(const string* name) {
    SSymbol* symbol = findOrCreateSymbol(name);

    switch (symbol->type) {
        case SYM_EQU:
        case SYM_SET: {
            char destination[16];
            sprintf(destination, "%d", sym_GetValue(symbol));
            return str_Create(destination);
        }
        case SYM_EQUS: {
            return sym_GetStringValue(symbol);
        }
        case SYM_LABEL:
        case SYM_MACRO:
        case SYM_IMPORT:
        case SYM_GROUP:
        case SYM_GLOBAL:
        case SYM_UNDEFINED:
        default: {
            return str_Create("[UNDEFINED]");
        }
    }
}

SSymbol*
sym_Export(string* name) {
    SSymbol* symbol = findOrCreateSymbol(name);

    if (symbol->flags & SYMF_EXPORTABLE) {
        symbol->flags |= SYMF_EXPORT;
    } else {
        err_Error(ERROR_SYMBOL_EXPORT);
    }

    return symbol;
}

SSymbol*
sym_Import(string* name) {
    SSymbol* symbol = findOrCreateSymbol(name);

    if (symbol->type == SYM_UNDEFINED) {
        SET_TYPE_AND_FLAGS(symbol, SYM_IMPORT);
        symbol->value.integer = 0;
        return symbol;
    }

    err_Error(ERROR_IMPORT_DEFINED);
    return NULL;
}

SSymbol*
sym_Global(string* name) {
    SSymbol* symbol = findOrCreateSymbol(name);

    if (symbol->type == SYM_UNDEFINED) {
        // symbol has not yet been defined, we'll leave this for later
        SET_TYPE_AND_FLAGS(symbol, SYM_GLOBAL);
        symbol->value.integer = 0;
        return symbol;
    }

    if (symbol->flags & SYMF_EXPORTABLE) {
        symbol->flags |= SYMF_EXPORT;
        return symbol;
    }

    err_Error(ERROR_SYMBOL_EXPORT);
    return NULL;
}

bool
sym_Purge(string* name) {
    SSymbol** hashTableEntry = &sym_hashedSymbols[hash(name)];
    SSymbol* symbol = getSymbol(name, assumedScopeOf(name));

    if (symbol != NULL) {
        list_Remove(*hashTableEntry, symbol);

        if (symbol->flags == SYMF_HAS_DATA) {
            str_Free(symbol->value.macro);
        }
        str_Free(symbol->name);
        mem_Free(symbol);
    }

    return true;
}

bool
sym_IsString(const string* name) {
    SSymbol* symbol = getSymbol(name, assumedScopeOf(name));

    return symbol != NULL && symbol->type == SYM_EQUS;
}

bool
sym_IsMacro(const string* name) {
    SSymbol* symbol = getSymbol(name, assumedScopeOf(name));

    return symbol != NULL && symbol->type == SYM_MACRO;
}

bool
sym_IsDefined(const string* pName) {
    SSymbol* pSym = getSymbol(pName, assumedScopeOf(pName));

    return pSym != NULL && pSym->type != SYM_UNDEFINED;
}

string*
sym_GetStringValue(SSymbol* symbol) {
    if (symbol->type == SYM_EQUS) {
        if (symbol->callback.string) {
            return symbol->callback.string(symbol);
        } else {
            return str_Copy(symbol->value.macro);
        }
    }

    err_Error(ERROR_SYMBOL_EQUS);
    return NULL;
}

string*
sym_GetStringValueByName(const string* name) {
    SSymbol* pSym = getSymbol(name, assumedScopeOf(name));

    if (pSym != NULL)
        return sym_GetStringValue(pSym);

    return NULL;
}

bool
sym_Init(void) {
    g_currentScope = NULL;

    xasm_Configuration->defineSymbols();

    createEquCallback("__NARG", callback__NARG);
    createEquCallback("__LINE", callback__LINE);
    createEqusCallback("__DATE", callback__DATE);
    createEqusCallback("__TIME", callback__TIME);

    if (xasm_Configuration->supportAmiga) {
        createEqusCallback("__AMIGADATE", callback__AMIGADATE);
    }

    string* asmotor = str_Create("__ASMOTOR");
    sym_CreateEqu(asmotor, 0);
    str_Free(asmotor);

    return true;
}
