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

#ifndef XASM_COMMON_LEXER_VARIADICS_H_INCLUDED_
#define XASM_COMMON_LEXER_VARIADICS_H_INCLUDED_

#include "tokens.h"

typedef struct {
    bool (* callback)(size_t size);
    uint32_t token;
} SVariadicWordDefinition;

extern void
lex_VariadicInit(void);

extern uint32_t
lex_VariadicCreateWord(const SVariadicWordDefinition* tok);

extern void
lex_VariadicAddCharRange(uint32_t id, uint8_t start, uint8_t end, uint32_t charNumber);

extern void
lex_VariadicAddCharRangeRepeating(uint32_t id, uint8_t start, uint8_t end, uint32_t charNumber);

extern void
lex_VariadicAddSuffix(uint32_t id, uint8_t ch);

extern void
lex_VariadicRemoveAll(uint32_t id);

extern void
lex_VariadicMatchString(size_t bufferLength, size_t* length, const SVariadicWordDefinition** variadicWord);

#endif
