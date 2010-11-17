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
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "asmotor.h"
#include "xasm.h"
#include "section.h"
#include "symbol.h"
#include "project.h"
#include "expr.h"
#include "patch.h"
#include "parse.h"
#include "options.h"
#include "fstack.h"




//	Private defines

#define	CHUNKSIZE	0x4000




//	Public variables

SSection* pCurrentSection;
SSection* pSectionList;




//	Private routines

static EGroupType sect_GetCurrentType(void)
{
	if(pCurrentSection == NULL)
		internalerror("No SECTION defined");

	if(pCurrentSection->pGroup == NULL)
		internalerror("No GROUP defined for SECTION");

	if(pCurrentSection->pGroup->Type == SYM_GROUP)
		return pCurrentSection->pGroup->Value.GroupType;
	else
		internalerror("SECTION's GROUP symbol is not of type SYM_GROUP");

	return -1;
}

static	SSection* sect_Create(char* name)
{
	SSection* sect;

	if((sect = malloc(sizeof(SSection))) != NULL)
	{
		memset(sect, 0, sizeof(SSection));
		sect->FreeSpace = g_pConfiguration->nMaxSectionSize;
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

static	void	sect_GrowCurrent(int32_t count)
{
	if(count+pCurrentSection->UsedSpace > pCurrentSection->AllocatedSpace)
	{
		int32_t	allocate;

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

static bool_t sect_CheckAvailableSpace(uint32_t count)
{
	if(pCurrentSection)
	{
		if(count<=pCurrentSection->FreeSpace)
		{
			if(sect_GetCurrentType()==GROUP_TEXT)
			{
				sect_GrowCurrent(count);
			}
			return true;
		}
		else
		{
			prj_Error(ERROR_SECTION_FULL);
			return false;
		}
	}
	else
	{
		prj_Error(ERROR_SECTION_MISSING);
		return false;
	}
}




//	Public routines

void sect_OutputConst8(uint8_t value)
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


void sect_OutputReloc8(SExpression* expr)
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


void sect_OutputExpr8(SExpression* expr)
{
	if(expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if(expr_IsRelocatable(expr))
		sect_OutputReloc8(expr);
	else if(expr_IsConstant(expr))
	{
		sect_OutputConst8((uint8_t)expr->Value.Value);
		expr_Free(expr);
	}
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
}

void sect_OutputConst16(uint16_t value)
{
	if(sect_CheckAvailableSpace(2))
	{
		pCurrentSection->FreeSpace -= 2;
		pCurrentSection->UsedSpace += 2;
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				switch(g_pOptions->Endian)
				{
					case ASM_LITTLE_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value>>8);
						break;
					}
					case ASM_BIG_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value>>8);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value);
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

void sect_OutputReloc16(SExpression* expr)
{
	if(sect_CheckAvailableSpace(2))
	{
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				pCurrentSection->FreeSpace -= 2;
				pCurrentSection->UsedSpace += 2;
				switch(g_pOptions->Endian)
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

void sect_OutputExpr16(SExpression* expr)
{
	if(expr == NULL)
		prj_Error(ERROR_EXPR_BAD);
	else if(expr_IsRelocatable(expr))
		sect_OutputReloc16(expr);
	else if(expr_IsConstant(expr))
	{
		sect_OutputConst16((uint16_t)(expr->Value.Value));
		expr_Free(expr);
	}
	else
		prj_Error(ERROR_EXPR_CONST_RELOC);
}

void sect_OutputConst32(uint32_t value)
{
	if(sect_CheckAvailableSpace(4))
	{
		switch(sect_GetCurrentType())
		{
			case GROUP_TEXT:
			{
				pCurrentSection->FreeSpace -= 4;
				pCurrentSection->UsedSpace += 4;
				switch(g_pOptions->Endian)
				{
					case ASM_LITTLE_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value >> 8);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value >> 16);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value >> 24);
						break;
					}
					case ASM_BIG_ENDIAN:
					{
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value >> 24);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value >> 16);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value >> 8);
						pCurrentSection->pData[pCurrentSection->PC++] = (uint8_t)(value);
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
				switch(g_pOptions->Endian)
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
	else if(expr_IsRelocatable(expr))
		sect_OutputRelLong(expr);
	else if(expr_IsConstant(expr))
	{
		sect_OutputConst32(expr->Value.Value);
		expr_Free(expr);
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
		uint32_t	size;

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

					read = fread((char*)&(pCurrentSection->pData[pCurrentSection->PC]), sizeof(uint8_t), size, f);
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

void sect_Align(int32_t align)
{
	int32_t t = pCurrentSection->PC + align - 1;
	t -= t % align;
	sect_SkipBytes(t - pCurrentSection->PC);
}

void	sect_SkipBytes(int32_t count)
{
	if(sect_CheckAvailableSpace(count))
	{
		//printf("*DEBUG* skipping %d bytes\n", count);
		switch(sect_GetCurrentType())
		{
			case	GROUP_TEXT:
			{
				if(g_pOptions->UninitChar!=-1)
				{
					while(count--)
						sect_OutputConst8((uint8_t)g_pOptions->UninitChar);
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

bool_t	sect_SwitchTo(char* sectname, SSymbol* group)
{
	SSection* sect;

	sect=sect_Find(sectname,group);
	if(sect)
	{
		pCurrentSection=sect;
		return true;
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

bool_t	sect_SwitchTo_ORG(char* sectname, SSymbol* group, int32_t org)
{
	SSection* sect;

	sect=sect_Find(sectname,group);
	if(sect)
	{
		if(sect->Flags==SECTF_ORGFIXED && sect->Org==org)
		{
			pCurrentSection=sect;
			return true;
		}
		else
		{
			prj_Fail(ERROR_SECT_EXISTS_ORG);
			return false;
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

bool_t sect_SwitchTo_BANK(char* sectname, SSymbol* group, int32_t bank)
{
	SSection* sect;

	if(!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	sect = sect_Find(sectname, group);
	if(sect)
	{
		if(sect->Flags == SECTF_BANKFIXED && sect->Bank == bank)
		{
			pCurrentSection = sect;
			return true;
		}

		prj_Fail(ERROR_SECT_EXISTS_BANK);
		return false;
	}

	sect = sect_Create(sectname);
	if(sect)
	{
		sect->pGroup = group;
		sect->Flags = SECTF_BANKFIXED;
		sect->Bank = bank;
	}
	pCurrentSection = sect;
	return sect != NULL;
}

bool_t sect_SwitchTo_ORG_BANK(char* sectname, SSymbol* group, int32_t org, int32_t bank)
{
	SSection* sect;

	if(!g_pConfiguration->bSupportBanks)
		internalerror("Banks not supported");

	sect = sect_Find(sectname,group);
	if(sect)
	{
		if(sect->Flags== (SECTF_BANKFIXED | SECTF_ORGFIXED)
		&& sect->Bank == bank
		&& sect->Org == org)
		{
			pCurrentSection = sect;
			return true;
		}

		prj_Fail(ERROR_SECT_EXISTS_BANK_ORG);
		return false;
	}

	sect=sect_Create(sectname);
	if(sect)
	{
		sect->pGroup = group;
		sect->Flags = SECTF_BANKFIXED | SECTF_ORGFIXED;
		sect->Bank = bank;
		sect->Org = org;
	}
	pCurrentSection=sect;
	return sect!=NULL;
}

bool_t	sect_SwitchTo_NAMEONLY(char* sectname)
{
	SSection* sect;

	sect=pCurrentSection=sect_Find(sectname,NULL);
	if(sect)
	{
		return true;
	}
	else
	{
		prj_Fail(ERROR_NO_SECT);
		return false;
	}
}

bool_t	sect_Init(void)
{
	pCurrentSection=NULL;
	pSectionList=NULL;
	return true;
}
