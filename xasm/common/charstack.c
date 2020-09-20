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
#include "str.h"

#include "charstack.h"


extern SCharStack*
chstk_Create(void) {
	SCharStack* stack = (SCharStack*) mem_Alloc(sizeof(SCharStack));
	chstk_Init(stack);
	return stack;
}


extern void
chstk_Copy(SCharStack* dest, const SCharStack* source) {
    memcpy(dest->stack, source->stack, source->count);
    dest->count = source->count;
}


extern size_t
chstk_Discard(SCharStack* stack, size_t count) {
	if (count < stack->count) {
		stack->count -= count;
		return count;
	} else {
		size_t discarded = stack->count;
		stack->count = 0;
		return discarded;
	}
}

extern void
chstk_PushString(SCharStack* stack, string* str) {
	for (ssize_t i = str_Length(str); i >= 0; --i) {
		chstk_Push(stack, str_CharAt(str, i));
	}
}
