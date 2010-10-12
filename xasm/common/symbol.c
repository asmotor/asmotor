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
#include "xasm.h"
#include "symbol.h"
#include "fstack.h"
#include "project.h"
#include "section.h"

extern void locsym_Init(void);



/*	Private defines */

#define	SetFlags(flags,type)	(flags)=((flags)&(SYMF_EXPORT|SYMF_REFERENCED))|g_aDefaultSymbolFlags[type]




/*	Private data */

static ULONG g_aDefaultSymbolFlags[] =
{
	SYMF_RELOC|SYMF_EXPORTABLE|SYMF_EXPR,		/*	SYM_LABEL		*/
	SYMF_CONSTANT|SYMF_EXPORTABLE|SYMF_EXPR,	/*	SYM_EQU			*/
	SYMF_CONSTANT|SYMF_EXPR|SYMF_MODIFY,		/*	SYM_SET			*/
	SYMF_HASDATA,								/*	SYM_EQUS		*/
	SYMF_HASDATA,								/*	SYM_MACRO		*/
	SYMF_EXPR|SYMF_RELOC,						/*	SYM_IMPORT		*/
	SYMF_EXPORT,								/*	SYM_GROUP		*/
	SYMF_EXPR|SYMF_MODIFY|SYMF_RELOC,			/*	SYM_GLOBAL		*/
	SYMF_MODIFY|SYMF_EXPR|SYMF_EXPORTABLE		/*	SYM_UNDEFINED	*/
};

SSymbol* g_pHashedSymbols[HASHSIZE];
SSymbol* pCurrentScope;
SSymbol* p__NARG__Symbol;
SSymbol* p__LINE__Symbol;
SSymbol* p__DATE__Symbol;
SSymbol* p__TIME__Symbol;
SSymbol* p__AMIGADATE__Symbol;




/*	Private routines */

static char* sym_GetStringValueBySymbol(SSymbol* sym);

static SLONG __NARG__Callback(SSymbol* sym)
{
	return fstk_GetMacroArgCount();
}

static SLONG __LINE__Callback(SSymbol* sym)
{
	SFileStack* p = g_pFileContext;
	while(list_GetNext(p))
	{
		p = list_GetNext(p);
	}

	return p->LineNumber;
}

static char* __DATE__Callback(SSymbol* sym)
{
	char s[16];
	time_t t = time(NULL);
	size_t len;

	len = strftime(s, sizeof(s), "%Y-%m-%d", localtime(&t));

	sym->Value.Macro.pData = realloc(sym->Value.Macro.pData, len + 1);
	sym->Value.Macro.Size = len;
	strcpy(sym->Value.Macro.pData, s);

	return sym->Value.Macro.pData;
}

static char* __TIME__Callback(SSymbol* sym)
{
	char s[16];
	time_t t = time(NULL);
	size_t len;

	len = strftime(s, sizeof(s), "%X", localtime(&t));

	sym->Value.Macro.pData = realloc(sym->Value.Macro.pData, len + 1);
	sym->Value.Macro.Size = len;
	strcpy(sym->Value.Macro.pData, s);

	return sym->Value.Macro.pData;
}

static char* __AMIGADATE__Callback(SSymbol* sym)
{
	char s[16];
	time_t t = time(NULL);
	struct tm* tm = localtime(&t);
	size_t len;

	len = sprintf(s, "%d.%d.%d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);

	sym->Value.Macro.pData = realloc(sym->Value.Macro.pData, len + 1);
	sym->Value.Macro.Size = len;
	strcpy(sym->Value.Macro.pData, s);

	return sym->Value.Macro.pData;
}

static ULONG sym_CalcHash(char* s)
{
	ULONG hash = 0;

	while(*s != 0)
	{
		hash += *s++;
		hash += hash << 10;
		hash ^= hash >> 6;
	}

	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;

	return hash & (HASHSIZE - 1);
}

static SSymbol* sym_Find(char* s, SSymbol* scope)
{
    SSymbol* psym = g_pHashedSymbols[sym_CalcHash(s)];

    while(psym)
    {
		if(strcmp(psym->Name, s) == 0 && psym->pScope == scope)
			return psym;

		psym = list_GetNext(psym);
    }

	return NULL;
}

static SSymbol* sym_Create(char* s)
{
    SSymbol** phash;
	SSymbol* psym;
    ULONG hash;

    hash = sym_CalcHash(s);
    phash = &g_pHashedSymbols[hash];

    if((psym = (SSymbol*)malloc(sizeof(SSymbol))) != NULL)
    {
		memset(psym, 0, sizeof(SSymbol));
		psym->Type = SYM_UNDEFINED;
		psym->Flags = g_aDefaultSymbolFlags[SYM_UNDEFINED];
	    strcpy(psym->Name, s);
		list_Insert(*phash, psym);

		return psym;
    }

	internalerror("Out of memory");
	return NULL;
}

static SSymbol* sym_FindOrCreate(char* s, SSymbol* scope)
{
	SSymbol* sym;

	if((sym = sym_Find(s,scope)) == NULL)
	{
		sym = sym_Create(s);
		sym->pScope = scope;
		return sym;
	}

	return sym;
}

static SSymbol* sym_GetScope(char* s)
{
	if(*s == '.' || s[strlen(s) - 1] == '$')
		return pCurrentScope;

	return NULL;
}

static BOOL sym_isType(SSymbol* sym, ESymbolType type)
{
	return sym->Type == type || sym->Type == SYM_UNDEFINED;
}

static SLONG sym_GetValueField(SSymbol* sym)
{
	if(sym->Callback.Integer)
		return sym->Callback.Integer(sym);

	return sym->Value.Value;
}




/*	Public routines */

SLONG sym_GetConstant(char* name)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if(sym->Flags & SYMF_CONSTANT)
		return sym_GetValueField(sym);

	prj_Fail(ERROR_SYMBOL_CONSTANT);
	return 0;
}

SSymbol* sym_FindSymbol(char* name)
{
	return sym_FindOrCreate(name, sym_GetScope(name));
}

SSymbol* sym_AddGROUP(char* name, EGroupType value)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

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

SSymbol* sym_AddEQUS(char* name, char* value)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if((sym->Flags & SYMF_MODIFY) && sym_isType(sym, SYM_EQUS))
	{
		sym->Type = SYM_EQUS;
		SetFlags(sym->Flags, SYM_EQUS);

		if(value == NULL)
		{
			sym->Value.Macro.Size = 0;
			sym->Value.Macro.pData = NULL;
			return sym;
		}

		sym->Value.Macro.Size = strlen(value);
		if((sym->Value.Macro.pData = _strdup(value)) != NULL)
			return sym;

		internalerror("Out of memory");
		return NULL;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_AddMACRO(char* name, char* value, ULONG size)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if((sym->Flags & SYMF_MODIFY) && sym_isType(sym, SYM_MACRO))
	{
		sym->Type = SYM_MACRO;
		SetFlags(sym->Flags, SYM_MACRO);
		sym->Value.Macro.Size = size;
		if((sym->Value.Macro.pData = malloc(size)) != NULL)
		{
			memcpy(sym->Value.Macro.pData, value, size);
			return sym;
		}

		internalerror("Out of memory");
		return NULL;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_AddEQU(char* name, SLONG value)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if((sym->Flags & SYMF_MODIFY)
	&& (sym_isType(sym, SYM_EQU) || sym->Type == SYM_GLOBAL))
	{
		if(sym->Type == SYM_GLOBAL)
			sym->Flags |= SYMF_EXPORT;

		sym->Type = SYM_EQU;
		SetFlags(sym->Flags, SYM_EQU);
		sym->Value.Value = value;
		return sym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_AddSET(char* name, SLONG value)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if((sym->Flags & SYMF_MODIFY) && sym_isType(sym, SYM_SET))
	{
		sym->Type = SYM_SET;
		SetFlags(sym->Flags, SYM_SET);
		sym->Value.Value = value;
		return sym;
	}

	prj_Error(ERROR_MODIFY_SYMBOL);
	return NULL;
}

SSymbol* sym_AddLabel(char* name)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if((sym->Flags & SYMF_MODIFY)
	&& (sym_isType(sym, SYM_LABEL) || sym->Type == SYM_GLOBAL))
	{
		if(sym->Type == SYM_GLOBAL)
			sym->Flags |= SYMF_EXPORT;

		if(pCurrentSection)
		{
			if(name[0] != '.' && name[strlen(name) - 1] != '$')
				pCurrentScope = sym;

			if((pCurrentSection->Flags & SECTF_ORGFIXED) == 0)
			{
				sym->Type = SYM_LABEL;
				SetFlags(sym->Flags, SYM_LABEL);
				sym->pSection = pCurrentSection;
				sym->Value.Value = pCurrentSection->PC;
				return sym;
			}
			else
			{
				sym->Type = SYM_EQU;
				SetFlags(sym->Flags, SYM_EQU);
				sym->Value.Value = pCurrentSection->PC + pCurrentSection->Org;
				return sym;
			}
		}
		else
			prj_Error(ERROR_LABEL_SECTION);
	}
	else
		prj_Error(ERROR_MODIFY_SYMBOL);

	return NULL;
}

char* sym_ConvertSymbolValueToString(char* dst, char* sym)
{
	SSymbol* psym;

	psym = sym_FindOrCreate(sym, sym_GetScope(sym));

	switch(psym->Type)
	{
		case SYM_EQU:
		case SYM_SET:
		{
			sprintf(dst, "$%lX", sym_GetValueField(psym));
			return dst + strlen(dst);
			break;
		}
		case SYM_EQUS:
		{
			strcpy(dst, sym_GetStringValueBySymbol(psym));
			return dst + strlen(dst);
			break;
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

SSymbol* sym_Export(char* name)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if(sym->Flags & SYMF_EXPORTABLE)
		sym->Flags |= SYMF_EXPORT;
	else
		prj_Error(ERROR_SYMBOL_EXPORT);

	return sym;
}

SSymbol* sym_Import(char* name)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if(sym->Type == SYM_UNDEFINED)
	{
		sym->Type = SYM_IMPORT;
		SetFlags(sym->Flags, SYM_IMPORT);
		sym->Value.Value = 0;
		return sym;
	}

	prj_Error(ERROR_IMPORT_DEFINED);
	return NULL;
}

SSymbol* sym_Global(char* name)
{
	SSymbol* sym = sym_FindOrCreate(name, sym_GetScope(name));

	if(sym->Type == SYM_UNDEFINED)
	{
		/*	Symbol has not yet been defined, we'll leave this till later */
		sym->Type = SYM_GLOBAL;
		SetFlags(sym->Flags, SYM_GLOBAL);
		sym->Value.Value = 0;
		return sym;
	}

	if(sym->Flags & SYMF_EXPORTABLE)
	{
		sym->Flags |= SYMF_EXPORT;
		return sym;
	}

	prj_Error(ERROR_SYMBOL_EXPORT);
	return NULL;
}

BOOL	sym_Purge(char* name)
{
    SSymbol** ppsym;
	SSymbol* sym;

    ppsym = &g_pHashedSymbols[sym_CalcHash(name)];

	if((sym = sym_Find(name, sym_GetScope(name))) != NULL)
	{
		list_Remove(*ppsym, sym);
		if(sym->Flags == SYMF_HASDATA)
		{
			free(sym->Value.Macro.pData);
		}
		free(sym);
		return TRUE;
	}

	prj_Warn(WARN_CANNOT_PURGE);
	return FALSE;
}

BOOL sym_isString(char* name)
{
	SSymbol* sym;

	if((sym = sym_Find(name,sym_GetScope(name))) != NULL)
		return sym->Type == SYM_EQUS;

	return FALSE;
}

BOOL sym_isMacro(char* name)
{
	SSymbol* sym;

	if((sym = sym_Find(name, sym_GetScope(name))) != NULL)
		return sym->Type == SYM_MACRO;

	return FALSE;
}

BOOL sym_isDefined(char* name)
{
	SSymbol* sym;

	if((sym = sym_Find(name, sym_GetScope(name))) != NULL)
		return sym->Type != SYM_UNDEFINED;


	return FALSE;
}

static char* sym_GetStringValueBySymbol(SSymbol* sym)
{
	if(sym->Type == SYM_EQUS)
	{
		if(sym->Callback.String)
			sym->Callback.String(sym);
		return sym->Value.Macro.pData;
	}

	prj_Fail(ERROR_SYMBOL_EQUS);
	return NULL;
}

char* sym_GetStringValue(char* name)
{
	SSymbol* sym;

	if((sym = sym_Find(name, sym_GetScope(name))) != NULL)
	{
		return sym_GetStringValueBySymbol(sym);
	}

	return NULL;
}

BOOL sym_Init(void)
{
	pCurrentScope = NULL;

	locsym_Init();
	p__NARG__Symbol = sym_AddEQU("__NARG", 0);
	p__NARG__Symbol->Callback.Integer = __NARG__Callback;
	p__LINE__Symbol = sym_AddEQU("__LINE", 0);
	p__LINE__Symbol->Callback.Integer = __LINE__Callback;
	p__DATE__Symbol = sym_AddEQUS("__DATE", 0);
	p__DATE__Symbol->Callback.String = __DATE__Callback;
	p__TIME__Symbol = sym_AddEQUS("__TIME", 0);
	p__TIME__Symbol->Callback.String = __TIME__Callback;

	if(g_pConfiguration->bSupportAmiga)
	{
		p__AMIGADATE__Symbol = sym_AddEQUS("__AMIGADATE", 0);
		p__AMIGADATE__Symbol->Callback.String = __AMIGADATE__Callback;
	}

	sym_AddEQU("__ASMOTOR", 0);

	return TRUE;
}
