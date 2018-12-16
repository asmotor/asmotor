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
#include "parse_blocks.h"
#include "parse_directive.h"
#include "parse_string.h"
#include "project.h"
#include "symbol.h"

extern bool
parse_TargetSpecific(void);

static string* rsName = NULL;

/* Private functions */

static uint32_t
colonCount(void) {
    if (lex_Current.token == ':') {
        parse_GetToken();
        if (lex_Current.token == ':') {
            parse_GetToken();
            return 2;
        }
    }
    return 1;
}

static void
createRsSymbol(string* symbolName, int32_t size) {
    int32_t rsValue = sym_GetValueByName(parse_GetRsName());
    sym_CreateSET(symbolName, rsValue);
    sym_CreateSET(rsName, rsValue + size);
}

static bool
symbolDefinition(void) {
    bool r = false;

    if (lex_Current.token == T_LABEL) {
        string* symbolName = str_Create(lex_Current.value.string);

        parse_GetToken();

        uint32_t totalColons = colonCount();

        if (lex_Current.token == T_POP_MACRO) {
            if (totalColons != 1) {
                prj_Error(ERROR_SYMBOL_EXPORT);
                return false;
            } else {
                uint32_t lineNumber = fstk_Current->lineNumber;

                size_t reptSize;
                char* reptBlock;
                if (parse_BlockCopyMacro(&reptBlock, &reptSize)) {
                    sym_CreateMACRO(symbolName, reptBlock, reptSize);
                    parse_GetToken();
                    r = true;
                } else {
                    prj_Fail(ERROR_NEED_ENDM, str_String(fstk_Current->name), lineNumber);
                    return false;
                }
            }
        } else {
            switch (lex_Current.token) {
                default: {
                    sym_CreateLabel(symbolName);
                    break;
                }
                case T_POP_RB: {
                    parse_GetToken();
                    createRsSymbol(symbolName, parse_ConstantExpression());
                    break;
                }
                case T_POP_RW: {
                    parse_GetToken();
                    createRsSymbol(symbolName, parse_ConstantExpression() * 2);
                    break;
                }
                case T_POP_RL: {
                    parse_GetToken();
                    createRsSymbol(symbolName, parse_ConstantExpression() * 4);
                    break;
                }
                case T_POP_EQU: {
                    parse_GetToken();
                    sym_CreateEQU(symbolName, parse_ConstantExpression());
                    break;
                }
                case T_POP_SET: {
                    parse_GetToken();
                    sym_CreateSET(symbolName, parse_ConstantExpression());
                    break;
                }
                case T_POP_EQUS: {
                    parse_GetToken();

                    string* value = parse_ExpectStringExpression();
                    if (value != NULL) {
                        sym_CreateEQUS(symbolName, value);
                        str_Free(value);
                    }
                    break;
                }
                case T_POP_GROUP: {
                    EGroupType groupType;

                    parse_GetToken();
                    switch (lex_Current.token) {
                        case T_GROUP_TEXT:
                            groupType = GROUP_TEXT;
                            break;
                        case T_GROUP_BSS:
                            groupType = GROUP_BSS;
                            break;
                        default:
                            prj_Error(ERROR_EXPECT_TEXT_BSS);
                            return false;
                    }
                    sym_CreateGROUP(symbolName, groupType);
                    break;
                }
            }

            if (totalColons == 2)
                sym_Export(symbolName);

            r = true;
        }
        str_Free(symbolName);
    }

    return r;
}


/* Public functions */

bool
parse_IsDot(SLexerBookmark* pBookmark) {
    if (pBookmark)
        lex_Bookmark(pBookmark);

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

static bool
parse_Misc(void) {
    switch (lex_Current.token) {
        case T_ID: {
            string* pName = str_Create(lex_Current.value.string);
            bool bIsMacro = sym_IsMacro(pName);
            str_Free(pName);

            if (bIsMacro) {
                string* s = str_Create(lex_Current.value.string);

                lex_SetState(LEX_STATE_MACRO_ARG0);
                parse_GetToken();
                while (lex_Current.token != '\n') {
                    if (lex_Current.token == T_STRING) {
                        fstk_AddMacroArgument(lex_Current.value.string);
                        parse_GetToken();
                        if (lex_Current.token == ',') {
                            parse_GetToken();
                        } else if (lex_Current.token != '\n') {
                            prj_Error(ERROR_CHAR_EXPECTED, ',');
                            lex_SetState(LEX_STATE_NORMAL);
                            parse_GetToken();
                            return false;
                        }
                    } else if (lex_Current.token == T_MACROARG0) {
                        fstk_SetMacroArgument0(lex_Current.value.string);
                        parse_GetToken();
                    } else {
                        internalerror("Must be T_STRING");
                    }
                }
                lex_SetState(LEX_STATE_NORMAL);
                fstk_ProcessMacro(s);
                str_Free(s);
                return true;
            } else {
                prj_Error(ERROR_INSTR_UNKNOWN, lex_Current.value.string);
                return false;
            }
        }
        default: {
            return false;
        }
    }
}


void
parse_GetToken(void) {
    if (lex_GetNextToken()) {
        return;
    }

    prj_Fail(ERROR_END_OF_FILE);
}

bool
parse_IncrementRs(int32_t size) {
    string* rs = parse_GetRsName();
    sym_CreateSET(rs, sym_GetValueByName(rs) + size);
    return true;
}

string*
parse_GetRsName() {
    if (rsName == NULL)
        rsName = str_Create("__RS");
    return rsName;
}

bool
parse_Do(void) {
    bool r = true;

    lex_GetNextToken();

    while (lex_Current.token && r) {
        if (!parse_TargetSpecific() && !symbolDefinition() && !parse_Directive() && !parse_Misc()) {
            if (lex_Current.token == '\n') {
                lex_GetNextToken();
                fstk_Current->lineNumber += 1;
                g_nTotalLines += 1;
            } else if (lex_Current.token == T_POP_END) {
                return true;
            } else {
                prj_Error(ERROR_SYNTAX);
                r = false;
            }
        }
    }

    return r;
}

