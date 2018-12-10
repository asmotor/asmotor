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

#if defined(__VBCC__) || defined(__GNUC__)

#include "asmotor.h"

#include <ctype.h>
#include <string.h>

#include "mem.h"

char*
_strdup(const char* str) {
    size_t l = strlen(str);
    char* r = mem_Alloc(l + 1);
    memcpy(r, str, l + 1);
    return r;
}

char*
_strupr(char* str) {
    char* r = str;
    while (*r) {
        *r = (char) toupper((unsigned char) *r);
        ++r;
    }
    return str;
}

char*
_strlwr(char* str) {
    char* r = str;
    while (*r) {
        *r = (char) tolower((unsigned char) *r);
        ++r;
    }
    return str;
}

int
_strnicmp(const char* string1, const char* string2, size_t length) {
    while (*string1 && *string2 && length-- > 0) {
        char l1 = (char) tolower((unsigned char) *string1++);
        char l2 = (char) tolower((unsigned char) *string2++);

        if (l1 != l2)
            return l1 - l2;
    }

    if (length == 0)
        return 0;

    return *string1 - *string2;
}

int
_stricmp(const char* string1, const char* string2) {
    while (*string1 && *string2) {
        char l1 = (char) tolower((unsigned char) *string1++);
        char l2 = (char) tolower((unsigned char) *string2++);

        if (l1 != l2)
            return l1 - l2;
    }

    return *string1 - *string2;
}

#endif
