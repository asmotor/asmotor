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

#include <string.h>
#include "mem.h"
#include "str.h"

INLINE string* str_Alloc(int nLength)
{
	string* pString = mem_Alloc(sizeof(string) + nLength + 1);
	pString->nLength = nLength;
	pString->nRefCount = 1;
	return pString;
}

string* str_Create(char* pszData)
{
	int nLength = strlen(pszData);
	string* pString = str_Alloc(nLength);
	memcpy(pString->szData, pszData, nLength + 1);
	return pString;
}

void str_Free(string* pString)
{
	if(--pString->nRefCount == 0)
		mem_Free(pString);
}

string* str_Concat(string* pString1, string* pString2)
{
	int nLength1 = str_Length(pString1);
	int nLength2 = str_Length(pString2);
	int nLength = nLength1 + nLength2;
	string* pString = str_Alloc(nLength);
	memcpy(pString->szData, str_String(pString1), nLength1);
	memcpy(&pString->szData[nLength1], str_String(pString2), nLength2 + 1);
	return pString;
}
