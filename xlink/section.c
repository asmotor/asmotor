/*  Copyright 2008-2015 Carsten Elton Sorensen

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

Section* sect_CreateNew(void)
{
	Section** section = &g_sections;

	while (*section != NULL)
		section = &(*section)->nextSection;

	*section = (Section*)mem_Alloc(sizeof(Section));
	if (*section == NULL)
		Error("Out of memory");

	(*section)->nextSection = NULL;
	(*section)->used = false;
	(*section)->assigned = false;
	(*section)->patches = NULL;

	return *section;
}


static void resolveSymbol(Section* section, Symbol* symbol)
{
	switch (symbol->type)
	{
		case SYM_LOCALEXPORT:
		case SYM_EXPORT:
		case SYM_LOCAL:
		{
			symbol->resolved = true;
			symbol->value += section->cpuLocation;
			symbol->section = section;
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
								resolveSymbol(definingSection, exportedSymbol);

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
								resolveSymbol(definingSection, exportedSymbol);

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


int32_t sect_GetSymbolValue(Section* section, int32_t symbolId)
{
	Symbol* symbol = &section->symbols[symbolId];

	if (!symbol->resolved)
		resolveSymbol(section, symbol);

	return symbol->value;
}


char* sect_GetSymbolName(Section* section, int32_t symbolId)
{
	Symbol* symbol = &section->symbols[symbolId];

	return symbol->name;
}


int32_t sect_GetSymbolBank(Section* section, int32_t symbolId)
{
	Symbol* symbol = &section->symbols[symbolId];

	if (!symbol->resolved)
		resolveSymbol(section, symbol);

	return symbol->section->cpuBank;
}
