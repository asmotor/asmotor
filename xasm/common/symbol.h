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

#ifndef	INCLUDE_SYMBOL_H
#define	INCLUDE_SYMBOL_H

#include <stdlib.h>

#include "xasm.h"
#include "lists.h"
#include "str.h"

#define	HASHSIZE 1024U

typedef	enum
{
	SYM_LABEL = 0,
	SYM_EQU,
	SYM_SET,
	SYM_EQUS,
	SYM_MACRO,
	SYM_IMPORT,
	SYM_GROUP,
	SYM_GLOBAL,
	SYM_UNDEFINED
}	ESymbolType;

typedef	enum
{
	GROUP_TEXT = 0,
	GROUP_BSS = 1
}	EGroupType;

#define	SYMF_CONSTANT		0x001u		/*	symbol has a constant value (the Value field is safe to use)	*/
#define	SYMF_RELOC			0x002u		/*	symbol will change its value during linking						*/
#define	SYMF_REFERENCED		0x004u		/*	symbol has been referenced										*/
#define	SYMF_EXPORT			0x008u		/*	symbol should be exported										*/
#define	SYMF_EXPORTABLE		0x010u		/*	symbol can be exported											*/
#define	SYMF_LOCAL			0x020u		/*	symbol is a local symbol (the pScope field will be non-NULL)	*/
#define	SYMF_EXPR			0x040u		/*	symbol can be used in expressions								*/
#define	SYMF_MODIFY			0x080u		/*	symbol can be redefined											*/
#define	SYMF_HASDATA		0x100u		/*	symbol has data attached (Macro.pData)							*/
#define	SYMF_LOCALEXPORT	0x200u		/*	symbol should be exported to sections local to this file        */
#define SYMF_DATA 			0x40000000u
#define SYMF_CHIP 			0x20000000u

typedef struct Symbol
{
	list_Data(struct Symbol);
	string*		pName;
	ESymbolType	eType;
	uint32_t	nFlags;
	struct Symbol*	pScope;
	struct Section*	pSection;
	union
	{
		int32_t (*fpInteger)(struct Symbol*);
		string*	(*fpString)(struct Symbol*);
	} Callback;
	union
	{
		int32_t		Value;
		EGroupType	GroupType;
		string*		pMacro;
	} Value;

	uint32_t ID;	/*	Used by object output routines */
} SSymbol;


extern bool	sym_Init(void);

extern SSymbol* sym_CreateLabel(string* pName);
extern SSymbol* sym_CreateEQUS(string* pName, string* pValue);
extern SSymbol* sym_CreateEQU(string* pName, int32_t value);
extern SSymbol* sym_CreateSET(string* pName, int32_t value);
extern SSymbol* sym_CreateGROUP(string* pName, EGroupType value);
extern SSymbol* sym_CreateMACRO(string* pName, char* value, size_t size);

extern SSymbol* sym_FindSymbol(string* name);

extern bool	sym_Purge(string* pName);

extern SSymbol*	sym_Export(string* pName);
extern SSymbol* sym_Import(string* pName);
extern SSymbol* sym_Global(string* pName);

extern char*	sym_GetValueAsStringByName(char* pDest, string* pSym);
extern string*	sym_GetStringValue(SSymbol* pSym);
extern string*	sym_GetStringValueByName(string* pName);
extern int32_t	sym_GetValue(SSymbol* pSym);
extern int32_t	sym_GetValueByName(string* pName);

extern bool	sym_IsDefined(const string* pName);
extern bool	sym_IsString(string* pName);
extern bool	sym_IsMacro(string* pName);

INLINE bool
sym_IsNotDefined(const string* symbolName) {
	return !sym_IsDefined((symbolName));
}

extern SSymbol* g_pHashedSymbols[HASHSIZE];

#endif	/*INCLUDE_SYMBOL_H*/
