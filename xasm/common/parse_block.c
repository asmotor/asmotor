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
skipPastDirective(bool (*directive)(EToken), bool (*getNextDirective)(void)) {
	for (;;)
		if (directive(lex_Current.token)) {
			lex_GetNextDirective();
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
				return skipPastDirective(isEndc, getNextDirective);
			}
			case T_SYM_MACRO: {
				lex_GetNextDirective();
				return skipPastDirective(isEndm, getNextDirective);
			}
			case T_DIRECTIVE_REPT: {
				lex_GetNextDirective();
				return skipPastDirective(isEndr, getNextDirective);
			}
			default: {
				if (!getNextDirective())
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
	return skipPastDirective(isFalseBranch, lex_GetNextDirective);
}

bool
parse_SkipPastEndc(void) {
	return skipPastDirective(isEndc, lex_GetNextDirective);
}

bool
parse_SkipPastEndr(void) {
	return skipPastDirective(isEndr, lex_GetNextDirective);
}

static size_t
getNextDirectiveIndex;

static bool
getNextDirectiveIndexed(void) {
	return lex_GetNextDirectiveUnexpanded(&getNextDirectiveIndex);
}

static bool
skipPastEndm(void) {
	return skipPastDirective(isEndr, getNextDirectiveIndexed);
}

bool
parse_CopyMacroBlock(char** dest, size_t* size) {
	getNextDirectiveIndex = 0;
	skipPastEndm();
    *size = getNextDirectiveIndex;
	lex_SkipBytes(getNextDirectiveIndex);

    *dest = (char*) mem_Alloc(*size + 1);
    lex_CopyUnexpandedContent(*dest, *size);
	dest[*size] = 0;

    return true;
}
