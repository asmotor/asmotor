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

#include <stdio.h>

#include "types.h"
#include "section.h"
#include "project.h"
#include "symbol.h"
#include "patch.h"

bool_t bin_Write(string* pName)
{
	FILE* f;

	SSection* sect;
	int32_t nAddress;
	int i;
	bool_t bNeedOrg = false;

	sect = pSectionList;
	while(sect)
	{
		if(sect->pPatches != NULL)
			bNeedOrg = true;
		sect = list_GetNext(sect);
	}
	
	sect = pSectionList;
	if(bNeedOrg && (sect->Flags & SECTF_ORGFIXED) == 0)
	{
		prj_Error(ERROR_SECTION_MUST_ORG);
		return false;
	}

	nAddress = sect->Org;
	do
	{
		nAddress += (sect->UsedSpace + 7) & ~7;
		sect = list_GetNext(sect);
		if(sect != NULL)
		{
			if(sect->Flags & SECTF_ORGFIXED)
			{
				if(sect->Org < nAddress)
				{
					prj_Error(ERROR_SECTION_ORG, sect->Name, sect->Org);
					return false;
				}
				nAddress = sect->Org;
			}
			else
			{
				sect->Flags |= SECTF_ORGFIXED;
				sect->Org = nAddress;
			}
		}
	} while(sect != NULL);

	for(i = 0; i < HASHSIZE; ++i)
	{
		SSymbol* sym = g_pHashedSymbols[i];
		while(sym)
		{
			if(sym->nFlags & SYMF_RELOC)
			{
				sym->nFlags &= ~SYMF_RELOC;
				sym->nFlags |= SYMF_CONSTANT;
				sym->Value.Value += sym->pSection->Org;
			}
			sym = list_GetNext(sym);
		}
	}

	patch_BackPatch();

	if((f = fopen(str_String(pName),"wb")) != NULL)
	{
		sect = pSectionList;
		nAddress = sect->Org;

		while(sect)
		{
			while(nAddress < sect->Org)
			{
				++nAddress;
				fputc(0, f);
			}

			fwrite(sect->pData, 1, sect->UsedSpace, f);
			nAddress += sect->UsedSpace;

			sect = list_GetNext(sect);
		}

		fclose(f);
		return true;
	}

	return false;
}

