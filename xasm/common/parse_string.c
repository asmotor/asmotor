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

/* From xasm */
#include "lexer.h"
#include "parse_string.h"

/* From util */
#include "str.h"
#include "parse.h"
#include "project.h"

/* Internal functions */

static bool expectEmptyParens() {
    if (!parse_ExpectChar('(')) {
        return false;
    }
    return parse_ExpectChar(')');
}

static string*
stringExpressionPri2(void) {
    SLexerBookmark bm;
    lex_Bookmark(&bm);

    switch (lex_Current.token) {
        case T_STRING: {
            string* r = str_CreateLength(lex_Current.value.string, lex_Current.length);
            parse_GetToken();
            return r;
        }
        case (EToken) '(': {
            parse_GetToken();

            string* r = parse_StringExpression();
            if (r != NULL) {
                if (parse_ExpectChar(')'))
                    return r;
            }

            lex_Goto(&bm);
            str_Free(r);
            return NULL;
        }
        default:
            break;
    }
    return NULL;
}

static string*
stringExpressionPri1(void) {
    string* t = stringExpressionPri2();

    SLexerBookmark bm;
    for (lex_Bookmark(&bm); parse_IsDot(); lex_Bookmark(&bm)) {
        switch (lex_Current.token) {
            case T_FUNC_SLICE: {
                parse_GetToken();

                if (!parse_ExpectChar('('))
                    return NULL;

                int32_t len = (int32_t) str_Length(t);
                int32_t start = parse_ConstantExpression();
                if (start < 0) {
                    start = len + start;
                    if (start < 0)
                        start = 0;
                } else if (start > len) {
                    start = len;
                }

                int32_t count;
                if (lex_Current.token == ',') {
                    parse_GetToken();
                    count = parse_ConstantExpression();
                } else {
                    count = len - start;
                }

                if (parse_ExpectChar(')')) {
                    if (start + count >= len)
                        count = len - start;

                    if (start >= len || count <= 0) {
                        str_Free(t);
                        t = str_Empty();
                    } else  {
                        assert (start >= 0);
                        assert (start < len);
                        assert (start + count <= len);
                        
                        string* r = str_Slice(t, start, (uint32_t) count);

                        STR_MOVE(t, r);
                    }
                }
                break;
            }
            case T_FUNC_TOUPPER: {
                parse_GetToken();
                if (expectEmptyParens()) {
                    str_ToUpperReplace(&t);
                }

                break;
            }
            case T_FUNC_TOLOWER: {
                parse_GetToken();
                if (expectEmptyParens()) {
                    str_ToLowerReplace(&t);
                }

                break;
            }
            default: {
                lex_Goto(&bm);
                return t;
            }
        }
    }

    return t;
}


/* Public functions */

string*
parse_StringExpression(void) {
    string* s1 = stringExpressionPri1();

    while (lex_Current.token == T_OP_ADD) {
        parse_GetToken();

        string* s2 = stringExpressionPri1();
        if (s2 == NULL)
            return NULL;

        string* r = str_Concat(s1, s2);

        str_Free(s2);
        str_Free(s1);

        return r;
    }

    return s1;
}

string*
parse_ExpectStringExpression(void) {
    string* s = parse_StringExpression();

    if (s == NULL)
        prj_Error(ERROR_EXPR_STRING);

    return s;
}
