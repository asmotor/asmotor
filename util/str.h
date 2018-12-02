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

#if !defined(UTIL_STR_H_INCLUDED_)
#define UTIL_STR_H_INCLUDED_

#include "asmotor.h"

typedef struct {
	int nRefCount;
	size_t nLength;
	char szData[];
} string;

extern string* str_Create(const char* pszData);
extern string* str_CreateLength(const char* pszData, size_t nLength);
extern string* str_Empty(void);
extern void str_Free(string* pString);
extern string* str_Concat(const string* pString1, const string* pString2);
extern string* str_Slice(const string* pString1, ssize_t nIndex, size_t nLength);
extern bool_t str_Equal(const string* pString1, const string* pString2);
extern bool_t str_EqualConst(const string* pString1, const char* pString2);
extern string* str_Replace(const string* pString, char search, char replace);
extern string* str_ToLower(const string* pString);
extern void str_ToUpperReplace(string** ppString);

INLINE string* str_Copy(string* pString) {
	if (pString != NULL)
		++pString->nRefCount;
	return pString;
}

INLINE size_t str_Length(const string* pString) {
	return pString->nLength;
}

INLINE const char* str_String(const string* pString) {
	return pString->szData;
}

INLINE char str_CharAt(const string* pString, ssize_t nIndex) {
	if (nIndex < 0)
		nIndex = str_Length(pString) + nIndex;
	return pString->szData[nIndex];
}

INLINE void str_Set(string* pString, ssize_t nIndex, char ch) {
	if (nIndex < 0)
		nIndex = str_Length(pString) + nIndex;
	pString->szData[nIndex] = ch;
}

INLINE void str_Assign(string** ppDest, string* pSrc) {
	str_Free(*ppDest);
	*ppDest = str_Copy(pSrc);
}

INLINE void str_Move(string** ppDest, string** ppSrc) {
	str_Free(*ppDest);
	*ppDest = *ppSrc;
	*ppSrc = NULL;
}

#define STR_ASSIGN(p, str)    str_Assign(&p, str)
#define STR_MOVE(p, str)    str_Move(&p, &str)

#endif
