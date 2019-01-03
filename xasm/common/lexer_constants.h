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

#ifndef XASM_COMMON_LEXER_CONSTANTS_H_INCLUDED_
#define XASM_COMMON_LEXER_CONSTANTS_H_INCLUDED_

#include <stdint.h>

#include "lists.h"
#include "str.h"

#include "tokens.h"

typedef struct {
    const char* name;
    uint32_t token;
} SLexConstantsWord;


extern void
lex_ConstantsDefineWord(const char* name, uint32_t token);

extern void
lex_ConstantsDefineWords(const SLexConstantsWord* lex);

extern void
lex_ConstantsUndefineWord(const char* name, uint32_t token);

extern void
lex_ConstantsUndefineWords(const SLexConstantsWord* lex);

extern void
lex_ConstantsMatchWord(size_t bufferLength, size_t* length, const SLexConstantsWord** word);

extern void
lex_ConstantsInit(void);

#endif
