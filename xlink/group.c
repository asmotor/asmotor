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

#include <string.h>

SMachineGroup* pMachineGroups = NULL;

static SMachineGroup* creategroup(char* name, uint32_t totalbanks)
{
	SMachineGroup** ppgroup;

	ppgroup = &pMachineGroups;
	while(*ppgroup)
	{
		ppgroup = &(*ppgroup)->pNext;
	}

	*ppgroup = (SMachineGroup*)mem_Alloc(sizeof(SMachineGroup) + sizeof(SMemoryPool*) * totalbanks);
	strcpy((*ppgroup)->Name, name);
	(*ppgroup)->pNext = NULL;
	(*ppgroup)->TotalPools = totalbanks;

	return* ppgroup;
}

static SMemoryPool* createpool(void)
{
	SMemoryPool* pool = (SMemoryPool*)mem_Alloc(sizeof(SMemoryPool));
	pool->pFreeChunks = NULL;
	return pool;
}

bool_t alloc_from_pool(SMemoryPool* pool, uint32_t size, uint32_t* org)
{
	SMemChunk* pchunk;
	
	for(pchunk = pool->pFreeChunks; pchunk != NULL; pchunk = pchunk->pNext)
	{
		if(pchunk->Size >= size)
		{
			*org = pchunk->Org;
			pchunk->Org += size;
			pchunk->Size -= size;

			return true;
		}
	}

	return false;
}

bool_t alloc_abs_from_pool(SMemoryPool* pool, uint32_t size, uint32_t org)
{
	SMemChunk** ppchunk = &pool->pFreeChunks;

	while(*ppchunk != NULL)
	{
		SMemChunk* pchunk = *ppchunk;

		if(org >= pchunk->Org && org + size <= pchunk->Org + pchunk->Size)
		{
			SMemChunk* newchunk = (SMemChunk*)mem_Alloc(sizeof(SMemChunk));

			newchunk->pNext = pchunk->pNext;
			pchunk->pNext = newchunk;

			newchunk->Org = org + size;
			newchunk->Size = pchunk->Org + pchunk->Size - (org + size);

			pchunk->Size = org - pchunk->Org;

			return true;
		}
		ppchunk = &pchunk->pNext;
	}

	return false;
}

void group_Alloc(SSection* sect)
{
	SMachineGroup* group;
	char* groupname;
	int foundgroup = 0;

	if(sect->GroupID == -1)
	{
		//	This is a special exported EQU symbol section

		sect->Position = 0;
		sect->BasePC = 0;
		sect->Bank = 0;
		sect->ImageOffset = -1;
		sect->Assigned = true;

		return;
	}

	groupname = sect->pGroups->Groups[sect->GroupID].Name;

	for(group = pMachineGroups; group != NULL; group = group->pNext)
	{
		if(strcmp(group->Name, groupname) == 0)
		{
			int32_t i;
			foundgroup = 1;

			for(i = 0; i < group->TotalPools; ++i)
			{
				SMemoryPool* pool = group->Pool[i];

				if(sect->Bank == -1 || sect->Bank == pool->BankId)
				{
					if(sect->Position == -1)
					{
						uint32_t position;

						if(alloc_from_pool(pool, sect->Size, &position))
						{
							sect->Position = position;
							sect->BasePC = position / sect->MinimumWordSize;
							sect->Bank = pool->BankId;
							sect->ImageOffset = pool->ImageOffset == -1 ? -1 : pool->ImageOffset + sect->Position - pool->AddressingOffset;
							sect->Assigned = true;
							return;
						}
					}
					else if(alloc_abs_from_pool(pool, sect->Size, sect->Position))
					{
						sect->Bank = pool->BankId;
						sect->ImageOffset = pool->ImageOffset == -1 ? -1 : pool->ImageOffset + sect->Position - pool->AddressingOffset;
						sect->Assigned = true;
						return;
					}
				}
			}
		}
	}

	if(foundgroup)
		Error("No free space for section \"%s\"", sect->Name);
	else
		Error("Section \"%s\" uses undefined group \"%s\"", sect->Name, groupname);
}

static void init_memchunks(void)
{
	SMachineGroup* group;

	for(group = pMachineGroups; group != NULL; group=group->pNext)
	{
		int32_t i;

		for(i = 0; i < group->TotalPools; ++i)
		{
			SMemoryPool* pool = group->Pool[i];

			if((pool->pFreeChunks = (SMemChunk*)mem_Alloc(sizeof(SMemChunk))) != NULL)
			{
				pool->pFreeChunks->Org = pool->AddressingOffset;
				pool->pFreeChunks->Size = pool->Size;
				pool->pFreeChunks->pNext = NULL;
			}
		}
	}
}

static void group_SetupCommonGameboy(void)
{
	SMachineGroup* group;

	//	Create VRAM group

	group = creategroup("VRAM", 1);
	group->Pool[0] = createpool();
	group->Pool[0]->ImageOffset = -1;
	group->Pool[0]->BankId = 0;
	group->Pool[0]->Size = group->Pool[0]->Available = 0x2000;
	group->Pool[0]->AddressingOffset = 0x8000;

	//	Create BSS group

	group = creategroup("BSS", 1);
	group->Pool[0] = createpool();
	group->Pool[0]->ImageOffset = -1;
	group->Pool[0]->BankId = 0;
	group->Pool[0]->Size = group->Pool[0]->Available = 0x2000;
	group->Pool[0]->AddressingOffset = 0xC000;

	//	Create HRAM group

	group = creategroup("HRAM", 1);
	group->Pool[0] = createpool();
	group->Pool[0]->ImageOffset = -1;
	group->Pool[0]->BankId = 0;
	group->Pool[0]->Size = group->Pool[0]->Available = 0x007F;
	group->Pool[0]->AddressingOffset = 0xFF80;
}

void	group_SetupGameboy(void)
{
	int i;
	SMemoryPool* codepools[256];
	SMachineGroup* group;

	for(i = 0; i < 256; ++i)
	{
		codepools[i] = createpool();
		codepools[i]->ImageOffset = i * 0x4000;
		codepools[i]->BankId = i;
		codepools[i]->Size = codepools[i]->Available = 0x4000;
		codepools[i]->AddressingOffset = i == 0 ? 0x0000 : 0x4000;
	}
	
	//	Create HOME group

	group = creategroup("HOME", 1);
	group->Pool[0] = codepools[0];

	//	Create CODE group

	group = creategroup("CODE", 256);
	for(i = 0; i < 256; ++i)
		group->Pool[i] = codepools[i];

	//	Create DATA group

	group = creategroup("DATA", 256);
	for(i = 0; i < 256; ++i)
		group->Pool[i] = codepools[i];

	// Create VRAM, BSS and HRAM

	group_SetupCommonGameboy();

	//	initialise memory chunks

	init_memchunks();
}

void	group_SetupSmallGameboy(void)
{
	int i;
	SMemoryPool* codepool;
	SMachineGroup* group;

	codepool = createpool();
	codepool->ImageOffset = 0;
	codepool->BankId = 1;
	codepool->Size = codepool->Available = 0x8000;
	codepool->AddressingOffset = 0x0000;
	
	//	Create HOME group

	group = creategroup("HOME", 1);
	group->Pool[0]=codepool;

	//	Create CODE group

	group = creategroup("CODE", 256);
	for(i = 0; i < 256; ++i)
		group->Pool[i] = codepool;

	//	Create DATA group

	group = creategroup("DATA", 256);
	for(i = 0; i < 256; ++i)
		group->Pool[i] = codepool;

	// Create VRAM, BSS and HRAM

	group_SetupCommonGameboy();

	//	initialise memory chunks

	init_memchunks();
}


void group_SetupAmigaExecutable(void)
{
	SMachineGroup* group;
	SMemoryPool* codepool;
	SMemoryPool* chippool;

	codepool = createpool();
	codepool->ImageOffset = 0;
	codepool->BankId = 0;
	codepool->Size = codepool->Available = 0x3FFFFFFF;
	codepool->AddressingOffset = 0;

	chippool = createpool();
	chippool->ImageOffset = 0x40000000;
	chippool->BankId = 1;
	chippool->Size = chippool->Available = 0x3FFFFFFF;
	chippool->AddressingOffset = 0;

	//	Create CODE group
	group = creategroup("CODE", 1);
	group->Pool[0] = codepool;

	//	Create CODE_C group
	group = creategroup("CODE_C", 1);
	group->Pool[0] = chippool;

	//	Create DATA group
	group = creategroup("DATA", 1);
	group->Pool[0] = codepool;

	//	Create DATA_C group
	group = creategroup("DATA_C", 1);
	group->Pool[0] = chippool;

	//	Create BSS group
	group = creategroup("BSS", 1);
	group->Pool[0] = createpool();
	group->Pool[0]->ImageOffset = -1;
	group->Pool[0]->BankId = 0;
	group->Pool[0]->Size = group->Pool[0]->Available = 0x3FFFFFFF;
	group->Pool[0]->AddressingOffset = 0;

	//	Create BSS_C group
	group = creategroup("BSS_C", 1);
	group->Pool[0] = createpool();
	group->Pool[0]->ImageOffset = -1;
	group->Pool[0]->BankId = 1;
	group->Pool[0]->Size = group->Pool[0]->Available = 0x3FFFFFFF;
	group->Pool[0]->AddressingOffset = 0;
}
