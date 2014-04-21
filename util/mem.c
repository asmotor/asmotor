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
#include "asmotor.h"
#include "mem.h"
#include "lists.h"

#define HEADERSIZE sizeof(SMemoryChunk)


typedef struct MemoryChunk
{
	list_Data(struct MemoryChunk);
	size_t nSize;
#if defined(_DEBUG)
	char* pszFile;
	int nLine;
#endif
} SMemoryChunk;

static SMemoryChunk* s_pMemory;


#if defined(_DEBUG)
static void* CheckMemPointer(SMemoryChunk* pChunk, size_t nSize, char* pszFile, int nLine)
#else
static void* CheckMemPointer(SMemoryChunk* pChunk, size_t nSize)
#endif
{
	if(!pChunk)
	{
		fprintf(stderr, "Unable to allocate memory. Critical error, exiting.\n");
		exit(EXIT_FAILURE);
	}
	
	pChunk->nSize = nSize;
#if defined(_DEBUG)
	pChunk->pszFile = pszFile;
	pChunk->nLine = nLine;
#endif
	list_Insert(s_pMemory, pChunk);
	
	return (char*)pChunk + HEADERSIZE;
}


#if defined(_DEBUG)
void* mem_AllocImpl(size_t nSize, char *pszFile, int nLine)
{
	return CheckMemPointer(malloc(nSize + HEADERSIZE), nSize, pszFile, nLine);
}
#else
void* mem_Alloc(size_t nSize)
{
	return CheckMemPointer(malloc(nSize + HEADERSIZE), nSize);
}
#endif

#if defined(_DEBUG)
void* mem_ReallocImpl(void* pMem, size_t nSize, char* pszFile, int nLine)
#else
void* mem_Realloc(void* pMem, size_t nSize)
#endif
{
	if(pMem == NULL)
	{
#if defined(_DEBUG)
		return mem_AllocImpl(nSize, pszFile, nLine);
#else
		return mem_Alloc(nSize);
#endif
	}
	else if(nSize == 0)
	{
		mem_Free(pMem);
		return NULL;
	}
	else
	{
		SMemoryChunk* pChunk = (SMemoryChunk*)((char*)pMem - HEADERSIZE);
		list_Remove(s_pMemory, pChunk);
	
#if defined(_DEBUG)
		return CheckMemPointer(realloc(pChunk, nSize + HEADERSIZE), nSize, pszFile, nLine);
#else
		return CheckMemPointer(realloc(pChunk, nSize + HEADERSIZE), nSize);
#endif
	}
}


void mem_Free(void* pMem)
{
	if(pMem != NULL)
	{
		SMemoryChunk* pChunk = (SMemoryChunk*)((char*)pMem - HEADERSIZE);

		if(pChunk->nSize == 0)
			internalerror("Chunk not allocated");

		list_Remove(s_pMemory, pChunk);
		pChunk->nSize = 0;
	
		free(pChunk);
	}
}
