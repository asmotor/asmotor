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

#include "xlink.h"

SSection* pSections = NULL;

SSection* sect_CreateNew(void)
{
	SSection** ppsect = &pSections;

	while(*ppsect)
		ppsect = &(*ppsect)->pNext;

	*ppsect = (SSection*)mem_Alloc(sizeof(SSection));
	if(*ppsect == NULL)
		Error("Out of memory");

	(*ppsect)->pNext = NULL;
	(*ppsect)->Used = false;
	(*ppsect)->Assigned = false;
	(*ppsect)->pPatches = NULL;

	return *ppsect;
}

static void resolve_symbol(SSection* sect, SSymbol* sym)
{
	switch(sym->Type)
	{
		case SYM_LOCALEXPORT:
		case SYM_EXPORT:
		case SYM_LOCAL:
		{
			sym->Resolved = true;
			sym->Value += sect->Org;
			sym->pSection = sect;
			break;
		}
		case SYM_IMPORT:
		{
			SSection* filesect;
			
			for(filesect = pSections; filesect != NULL; filesect = filesect->pNext)
			{
				if(filesect->Used)
				{
					uint32_t i;
					SSymbol* filesym = filesect->pSymbols;

					for(i = 0; i < filesect->TotalSymbols; ++i, ++filesym)
					{
						if(filesym->Type == SYM_EXPORT && strcmp(filesym->Name, sym->Name) == 0)
						{
							if(!filesym->Resolved)
								resolve_symbol(filesect, filesym);

							sym->Resolved = true;
							sym->Value = filesym->Value;
							sym->pSection = filesect;

							return;
						}
					}
				}
			}

			Error("unresolved symbol \"%s\"", sym->Name);
			break;
		}
		case SYM_LOCALIMPORT:
		{
			SSection* filesect;

			for(filesect = pSections; filesect != NULL; filesect = filesect->pNext)
			{
				if(filesect->Used && filesect->FileID == sect->FileID)
				{
					uint32_t i;
					SSymbol* filesym = filesect->pSymbols;

					for(i = 0; i < filesect->TotalSymbols; ++i, ++filesym)
					{
						if((filesym->Type == SYM_LOCALEXPORT || filesym->Type == SYM_EXPORT)
						&&	strcmp(filesym->Name, sym->Name) == 0)
						{
							if(!filesym->Resolved)
								resolve_symbol(filesect, filesym);

							sym->Resolved = true;
							sym->Value = filesym->Value;
							sym->pSection = filesect;

							return;
						}
					}
				}
			}

			Error("unresolved symbol \"%s\"", sym->Name);
			break;
		}
		default:
		{
			Error("unhandled symbol type");
			break;
		}
	}
}

int32_t sect_GetSymbolValue(SSection* sect, int32_t symbolid)
{
	SSymbol* sym = &sect->pSymbols[symbolid];

	if(!sym->Resolved)
		resolve_symbol(sect, sym);

	return sym->Value;
}

char* sect_GetSymbolName(SSection* sect, int32_t symbolid)
{
	SSymbol* sym = &sect->pSymbols[symbolid];

	return sym->Name;
}

int32_t sect_GetSymbolBank(SSection* sect, int32_t symbolid)
{
	SSymbol* sym = &sect->pSymbols[symbolid];

	if(!sym->Resolved)
		resolve_symbol(sect, sym);

	return sym->pSection->Bank;
}