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

#include "asmotor.h"
#include "mem.h"
#include "str.h"

static string* s_pEmptyString = NULL;

INLINE string* str_Alloc(int nLength)
{
	string* pString = mem_Alloc(sizeof(string) + nLength + 1);
	pString->nLength = nLength;
	pString->nRefCount = 1;
	return pString;
}

string* str_Create(char* pszData)
{
	int nLength = (int)strlen(pszData);
	string* pString = str_Alloc(nLength);
	memcpy(pString->szData, pszData, nLength + 1);
	return pString;
}

string* str_CreateLength(char* pszData, int nLength)
{
	string* pString = str_Alloc(nLength);
	memcpy(pString->szData, pszData, nLength);
	pString->szData[nLength] = 0;
	return pString;
}

string* str_Empty()
{
	if(s_pEmptyString == NULL)
	{
		s_pEmptyString = str_Alloc(0);
		s_pEmptyString->szData[0] = 0;
		return s_pEmptyString;
	}
	return str_Copy(s_pEmptyString);
}

void str_Free(string* pString)
{
	if(pString != NULL && --pString->nRefCount == 0)
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

string* str_Slice(string* pString, int nIndex, int nLength)
{
	if(nIndex < 0)
		nIndex = str_Length(pString) + nIndex;
		
	if(nIndex >= str_Length(pString))
		return str_Empty();

	if(nIndex + nLength > str_Length(pString))
		nLength = str_Length(pString) - nIndex;

	return str_CreateLength(str_String(pString) + nIndex, nLength);
}

bool_t str_Equal(string* pString1, string* pString2)
{
	int i;
	int len = str_Length(pString1);
	
	if(len != str_Length(pString2))
		return false;
		
	for(i = 0; i < len; ++i)
	{
		if(str_CharAt(pString1, i) != str_CharAt(pString2, i))
			return false;
	}

	return true;
}

string* str_Replace(string* pString, char search, char replace)
{
	int i;
	int len = str_Length(pString);
	
	pString = str_CreateLength(pString->szData, len);
	for(i = 0; i < len; ++i)
	{
		if(str_CharAt(pString, i) == search)
			str_Set(pString, i, replace);
	}
	
	return pString;
}
