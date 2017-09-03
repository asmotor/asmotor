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

#include "mem.h"
#include "strbuf.h"

stringbuffer* strbuf_Create(void)
{
	stringbuffer* pBuffer = mem_Alloc(sizeof(stringbuffer));
	pBuffer->nSize = 0;
	pBuffer->pBuffer = mem_Alloc(pBuffer->nAllocated = 32);
	
	return pBuffer;
}

void strbuf_Free(stringbuffer* pBuffer)
{
	mem_Free(pBuffer->pBuffer);
	mem_Free(pBuffer);
}

string* strbuf_String(stringbuffer* pBuffer)
{
	return str_CreateLength(pBuffer->pBuffer, pBuffer->nSize);
}

void strbuf_AppendChars(stringbuffer* pBuffer, char* pChars, int nCount)
{
	if(pChars == NULL)
		return;
		
	if(nCount + pBuffer->nSize > pBuffer->nAllocated)
	{
		int nNewSize = nCount + pBuffer->nSize;
		nNewSize += nNewSize >> 1;
		
		pBuffer->pBuffer = mem_Realloc(pBuffer->pBuffer, nNewSize);
		pBuffer->nAllocated = nNewSize;
	}
	
	memcpy(pBuffer->pBuffer + pBuffer->nSize, pChars, nCount);
	pBuffer->nSize += nCount;
}

