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

#include <string.h>

#include "xlink.h"

Section* g_sections = NULL;

static uint32_t s_sectionId = 0;

Section* sect_CreateNew(void)
{
	Section** section = &g_sections;

	while (*section != NULL)
		section = &(*section)->nextSection;

	*section = (Section*)mem_Alloc(sizeof(Section));
	if (*section == NULL)
		Error("Out of memory");

	(*section)->sectionId = s_sectionId++;
	(*section)->nextSection = NULL;
	(*section)->used = false;
	(*section)->assigned = false;
	(*section)->patches = NULL;
	(*section)->relocs = NULL;

	return *section;
}

uint32_t sect_TotalSections(void)
{
	return s_sectionId;
}


static void resolveSymbol(Section* section, Symbol* symbol, bool_t allowImports)
{
	switch (symbol->type)
	{
		case SYM_LOCALEXPORT:
		case SYM_EXPORT:
		case SYM_LOCAL:
		{
			symbol->resolved = true;
			symbol->section = section;

			if (section->cpuLocation != -1)
				symbol->value += section->cpuLocation;

			break;
		}

		case SYM_IMPORT:
		{
			Section* definingSection;
			
			for (definingSection = g_sections; definingSection != NULL; definingSection = definingSection->nextSection)
			{
				if (definingSection->used)
				{
					uint32_t i;
					Symbol* exportedSymbol = definingSection->symbols;

					for (i = 0; i < definingSection->totalSymbols; ++i, ++exportedSymbol)
					{
						if (exportedSymbol->type == SYM_EXPORT && strcmp(exportedSymbol->name, symbol->name) == 0)
						{
							if (!exportedSymbol->resolved)
								resolveSymbol(definingSection, exportedSymbol, allowImports);

							symbol->resolved = true;
							symbol->value = exportedSymbol->value;
							symbol->section = definingSection;

							return;
						}
					}
				}
			}

            if (!allowImports)
                Error("Unresolved symbol \"%s\"", symbol->name);
            
			break;
		}

		case SYM_LOCALIMPORT:
		{
			Section* definingSection;

			for (definingSection = g_sections; definingSection != NULL; definingSection = definingSection->nextSection)
			{
				if (definingSection->used && definingSection->fileId == section->fileId)
				{
					uint32_t i;
					Symbol* exportedSymbol = definingSection->symbols;

					for (i = 0; i < definingSection->totalSymbols; ++i, ++exportedSymbol)
					{
						if ((exportedSymbol->type == SYM_LOCALEXPORT || exportedSymbol->type == SYM_EXPORT)
						&&	strcmp(exportedSymbol->name, symbol->name) == 0)
						{
							if (!exportedSymbol->resolved)
								resolveSymbol(definingSection, exportedSymbol, allowImports);

							symbol->resolved = true;
							symbol->value = exportedSymbol->value;
							symbol->section = definingSection;

							return;
						}
					}
				}
			}

			Error("Unresolved symbol \"%s\"", symbol->name);
			break;
		}

		default:
		{
			Error("Unhandled symbol type");
			break;
		}
	}
}


Symbol* sect_GetSymbol(Section* section, int32_t symbolId, bool_t allowImports)
{
	if (symbolId < 0 || symbolId >= section->totalSymbols)
	{
		Error("Symbol ID out of range");
		return NULL;
	}
	
	Symbol* symbol = &section->symbols[symbolId];

	if (!symbol->resolved)
		resolveSymbol(section, symbol, allowImports);

	return symbol;
}


void sect_GetSymbolValue(Section* section, int32_t symbolId, int32_t* outValue, Section** outSection)
{
	Symbol* symbol = sect_GetSymbol(section, symbolId, false);

	*outValue = symbol->value;
	if (symbol->section->cpuLocation != -1)
		*outSection = NULL;
	else
		*outSection = symbol->section;
}


char* sect_GetSymbolName(Section* section, int32_t symbolId)
{
	Symbol* symbol = &section->symbols[symbolId];

	return symbol->name;
}


bool_t sect_GetConstantSymbolBank(Section* section, int32_t symbolId, int32_t* outValue)
{
	int32_t bank;
	Symbol* symbol = &section->symbols[symbolId];

	if (!symbol->resolved)
		resolveSymbol(section, symbol, false);

	bank = symbol->section->cpuBank;
	if (bank != -1)
	{
		*outValue = bank;
		return true;
	}

	return false;
}


void sect_ForEachUsedSection(void (*function)(Section*))
{
    Section* section;

    for (section = g_sections; section != NULL; section = section->nextSection)
    {
        if (section->used)
            function(section);
    }

}
