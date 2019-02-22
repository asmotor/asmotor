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

#include <string.h>

#include "asmotor.h"

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable: 4200)
#endif

typedef struct {
    uint32_t refCount;
    size_t length;
    char data[];
} string;

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

extern string*
str_CreateLength(const char* data, size_t length);

extern string*
str_CreateStream(char (*nextChar)(void), size_t length);

extern string*
str_Empty(void);

extern void
str_Free(string* str);

extern string*
str_Concat(const string* str1, const string* str2);

extern string*
str_Slice(const string* str1, ssize_t index, size_t length);

extern uint32_t
str_Find(const string* haystack, const string* needle);

extern bool
str_Equal(const string* str1, const string* str2);

extern int
str_Compare(const string* str1, const string* str2);

extern bool
str_EqualConst(const string* str1, const char* str2);

extern string*
str_Replace(const string* str, char search, char replace);

extern string*
str_ToLower(const string* str);

extern void
str_TransformReplace(string** str, char (*transform)(char));

extern void
str_ToUpperReplace(string** str);

extern void
str_ToLowerReplace(string** str);

extern string*
str_CreateSpaces(uint32_t count);

extern string*
str_Align(string* str, int32_t alignment);

INLINE bool
str_NotEqual(const string* str1, const string* str2) {
    return !str_Equal(str1, str2);
}

INLINE string*
str_Create(const char* data) {
    return str_CreateLength(data, strlen(data));
}

INLINE string*
str_Copy(const string* str) {
    if (str != NULL)
        ++((string*) str)->refCount;
    return (string*) str;
}

INLINE size_t
str_Length(const string* str) {
    return str->length;
}

INLINE const char*
str_String(const string* str) {
    return str->data;
}

INLINE char
str_CharAt(const string* str, ssize_t index) {
    if (index < 0)
        index = str_Length(str) + index;
    return str->data[index];
}

INLINE void
str_Assign(string** dest, const string* src) {
    str_Free(*dest);
    *dest = str_Copy(src);
}

INLINE void
str_Move(string** dest, string** src) {
    str_Free(*dest);
    *dest = *src;
    *src = NULL;
}


#define STR_ASSIGN(p, str) str_Assign(&(p), (str))
#define STR_MOVE(p, str)   str_Move(&(p), &(str))

#endif
