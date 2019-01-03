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

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "str.h"

#include "filestack.h"
#include "lexer.h"
#include "parse.h"
#include "parse_block.h"
#include "parse_directive.h"
#include "parse_string.h"
#include "parse_symbol.h"
#include "project.h"
#include "symbol.h"

extern bool
parse_TargetSpecific(void);

static bool
handleMacroArgument() {
    if (lex_Current.token == T_STRING) {
        fstk_AddMacroArgument(lex_Current.value.string);
        parse_GetToken();
        return true;
    } else {
        return false;
    }
}

static bool
handleMacroArguments() {
    lex_SetState(LEX_STATE_MACRO_ARGUMENT0);
    parse_GetToken();

    if (lex_Current.token == T_MACROARG0) {
        fstk_SetMacroArgument0(lex_Current.value.string);
        parse_GetToken();
    }

    if (handleMacroArgument()) {
        while (lex_Current.token == ',') {
            parse_GetToken();
            if (!handleMacroArgument()) {
                prj_Error(ERROR_INVALID_MACRO_ARGUMENT);
                break;
            }
        }
    }

    lex_SetState(LEX_STATE_NORMAL);
    return lex_Current.token == '\n';
}

static bool
handleMacroInvocation(void) {
    bool r = false;
    if (lex_Current.token == T_ID) {
        string* symbolName = str_Create(lex_Current.value.string);

        if (sym_IsMacro(symbolName)) {
            if (handleMacroArguments()) {
                fstk_ProcessMacro(symbolName);
                parse_GetToken();
                r = true;
            }
        } else {
            prj_Error(ERROR_INSTR_UNKNOWN, lex_Current.value.string);
        }
        str_Free(symbolName);
    }
    return r;
}

static bool
handleLineBreak() {
    if (lex_Current.token == '\n') {
        lex_GetNextToken();
        fstk_Current->lineNumber += 1;
        g_nTotalLines += 1;
        return true;
    }
    return false;
}


/* Public functions */

bool
parse_IsDot(void) {
    if (lex_Current.token == '.') {
        parse_GetToken();
        return true;
    }

    if (lex_Current.token == T_ID && lex_Current.value.string[0] == '.') {
        lex_UnputString(lex_Current.value.string + 1);
        parse_GetToken();
        return true;
    }

    return false;
}

bool
parse_ExpectChar(char ch) {
    if (lex_Current.token == (uint32_t) ch) {
        parse_GetToken();
        return true;
    } else {
        prj_Error(ERROR_CHAR_EXPECTED, ch);
        return false;
    }
}

void
parse_GetToken(void) {
    if (!lex_GetNextToken()) {
        prj_Fail(ERROR_END_OF_FILE);
    }
}

bool
parse_Do(void) {
    lex_GetNextToken();

    while (lex_Current.token) {
        if (parse_TargetSpecific())
            continue;

        if (parse_SymbolDefinition())
            continue;

        if (parse_Directive())
            continue;

        if (handleMacroInvocation())
            continue;

        if (handleLineBreak())
            continue;

        if (lex_Current.token == T_POP_END)
            return true;

        return prj_Error(ERROR_SYNTAX);
    }

    return true;
}
