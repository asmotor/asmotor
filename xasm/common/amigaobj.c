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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asmotor.h"
#include "mem.h"

#include "types.h"

#include "section.h"
#include "symbol.h"
#include "patch.h"
#include "project.h"
#include "amigaobj.h"

#define	HUNK_UNIT	0x3E7
#define	HUNK_NAME	0x3E8
#define	HUNK_CODE	0x3E9
#define	HUNK_DATA	0x3EA
#define	HUNK_BSS	0x3EB
#define	HUNK_RELOC32 0x3EC
#define	HUNK_EXT	0x3EF
#define	HUNK_SYMBOL	0x3F0
#define	HUNK_END	0x3F2
#define	HUNK_HEADER	0x3F3
#define	HUNKF_CHIP	(1 << 30)

#define EXT_DEF		0x01000000
#define EXT_REF32	0x81000000

static void fputml(int32_t d, FILE* f)
{
	fputc((d >> 24) & 0xFF, f);
	fputc((d >> 16) & 0xFF, f);
	fputc((d >> 8) & 0xFF, f);
	fputc(d & 0xFF, f);
}

static void fputbuf(void* pBuf, int nLen, FILE* f)
{
	fwrite(pBuf, 1, nLen, f);

	while((nLen & 3) != 0)
	{
		fputc(0, f);
		++nLen;
	}
}

static void fputstr(char* pStr, FILE* f, uint32_t def)
{
	int nLen = (int)strlen(pStr);
	fputml(((nLen + 3) / 4) | def, f);
	fputbuf(pStr, nLen, f);
}

void ami_WriteSymbolHunk(FILE* f, SSection* pSect, bool_t bSkipExt)
{
	int i;
	int count = 0;
	long fpos;

	fpos = ftell(f);
	fputml(HUNK_SYMBOL, f);

	for(i = 0; i < HASHSIZE; ++i)
	{
		SSymbol* sym = g_pHashedSymbols[i];
		while(sym)
		{
			if((sym->nFlags & SYMF_RELOC) != 0
			&& sym->pSection == pSect)
			{
				if(!((sym->nFlags & SYMF_EXPORT) && bSkipExt))
				{
					fputstr(str_String(sym->pName), f, 0);
					fputml(sym->Value.Value, f);
					++count;
				}
			}
			sym = list_GetNext(sym);
		}
	}

	if(count == 0)
		fseek(f, fpos, SEEK_SET);
	else
		fputml(0, f);
}

void ami_WriteExtHunk(FILE* f, struct Section* pSect, struct Patch* pImportPatches, uint32_t nCodePos)
{
	int i;
	int count = 0;
	long fpos;

	fpos = ftell(f);
	fputml(HUNK_EXT, f);

	while(pImportPatches != NULL)
	{
		uint32_t offset;
		SSymbol* pSym = NULL;
		if(patch_GetImportOffset(&offset, &pSym, pImportPatches->pExpression))
		{
			long oldpos;
			long symcountpos;
			long loccount = 0;
			SPatch* patch;

			fputstr(str_String(pSym->pName), f, EXT_REF32);
			symcountpos = ftell(f);
			fputml(0, f);

			patch = pImportPatches;
			do
			{
				long pos = ftell(f);
				fseek(f, patch->Offset + nCodePos, SEEK_SET);
				fputml(offset, f);
				fseek(f, pos, SEEK_SET);
				fputml(patch->Offset, f);
				++count;
				++loccount;

				patch = list_GetNext(patch);
				while(patch != NULL)
				{
					SSymbol* sym = NULL;
					if(patch_GetImportOffset(&offset, &sym, patch->pExpression)
					&& sym == pSym)
					{
						if(patch->pPrev)
							patch->pPrev->pNext = patch->pNext;
						else
							internalerror("shouldn't happen");
						if(patch->pNext)
							patch->pNext->pPrev = patch->pPrev;

						break;
					}
					patch = list_GetNext(patch);
				}
			} while(patch != NULL);

			oldpos = ftell(f);
			fseek(f, symcountpos, SEEK_SET);
			fputml(loccount, f);
			fseek(f, oldpos, SEEK_SET);
		}
		pImportPatches = list_GetNext(pImportPatches);
	}

	for(i = 0; i < HASHSIZE; ++i)
	{
		SSymbol* sym = g_pHashedSymbols[i];
		while(sym)
		{
			if((sym->nFlags & (SYMF_RELOC | SYMF_EXPORT)) == (SYMF_RELOC | SYMF_EXPORT)
			&& sym->pSection == pSect)
			{
				fputstr(str_String(sym->pName), f, EXT_DEF);
				fputml(sym->Value.Value, f);
				++count;
			}
			sym = list_GetNext(sym);
		}
	}

	if(count == 0)
		fseek(f, fpos, SEEK_SET);
	else
		fputml(0, f);

}

bool_t ami_WriteSection(FILE* f, SSection* pSect, bool_t bDebugInfo, uint32_t nSections, bool_t bLink)
{
	if(pSect->pGroup->Value.GroupType == GROUP_TEXT)
	{
		SPatch** pPatches = mem_Alloc(sizeof(SPatch*) * nSections);
		SPatch* pImportPatches = NULL;
		SPatch* patch;
		long hunkpos;
		uint32_t i;
		uint32_t hunktype;
		bool_t bHasReloc32 = false;

		for(i = 0; i < nSections; ++i)
			pPatches[i] = NULL;

		if(g_pConfiguration->bSupportAmiga && (pSect->pGroup->nFlags & SYMF_DATA))
			hunktype = HUNK_DATA;
		else
			hunktype = HUNK_CODE;

		fputml(hunktype, f);
		fputml((pSect->UsedSpace + 3) / 4, f);
		hunkpos = ftell(f);
		fputbuf(pSect->pData, pSect->UsedSpace, f);

		/* move the patches into the pPatches array according to
		   the section to which their value is relative
		 */
		patch = pSect->pPatches;
		while(patch)
		{
			SPatch* nextpatch = list_GetNext(patch);
			if(patch->Type == PATCH_BLONG)
			{
				SSection* fsect = pSectionList;
				int nsect = 0;
				bool_t foundsect = false;
				while(fsect != NULL)
				{
					if(patch_IsRelativeToSection(patch->pExpression, fsect))
					{
						if(patch->pPrev)
							patch->pPrev = patch->pNext;
						else
							pSect->pPatches = patch->pNext;
						if(patch->pNext)
							patch->pNext->pPrev = patch->pPrev;

						patch->pPrev = NULL;
						patch->pNext = pPatches[nsect];
						if(pPatches[nsect])
							pPatches[nsect]->pPrev = patch;
						pPatches[nsect] = patch;
						bHasReloc32 = true;
						foundsect = true;
						break;
					}
					++nsect;
					fsect = list_GetNext(fsect);
				}

				if((!foundsect) && bLink)
				{
					uint32_t offset;
					SSymbol* pSym = NULL;
					if(patch_GetImportOffset(&offset, &pSym, patch->pExpression))
					{
						if(patch->pPrev)
							patch->pPrev = patch->pNext;
						else
							pSect->pPatches = patch->pNext;
						if(patch->pNext)
							patch->pNext->pPrev = patch->pPrev;

						patch->pPrev = NULL;
						patch->pNext = pImportPatches;
						if(pImportPatches)
							pImportPatches->pPrev = patch;
						pImportPatches = patch;
					}
				}
			}
			patch = nextpatch;
		}
		if(pSect->pPatches != NULL)
		{
			prj_Error(ERROR_OBJECTFILE_PATCH);
			return false;
		}

		if(bHasReloc32)
		{
			uint32_t i;
			SSection* fsect = pSectionList;

			fputml(HUNK_RELOC32, f);

			for(i = 0; i < nSections; ++i)
			{
				uint32_t nReloc = 0;
				SPatch* patch = pPatches[i];
				while(patch)
				{
					++nReloc;
					patch = list_GetNext(patch);
				}
				if(nReloc > 0)
				{
					fputml(nReloc, f);
					fputml(i, f);
					patch = pPatches[i];
					while(patch)
					{
						uint32_t value;
						size_t fpos;
						patch_GetSectionPcOffset(&value, patch->pExpression, fsect);
						fpos = ftell(f);
						fseek(f, (long)(patch->Offset + hunkpos), SEEK_SET);
						fputml(value, f);
						fseek(f, (long)fpos, SEEK_SET);
						fputml(patch->Offset, f);
						patch = list_GetNext(patch);
					}
				}

				fsect = list_GetNext(fsect);
			}
			fputml(0, f);
		}

		if(bLink)
			ami_WriteExtHunk(f, pSect, pImportPatches, hunkpos);

		mem_Free(pPatches);
	}
	else /*if(pSect->pGroup->Flags & GROUP_BSS)*/
	{
		uint32_t hunktype = HUNK_BSS;
		fputml(hunktype, f);
		fputml((pSect->UsedSpace + 3) / 4, f);

		if(bLink)
			ami_WriteExtHunk(f, pSect, NULL, 0);
	}

	if(bDebugInfo)
		ami_WriteSymbolHunk(f, pSect, /*bLink*/ false);

	fputml(HUNK_END, f);
	return true;
}

void ami_WriteSectionNames(FILE* f, bool_t bDebugInfo)
{
	if(bDebugInfo)
	{
		SSection* pSect = pSectionList;
		while(pSect != NULL)
		{
			fputstr(pSect->Name, f, 0);
			pSect = list_GetNext(pSect);
		}
	}

	/* name list terminator */
	fputml(0, f);
}

bool_t ami_WriteObject(string* pDestFilename, string* pSourceFilename, bool_t bDebugInfo)
{
	FILE* f;
	SSection* pSect;
	uint32_t nSections;
	bool_t r = true;

	f = fopen(str_String(pDestFilename), "wb");
	if(f == NULL)
		return false;

	nSections = 0;
	pSect = pSectionList;
	while(pSect != NULL)
	{
		pSect = list_GetNext(pSect);
		++nSections;
	}

	fputml(HUNK_UNIT, f);
	fputstr(str_String(pSourceFilename), f, 0);

	pSect = pSectionList;
	while(pSect != NULL)
	{
		fputml(HUNK_NAME, f);
		fputstr(pSect->Name, f, 0);
		if(!ami_WriteSection(f, pSect, true, nSections, true))
		{
			r = false;
			break;
		}

		pSect = list_GetNext(pSect);
	}

	fclose(f);
	return r;
}

bool_t ami_WriteExecutable(string* pDestFilename, bool_t bDebugInfo)
{
	FILE* f;
	SSection* pSect;
	uint32_t nSections;
	bool_t r = true;

	f = fopen(str_String(pDestFilename), "wb");
	if(f == NULL)
		return false;

	fputml(HUNK_HEADER, f);
	ami_WriteSectionNames(f, bDebugInfo);

	nSections = 0;
	pSect = pSectionList;
	while(pSect != NULL)
	{
		pSect = list_GetNext(pSect);
		++nSections;
	}

	fputml(nSections, f);
	fputml(0, f);
	fputml(nSections - 1, f);

	pSect = pSectionList;
	while(pSect != NULL)
	{
		uint32_t size = (pSect->UsedSpace + 3) / 4;
		if(g_pConfiguration->bSupportAmiga && (pSect->pGroup->nFlags & SYMF_CHIP))
			size |= HUNKF_CHIP;
		fputml(size, f);
		pSect = list_GetNext(pSect);
	}

	pSect = pSectionList;
	while(pSect != NULL)
	{
		if(!ami_WriteSection(f, pSect, bDebugInfo, nSections, false))
		{
			r = false;
			break;
		}

		pSect = list_GetNext(pSect);
	}

	fclose(f);
	return r;
}
