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
#include "parse.h"
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
	return token == T_DIRECTIVE_ENDM;
}

static bool
isEndr(EToken token) {
	return token == T_DIRECTIVE_ENDR;
}

static bool
skipPastDirective(bool (*directive)(EToken), bool (*getNextDirective)(void));

static bool
skipToDirective(bool (*directive)(EToken), bool (*getNextDirective)(void)) {
	for (;;) {
		if (directive(lex_Current.token)) {
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
				getNextDirective();
				if (!skipPastDirective(isEndc, getNextDirective))
					return false;
				break;
			}
			case T_SYM_MACRO: {
				getNextDirective();
				if (!skipPastDirective(isEndm, getNextDirective))
					return false;
				break;
			}
			case T_DIRECTIVE_REPT: {
				getNextDirective();
				if (!skipPastDirective(isEndr, getNextDirective))
					return false;
				break;
			}
			default: {
				if (!getNextDirective())
					return false;
				break;
			}
		}
	}
}

static bool
skipPastDirective(bool (*directive)(EToken), bool (*getNextDirective)(void)) {
	bool r = skipToDirective(directive, getNextDirective);
	if (r)
		getNextDirective();
	return r;
}

static bool
isFalseBranch(EToken token) {
	return token == T_DIRECTIVE_ELSE || token == T_DIRECTIVE_ENDC;
}

/* Public functions */


bool
parse_SkipPastTrueBranch(void) {
	if (lex_Current.token == '\n')
		lex_UnputChar('\n');
	if (skipToDirective(isFalseBranch, lex_GetNextDirective)) {
		parse_GetToken();
		return true;
	}
	return false;
}

bool
parse_SkipPastEndc(void) {
	if (lex_Current.token == '\n')
		lex_UnputChar('\n');
	if (skipToDirective(isEndc, lex_GetNextDirective)) {
		parse_GetToken();
		return true;
	}
	return false;
}

bool
parse_SkipPastEndr(void) {
	if (lex_Current.token == '\n')
		lex_UnputChar('\n');
	if (skipToDirective(isEndr, lex_GetNextDirective)) {
		parse_GetToken();
		return true;
	}
	return false;
}

static size_t
getNextDirectiveIndex;

static bool
getNextDirectiveIndexed(void) {
	return lex_GetNextDirectiveUnexpanded(&getNextDirectiveIndex);
}

static bool
skipToEndm(void) {
	return skipToDirective(isEndm, getNextDirectiveIndexed);
}

bool
parse_CopyMacroBlock(char** dest, size_t* size) {
	getNextDirectiveIndex = 0;
	lex_Current.token = T_NONE;
	skipToEndm();

    *dest = (char*) mem_Alloc(getNextDirectiveIndex + 1);
    lex_CopyUnexpandedContent(*dest, getNextDirectiveIndex);
	(*dest)[getNextDirectiveIndex] = 0;

    *size = getNextDirectiveIndex;

	lex_SkipBytes(getNextDirectiveIndex);

    return true;
}
