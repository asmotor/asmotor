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

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "str.h"

#include "lexer_context.h"
#include "lexer.h"
#include "parse.h"
#include "parse_block.h"
#include "parse_directive.h"
#include "parse_string.h"
#include "parse_symbol.h"
#include "errors.h"
#include "symbol.h"

bool
parse_ExpandStrings = true;

static bool
handleMacroArgument() {
    if (lex_Context->token.id == T_STRING) {
		string* arg = lex_TokenString();
        lexctx_AddMacroArgument(arg);
		str_Free(arg);
        parse_GetToken();
        return true;
    } else {
        return false;
    }
}

static bool
handleMacroArguments() {
    lex_SetMode(LEXER_MODE_MACRO_ARGUMENT0);
    parse_GetToken();

    if (lex_Context->token.id == T_MACROARG0) {
		string* arg = lex_TokenString();
        lexctx_SetMacroArgument0(arg);
		str_Free(arg);
        parse_GetToken();
    }

    if (handleMacroArgument()) {
        while (lex_Context->token.id == ',') {
            parse_GetToken();
            if (!handleMacroArgument()) {
                err_Error(ERROR_INVALID_MACRO_ARGUMENT);
                break;
            }
        }
    }

    lex_SetMode(LEXER_MODE_NORMAL);
    return lex_Context->token.id == '\n';
}

static bool
handleMacroInvocation(void) {
    bool r = false;
    if (lex_Context->token.id == T_ID) {
        string* symbolName = lex_TokenString();

        if (sym_IsMacro(symbolName)) {
            if (handleMacroArguments()) {
                lexctx_ProcessMacro(symbolName);
                parse_GetToken();
                r = true;
            }
        } else {
            err_Error(ERROR_INSTR_UNKNOWN, lex_Context->token.value.string);
        }
        str_Free(symbolName);
    }
    return r;
}

static bool
handleLineBreak() {
    if (lex_Context->token.id == '\n') {
        parse_GetToken();
        lex_Context->lineNumber += 1;
        xasm_TotalLines += 1;
        return true;
    }
    return false;
}


/* Public functions */

bool
parse_IsDot(void) {
    if (lex_Context->token.id == '.') {
        parse_GetToken();
        return true;
    }

    if (lex_Context->token.id == T_ID && lex_Context->token.value.string[0] == '.') {
        lex_UnputString(lex_Context->token.value.string + 1);
        parse_GetToken();
        return true;
    }

    return false;
}

bool
parse_ExpectChar(char ch) {
    if (lex_Context->token.id == (uint32_t) ch) {
        parse_GetToken();
        return true;
    } else {
        err_Error(ERROR_CHAR_EXPECTED, ch);
        return false;
    }
}

void
parse_GetToken(void) {
	for (;;) {
		if (!lex_GetNextToken()) {
			err_Error(ERROR_END_OF_FILE);
		}
		if (!parse_ExpandStrings || lex_Context->token.id != T_ID)
			break;

		string* symbolName = lex_TokenString();
		string* value = sym_GetStringSymbolValueByName(symbolName);
		str_Free(symbolName);

		if (value == NULL)
			break;

		lex_UnputStringLength(str_String(value), str_Length(value));
		str_Free(value);
	}
}

bool
parse_Until(EToken endToken) {
    while (lex_Context->token.id) {
        if (xasm_Configuration->parseInstruction())
            continue;

        if (parse_SymbolDefinition())
            continue;

        if (parse_Directive())
            continue;

        if (handleMacroInvocation())
            continue;

        if (handleLineBreak())
            continue;

        if (lex_Context->token.id == endToken)
            return true;

        return err_Error(ERROR_SYNTAX);
    }

    return true;
}

bool
parse_Do(void) {
    parse_GetToken();
    return parse_Until(T_POP_END);
}
