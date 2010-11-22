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

#if !defined(UTIL_STR_H_INCLUDED_)
#define UTIL_STR_H_INCLUDED_

#include "asmotor.h"

typedef struct
{
	int nRefCount;
	int	nLength;
	char szData[];
} string;

extern string* str_Create(char* pszData);
extern string* str_CreateLength(char* pszData, int nLength);
extern string* str_Empty();
extern void str_Free(string* pString);
extern string* str_Concat(string* pString1, string* pString2);
extern string* str_Slice(string* pString1, int nIndex, int nLength);

INLINE string* str_Copy(string* pString)
{
	++pString->nRefCount;
	return pString;
}

INLINE int str_Length(string* pString)
{
	return pString->nLength;
}

INLINE char* str_String(string* pString)
{
	return pString->szData;
}

INLINE char str_CharAt(string* pString, int nIndex)
{
	return pString->szData[nIndex];
}

#endif
