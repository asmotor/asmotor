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

#ifndef XASM_COMMON_SYMBOL_H_INCLUDED_
#define XASM_COMMON_SYMBOL_H_INCLUDED_

#include <stdlib.h>

#include "lists.h"
#include "str.h"

#define SYMBOL_HASH_SIZE 1024U

struct FileInfo;

typedef enum {
    SYM_LABEL = 0,
    SYM_EQU,
    SYM_SET,
    SYM_EQUS,
    SYM_EQUF,
    SYM_MACRO,
    SYM_IMPORT,
    SYM_GROUP,
    SYM_GLOBAL,
    SYM_UNDEFINED
} ESymbolType;

typedef enum {
    GROUP_TEXT = 0,
    GROUP_BSS = 1
} EGroupType;

#define SYMF_CONSTANT       0x001u      // symbol has a constant value (the Value field is safe to use)
#define SYMF_RELOC          0x002u      // symbol will change its value during linking
#define SYMF_EXPORT         0x008u      // symbol should be exported
#define SYMF_EXPORTABLE     0x010u      // symbol can be exported
#define SYMF_EXPRESSION     0x020u      // symbol can be used in expressions
#define SYMF_MODIFIABLE     0x040u      // symbol can be redefined
#define SYMF_HAS_DATA       0x080u      // symbol has data attached (Macro.pData)
#define SYMF_FILE_EXPORT    0x100u      // symbol should be exported to sections local to this file
#define SYMF_DATA           0x40000000u
#define SYMF_CHIP           0x20000000u
#define SYMF_USED           0x10000000u

typedef struct Symbol {
    list_Data(struct Symbol);

    string* name;
    ESymbolType type;
    uint32_t flags;

    struct FileInfo* fileInfo;
    uint32_t lineNumber;
    
    struct Symbol* scope;
    struct Section* section;

    union {
        int32_t (* integer)(struct Symbol*);
        string* (* string)(struct Symbol*);
    } callback;

    union {
        int32_t integer;
        long double floating;
        EGroupType groupType;
        string* macro;
    } value;

    uint32_t id;    // used by object output routines
} SSymbol;

extern bool
sym_Init(void);

extern SSymbol*
sym_CreateLabel(string* name);

extern SSymbol*
sym_CreateEqus(string* name, string* value);

extern SSymbol*
sym_CreateEqu(string* name, int32_t value);

extern SSymbol*
sym_CreateSet(string* name, int32_t value);

extern SSymbol*
sym_CreateGroup(string* name, EGroupType value);

extern SSymbol*
sym_CreateMacro(string* name, char* macroData, size_t macroSize, uint32_t lineNumber);

extern SSymbol*
sym_GetSymbol(string* name);

extern SSymbol*
sym_GetSymbolInScope(SSymbol* scope, string* name);

extern bool
sym_Purge(string* name);

extern SSymbol*
sym_Export(string* name);

extern SSymbol*
sym_Import(string* name);

extern SSymbol*
sym_Global(string* name);

extern string*
sym_GetSymbolValueAsStringByName(const string* name);

extern string*
sym_GetStringSymbolValue(SSymbol* symbol);

extern string*
sym_GetStringSymbolValueByName(const string* name);

extern int32_t
sym_GetValue(SSymbol* symbol);

extern int32_t
sym_GetValueByName(string* name);

extern bool
sym_IsDefined(const string* pName);

extern bool
sym_IsString(const string* name);

extern bool
sym_IsMacro(const string* name);

extern void
sym_ErrorOnUndefined(void);

INLINE bool
sym_IsNotDefined(const string* symbolName) {
    return !sym_IsDefined((symbolName));
}

extern SSymbol* sym_hashedSymbols[SYMBOL_HASH_SIZE];

#endif // XASM_COMMON_SYMBOL_H_INCLUDED_
