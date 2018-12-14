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

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "asmotor.h"
#include "mem.h"
#include "str.h"

static string* g_emptyString = NULL;

static void
copyOnWrite(string** str) {
    if ((*str)->refCount != 1) {
        string* newString = str_CreateLength((*str)->data, (*str)->length);
        str_Free(*str);
        *str = newString;
    }
}

static void
str_Set(string* str, ssize_t index, char ch) {
    if (index < 0)
        index = str_Length(str) + index;
    str->data[index] = ch;
}

static string*
str_Alloc(size_t length) {
    string* pString = mem_Alloc(sizeof(string) + length + 1);
    pString->length = length;
    pString->refCount = 1;
    return pString;
}

string*
str_CreateLength(const char* data, size_t length) {
    string* str = str_Alloc(length);
    memcpy(str->data, data, length);
    str->data[length] = 0;
    return str;
}

string*
str_CreateStream(char (*nextChar)(void), size_t length) {
    string* str = str_Alloc(length);
    for (size_t i = 0; i < length; ++i) {
        str_Set(str, i, nextChar());
    }
    str->data[length] = 0;
    return str;
}

string*
str_Empty() {
    if (g_emptyString == NULL) {
        g_emptyString = str_Alloc(0);
        g_emptyString->data[0] = 0;
        return g_emptyString;
    }
    return str_Copy(g_emptyString);
}

void
str_Free(string* str) {
    if (str != NULL) {
        assert (str->refCount != 0);

        if (--str->refCount == 0)
            mem_Free(str);
    }
}

string*
str_Concat(const string* str1, const string* str2) {
    size_t length1 = str_Length(str1);
    size_t length2 = str_Length(str2);

    size_t newLength = length1 + length2;
    string* newString = str_Alloc(newLength);

    memcpy(newString->data, str_String(str1), length1);
    memcpy(&newString->data[length1], str_String(str2), length2 + 1);

    return newString;
}

string*
str_Slice(const string* str1, ssize_t index, size_t length) {
    if (index < 0)
        index = str_Length(str1) + index;

    if (index >= str_Length(str1))
        return str_Empty();

    if (index + length > str_Length(str1))
        length = str_Length(str1) - index;

    return str_CreateLength(str_String(str1) + index, length);
}

uint32_t
str_Find(const string* haystack, const string* needle) {
    char* p = strstr(str_String(haystack), str_String(needle));
    if (p != NULL) {
        return (uint32_t)(p - str_String(haystack));
    } else {
        return UINT32_MAX;
    }
}

bool
str_Equal(const string* str1, const string* str2) {
    size_t length1 = str_Length(str1);

    if (length1 != str_Length(str2))
        return false;

    for (size_t i = 0; i < length1; ++i) {
        if (str_CharAt(str1, i) != str_CharAt(str2, i))
            return false;
    }

    return true;
}

int
str_Compare(const string* str1, const string* str2) {
    const char* string1 = str_String(str1);
    const char* string2 = str_String(str2);

    while (*string1 && *string2) {
        uint8_t l1 = (uint8_t) *string1++;
        uint8_t l2 = (uint8_t) *string2++;

        if (l1 != l2)
            return l1 - l2;
    }

    return *string1 - *string2;
}

bool
str_EqualConst(const string* str1, const char* str2) {
    size_t length1 = str_Length(str1);
    char ch2;

    ch2 = *str2++;
    for (size_t i = 0; i < length1; ++i) {
        if (ch2 == 0 || str_CharAt(str1, i) != ch2)
            return false;

        ch2 = *str2++;
    }

    return ch2 == 0;
}

string*
str_Replace(const string* str, char search, char replace) {
    size_t length = str_Length(str);
    string* result = str_CreateLength(str->data, length);

    for (size_t i = 0; i < length; ++i) {
        if (str_CharAt(result, i) == search)
            str_Set(result, i, replace);
    }

    return result;
}

string*
str_ToLower(const string* str) {
    size_t length = str_Length(str);
    string* pLowerString = str_Alloc(length);

    for (size_t i = 0; i < length; ++i) {
        str_Set(pLowerString, i, (char) tolower(str_CharAt(str, i)));
    }

    return pLowerString;
}

void
str_TransformReplace(string** str, char (*transform)(char)) {
    copyOnWrite(str);

    size_t len = str_Length(*str);
    for (size_t i = 0; i < len; ++i) {
        str_Set(*str, i, transform(str_CharAt(*str, i)));
    }
}

INLINE char charToUpper(char ch) {
    return (char) toupper(ch);
}

INLINE char charToLower(char ch) {
    return (char) tolower(ch);
}

void
str_ToUpperReplace(string** str) {
    str_TransformReplace(str, charToUpper);
}

void
str_ToLowerReplace(string** str) {
    str_TransformReplace(str, charToLower);
}
