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

#include <memory.h>

/* From util */
#include "mem.h"

/* From xasm */
#include "parse_block.h"
#include "lexer.h"
#include "filestack.h"


/* Internal functions */

#define REPT_LEN 4
#define ENDR_LEN 4
#define ELSE_LEN 4
#define ENDC_LEN 4
#define MACRO_LEN 5
#define ENDM_LEN 4

static bool
isEndc(EToken token) {
	return token == T_DIRECTIVE_ENDC;
}

static bool
isEndm(EToken token) {
	return token == T_SYM_ENDM;
}

static bool
isEndr(EToken token) {
	return token == T_DIRECTIVE_ENDR;
}

static bool
skipPastDirective(bool (*directive)(EToken)) {
	for (;;)
		if (directive(lex_Current.token)) {
			lex_GetNextToken();
			return true;
		}

		switch (lex_Current.token) {
			case T_DIRECTIVE_IF:
			case T_DIRECTIVE_IFC:
			case T_DIRECTIVE_IFD:
			case T_DIRECTIVE_IFEQ:
			case T_DIRECTIVE_IFGE:
			case T_DIRECTIVE_IFGT:
			case T_DIRECTIVE_IFLE:
			case T_DIRECTIVE_IFLT:
			case T_DIRECTIVE_IFNC:
			case T_DIRECTIVE_IFND: {
				lex_GetNextDirective();
				return skipPastDirective(isEndc);
			}
			case T_SYM_MACRO: {
				lex_GetNextDirective();
				return skipPastDirective(isEndm);
			}
			case T_DIRECTIVE_REPT: {
				lex_GetNextDirective();
				return skipPastDirective(isEndr);
			}
			default: {
				if (!lex_GetNextDirective())
					return false;
				break;
			}
	}
}

static bool
isFalseBranch(EToken token) {
	return token == T_DIRECTIVE_ELSE || token == T_DIRECTIVE_ENDC;
}

/* Public functions */

bool
parse_SkipPastTrueBranch(void) {
	return skipPastDirective(isFalseBranch);
}

bool
parse_SkipPastEndc(void) {
	return skipPastDirective(isEndc);
}

bool
parse_SkipPastEndr(void) {
	return skipPastDirective(isEndr);
}

/*
bool
parse_CopyMacroBlock(char** dest, size_t* size) {
    size_t len = getMacroBodySize(0);

    *size = len;

    *dest = (char*) mem_Alloc(len + 1);
    fstk_Current->lineNumber += (uint32_t) lex_GetZeroTerminatedString(*dest, len);
    fstk_Current->lineNumber += (uint32_t) lex_SkipBytes(ENDM_LEN);
    return true;
}

*/
