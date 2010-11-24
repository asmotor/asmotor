/*  Copyright 2008 Carsten Sørensen

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

#include "asmotor.h"
#include "mem.h"
#include "xasm.h"
#include "symbol.h"
#include "fstack.h"
#include "project.h"
#include "section.h"


/* ----------------------------------------------------------------------- */


extern void locsym_Init(void);


/* ----------------------------------------------------------------------- */


#define	SetFlags(flags,type)	(flags)=((flags)&(SYMF_EXPORT|SYMF_REFERENCED))|g_aDefaultSymbolFlags[type]


/* ----------------------------------------------------------------------- */


static uint32_t g_aDefaultSymbolFlags[] =
{
	SYMF_RELOC | SYMF_EXPORTABLE | SYMF_EXPR,		/*	SYM_LABEL		*/
	SYMF_CONSTANT | SYMF_EXPORTABLE | SYMF_EXPR,	/*	SYM_EQU			*/
	SYMF_CONSTANT | SYMF_EXPR | SYMF_MODIFY,		/*	SYM_SET			*/
	SYMF_HASDATA,									/*	SYM_EQUS		*/
	SYMF_HASDATA,									/*	SYM_MACRO		*/
	SYMF_EXPR | SYMF_RELOC,							/*	SYM_IMPORT		*/
	SYMF_EXPORT,									/*	SYM_GROUP		*/
	SYMF_EXPR | SYMF_MODIFY | SYMF_RELOC,			/*	SYM_GLOBAL		*/
	SYMF_MODIFY | SYMF_EXPR | SYMF_EXPORTABLE		/*	SYM_UNDEFINED	*/
};

SSymbol* g_pHashedSymbols[HASHSIZE];

SSymbol* pCurrentScope;


/* ----------------------------------------------------------------------- */


static int32_t Callback__NARG(SSymbol* pSym)
{
	return fstk_GetMacroArgCount();
}


static int32_t Callback__LINE(SSymbol* pSym)
{
	SFileStack* p = g_pFileContext;
	while(list_GetNext(p))
	{
		p = list_GetNext(p);
	}

	return p->LineNumber;
}


static string* Callback__DATE(SSymbol* pSym)
{
	char s[16];
	time_t t = time(NULL);
	size_t len;

	len = strftime(s, sizeof(s), "%Y-%m-%d", localtime(&t));

	pSym->Value.Macro.pData = mem_Realloc(pSym->Value.Macro.pData, len + 1);
	pSym->Value.Macro.Size = len;
	strcpy(pSym->Value.Macro.pData, s);

	return str_Create(pSym->Value.Macro.pData);
}


static string* Callback__TIME(SSymbol* pSym)
{
	char s[16];
	time_t t = time(NULL);
	size_t len;

	len = strftime(s, sizeof(s), "%X", localtime(&t));

	pSym->Value.Macro.pData = mem_Realloc(pSym->Value.Macro.pData, len + 1);
	pSym->Value.Macro.Size = len;
	strcpy(pSym->Value.Macro.pData, s);

	return str_Create(pSym->Value.Macro.pData);
}


static string* Callback__AMIGADATE(SSymbol* pSym)
{
	char s[16];
	time_t t = time(NULL);
	struct tm* tm = localtime(&t);
	size_t len;

	len = sprintf(s, "%d.%d.%d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);

	pSym->Value.Macro.pData = mem_Realloc(pSym->Value.Macro.pData, len + 1);
	pSym->Value.Macro.Size = len;
	strcpy(pSym->Value.Macro.pData, s);

	return str_Create(pSym->Value.Macro.pData);
}


/* ----------------------------------------------------------------------- */


static uint32_t sym_CalcHash(string* pName)
{
	uint32_t hash = 0;
	int i;
	int len = str_Length(pName);

	for(i = 0; i < len; ++i)
	{
		hash += str_CharAt(pName, i);
		hash += hash << 10;
		hash ^= hash >> 6;
	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return hash & (HASHSIZE - 1);
}


static SSymbol* sym_Find(string* pName, SSymbol* pScope)
{
    SSymbol* pSym = g_pHashedSymbols[sym_CalcHash(pName)];

    while(pSym)
    {
		if(strcmp(str_String(pSym->pName), str_String(pName)) == 0 && pSym->pScope == pScope)
			return pSym;

		pSym = list_GetNext(pSym);
    }

	return NULL;
}


static SSymbol* sym_Create(string* pName)
{
    SSymbol** ppHash = &g_pHashedSymbols[sym_CalcHash(pName)];
	SSymbol* pSym = (SSymbol*)mem_Alloc(sizeof(SSymbol));
	
	memset(pSym, 0, sizeof(SSymbol));
	
	pSym->Type = SYM_UNDEFINED;
	pSym->Flags = g_aDefaultSymbolFlags[SYM_UNDEFINED];
	pSym->pName = str_Copy(pName);

	list_Insert(*ppHash, pSym);
	return pSym;
}


static SSymbol* sym_GetScope(string* pName)
{
	if(str_CharAt(pName, 0) == '.' || str_CharAt(pName, -1) == '$')
		return pCurrentScope;

	return NULL;
}


static SSymbol* sym_FindOrCreate(string* pName)
{
	SSymbol* pScope = sym_GetScope(pName);
	SSymbol* pSym = sym_Find(pName, pScope);

	if(pSym == NULL)
	{
		pSym = sym_Create(pName);
		pSym->pScope = pScope;
		return pSym;
	}

	return pSym;
}


static bool_t sym_isType(SSymbol* sym, ESymbolType type)
{
	return sym->Type == type || sym->Type == SYM_UNDEFINED;
}


/* ----------------------------------------------------------------------- */


int32_t sym_GetValue(SSymbol* pSym)
{
	if(pSym->Callback.Integer)
		return pSym->Callback.Integer(pSym);

	return pSym->Value.Value;
}


int32_t sym_GetValueByName(string* pName)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if(pSym->Flags & SYMF_CONSTANT)
		return sym_GetValue(pSym);

	prj_Fail(ERROR_SYMBOL_CONSTANT);
	return 0;
}


SSymbol* sym_FindSymbol(string* pName)
{
	return sym_FindOrCreate(pName);
}


SSymbol* sym_CreateGROUP(string* pName, EGroupType value)
{
	SSymbol* sym = sym_FindOrCreate(pName);

	if((sym->Flags & SYMF_MODIFY) && sym_isType(sym,SYM_GROUP))
	{
		sym->Type = SYM_GROUP;
		SetFlags(sym->Flags, SYM_GROUP);
		sym->Value.GroupType = value;
		return sym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}


SSymbol* sym_CreateEQUS(string* pName, char* value)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if((pSym->Flags & SYMF_MODIFY) && sym_isType(pSym, SYM_EQUS))
	{
		pSym->Type = SYM_EQUS;
		SetFlags(pSym->Flags, SYM_EQUS);

		if(value == NULL)
		{
			pSym->Value.Macro.Size = 0;
			pSym->Value.Macro.pData = NULL;
			return pSym;
		}

		pSym->Value.Macro.Size = strlen(value);
		pSym->Value.Macro.pData = mem_Alloc(strlen(value) + 1);
		strcpy(pSym->Value.Macro.pData, value);

		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}


SSymbol* sym_CreateMACRO(string* pName, char* value, uint32_t size)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if((pSym->Flags & SYMF_MODIFY) && sym_isType(pSym, SYM_MACRO))
	{
		pSym->Type = SYM_MACRO;
		SetFlags(pSym->Flags, SYM_MACRO);
		pSym->Value.Macro.Size = size;
		
		pSym->Value.Macro.pData = mem_Alloc(size);
		memcpy(pSym->Value.Macro.pData, value, size);
		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}


SSymbol* sym_CreateEQU(string* pName, int32_t value)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if((pSym->Flags & SYMF_MODIFY)
	&& (sym_isType(pSym, SYM_EQU) || pSym->Type == SYM_GLOBAL))
	{
		if(pSym->Type == SYM_GLOBAL)
			pSym->Flags |= SYMF_EXPORT;

		pSym->Type = SYM_EQU;
		SetFlags(pSym->Flags, SYM_EQU);
		pSym->Value.Value = value;
		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}


SSymbol* sym_CreateSET(string* pName, int32_t value)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if((pSym->Flags & SYMF_MODIFY) && sym_isType(pSym, SYM_SET))
	{
		pSym->Type = SYM_SET;
		SetFlags(pSym->Flags, SYM_SET);
		pSym->Value.Value = value;
		return pSym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}


SSymbol* sym_CreateLabel(string* pName)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if((pSym->Flags & SYMF_MODIFY)
	&& (sym_isType(pSym, SYM_LABEL) || pSym->Type == SYM_GLOBAL))
	{
		if(pSym->Type == SYM_GLOBAL)
			pSym->Flags |= SYMF_EXPORT;

		if(pCurrentSection)
		{
			if(str_CharAt(pName, 0) != '.' && str_CharAt(pName, -1) != '$')
				pCurrentScope = pSym;

			if((pCurrentSection->Flags & SECTF_ORGFIXED) == 0)
			{
				pSym->Type = SYM_LABEL;
				SetFlags(pSym->Flags, SYM_LABEL);
				pSym->pSection = pCurrentSection;
				pSym->Value.Value = pCurrentSection->PC;
				return pSym;
			}
			else
			{
				pSym->Type = SYM_EQU;
				SetFlags(pSym->Flags, SYM_EQU);
				pSym->Value.Value = pCurrentSection->PC + pCurrentSection->Org;
				return pSym;
			}
		}
		else
			prj_Error(ERROR_LABEL_SECTION);
	}
	else
		prj_Error(ERROR_MODIFY_SYMBOL);

	return NULL;
}


char* sym_GetValueAsStringByName(char* dst, string* pName)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	switch(pSym->Type)
	{
		case SYM_EQU:
		case SYM_SET:
		{
			sprintf(dst, "$%X", sym_GetValue(pSym));
			return dst + strlen(dst);
			break;
		}
		case SYM_EQUS:
		{
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
		default:
		{
			strcpy(dst, "*UNDEFINED*");
			return dst + strlen(dst);
			break;
		}
	}
}


SSymbol* sym_Export(string* pName)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if(pSym->Flags & SYMF_EXPORTABLE)
		pSym->Flags |= SYMF_EXPORT;
	else
		prj_Error(ERROR_SYMBOL_EXPORT);

	return pSym;
}


SSymbol* sym_Import(string* pName)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if(pSym->Type == SYM_UNDEFINED)
	{
		pSym->Type = SYM_IMPORT;
		SetFlags(pSym->Flags, SYM_IMPORT);
		pSym->Value.Value = 0;
		return pSym;
	}

	prj_Error(ERROR_IMPORT_DEFINED);
	return NULL;
}


SSymbol* sym_Global(string* pName)
{
	SSymbol* pSym = sym_FindOrCreate(pName);

	if(pSym->Type == SYM_UNDEFINED)
	{
		/* Symbol has not yet been defined, we'll leave this till later */
		pSym->Type = SYM_GLOBAL;
		SetFlags(pSym->Flags, SYM_GLOBAL);
		pSym->Value.Value = 0;
		return pSym;
	}

	if(pSym->Flags & SYMF_EXPORTABLE)
	{
		pSym->Flags |= SYMF_EXPORT;
		return pSym;
	}

	prj_Error(ERROR_SYMBOL_EXPORT);
	return NULL;
}


bool_t sym_Purge(string* pName)
{
    SSymbol** ppSym = &g_pHashedSymbols[sym_CalcHash(pName)];
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	if(pSym != NULL)
	{
		list_Remove(*ppSym, pSym);
		if(pSym->Flags == SYMF_HASDATA)
		{
			mem_Free(pSym->Value.Macro.pData);
		}
		str_Free(pSym->pName);
		mem_Free(pSym);
		return true;
	}

	prj_Warn(WARN_CANNOT_PURGE);
	return false;
}


bool_t sym_IsString(string* pName)
{
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	return pSym != NULL && pSym->Type == SYM_EQUS;
}


bool_t sym_IsMacro(string* pName)
{
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	return pSym != NULL && pSym->Type == SYM_MACRO;
}


bool_t sym_IsDefined(string* pName)
{
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	return pSym != NULL && pSym->Type != SYM_UNDEFINED;
}


string* sym_GetStringValue(SSymbol* pSym)
{
	if(pSym->Type == SYM_EQUS)
	{
		if(pSym->Callback.String)
			return pSym->Callback.String(pSym);
		return str_Create(pSym->Value.Macro.pData);
	}

	prj_Fail(ERROR_SYMBOL_EQUS);
	return NULL;
}


string* sym_GetStringValueByName(string* pName)
{
	SSymbol* pSym = sym_Find(pName, sym_GetScope(pName));

	if(pSym != NULL)
		return sym_GetStringValue(pSym);

	return NULL;
}


bool_t sym_Init(void)
{
	string* pName;
	SSymbol* pSym;
	
	pCurrentScope = NULL;

	locsym_Init();
	
	pName = str_Create("__NARG");
	pSym = sym_CreateEQU(pName, 0);
	pSym->Callback.Integer = Callback__NARG;
	str_Free(pName);
	
	pName = str_Create("__LINE");
	pSym = sym_CreateEQU(pName, 0);
	pSym->Callback.Integer = Callback__LINE;
	str_Free(pName);

	pName = str_Create("__DATE");
	pSym = sym_CreateEQUS(pName, 0);
	pSym->Callback.String = Callback__DATE;
	str_Free(pName);
	
	pName = str_Create("__TIME");
	pSym = sym_CreateEQUS(pName, 0);
	pSym->Callback.String = Callback__TIME;
	str_Free(pName);

	if(g_pConfiguration->bSupportAmiga)
	{
		pName = str_Create("__AMIGADATE");
		pSym = sym_CreateEQUS(pName, 0);
		pSym->Callback.String = Callback__AMIGADATE;
		str_Free(pName);
	}

	pName = str_Create("__ASMOTOR");
	sym_CreateEQU(pName, 0);
	str_Free(pName);

	return true;
}
