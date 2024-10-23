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
#include "errors.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"

static bool
expressionPriority0(size_t maxStringConstLength, long double* result);

static bool
expressionPriority3(size_t maxStringConstLength, long double* result) {
    switch (lex_Context->token.id) {
        case T_FLOAT: {
            *result = lex_Context->token.value.floating;
            parse_GetToken();
            return true;
        }
        case T_LEFT_PARENS: {
            SLexerContext bookmark;
            lex_Bookmark(&bookmark);

            parse_GetToken();

            if (expressionPriority0(maxStringConstLength, result)) {
                if (lex_Context->token.id == ')') {
                    parse_GetToken();
                    return true;
                }
            }

            lex_Goto(&bookmark);
            return false;
        }
        case T_ID: {
            string* str = lex_TokenString();;
            SSymbol* sym = sym_GetSymbol(str);

            str_Free(str);
            parse_GetToken();

            if (sym != NULL) {
                if (sym->type == SYM_EQUF) {
                    *result = sym->value.floating;
                    return true;
                } else {
                    err_Error(ERROR_SYMBOL_EQUF);
                }
            } else {
                err_Error(ERROR_SYMBOL_UNDEFINED, str_String(sym->name));
            }

            return false;
        }
        default: {
            return false;
        }
    }
}

static bool
expressionPriority2(size_t maxStringConstLength, long double* result) {
    switch (lex_Context->token.id) {
        case T_FUNC_ASFLOAT: {
            parse_GetToken();

            if (parse_ExpectChar('(')) {
                int32_t intValue = parse_ConstantExpression();
                if (parse_ExpectChar(')')) {
                    *result = intValue;
                    return true;
                }
            }
            return false;
        }
        default: {
            return expressionPriority3(maxStringConstLength, result);
        }
    }
}

static bool
expressionPriority1(size_t maxStringConstLength, long double* result) {
    long double t1;
    if (expressionPriority2(maxStringConstLength, &t1)) {
        for (;;) {
            switch (lex_Context->token.id) {
                case T_OP_MULTIPLY:
                case T_OP_DIVIDE: {
                    long double t2;
                    EToken op = lex_Context->token.id;
                    parse_GetToken();
                    if (expressionPriority2(maxStringConstLength, &t2)) {
                        t1 = op == T_OP_MULTIPLY ? t1 * t2 : t1 / t2;
                    } else {
                        return false;
                    }
                    break;
                }
                default: {
                    *result = t1;
                    return true;
                }
            }
        }
    } else {
        return false;
    }
}

static bool
expressionPriority0(size_t maxStringConstLength, long double* result) {
    long double t1;
    if (expressionPriority1(maxStringConstLength, &t1)) {
        for (;;) {
            switch (lex_Context->token.id) {
                case T_OP_ADD:
                case T_OP_SUBTRACT: {
                    long double t2;
                    EToken op = lex_Context->token.id;
                    parse_GetToken();
                    if (expressionPriority1(maxStringConstLength, &t2)) {
                        t1 = op == T_OP_ADD ? t1 + t2 : t1 - t2;
                    } else {
                        return false;
                    }
                    break;
                }
                default: {
                    *result = t1;
                    return true;
                }
            }
        }
    } else {
        return false;
    }
}


extern bool
parse_TryFloatExpression(size_t maxStringConstLength, long double* result) {
    return expressionPriority0(maxStringConstLength, result);
}


extern long double
parse_FloatExpression(size_t maxStringConstLength) {
    long double result = 0;
    if (!expressionPriority0(maxStringConstLength, &result)) {
        err_Error(ERROR_INVALID_EXPRESSION);
    }
    return result;
}
