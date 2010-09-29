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

#include "xasm.h"




//	Private defines

#define	CHUNKSIZE	0x4000




//	Public variables

SSection* pCurrentSection;
SSection* pSectionList;




//	Private routines

static	eGroupType	sect_GetCurrentType(void)
{
	if(pCurrentSection)
	{
		if(pCurrentSection->pGroup)
		{
			if(pCurrentSection->pGroup->Type==SYM_GROUP)
			{
				return pCurrentSection->pGroup->Value.GroupType;
			}
			else
			{
				internalerror("SECTION's GROUP symbol is not of type SYM_GROUP");
			}
		}
		else
		{
			internalerror("No GROUP defined for SECTION");
		}
	}
	else
	{
		internalerror("No SECTION defined");
	}
	return -1;
}

static	SSection* sect_Create(char* name)
{
	SSection* sect;

	if((sect=malloc(sizeof(SSection)))!=NULL)
	{
		memset(sect, 0, sizeof(SSection));
		sect->FreeSpace=MAXSECTIONSIZE;
		if(pSectionList)
		{
			SSection* list = pSectionList;
			while(list->pNext)
				list = list_GetNext(list);
			list_InsertAfter(list, sect);
		}
		else
		{
			pSectionList=sect;
		}
		strcpy(sect->Name, name);
		return sect;
	}
	else
	{
		internalerror("Out of memory");
	}
}

static	SSection* sect_Find(char* name, SSymbol* group)
{
	SSection* sect;

	sect=pSectionList;
	while(sect)
	{
		if(strcmp(sect->Name,name)==0)
		{
			if(group)
			{
				if(sect->pGroup==group)
				{
					return sect;
				}
				else
				{
					prj_Fail(ERROR_SECT_EXISTS);
					return NULL;
				}
			}
			else
			{
				return sect;
			}
		}
		sect=list_GetNext(sect);
	}

	return NULL;
}

static	void	sect_GrowCurrent(SLONG count)
{
	if(count+pCurrentSection->UsedSpace > pCurrentSection->AllocatedSpace)
	{
		SLONG	allocate;

		allocate=(count+pCurrentSection->UsedSpace+CHUNKSIZE-1)&(~(CHUNKSIZE-1));
		if((pCurrentSection->pData=realloc(pCurrentSection->pData,allocate))!=NULL)
		{
			pCurrentSection->AllocatedSpace=allocate;
		}
		else
		{
			internalerror("Out of memory!");
		}
	}
}

static	BOOL	sect_CheckAvailableSpace(ULONG count)
{
	if(pCurrentSection)
	{
		if(count<=pCurrentSection->FreeSpace)
		{
			if(sect_GetCurrentType()==GROUP_TEXT)
			{
				sect_GrowCurrent(count);
			}
			return TRUE;
		}
		else
		{
			prj_Error(ERROR_SECTION_FULL);
			return FALSE;
		}
	}
	else
	{
		prj_Error(ERROR_SECTION_MISSING);
		return FALSE;
	}
}




//	Public routines

void sect_OutputAbsByte(UBYTE value)
{
	if(sect_CheckAvailableSpace(1))
	{
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				pCurrentSection->FreeSpace -= 1;
				pCurrentSection->UsedSpace += 1;
				pCurrentSection->pData[pCurrentSection->PC++] = value;
				break;
			}
			case GROUP_BSS:
			{
				prj_Error(ERROR_SECTION_DATA);
				break;
			}
			default:
			{
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}


void sect_OutputRelByte(SExpression* expr)
{
	if(sect_CheckAvailableSpace(1))
	{
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				patch_Create(pCurrentSection, pCurrentSection->PC, expr, PATCH_BYTE);
				pCurrentSection->PC += 1;
				pCurrentSection->FreeSpace -= 1;
				pCurrentSection->UsedSpace += 1;
				break;
			}
			case GROUP_BSS:
			{
				prj_Error(ERROR_SECTION_DATA);
				sect_SkipBytes(1);
				break;
			}
			default:
			{
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}


void sect_OutputExprByte(SExpression* expr)
{
	if(expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if(expr->Flags&EXPRF_isRELOC)
		sect_OutputRelByte(expr);
	else if(expr->Flags & EXPRF_isCONSTANT)
	{
		sect_OutputAbsByte((UBYTE)(expr->Value.Value));
		parse_FreeExpression(expr);
	}
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
}

void sect_OutputAbsWord(UWORD value)
{
	if(sect_CheckAvailableSpace(2))
	{
		pCurrentSection->FreeSpace -= 2;
		pCurrentSection->UsedSpace += 2;
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				switch(pOptions->Endian)
				{
					case ASM_LITTLE_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value>>8);
						break;
					}
					case ASM_BIG_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value>>8);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value);
						break;
					}
					default:
					{
						internalerror("Unknown endianness");
						break;
					}
				}
				break;
			}
			case GROUP_BSS:
			{
				prj_Error(ERROR_SECTION_DATA);
				break;
			}
			default:
			{
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputRelWord(SExpression* expr)
{
	if(sect_CheckAvailableSpace(2))
	{
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				pCurrentSection->FreeSpace -= 2;
				pCurrentSection->UsedSpace += 2;
				switch(pOptions->Endian)
				{
					case ASM_LITTLE_ENDIAN:
					{
						patch_Create(pCurrentSection, pCurrentSection->PC, expr, PATCH_LWORD);
						break;
					}
					case ASM_BIG_ENDIAN:
					{
						patch_Create(pCurrentSection, pCurrentSection->PC, expr, PATCH_BWORD);
						break;
					}
					default:
					{
						internalerror("Unknown endianness");
						break;
					}
				}
				pCurrentSection->PC += 2;
				break;
			}
			case GROUP_BSS:
			{
				prj_Error(ERROR_SECTION_DATA);
				sect_SkipBytes(2);
				break;
			}
			default:
			{
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputExprWord(SExpression* expr)
{
	if(expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if(expr->Flags & EXPRF_isRELOC)
		sect_OutputRelWord(expr);
	else if(expr->Flags & EXPRF_isCONSTANT)
	{
		sect_OutputAbsWord((UWORD)(expr->Value.Value));
		parse_FreeExpression(expr);
	}
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
}

void sect_OutputAbsLong(ULONG value)
{
	if(sect_CheckAvailableSpace(4))
	{
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				pCurrentSection->FreeSpace -= 4;
				pCurrentSection->UsedSpace += 4;
				switch(pOptions->Endian)
				{
					case ASM_LITTLE_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value >> 8);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value >> 16);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value >> 24);
						break;
					}
					case ASM_BIG_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value >> 24);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value >> 16);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value >> 8);
						pCurrentSection->pData[pCurrentSection->PC++] = (UBYTE)(value);
						break;
					}
					default:
					{
						internalerror("Unknown endianness");
						break;
					}
				}
				break;
			}
			case GROUP_BSS:
			{
				prj_Error(ERROR_SECTION_DATA);
				break;
			}
			default:
			{
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputRelLong(SExpression* expr)
{
	if(sect_CheckAvailableSpace(4))
	{
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				pCurrentSection->FreeSpace -= 4;
				pCurrentSection->UsedSpace += 4;
				switch(pOptions->Endian)
				{
					case ASM_LITTLE_ENDIAN:
					{
						patch_Create(pCurrentSection, pCurrentSection->PC, expr, PATCH_LLONG);
						break;
					}
					case ASM_BIG_ENDIAN:
					{
						patch_Create(pCurrentSection, pCurrentSection->PC, expr, PATCH_BLONG);
						break;
					}
					default:
					{
						internalerror("Unknown endianness");
						break;
					}
				}
				pCurrentSection->PC += 4;
				break;
			}
			case GROUP_BSS:
			{
				prj_Error(ERROR_SECTION_DATA);
				sect_SkipBytes(1);
				break;
			}
			default:
			{
				internalerror("Unknown GROUP type");
				break;
			}
		}
	}
}

void sect_OutputExprLong(SExpression* expr)
{
	if(expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if(expr->Flags & EXPRF_isRELOC)
		sect_OutputRelLong(expr);
	else if(expr->Flags & EXPRF_isCONSTANT)
	{
		sect_OutputAbsLong(expr->Value.Value);
		parse_FreeExpression(expr);
	}
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
}

void sect_OutputBinaryFile(char* s)
{
	FILE* f;

	fstk_FindFile(&s);

	if( s!=NULL
	&&	(f=fopen(s,"rb"))!=NULL )
	{
		ULONG	size;

		fseek(f, 0, SEEK_END);
		size=ftell(f);
		fseek(f, 0, SEEK_SET);

		if(sect_CheckAvailableSpace(size))
		{
			pCurrentSection->FreeSpace-=size;
			pCurrentSection->UsedSpace+=size;
			switch(sect_GetCurrentType())
			{
				case	GROUP_TEXT:
				{
					size_t read;

					read = fread((char*)&(pCurrentSection->pData[pCurrentSection->PC]), sizeof(UBYTE), size, f);
					pCurrentSection->PC+=size;
					if(read!=size)
					{
						prj_Fail(ERROR_READ);
					}
					break;
				}
				case	GROUP_BSS:
				{
					prj_Error(ERROR_SECTION_DATA);
					break;
				}
				default:
				{
					internalerror("Unknown GROUP type");
					break;
				}
			}
		}

		fclose(f);
	}
	else
	{
		prj_Fail(ERROR_NO_FILE);
	}
}

void sect_Align(SLONG align)
{
	SLONG t = pCurrentSection->PC + align - 1;
	t -= t % align;
	sect_SkipBytes(t - pCurrentSection->PC);
}

void	sect_SkipBytes(SLONG count)
{
	if(sect_CheckAvailableSpace(count))
	{
		//printf("*DEBUG* skipping %d bytes\n", count);
		switch(sect_GetCurrentType())
		{
			case	GROUP_TEXT:
			{
				if(pOptions->UninitChar!=-1)
				{
					while(count--)
						sect_OutputAbsByte((UBYTE)pOptions->UninitChar);
					return;
				}
				//	Fall through to GROUP_BSS
			}
			case	GROUP_BSS:
			{
				pCurrentSection->FreeSpace -= count;
				pCurrentSection->UsedSpace += count;
				pCurrentSection->PC += count;
				break;
			}
			default:
			{
				internalerror("Unknown GROUP type");
			}
		}
	}
}

BOOL	sect_SwitchTo(char* sectname, SSymbol* group)
{
	SSection* sect;

	sect=sect_Find(sectname,group);
	if(sect)
	{
		pCurrentSection=sect;
		return TRUE;
	}
	else
	{
		sect=sect_Create(sectname);
		if(sect)
		{
			sect->pGroup=group;
			sect->Flags=0;
		}
		pCurrentSection=sect;
		return sect!=NULL;
	}
}

BOOL	sect_SwitchTo_ORG(char* sectname, SSymbol* group, SLONG org)
{
	SSection* sect;

	sect=sect_Find(sectname,group);
	if(sect)
	{
		if(sect->Flags==SECTF_ORGFIXED && sect->Org==org)
		{
			pCurrentSection=sect;
			return TRUE;
		}
		else
		{
			prj_Fail(ERROR_SECT_EXISTS_ORG);
			return FALSE;
		}
	}
	else
	{
		sect=sect_Create(sectname);
		if(sect)
		{
			sect->pGroup=group;
			sect->Flags=SECTF_ORGFIXED;
			sect->Org=org;
		}
		pCurrentSection=sect;
		return sect!=NULL;
	}
}

#ifdef	HASBANKS
BOOL	sect_SwitchTo_BANK(char* sectname, SSymbol* group, SLONG bank)
{
	SSection* sect;

	sect=sect_Find(sectname,group);
	if(sect)
	{
		if(sect->Flags==SECTF_BANKFIXED && sect->Bank==bank)
		{
			pCurrentSection=sect;
			return TRUE;
		}
		else
		{
			prj_Fail(ERROR_SECT_EXISTS_BANK);
			return FALSE;
		}
	}
	else
	{
		sect=sect_Create(sectname);
		if(sect)
		{
			sect->pGroup=group;
			sect->Flags=SECTF_BANKFIXED;
			sect->Bank=bank;
		}
		pCurrentSection=sect;
		return sect!=NULL;
	}
}
#endif

#ifdef	HASBANKS
BOOL	sect_SwitchTo_ORG_BANK(char* sectname, SSymbol* group, SLONG org, SLONG bank)
{
	SSection* sect;

	sect=sect_Find(sectname,group);
	if(sect)
	{
		if(sect->Flags==(SECTF_BANKFIXED|SECTF_ORGFIXED) && sect->Bank==bank && sect->Org==org)
		{
			pCurrentSection=sect;
			return TRUE;
		}
		else
		{
			prj_Fail(ERROR_SECT_EXISTS_BANK_ORG);
			return FALSE;
		}
	}
	else
	{
		sect=sect_Create(sectname);
		if(sect)
		{
			sect->pGroup=group;
			sect->Flags=SECTF_BANKFIXED|SECTF_ORGFIXED;
			sect->Bank=bank;
			sect->Org=org;
		}
		pCurrentSection=sect;
		return sect!=NULL;
	}
}
#endif

BOOL	sect_SwitchTo_NAMEONLY(char* sectname)
{
	SSection* sect;

	sect=pCurrentSection=sect_Find(sectname,NULL);
	if(sect)
	{
		return TRUE;
	}
	else
	{
		prj_Fail(ERROR_NO_SECT);
		return FALSE;
	}
}

BOOL	sect_Init(void)
{
	pCurrentSection=NULL;
	pSectionList=NULL;
	return TRUE;
}