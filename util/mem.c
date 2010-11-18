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

#include <stdlib.h>
#include <stdio.h>
#include "memory.h"
#include "lists.h"

#define HEADERSIZE sizeof(SMemoryChunk)


typedef struct MemoryChunk
{
	list_Data(struct MemoryChunk);
	size_t	nSize;
} SMemoryChunk;

static SMemoryChunk* s_pMemory;

static void* CheckMemPointer(SMemoryChunk* pChunk)
{
	if(!pChunk)
	{
		fprintf(stderr, "Unable to allocate memory. Critical error, exiting.\n");
		exit(EXIT_FAILURE);
	}
	
	list_Insert(s_pMemory, pChunk);
	
	return (char*)pChunk + HEADERSIZE;
}

void* mem_Alloc(size_t nSize)
{
	return CheckMemPointer(malloc(nSize + HEADERSIZE));
}


void* mem_Realloc(void* pMem, size_t nSize)
{
	SMemoryChunk* pChunk = pMem == NULL ? NULL : (SMemoryChunk*)((char*)pMem - HEADERSIZE);
	if(pChunk)
	{
		list_Remove(s_pMemory, pChunk);
	}
	
	return CheckMemPointer(realloc(pChunk, nSize));
}


void mem_Free(void* pMem)
{
	if(!pMem)
		return;
		
	SMemoryChunk* pChunk = (SMemoryChunk*)((char*)pMem - HEADERSIZE);
	
	list_Remove(s_pMemory, pChunk);
	
	free(pChunk);
}
