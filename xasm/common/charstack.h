/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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

#ifndef XASM_COMMON_CHARSTACK_H_INCLUDED_
#define XASM_COMMON_CHARSTACK_H_INCLUDED_

#include <stdlib.h>
#include <assert.h>

#include "xasm.h"

typedef struct CharStack {
    char stack[MAX_STRING_SYMBOL_SIZE];
    size_t count;
} SCharStack;

INLINE void
chstk_Init(SCharStack* stack) {
	stack->count = 0;
}

extern SCharStack*
chstk_Create(void);

extern void
chstk_Copy(SCharStack* dest, const SCharStack* source);

INLINE void
chstk_Push(SCharStack* stack, char ch) {
    stack->stack[stack->count++] = ch;
}

extern void
chstk_PushString(SCharStack* stack, string* str);

INLINE size_t
chstk_Count(SCharStack* stack) {
    return stack->count;
}

INLINE char
chstk_Pop(SCharStack* stack) {
	if (stack->count > 0) {
	    return stack->stack[--(stack->count)];
	}
	return 0;
}

INLINE char
chstk_PeekAt(SCharStack* stack, size_t index) {
	if (stack->count > index) {
	    return stack->stack[stack->count - index - 1];
	}
	return 0;
}

INLINE char
chstk_Peek(SCharStack* stack) {
	return chstk_PeekAt(stack, 0);
}

extern size_t
chstk_Discard(SCharStack* stack, size_t count);


#endif /* XASM_COMMON_CHARSTACK_H_INCLUDED_ */
