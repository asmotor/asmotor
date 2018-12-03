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
#include "project.h"
#include "section.h"

/* ----------------------------------------------------------------------- */


extern void locsym_Init(void);


/* ----------------------------------------------------------------------- */


#define SET_FLAGS(flags, type) ((flags)=((flags)&(SYMF_EXPORT|SYMF_REFERENCED))|s_aDefaultSymbolFlags[type])

/* ----------------------------------------------------------------------- */


static uint32_t s_aDefaultSymbolFlags[] = {SYMF_RELOC | SYMF_EXPORTABLE | SYMF_EXPR,        /*	SYM_LABEL		*/
										   SYMF_CONSTANT | SYMF_EXPORTABLE | SYMF_EXPR,    /*	SYM_EQU			*/
										   SYMF_CONSTANT | SYMF_EXPR | SYMF_MODIFY,        /*	SYM_SET			*/
										   SYMF_HASDATA,                                    /*	SYM_EQUS		*/
										   SYMF_HASDATA,                                    /*	SYM_MACRO		*/
										   SYMF_EXPR | SYMF_RELOC,                            /*	SYM_IMPORT		*/
										   SYMF_EXPORT,                                    /*	SYM_GROUP		*/
										   SYMF_EXPR | SYMF_MODIFY | SYMF_RELOC,            /*	SYM_GLOBAL		*/
										   SYMF_MODIFY | SYMF_EXPR | SYMF_EXPORTABLE        /*	SYM_UNDEFINED	*/
};

static SSymbol* s_pCurrentScope;

SSymbol* g_pHashedSymbols[HASHSIZE];

/* ----------------------------------------------------------------------- */


static int32_t Callback__NARG(SSymbol* pSym) {
	assert(pSym != NULL);

	return fstk_GetMacroArgCount();
}

static int32_t Callback__LINE(SSymbol* pSym) {
	assert(pSym != NULL);

	SFileStack* p = g_pFileContext;
	while (list_GetNext(p)) {
		p = list_GetNext(p);
	}

	return p->LineNumber;
}

static string* Callback__DATE(SSymbol* pSym) {
	char s[16];
	time_t t = time(NULL);
	size_t len;

	len = strftime(s, sizeof(s), "%Y-%m-%d", localtime(&t));

	str_Free(pSym->Value.pMacro);
	pSym->Value.pMacro = str_CreateLength(s, len);

	return str_Copy(pSym->Value.pMacro);
}

static string* Callback__TIME(SSymbol* pSym) {
	char s[16];
	time_t t = time(NULL);
	size_t len;

	len = strftime(s, sizeof(s), "%X", localtime(&t));

	str_Free(pSym->Value.pMacro);
	pSym->Value.pMacro = str_CreateLength(s, len);

	return str_Copy(pSym->Value.pMacro);
}

static string* Callback__AMIGADATE(SSymbol* pSym) {
	char s[16];
	time_t t = time(NULL);
	struct tm* tm = localtime(&t);
	size_t len;

	len = sprintf(s, "%d.%d.%d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);

	str_Free(pSym->Value.pMacro);
	pSym->Value.pMacro = str_CreateLength(s, len);

	return str_Copy(pSym->Value.pMacro);
}

/* ----------------------------------------------------------------------- */


static uint32_t sym_CalcHash(string* pName) {
	uint32_t hash = 0;
	size_t len = str_Length(pName);

	for (size_t i = 0; i < len; ++i) {
		hash += str_CharAt(pName, i);
		hash += hash << 10U;
		hash ^= hash >> 6U;
	}

	hash += hash << 3U;
	hash ^= hash >> 11U;
	hash += hash << 15U;

	return hash & (HASHSIZE - 1);
}

static SSymbol* sym_Find(string* pName, SSymbol* pScope) {
	SSymbol* pSym;

	for (pSym = g_pHashedSymbols[sym_CalcHash(pName)]; pSym; pSym = list_GetNext(pSym)) {
		if (str_Equal(pSym->pName, pName) && pSym->pScope == pScope)
			return pSym;
	}

	return NULL;
}

static SSymbol* sym_Create(string* pName) {
	SSymbol** ppHash = &g_pHashedSymbols[sym_CalcHash(pName)];
	SSymbol* pSym = (SSymbol*) mem_Alloc(sizeof(SSymbol));

	memset(pSym, 0, sizeof(SSymbol));

	pSym->eType = SYM_UNDEFINED;
	pSym->nFlags = s_aDefaultSymbolFlags[SYM_UNDEFINED];
	pSym->pName = str_Copy(pName);

	list_Insert(*ppHash, pSym);
	return pSym;
}

static SSymbol* sym_GetScope(string* pName) {
	if (str_CharAt(pName, 0) == '.' || str_CharAt(pName, -1) == '$')
		return s_pCurrentScope;

	return NULL;
}

static SSymbol* sym_FindOrCreate(string* pName) {
	SSymbol* pScope = sym_GetScope(pName);
	SSymbol* pSym = sym_Find(pName, pScope);

	if (pSym == NULL) {
		pSym = sym_Create(pName);
		pSym->pScope = pScope;
		return pSym;
	}

	return pSym;
}

static bool_t sym_isType(SSymbol* sym, ESymbolType type) {
	return sym->eType == type || sym->eType == SYM_UNDEFINED;
}

/* ----------------------------------------------------------------------- */


int32_t sym_GetValue(SSymbol* pSym) {
	if (pSym->Callback.fpInteger)
		return pSym->Callback.fpInteger(pSym);

	return pSym->Value.Value;
}

int32_t sym_GetValueByName(string* pName) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if (pSym->nFlags & SYMF_CONSTANT)
		return sym_GetValue(pSym);

	prj_Fail(ERROR_SYMBOL_CONSTANT);
	return 0;
}

SSymbol* sym_FindSymbol(string* pName) {
	return sym_FindOrCreate(pName);
}

SSymbol* sym_CreateGROUP(string* pName, EGroupType value) {
	SSymbol* sym = sym_FindOrCreate(pName);

	if ((sym->nFlags & SYMF_MODIFY) && sym_isType(sym, SYM_GROUP)) {
		sym->eType = SYM_GROUP;
		SET_FLAGS(sym->nFlags, SYM_GROUP);
		sym->Value.GroupType = value;
		return sym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_CreateEQUS(string* pName, string* pValue) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if ((pSym->nFlags & SYMF_MODIFY) && sym_isType(pSym, SYM_EQUS)) {
		pSym->eType = SYM_EQUS;
		SET_FLAGS(pSym->nFlags, SYM_EQUS);

		pSym->Value.pMacro = str_Copy(pValue);
		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_CreateMACRO(string* pName, char* value, size_t size) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if ((pSym->nFlags & SYMF_MODIFY) && sym_isType(pSym, SYM_MACRO)) {
		pSym->eType = SYM_MACRO;
		SET_FLAGS(pSym->nFlags, SYM_MACRO);
		pSym->Value.pMacro = str_CreateLength(value, size);

		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_CreateEQU(string* pName, int32_t value) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if ((pSym->nFlags & SYMF_MODIFY) && (sym_isType(pSym, SYM_EQU) || pSym->eType == SYM_GLOBAL)) {
		if (pSym->eType == SYM_GLOBAL)
			pSym->nFlags |= SYMF_EXPORT;

		pSym->eType = SYM_EQU;
		SET_FLAGS(pSym->nFlags, SYM_EQU);
		pSym->Value.Value = value;

		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_CreateSET(string* pName, int32_t value) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if ((pSym->nFlags & SYMF_MODIFY) && sym_isType(pSym, SYM_SET)) {
		pSym->eType = SYM_SET;
		SET_FLAGS(pSym->nFlags, SYM_SET);
		pSym->Value.Value = value;

		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_CreateLabel(string* pName) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if ((pSym->nFlags & SYMF_MODIFY) && (sym_isType(pSym, SYM_LABEL) || pSym->eType == SYM_GLOBAL)) {
		if (pSym->eType == SYM_GLOBAL)
			pSym->nFlags |= SYMF_EXPORT;

		if (g_pCurrentSection) {
			if (str_CharAt(pName, 0) != '$' && str_CharAt(pName, 0) != '.' && str_CharAt(pName, -1) != '$')
				s_pCurrentScope = pSym;

			if ((g_pCurrentSection->Flags & (SECTF_LOADFIXED | SECTF_ORGFIXED)) == 0) {
				pSym->eType = SYM_LABEL;
				SET_FLAGS(pSym->nFlags, SYM_LABEL);
				pSym->pSection = g_pCurrentSection;
				pSym->Value.Value = g_pCurrentSection->PC;
				return pSym;
			} else {
				pSym->eType = SYM_EQU;
				SET_FLAGS(pSym->nFlags, SYM_EQU);
				pSym->Value.Value = g_pCurrentSection->PC + g_pCurrentSection->OrgOffset + g_pCurrentSection->BasePC;
				return pSym;
			}
		} else
			prj_Error(ERROR_LABEL_SECTION);
	} else
		prj_Error(ERROR_MODIFY_SYMBOL);

	return NULL;
}

char* sym_GetValueAsStringByName(char* dst, string* pName) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	switch (pSym->eType) {
		case SYM_EQU:
		case SYM_SET: {
			sprintf(dst, "$%X", sym_GetValue(pSym));
			return dst + strlen(dst);
			break;
		}
		case SYM_EQUS: {
			string* pValue = sym_GetStringValue(pSym);
			int len = str_Length(pValue);

			strcpy(dst, str_String(pValue));
			str_Free(pValue);

			return dst + len;
		}
		case SYM_LABEL:
		case SYM_MACRO:
		case SYM_IMPORT:
		case SYM_GROUP:
		case SYM_GLOBAL:
		case SYM_UNDEFINED:
		default: {
			strcpy(dst, "*UNDEFINED*");
			return dst + strlen(dst);
			break;
		}
	}
}

SSymbol* sym_Export(string* pName) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if (pSym->nFlags & SYMF_EXPORTABLE)
		pSym->nFlags |= SYMF_EXPORT;
	else
		prj_Error(ERROR_SYMBOL_EXPORT);

	return pSym;
}

SSymbol* sym_Import(string* pName) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if (pSym->eType == SYM_UNDEFINED) {
		pSym->eType = SYM_IMPORT;
		SET_FLAGS(pSym->nFlags, SYM_IMPORT);
		pSym->Value.Value = 0;
		return pSym;
	}

	prj_Error(ERROR_IMPORT_DEFINED);
	return NULL;
}

SSymbol* sym_Global(string* pName) {
	SSymbol* pSym = sym_FindOrCreate(pName);

	if (pSym->eType == SYM_UNDEFINED) {
		/* Symbol has not yet been defined, we'll leave this till later */
		pSym->eType = SYM_GLOBAL;
		SET_FLAGS(pSym->nFlags, SYM_GLOBAL);
		pSym->Value.Value = 0;
		return pSym;
	}

	if (pSym->nFlags & SYMF_EXPORTABLE) {
		pSym->nFlags |= SYMF_EXPORT;
		return pSym;
	}

	prj_Error(ERROR_SYMBOL_EXPORT);
	return NULL;
}

bool_t sym_Purge(string* pName) {
	SSymbol** ppSym = &g_pHashedSymbols[sym_CalcHash(pName)];
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	if (pSym != NULL) {
		list_Remove(*ppSym, pSym);

		if (pSym->nFlags == SYMF_HASDATA)
			str_Free(pSym->Value.pMacro);
		str_Free(pSym->pName);
		mem_Free(pSym);

		return true;
	}

	prj_Warn(WARN_CANNOT_PURGE);
	return false;
}

bool_t sym_IsString(string* pName) {
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	return pSym != NULL && pSym->eType == SYM_EQUS;
}

bool_t sym_IsMacro(string* pName) {
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	return pSym != NULL && pSym->eType == SYM_MACRO;
}

bool_t sym_IsDefined(string* pName) {
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	return pSym != NULL && pSym->eType != SYM_UNDEFINED;
}

string* sym_GetStringValue(SSymbol* pSym) {
	if (pSym->eType == SYM_EQUS) {
		if (pSym->Callback.fpString)
			return pSym->Callback.fpString(pSym);

		return str_Copy(pSym->Value.pMacro);
	}

	prj_Fail(ERROR_SYMBOL_EQUS);
	return NULL;
}

string* sym_GetStringValueByName(string* pName) {
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	if (pSym != NULL)
		return sym_GetStringValue(pSym);

	return NULL;
}

bool_t sym_Init(void) {
	string* pName;
	SSymbol* pSym;

	s_pCurrentScope = NULL;

	locsym_Init();

	pName = str_Create("__NARG");
	pSym = sym_CreateEQU(pName, 0);
	pSym->Callback.fpInteger = Callback__NARG;
	str_Free(pName);

	pName = str_Create("__LINE");
	pSym = sym_CreateEQU(pName, 0);
	pSym->Callback.fpInteger = Callback__LINE;
	str_Free(pName);

	pName = str_Create("__DATE");
	pSym = sym_CreateEQUS(pName, 0);
	pSym->Callback.fpString = Callback__DATE;
	str_Free(pName);

	pName = str_Create("__TIME");
	pSym = sym_CreateEQUS(pName, 0);
	pSym->Callback.fpString = Callback__TIME;
	str_Free(pName);

	if (g_pConfiguration->bSupportAmiga) {
		pName = str_Create("__AMIGADATE");
		pSym = sym_CreateEQUS(pName, 0);
		pSym->Callback.fpString = Callback__AMIGADATE;
		str_Free(pName);
	}

	pName = str_Create("__ASMOTOR");
	sym_CreateEQU(pName, 0);
	str_Free(pName);

	return true;
}
