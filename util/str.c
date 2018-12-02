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

#include <string.h>
#include <ctype.h>

#include "asmotor.h"
#include "mem.h"
#include "str.h"

static string* s_pEmptyString = NULL;

INLINE string* str_Alloc(size_t nLength) {
	string* pString = mem_Alloc(sizeof(string) + nLength + 1);
	pString->nLength = nLength;
	pString->nRefCount = 1;
	return pString;
}

string* str_Create(const char* pszData) {
	size_t nLength = strlen(pszData);
	string* pString = str_Alloc(nLength);
	memcpy(pString->szData, pszData, nLength + 1);
	return pString;
}

string* str_CreateLength(const char* pszData, size_t nLength) {
	string* pString = str_Alloc(nLength);
	memcpy(pString->szData, pszData, nLength);
	pString->szData[nLength] = 0;
	return pString;
}

string* str_Empty() {
	if (s_pEmptyString == NULL) {
		s_pEmptyString = str_Alloc(0);
		s_pEmptyString->szData[0] = 0;
		return s_pEmptyString;
	}
	return str_Copy(s_pEmptyString);
}

void str_Free(string* pString) {
	if (pString != NULL && --pString->nRefCount == 0)
		mem_Free(pString);
}

string* str_Concat(const string* pString1, const string* pString2) {
	size_t nLength1 = str_Length(pString1);
	size_t nLength2 = str_Length(pString2);
	size_t nLength = nLength1 + nLength2;

	string* pString = str_Alloc(nLength);
	memcpy(pString->szData, str_String(pString1), nLength1);
	memcpy(&pString->szData[nLength1], str_String(pString2), nLength2 + 1);

	return pString;
}

string* str_Slice(const string* pString, ssize_t nIndex, size_t nLength) {
	if (nIndex < 0)
		nIndex = str_Length(pString) + nIndex;

	if (nIndex >= str_Length(pString))
		return str_Empty();

	if (nIndex + nLength > str_Length(pString))
		nLength = str_Length(pString) - nIndex;

	return str_CreateLength(str_String(pString) + nIndex, nLength);
}

bool_t str_Equal(const string* pString1, const string* pString2) {
	size_t length1 = str_Length(pString1);

	if (length1 != str_Length(pString2))
		return false;

	for (size_t i = 0; i < length1; ++i) {
		if (str_CharAt(pString1, i) != str_CharAt(pString2, i))
			return false;
	}

	return true;
}

bool_t str_EqualConst(const string* pString1, const char* pString2) {
	size_t len = str_Length(pString1);
	char ch2;

	ch2 = *pString2++;
	for (size_t i = 0; i < len; ++i) {
		if (ch2 == 0 || str_CharAt(pString1, i) != ch2)
			return false;

		ch2 = *pString2++;
	}

	return ch2 == 0;
}

string* str_Replace(const string* pString, char search, char replace) {
	size_t len = str_Length(pString);
	string* result = str_CreateLength(pString->szData, len);

	for (size_t i = 0; i < len; ++i) {
		if (str_CharAt(result, i) == search)
			str_Set(result, i, replace);
	}

	return result;
}

string* str_ToLower(const string* pString) {
	size_t len = str_Length(pString);
	string* pLowerString = str_Alloc(len);

	for (size_t i = 0; i < len; ++i) {
		str_Set(pLowerString, i, (char)tolower(str_CharAt(pString, i)));
	}

	return pLowerString;
}

void str_ToUpperReplace(string** ppString) {
	if ((*ppString)->nRefCount != 1) {
		string* newString = str_CreateLength((*ppString)->szData, (*ppString)->nLength);
		str_Free(*ppString);
		*ppString = newString;
	}

	size_t len = str_Length(*ppString);

	for (size_t i = 0; i < len; ++i) {
		str_Set(*ppString, i, (char) toupper(str_CharAt(*ppString, i)));
	}
}

