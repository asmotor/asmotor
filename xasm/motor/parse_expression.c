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
#include "xasm.h"
#include "expression.h"
#include "lexer.h"
#include "parse.h"
#include "options.h"
#include "errors.h"
#include "parse_string.h"

static int32_t
stringCompare(string* s) {
    int32_t r = 0;

    parse_GetToken();

    string* t = parse_ExpectStringExpression();
    if (t != NULL) {
        r = str_Compare(s, t);
    } else {
        err_Error(ERROR_EXPR_STRING);
    }

    str_Free(t);
    str_Free(s);

    return r;
}

static void
switchToLiteralSection(void) {
    string* groupName = str_Create(xasm_Configuration->literalGroup);
    SSymbol* group = sym_GetSymbol(groupName);

    string* section = str_Create("$LITERALS$");
    sect_SwitchTo(section, group);
    str_Free(groupName);
    str_Free(section);
}

static string*
createLiteralName(void) {
    static uint32_t id = 0;
    return str_CreateFormat("__$Literal_%u", id++);
}

static SExpression*
expressionPriority0(size_t maxStringConstLength);

static SExpression*
expressionPriority9(size_t maxStringConstLength) {
    switch (lex_Context->token.id) {
        case T_STRING: {
            if (lex_Context->token.length <= maxStringConstLength) {
                uint32_t val = 0;
                for (size_t i = 0; i < lex_Context->token.length; ++i) {
                    val = val << 8u;
                    val |= (uint8_t) lex_Context->token.value.string[i];
                }
                parse_GetToken();
                return expr_Const(val);
            }
            return NULL;
        }
        case T_NUMBER: {
            int32_t val = lex_Context->token.value.integer;
            parse_GetToken();
            return expr_Const(val);
        }
        case '{': {
            parse_GetToken();

            sect_Push();
            switchToLiteralSection();
            string* symbolName = createLiteralName();
            sym_CreateLabel(symbolName);
            parse_Until('}');
            sect_Pop();
            parse_GetToken();

            SExpression* expression = expr_SymbolByName(symbolName);
            str_Free(symbolName);
            return expression;
        }
        case T_LEFT_PARENS: {
            SLexerContext bookmark;
            lex_Bookmark(&bookmark);

            parse_GetToken();

            SExpression* expr = expressionPriority0(maxStringConstLength);
            if (expr != NULL) {
                if (lex_Context->token.id == ')') {
                    parse_GetToken();
                    return expr_Parens(expr);
                }

                expr_Free(expr);
            }

            lex_Goto(&bookmark);
            return NULL;
        }
        case T_ID: {
            if (strcmp(lex_Context->token.value.string, "@") != 0) {
				string* name = lex_TokenString();
                SSymbol* symbol = sym_GetSymbol(name);
				str_Free(name);

				parse_GetToken();

				while (true) {
					bool force_local = false;

					// Allow backslash for backwards compatibility
	                if (lex_Context->token.id == '\\') {
	                    parse_GetToken();
						force_local = true;
					}

                    if (lex_Context->token.id == T_ID && lex_Context->token.value.string[0] == '.') {
						name = lex_TokenString();

						if (!force_local && !xasm_Configuration->isValidLocalName(name)) {
							str_Free(name);
							break;
						}

						symbol = sym_GetSymbolInScope(symbol, name);
						parse_GetToken();
						str_Free(name);
					} else {
						break;
					}
				}

				return expr_Symbol(symbol);
            }
        }
        // fall through
        case T_OP_MULTIPLY:
        case T_AT: {
            SExpression* expr = expr_Pc();
            parse_GetToken();
            return expr;
        }
        case T_FLOAT:
            return NULL;
        default: {
            if (opt_Current->allowReservedKeywordLabels) {
                if (lex_Context->token.length > 0 && lex_Context->token.id >= T_FIRST_TOKEN) {
                    string* str = lex_TokenString();
                    SExpression* expr = expr_SymbolByName(str);
                    str_Free(str);
                    parse_GetToken();
                    return expr;
                }
            }
            return NULL;
        }

    }
}

static SExpression*
handleStringMemberFunctionReturningInt(string* s) {
    switch (lex_Context->token.id) {
        case T_STR_MEMBER_COMPARETO: {
            parse_GetToken();

            SExpression* r = NULL;
            if (parse_ExpectChar('(')) {
                string* t = parse_ExpectStringExpression();
                if (t != NULL) {
                    if (parse_ExpectChar(')'))
                        r = expr_Const(str_Compare(s, t));

                    str_Free(t);
                }
            }

            str_Free(s);
            return r;
        }
        case T_STR_MEMBER_LENGTH: {
            parse_GetToken();

            SExpression* r = expr_Const((int32_t) str_Length(s));
            str_Free(s);
            return r;
        }
        case T_STR_MEMBER_INDEXOF: {
            parse_GetToken();

            SExpression* r = NULL;
            if (parse_ExpectChar('(')) {
                string* needle = parse_ExpectStringExpression();
                if (needle != NULL) {
                    if (parse_ExpectChar(')')) {
                        uint32_t val = str_Find(s, needle);
                        r = expr_Const(val == UINT32_MAX ? -1 : (int32_t) val);
                    }
                    str_Free(needle);
                }
            }
            str_Free(s);
            return r;
        }
        default: {
            err_Error(ERROR_STRING_MEMBER_NOT_INT);
            return NULL;
        }
    }

}

static SExpression*
expressionPriority8(size_t maxStringConstLength) {
    SLexerContext bm;
    lex_Bookmark(&bm);

    string* s = parse_StringExpression();
    if (s != NULL) {
        if (parse_IsDot()) {
            SExpression* expression = handleStringMemberFunctionReturningInt(s);
            if (expression != NULL)
                return expression;
        } else {
            switch (lex_Context->token.id) {
                case T_OP_EQUAL: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v == 0 ? true : false);
                }
                case T_OP_NOT_EQUAL: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v != 0 ? true : false);
                }
                case T_OP_GREATER_OR_EQUAL: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v >= 0 ? true : false);
                }
                case T_OP_GREATER_THAN: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v > 0 ? true : false);
                }
                case T_OP_LESS_OR_EQUAL: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v <= 0 ? true : false);
                }
                case T_OP_LESS_THAN: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v < 0 ? true : false);
                }
                default: {
					str_Free(s);
                    break;
				}
            }
        }
    }

    lex_Goto(&bm);
    return expressionPriority9(maxStringConstLength);
}

static SExpression*
handleArityTwoFunction(SExpression* (* pFunc)(SExpression*, SExpression*), size_t maxStringConstLength) {
    parse_GetToken();
    if (!parse_ExpectChar('('))
        return NULL;

    SExpression* t1 = expressionPriority0(maxStringConstLength);
    if (t1 != NULL) {
        if (parse_ExpectChar(',')) {
            SExpression* t2 = expressionPriority0(maxStringConstLength);
            if (t2 != NULL) {
                if (parse_ExpectChar(')'))
                    return pFunc(t1, t2);
            }
            expr_Free(t2);
        }
        expr_Free(t1);
    }
    return NULL;
}

static SExpression*
handleArityOneFunction(SExpression* (* pFunc)(SExpression*), size_t maxStringConstLength) {
    parse_GetToken();

    if (parse_ExpectChar('(')) {
        SExpression* t1 = expressionPriority0(maxStringConstLength);
        if (t1 != NULL) {
            if (parse_ExpectChar(')'))
                return pFunc(t1);
            expr_Free(t1);
        }
    }
    return NULL;
}

static SExpression*
handleDefFunction(void) {
    parse_GetToken();

    if (parse_ExpectChar('(')) {
        if (lex_Context->token.id == T_ID) {
            string* symbolName = lex_TokenString();
            SExpression* t1 = expr_Const(sym_IsDefined(symbolName));
            str_Free(symbolName);

            if (t1 != NULL) {
                parse_GetToken();

                if (parse_ExpectChar(')'))
                    return t1;
            }
            expr_Free(t1);
        } else {
            err_Error(ERROR_DEF_SYMBOL);
        }
    }
    return NULL;
}

SExpression*
handleBankFunction(void) {
    assert(xasm_Configuration->supportBanks);

    parse_GetToken();

    if (parse_ExpectChar('(')) {
        if (lex_Context->token.id == T_ID) {
            string* str = lex_TokenString();
            SExpression* t1 = expr_Bank(str);
            str_Free(str);

            if (t1 != NULL) {
                parse_GetToken();

                if (parse_ExpectChar(')'))
                    return t1;
            }
            expr_Free(t1);
        } else {
            err_Error(ERROR_BANK_SYMBOL);
        }
    }
    return NULL;
}

static SExpression*
expressionPriority7(size_t maxStringConstLength) {
    switch (lex_Context->token.id) {
        case T_FUNC_ATAN2:
            return handleArityTwoFunction(expr_Atan2, maxStringConstLength);
        case T_FUNC_SIN:
            return handleArityOneFunction(expr_Sin, maxStringConstLength);
        case T_FUNC_COS:
            return handleArityOneFunction(expr_Cos, maxStringConstLength);
        case T_FUNC_TAN:
            return handleArityOneFunction(expr_Tan, maxStringConstLength);
        case T_FUNC_ASIN:
            return handleArityOneFunction(expr_Asin, maxStringConstLength);
        case T_FUNC_ACOS:
            return handleArityOneFunction(expr_Acos, maxStringConstLength);
        case T_FUNC_ATAN:
            return handleArityOneFunction(expr_Atan, maxStringConstLength);
        case T_FUNC_DEF:
            return handleDefFunction();
        case T_FUNC_BANK:
            return handleBankFunction();
        default: {
            SExpression* expr = xasm_Configuration->parseFunction();
            if (expr != NULL)
                return expr;

            return expressionPriority8(maxStringConstLength);
        }
    }
}

static SExpression*
expressionPriority6(size_t maxStringConstLength) {
    switch (lex_Context->token.id) {
        case T_OP_SUBTRACT: {
            parse_GetToken();
            return expr_Sub(expr_Const(0), expressionPriority6(maxStringConstLength));
        }
        case T_OP_BOOLEAN_NOT: {
            parse_GetToken();
            return expr_BooleanNot(expressionPriority6(maxStringConstLength));
        }
        case T_OP_BITWISE_NOT: {
            parse_GetToken();
            return expr_Xor(expr_Const(0xFFFFFFFF), expressionPriority6(maxStringConstLength));
        }
        case T_OP_ADD: {
            parse_GetToken();
            return expressionPriority6(maxStringConstLength);
        }
        default: {
            return expressionPriority7(maxStringConstLength);
        }
    }

}

static SExpression*
expressionPriority5(size_t maxStringConstLength) {
    SExpression* t1 = expressionPriority6(maxStringConstLength);

    for (;;) {
        switch (lex_Context->token.id) {
            case T_OP_BITWISE_ASL: {
                parse_GetToken();
                t1 = expr_Asl(t1, expressionPriority6(maxStringConstLength));
                break;
            }
            case T_OP_BITWISE_ASR: {
                parse_GetToken();
                t1 = expr_Asr(t1, expressionPriority6(maxStringConstLength));
                break;
            }
            case T_OP_FMUL: {
                parse_GetToken();
                t1 = expr_FixedMultiplication(t1, expressionPriority6(maxStringConstLength));
                break;
            }
            case T_OP_MULTIPLY: {
                parse_GetToken();
                t1 = expr_Mul(t1, expressionPriority6(maxStringConstLength));
                break;
            }
            case T_OP_FDIV: {
                parse_GetToken();
                t1 = expr_FixedDivision(t1, expressionPriority6(maxStringConstLength));
                break;
            }
            case T_OP_DIVIDE: {
                parse_GetToken();
                t1 = expr_Div(t1, expressionPriority6(maxStringConstLength));
                break;
            }
            case T_OP_MODULO: {
                parse_GetToken();
                t1 = expr_Mod(t1, expressionPriority6(maxStringConstLength));
                break;
            }
            default:
                return t1;
        }
    }
}



static SExpression*
expressionPriority4(size_t maxStringConstLength) {
    SExpression* t1 = expressionPriority5(maxStringConstLength);

    for (;;) {
        switch (lex_Context->token.id) {
            case T_OP_BITWISE_XOR: {
                parse_GetToken();
                t1 = expr_Xor(t1, expressionPriority5(maxStringConstLength));
                break;
            }
            case T_OP_BITWISE_OR: {
                parse_GetToken();
                t1 = expr_Or(t1, expressionPriority5(maxStringConstLength));
                break;
            }
            case T_OP_BITWISE_AND: {
                parse_GetToken();
                t1 = expr_And(t1, expressionPriority5(maxStringConstLength));
                break;
            }
            default:
                return t1;
        }
    }
}

static SExpression*
expressionPriority3(size_t maxStringConstLength) {
    SExpression* t1 = expressionPriority4(maxStringConstLength);

    for (;;) {
        switch (lex_Context->token.id) {
            case T_OP_ADD: {
                SExpression* t2;
                SLexerContext mark;

                lex_Bookmark(&mark);
                parse_GetToken();
                t2 = expressionPriority4(maxStringConstLength);
                if (t2 != NULL) {
                    t1 = expr_Add(t1, t2);
                } else {
                    lex_Goto(&mark);
                    return t1;
                }
                break;
            }
            case T_OP_SUBTRACT: {
                parse_GetToken();
                t1 = expr_Sub(t1, expressionPriority4(maxStringConstLength));
                break;
            }
            default:
                return t1;
        }
    }
}

static SExpression*
expressionPriority2(size_t maxStringConstLength) {
    SExpression* t1 = expressionPriority3(maxStringConstLength);

    for (;;) {
        switch (lex_Context->token.id) {
            case T_OP_EQUAL: {
                parse_GetToken();
                t1 = expr_Equal(t1, expressionPriority3(maxStringConstLength));
                break;
            }
            case T_OP_GREATER_THAN: {
                parse_GetToken();
                t1 = expr_GreaterThan(t1, expressionPriority3(maxStringConstLength));
                break;
            }
            case T_OP_LESS_THAN: {
                parse_GetToken();
                t1 = expr_LessThan(t1, expressionPriority3(maxStringConstLength));
                break;
            }
            case T_OP_GREATER_OR_EQUAL: {
                parse_GetToken();
                t1 = expr_GreaterEqual(t1, expressionPriority3(maxStringConstLength));
                break;
            }
            case T_OP_LESS_OR_EQUAL: {
                parse_GetToken();
                t1 = expr_LessEqual(t1, expressionPriority3(maxStringConstLength));
                break;
            }
            case T_OP_NOT_EQUAL: {
                parse_GetToken();
                t1 = expr_NotEqual(t1, expressionPriority3(maxStringConstLength));
                break;
            }
            default:
                return t1;
        }
    }
}

static SExpression*
expressionPriority1(size_t maxStringConstLength) {
    switch (lex_Context->token.id) {
        case T_OP_BOOLEAN_NOT: {
            parse_GetToken();
            return expr_BooleanNot(expressionPriority1(maxStringConstLength));
        }
        default: {
            return expressionPriority2(maxStringConstLength);
        }
    }

}

static SExpression*
expressionPriority0(size_t maxStringConstLength) {
    SExpression* t1 = expressionPriority1(maxStringConstLength);

    for (;;) {
        switch (lex_Context->token.id) {
            case T_OP_BOOLEAN_OR: {
                parse_GetToken();
                t1 = expr_BooleanOr(t1, expressionPriority1(maxStringConstLength));
                break;
            }
            case T_OP_BOOLEAN_AND: {
                parse_GetToken();
                t1 = expr_BooleanAnd(t1, expressionPriority1(maxStringConstLength));
                break;
            }
            default:
                return t1;
        }
    }
}


/* Public functions */

SExpression*
parse_Expression(size_t maxStringConstLength) {
    return expressionPriority0(maxStringConstLength);
}

int32_t
parse_ConstantExpression(void) {
    SExpression* expr = parse_Expression(4);

    if (expr != NULL) {
        if (expr_IsConstant(expr)) {
            int32_t r = expr->value.integer;
            expr_Free(expr);
            return r;
        }

        err_Error(ERROR_EXPR_CONST);
    } else
        err_Error(ERROR_INVALID_EXPRESSION);

    return 0;
}

SExpression* 
parse_ExpressionS16(void) {
	SExpression* expression = parse_Expression(2);
	if (expression == NULL)
		return NULL;
		
	expression = expr_CheckRange(expression, -32768, 32767);
	if (expression == NULL)
        err_Error(ERROR_OPERAND_RANGE);
	
	return expr_And(expression, expr_Const(0xFFFF));
}

SExpression*
parse_ExpressionU16(void) {
	SExpression* expression = parse_Expression(2);
	if (expression == NULL)
		return NULL;
		
	expression = expr_CheckRange(expression, 0, 65535);
	if (expression == NULL)
        err_Error(ERROR_OPERAND_RANGE);
        
	return expression;
}


