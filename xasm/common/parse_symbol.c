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

#include <stdint.h>
#include <stdbool.h>

#include "filestack.h"
#include "lexer.h"
#include "parse.h"
#include "parse_block.h"
#include "parse_string.h"
#include "parse_symbol.h"
#include "project.h"
#include "symbol.h"


static string* rsName = NULL;

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

static string*
getRsName() {
    if (rsName == NULL)
        rsName = str_Create("__RS");
    return rsName;
}

static void
createRsSymbol(string* symbolName, int32_t multiplier) {
    parse_GetToken();
    sym_CreateSET(symbolName, parse_GetRs(multiplier * parse_ConstantExpression()));
}

void
parse_SetRs(int32_t rsValue) {
    sym_CreateSET(getRsName(), rsValue);
}

int32_t
parse_GetRs(int32_t size) {
    string* rs = getRsName();
    int32_t rsValue = sym_GetValueByName(rs);
    sym_CreateSET(rs, rsValue + size);
    return rsValue;
}

bool
parse_SymbolDefinition(void) {
    bool r = false;

    if (lex_Current.token == T_LABEL) {
        string* symbolName = str_Create(lex_Current.value.string);

        parse_GetToken();

        uint32_t totalColons = colonCount();

        if (lex_Current.token == T_SYM_MACRO) {
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
                case T_DIRECTIVE_RB: {
                    createRsSymbol(symbolName, 1);
                    break;
                }
                case T_DIRECTIVE_RW: {
                    createRsSymbol(symbolName, 2);
                    break;
                }
                case T_DIRECTIVE_RL: {
                    createRsSymbol(symbolName, 4);
                    break;
                }
                case T_SYM_EQU: {
                    parse_GetToken();
                    sym_CreateEQU(symbolName, parse_ConstantExpression());
                    break;
                }
                case T_SYM_SET: {
                    parse_GetToken();
                    sym_CreateSET(symbolName, parse_ConstantExpression());
                    break;
                }
                case T_SYM_EQUS: {
                    parse_GetToken();

                    string* value = parse_ExpectStringExpression();
                    if (value != NULL) {
                        sym_CreateEQUS(symbolName, value);
                        str_Free(value);
                    }
                    break;
                }
                case T_SYM_GROUP: {
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
