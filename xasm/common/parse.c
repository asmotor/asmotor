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

#include <fmath.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "asmotor.h"
#include "types.h"
#include "mem.h"
#include "expression.h"
#include "project.h"
#include "filestack.h"
#include "lexer.h"
#include "parse.h"
#include "symbol.h"
#include "section.h"
#include "tokens.h"
#include "options.h"

#define REPT_LEN 4
#define ENDR_LEN 4
#define ENDC_LEN 4
#define MACRO_LEN 5
#define ENDM_LEN 4

extern bool
parse_TargetSpecific(void);

extern SExpression*
parse_TargetFunction(void);

/* Private functions */

static bool
isWhiteSpace(char s) {
    return s == ' ' || s == '\t' || s == '\0' || s == '\n';
}

static bool
isToken(size_t index, const char* token) {
    size_t len = strlen(token);
    return lex_CompareNoCase(index, token, len) && isWhiteSpace(lex_PeekChar(index + len));
}

static bool
isRept(size_t index) {
    return isToken(index, "REPT");
}

static bool
isEndr(size_t index) {
    return isToken(index, "ENDR");
}

static bool
isIf(size_t index) {
    return isToken(index, "IF") || isToken(index, "IFC") || isToken(index, "IFD") || isToken(index, "IFNC")
           || isToken(index, "IFND") || isToken(index, "IFNE") || isToken(index, "IFEQ") || isToken(index, "IFGT")
           || isToken(index, "IFGE") || isToken(index, "IFLT") || isToken(index, "IFLE");
}

static bool
isElse(size_t index) {
    return isToken(index, "ELSE");
}

static bool
isEndc(size_t index) {
    return isToken(index, "ENDC");
}

static bool
isMacro(size_t index) {
    return isToken(index, "MACRO");
}

static bool
isEndm(size_t index) {
    return isToken(index, "ENDM");
}

static size_t
skipLine(size_t index) {
    while (lex_PeekChar(index) != 0) {
        if (lex_PeekChar(index++) == '\n')
            return index;
    }

    return SIZE_MAX;
}

static size_t
findControlToken(size_t index) {
    if (index == SIZE_MAX)
        return SIZE_MAX;

    if (isRept(index) || isEndr(index) || isIf(index) || isElse(index) || isEndc(index) || isMacro(index)
        || isEndm(index)) {
        return index;
    }

    while (!isWhiteSpace(lex_PeekChar(index))) {
        if (lex_PeekChar(index++) == ':')
            break;
    }
    while (isWhiteSpace(lex_PeekChar(index)))
        ++index;

    return index;
}

static size_t
getReptBodySize(size_t index);

static size_t
getIfBodySize(size_t index);

static size_t
getMacroBodySize(size_t index);

static bool
skipRept(size_t* index) {
    if (isRept(*index)) {
		*index = skipLine(*index + getReptBodySize(*index + REPT_LEN) + REPT_LEN + ENDR_LEN);
		return true;
	} else {
		return false;
	}
}

static bool
skipIf(size_t* index) {
	if (isIf(*index)) {
		while (!isWhiteSpace(lex_PeekChar(*index)))
			*index += 1;
		*index = skipLine(*index + getIfBodySize(*index) + ENDC_LEN);
		return true;
	} else {
		return false;
	}
}

static bool
skipMacro(size_t* index) {
    if (isMacro(*index)) {
		*index = skipLine(*index + getMacroBodySize(*index + MACRO_LEN) + MACRO_LEN + ENDM_LEN);
		return true;
	} else {
		return false;
	}
}

static size_t
getBlockBodySize(size_t index, bool (*endPredicate)(size_t)) {
    size_t start = index;

    index = skipLine(index);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
            if (endPredicate(index)) {
                return index - start;
            } else {
                index = skipLine(index);
            }
        }
    }

    return 0;
}

static size_t
getReptBodySize(size_t index) {
    return getBlockBodySize(index, isEndr);
}

static size_t
getMacroBodySize(size_t index) {
    return getBlockBodySize(index, isEndm);
}

static size_t
getIfBodySize(size_t index) {
    return getBlockBodySize(index, isEndc);
}

static bool
copyReptBlock(char** reptBlock, size_t* size) {
    size_t len = getReptBodySize(0);

    if (len == 0)
        return false;

    *size = len;

    *reptBlock = (char*) mem_Alloc(len + 1);
    (*reptBlock)[len] = 0;
    lex_GetChars(*reptBlock, len);
    fstk_Current->lineNumber += lex_SkipBytes(4);

    return true;
}

static bool
skipToElse(void) {
    size_t index = skipLine(0);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
			if (isEndc(index)) {
				fstk_Current->lineNumber += lex_SkipBytes(index) + 1;
				return true;
			} else if (isElse(index)) {
				fstk_Current->lineNumber += lex_SkipBytes(index + 4) + 1;
				return true;
			} else {
				index = skipLine(index);
			}
		}
    }

    return false;
}

static bool
skipToEndc(void) {
    size_t index = skipLine(0);
    while ((index = findControlToken(index)) != SIZE_MAX) {
        if (!skipRept(&index) && !skipIf(&index) && !skipMacro(&index)) {
			if (isEndc(index)) {
				fstk_Current->lineNumber += lex_SkipBytes(index);
				return true;
			} else {
				index = skipLine(index);
			}
		}
    }

    return 0;
}

static bool
copyMacroBlock(char** dest, size_t* size) {
    size_t len = getMacroBodySize(0);

    *size = len;

    *dest = (char*) mem_Alloc(len + 1);
    fstk_Current->lineNumber += lex_GetChars(*dest, len);
    fstk_Current->lineNumber += lex_SkipBytes(4);
    return true;
}

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

static bool
isDot(SLexerBookmark* pBookmark) {
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


/* Public functions */

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

/*
 *	Expression parser
 */

static SExpression*
expressionPriority0(size_t maxStringConstLength);

static SExpression*
expressionPriority9(size_t maxStringConstLength) {
    switch (lex_Current.token) {
        case T_STRING: {
            if (lex_Current.length <= maxStringConstLength) {
                uint32_t val = 0;
                for (size_t i = 0; i < lex_Current.length; ++i) {
                    val = val << 8u;
                    val |= (uint8_t) lex_Current.value.string[i];
                }
                parse_GetToken();
                return expr_Const(val);
            }
            return NULL;
        }
        case T_NUMBER: {
            int32_t val = lex_Current.value.integer;
            parse_GetToken();
            return expr_Const(val);
        }
        case T_LEFT_PARENS: {
            SLexerBookmark bookmark;
            lex_Bookmark(&bookmark);

            parse_GetToken();

            SExpression* expr = expressionPriority0(maxStringConstLength);
            if (expr != NULL) {
                if (lex_Current.token == ')') {
                    parse_GetToken();
                    return expr_Parens(expr);
                }

                expr_Free(expr);
            }

            lex_Goto(&bookmark);
            return NULL;
        }
        case T_ID: {
            if (strcmp(lex_Current.value.string, "@") != 0) {
                SExpression* expr = expr_Symbol(lex_Current.value.string);
                parse_GetToken();
                return expr;
            }
            // fall through to @
        }
        case T_OP_MUL:
        case T_AT: {
            SExpression* expr = expr_Pc();
            parse_GetToken();
            return expr;
        }
        default: {
            if (opt_Current->allowReservedKeywordLabels) {
                if (lex_Current.length > 0 && lex_Current.token >= T_FIRST_TOKEN) {
                    SExpression* expr = expr_Symbol(lex_Current.value.string);
                    parse_GetToken();
                    return expr;
                }
            }
            return NULL;
        }

    }
}

static char*
stringExpression(void);

static int32_t
stringCompare(char* s) {
    int32_t r = 0;
    char* t;

    parse_GetToken();
    if ((t = stringExpression()) != NULL) {
        r = strcmp(s, t);
        mem_Free(t);
    }
    mem_Free(s);

    return r;
}

static char*
parse_StringExpressionRaw_Pri0(void);

static SExpression*
parse_ExprPri8(size_t maxStringConstLength) {
    SLexerBookmark bm;
    char* s;

    lex_Bookmark(&bm);
    if ((s = parse_StringExpressionRaw_Pri0()) != NULL) {
        if (isDot(NULL)) {
            switch (lex_Current.token) {
                case T_FUNC_COMPARETO: {
                    SExpression* r = NULL;
                    char* t;

                    parse_GetToken();

                    if (parse_ExpectChar('(')) {
                        if ((t = stringExpression()) != NULL) {
                            if (parse_ExpectChar(')'))
                                r = expr_Const(strcmp(s, t));

                            mem_Free(t);
                        }
                    }

                    mem_Free(s);
                    return r;
                }
                case T_FUNC_LENGTH: {
                    SExpression* r = expr_Const((int32_t) strlen(s));
                    mem_Free(s);
                    parse_GetToken();

                    return r;
                }
                case T_FUNC_INDEXOF: {
                    SExpression* r = NULL;
                    parse_GetToken();

                    if (parse_ExpectChar('(')) {
                        char* needle;
                        if ((needle = stringExpression()) != NULL) {
                            if (parse_ExpectChar(')')) {
                                char* p;
                                int32_t val = -1;

                                if ((p = strstr(s, needle)) != NULL)
                                    val = (int32_t) (p - s);

                                r = expr_Const(val);
                            }
                            mem_Free(needle);
                        }
                    }
                    mem_Free(s);
                    return r;
                }
                case T_OP_LOGICEQU: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v == 0 ? true : false);
                }
                case T_OP_LOGICNE: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v != 0 ? true : false);
                }
                case T_OP_LOGICGE: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v >= 0 ? true : false);
                }
                case T_OP_LOGICGT: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v > 0 ? true : false);
                }
                case T_OP_LOGICLE: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v <= 0 ? true : false);
                }
                case T_OP_LOGICLT: {
                    int32_t v = stringCompare(s);
                    return expr_Const(v < 0 ? true : false);
                }
                default:
                    break;
            }
        }
    }

    mem_Free(s);
    lex_Goto(&bm);
    return expressionPriority9(maxStringConstLength);
}

static SExpression*
parse_TwoArgFunc(SExpression* (* pFunc)(SExpression*, SExpression*), size_t maxStringConstLength) {
    SExpression* t1;
    SExpression* t2;

    parse_GetToken();
    if (!parse_ExpectChar('('))
        return NULL;

    t1 = expressionPriority0(maxStringConstLength);

    if (!parse_ExpectChar(','))
        return NULL;

    t2 = expressionPriority0(maxStringConstLength);

    if (!parse_ExpectChar(')'))
        return NULL;

    return pFunc(t1, t2);
}

static SExpression*
parse_SingleArgFunc(SExpression* (* pFunc)(SExpression*), size_t maxStringConstLength) {
    SExpression* t1;

    parse_GetToken();

    if (!parse_ExpectChar('('))
        return NULL;

    t1 = expressionPriority0(maxStringConstLength);

    if (!parse_ExpectChar(')'))
        return NULL;

    return pFunc(t1);
}

static SExpression*
parse_ExprPri7(size_t maxStringConstLength) {
    switch (lex_Current.token) {
        case T_FUNC_ATAN2:
            return parse_TwoArgFunc(expr_Atan2, maxStringConstLength);
        case T_FUNC_SIN:
            return parse_SingleArgFunc(expr_Sin, maxStringConstLength);
        case T_FUNC_COS:
            return parse_SingleArgFunc(expr_Cos, maxStringConstLength);
        case T_FUNC_TAN:
            return parse_SingleArgFunc(expr_Tan, maxStringConstLength);
        case T_FUNC_ASIN:
            return parse_SingleArgFunc(expr_Asin, maxStringConstLength);
        case T_FUNC_ACOS:
            return parse_SingleArgFunc(expr_Acos, maxStringConstLength);
        case T_FUNC_ATAN:
            return parse_SingleArgFunc(expr_Atan, maxStringConstLength);
        case T_FUNC_DEF: {
            SExpression* t1;
            string* pName;

            parse_GetToken();

            if (!parse_ExpectChar('('))
                return NULL;

            if (lex_Current.token != T_ID) {
                prj_Fail(ERROR_DEF_SYMBOL);
                return NULL;
            }

            pName = str_Create(lex_Current.value.string);
            t1 = expr_Const(sym_IsDefined(pName));
            str_Free(pName);
            parse_GetToken();

            if (!parse_ExpectChar(')'))
                return NULL;

            return t1;
        }
        case T_FUNC_BANK: {
            SExpression* t1;

            if (!g_pConfiguration->bSupportBanks)
                internalerror("Banks not supported");

            parse_GetToken();

            if (!parse_ExpectChar('('))
                return NULL;

            if (lex_Current.token != T_ID) {
                prj_Fail(ERROR_BANK_SYMBOL);
                return NULL;
            }

            t1 = expr_Bank(lex_Current.value.string);
            parse_GetToken();

            if (!parse_ExpectChar(')'))
                return NULL;

            return t1;
        }
        default: {
            SExpression* expr;
            if ((expr = parse_TargetFunction()) != NULL)
                return expr;

            return parse_ExprPri8(maxStringConstLength);
        }
    }
}

static SExpression*
parse_ExprPri6(size_t maxStringConstLength) {
    switch (lex_Current.token) {
        case T_OP_SUB: {
            parse_GetToken();
            return expr_Sub(expr_Const(0), parse_ExprPri6(maxStringConstLength));
        }
        case T_OP_NOT: {
            parse_GetToken();
            return expr_Xor(expr_Const(0xFFFFFFFF), parse_ExprPri6(maxStringConstLength));
        }
        case T_OP_ADD: {
            parse_GetToken();
            return parse_ExprPri6(maxStringConstLength);
        }
        default: {
            return parse_ExprPri7(maxStringConstLength);
        }
    }

}

static SExpression*
parse_ExprPri5(size_t maxStringConstLength) {
    SExpression* t1 = parse_ExprPri6(maxStringConstLength);

    while (lex_Current.token == T_OP_SHL || lex_Current.token == T_OP_SHR || lex_Current.token == T_OP_MUL
           || lex_Current.token == T_OP_DIV || lex_Current.token == T_FUNC_FMUL || lex_Current.token == T_FUNC_FDIV
           || lex_Current.token == T_OP_MOD) {
        switch (lex_Current.token) {
            case T_OP_SHL: {
                parse_GetToken();
                t1 = expr_Asl(t1, parse_ExprPri6(maxStringConstLength));
                break;
            }
            case T_OP_SHR: {
                parse_GetToken();
                t1 = expr_Asr(t1, parse_ExprPri6(maxStringConstLength));
                break;
            }
            case T_FUNC_FMUL: {
                parse_GetToken();
                t1 = expr_FixedMultiplication(t1, parse_ExprPri6(maxStringConstLength));
                break;
            }
            case T_OP_MUL: {
                parse_GetToken();
                t1 = expr_Mul(t1, parse_ExprPri6(maxStringConstLength));
                break;
            }
            case T_FUNC_FDIV: {
                parse_GetToken();
                t1 = expr_FixedDivision(t1, parse_ExprPri6(maxStringConstLength));
                break;
            }
            case T_OP_DIV: {
                parse_GetToken();
                t1 = expr_Div(t1, parse_ExprPri6(maxStringConstLength));
                break;
            }
            case T_OP_MOD: {
                parse_GetToken();
                t1 = expr_Mod(t1, parse_ExprPri6(maxStringConstLength));
                break;
            }
            default:
                break;
        }
    }

    return t1;
}

static SExpression*
parse_ExprPri4(size_t maxStringConstLength) {
    SExpression* t1 = parse_ExprPri5(maxStringConstLength);

    while (lex_Current.token == T_OP_XOR || lex_Current.token == T_OP_OR || lex_Current.token == T_OP_AND) {
        switch (lex_Current.token) {
            case T_OP_XOR: {
                parse_GetToken();
                t1 = expr_Xor(t1, parse_ExprPri5(maxStringConstLength));
                break;
            }
            case T_OP_OR: {
                parse_GetToken();
                t1 = expr_Or(t1, parse_ExprPri5(maxStringConstLength));
                break;
            }
            case T_OP_AND: {
                parse_GetToken();
                t1 = expr_And(t1, parse_ExprPri5(maxStringConstLength));
                break;
            }
            default:
                break;
        }
    }

    return t1;
}

static SExpression*
parse_ExprPri3(size_t maxStringConstLength) {
    SExpression* t1 = parse_ExprPri4(maxStringConstLength);

    while (lex_Current.token == T_OP_ADD || lex_Current.token == T_OP_SUB) {
        switch (lex_Current.token) {
            case T_OP_ADD: {
                SExpression* t2;
                SLexerBookmark mark;

                lex_Bookmark(&mark);
                parse_GetToken();
                t2 = parse_ExprPri4(maxStringConstLength);
                if (t2 != NULL) {
                    t1 = expr_Add(t1, t2);
                } else {
                    lex_Goto(&mark);
                    return t1;
                }
                break;
            }
            case T_OP_SUB: {
                parse_GetToken();
                t1 = expr_Sub(t1, parse_ExprPri4(maxStringConstLength));
                break;
            }
            default:
                break;
        }
    }
    return t1;
}

static SExpression*
parse_ExprPri2(size_t maxStringConstLength) {
    SExpression* t1;

    t1 = parse_ExprPri3(maxStringConstLength);
    while (lex_Current.token == T_OP_LOGICEQU || lex_Current.token == T_OP_LOGICGT || lex_Current.token == T_OP_LOGICLT
           || lex_Current.token == T_OP_LOGICGE || lex_Current.token == T_OP_LOGICLE
           || lex_Current.token == T_OP_LOGICNE) {
        switch (lex_Current.token) {
            case T_OP_LOGICEQU: {
                parse_GetToken();
                t1 = expr_Equal(t1, parse_ExprPri3(maxStringConstLength));
                break;
            }
            case T_OP_LOGICGT: {
                parse_GetToken();
                t1 = expr_GreaterThan(t1, parse_ExprPri3(maxStringConstLength));
                break;
            }
            case T_OP_LOGICLT: {
                parse_GetToken();
                t1 = expr_LessThan(t1, parse_ExprPri3(maxStringConstLength));
                break;
            }
            case T_OP_LOGICGE: {
                parse_GetToken();
                t1 = expr_GreaterEqual(t1, parse_ExprPri3(maxStringConstLength));
                break;
            }
            case T_OP_LOGICLE: {
                parse_GetToken();
                t1 = expr_LessEqual(t1, parse_ExprPri3(maxStringConstLength));
                break;
            }
            case T_OP_LOGICNE: {
                parse_GetToken();
                t1 = expr_NotEqual(t1, parse_ExprPri3(maxStringConstLength));
                break;
            }
            default:
                break;
        }
    }

    return t1;
}

static SExpression*
parse_ExprPri1(size_t maxStringConstLength) {
    switch (lex_Current.token) {
        case T_OP_OR:
        case T_OP_LOGICNOT: {
            parse_GetToken();
            return expr_BooleanNot(parse_ExprPri1(maxStringConstLength));
        }
        default: {
            return parse_ExprPri2(maxStringConstLength);
        }
    }

}

static SExpression*
expressionPriority0(size_t maxStringConstLength) {
    SExpression* t1 = parse_ExprPri1(maxStringConstLength);

    while (lex_Current.token == T_OP_LOGICOR || lex_Current.token == T_OP_LOGICAND) {
        switch (lex_Current.token) {
            case T_OP_LOGICOR: {
                parse_GetToken();
                t1 = expr_BooleanOr(t1, parse_ExprPri1(maxStringConstLength));
                break;
            }
            case T_OP_LOGICAND: {
                parse_GetToken();
                t1 = expr_BooleanAnd(t1, parse_ExprPri1(maxStringConstLength));
                break;
            }
            default:
                break;
        }
    }

    return t1;
}

SExpression*
parse_Expression(size_t maxStringConstLength) {
    return expressionPriority0(maxStringConstLength);
}

int32_t
parse_ConstantExpression() {
    SExpression* expr;

    if ((expr = parse_Expression(4)) != NULL) {
        if (expr_IsConstant(expr)) {
            int32_t r = expr->value.integer;
            expr_Free(expr);
            return r;
        }

        prj_Error(ERROR_EXPR_CONST);
    } else
        prj_Error(ERROR_INVALID_EXPRESSION);

    return 0;
}

static char*
parse_StringExpressionRaw_Pri2(void) {
    SLexerBookmark bm;
    lex_Bookmark(&bm);

    switch (lex_Current.token) {
        case T_STRING: {
            char* r = mem_Alloc(strlen(lex_Current.value.string) + 1);
            strcpy(r, lex_Current.value.string);
            parse_GetToken();
            return r;
        }
        case (EToken) '(': {
            char* r;

            parse_GetToken();
            r = parse_StringExpressionRaw_Pri0();
            if (r != NULL) {
                if (parse_ExpectChar(')'))
                    return r;
            }

            lex_Goto(&bm);
            mem_Free(r);
            return NULL;
        }
        default:
            break;
    }
    return NULL;
}

static char*
parse_StringExpressionRaw_Pri1(void) {
    SLexerBookmark bm;
    char* t = parse_StringExpressionRaw_Pri2();

    while (isDot(&bm)) {
        switch (lex_Current.token) {
            case T_FUNC_SLICE: {
                int32_t start;
                int32_t count;
                int32_t len = (int32_t) strlen(t);

                parse_GetToken();

                if (!parse_ExpectChar('('))
                    return NULL;

                start = parse_ConstantExpression();
                if (start < 0) {
                    start = len + start;
                    if (start < 0)
                        start = 0;
                }

                if (lex_Current.token == ',') {
                    parse_GetToken();
                    count = parse_ConstantExpression();
                } else
                    count = len - start;

                if (parse_ExpectChar(')')) {
                    char* r = mem_Alloc((size_t) count + 1);

                    if (start + count >= len)
                        count = len - start;

                    strncpy(r, t + start, count);
                    r[count] = 0;

                    mem_Free(t);
                    t = r;
                }
                break;
            }
            case T_FUNC_TOUPPER: {
                parse_GetToken();
                if (!parse_ExpectChar('('))
                    return NULL;

                if (parse_ExpectChar(')'))
                    _strupr(t);

                break;
            }
            case T_FUNC_TOLOWER: {
                parse_GetToken();
                if (!parse_ExpectChar('('))
                    return NULL;

                if (parse_ExpectChar(')'))
                    _strlwr(t);

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

static char*
parse_StringExpressionRaw_Pri0(void) {
    char* t1 = parse_StringExpressionRaw_Pri1();

    while (lex_Current.token == T_OP_ADD) {
        char* r = NULL;
        char* t2;

        parse_GetToken();

        if ((t2 = parse_StringExpressionRaw_Pri1()) == NULL)
            return NULL;

        r = mem_Alloc(strlen(t1) + strlen(t2) + 1);
        strcpy(r, t1);
        strcat(r, t2);

        mem_Free(t2);
        mem_Free(t1);

        return r;
    }

    return t1;
}

static char*
stringExpression(void) {
    char* s = parse_StringExpressionRaw_Pri0();

    if (s == NULL)
        prj_Error(ERROR_EXPR_STRING);

    return s;
}

static void
parse_RS(string* pName, int32_t size, int coloncount) {
    string* pRS = str_Create("__RS");
    int32_t nRS = sym_GetValueByName(pRS);

    sym_CreateSET(pName, nRS);
    sym_CreateSET(pRS, nRS + size);

    str_Free(pRS);

    if (coloncount == 2)
        sym_Export(pName);
}

static void
parse_RS_Skip(int32_t size) {
    string* pRS = str_Create("__RS");
    sym_CreateSET(pRS, sym_GetValueByName(pRS) + size);
    str_Free(pRS);
}

static bool
parse_Symbol(void) {
    bool r = false;

    if (lex_Current.token == T_LABEL) {
        string* pName = str_Create(lex_Current.value.string);
        uint32_t coloncount;

        parse_GetToken();
        coloncount = colonCount();

        switch (lex_Current.token) {
            default:
                sym_CreateLabel(pName);
                if (coloncount == 2)
                    sym_Export(pName);
                r = true;
                break;
            case T_POP_RB:
                parse_GetToken();
                parse_RS(pName, parse_ConstantExpression(), coloncount);
                r = true;
                break;
            case T_POP_RW:
                parse_GetToken();
                parse_RS(pName, parse_ConstantExpression() * 2, coloncount);
                r = true;
                break;
            case T_POP_RL:
                parse_GetToken();
                parse_RS(pName, parse_ConstantExpression() * 4, coloncount);
                r = true;
                break;
            case T_POP_EQU:
                parse_GetToken();
                sym_CreateEQU(pName, parse_ConstantExpression());
                if (coloncount == 2)
                    sym_Export(pName);
                r = true;
                break;
            case T_POP_SET:
                parse_GetToken();
                sym_CreateSET(pName, parse_ConstantExpression());
                if (coloncount == 2)
                    sym_Export(pName);
                r = true;
                break;
            case T_POP_EQUS: {
                char* pExpr;

                parse_GetToken();
                if ((pExpr = stringExpression()) != NULL) {
                    string* pValue = str_Create(pExpr);
                    sym_CreateEQUS(pName, pValue);
                    mem_Free(pExpr);
                    str_Free(pValue);
                    if (coloncount == 2)
                        sym_Export(pName);
                    r = true;
                }
                break;
            }
            case T_POP_GROUP: {
                parse_GetToken();
                switch (lex_Current.token) {
                    case T_GROUP_TEXT:
                        sym_CreateGROUP(pName, GROUP_TEXT);
                        r = true;
                        break;
                    case T_GROUP_BSS:
                        sym_CreateGROUP(pName, GROUP_BSS);
                        r = true;
                        break;
                    default:
                        prj_Error(ERROR_EXPECT_TEXT_BSS);
                        r = false;
                        break;
                }
                if (coloncount == 2)
                    sym_Export(pName);
                break;
            }
            case T_POP_MACRO: {
                size_t reptSize;
                char* reptBlock;
                int32_t lineno = fstk_Current->lineNumber;
                const char* pszfile = str_String(fstk_Current->name);

                if (copyMacroBlock(&reptBlock, &reptSize)) {
                    sym_CreateMACRO(pName, reptBlock, reptSize);
                    parse_GetToken();
                    r = true;
                } else
                    prj_Fail(ERROR_NEED_ENDM, pszfile, lineno);
                break;
            }
        }
        str_Free(pName);
    }

    return r;
}

static bool
parse_Import(string* pName) {
    return sym_Import(pName) != NULL;
}

static bool
parse_Export(string* pName) {
    return sym_Export(pName) != NULL;
}

static bool
parse_Global(string* pName) {
    return sym_Global(pName) != NULL;
}

static bool
parse_SymbolOp(bool (* pOp)(string* pName)) {
    bool r = false;

    parse_GetToken();
    if (lex_Current.token == T_ID) {
        string* pName = str_Create(lex_Current.value.string);
        pOp(pName);
        str_Free(pName);
        parse_GetToken();
        while (lex_Current.token == ',') {
            parse_GetToken();
            if (lex_Current.token == T_ID) {
                pName = str_Create(lex_Current.value.string);
                pOp(pName);
                str_Free(pName);
                r = true;
                parse_GetToken();
            } else
                prj_Error(ERROR_EXPECT_IDENTIFIER);
        }
    } else
        prj_Error(ERROR_EXPECT_IDENTIFIER);

    return r;
}

static int32_t
parse_ExpectBankFixed(void) {
    int32_t bank;

    if (!g_pConfiguration->bSupportBanks)
        internalerror("Banks not supported");

    if (lex_Current.token != T_FUNC_BANK) {
        prj_Error(ERROR_EXPECT_BANK);
        return -1;
    }

    parse_GetToken();
    if (!parse_ExpectChar('['))
        return -1;

    parse_GetToken();
    bank = parse_ConstantExpression();
    if (!parse_ExpectChar(']'))
        return -1;

    return bank;
}

static bool
parse_PseudoOp(void) {
    switch (lex_Current.token) {
        case T_POP_REXIT: {
            if (fstk_Current->type != CONTEXT_REPT) {
                prj_Warn(WARN_REXIT_OUTSIDE_REPT);
            } else {
                fstk_Current->block.repeat.remaining = 0;
                fstk_ProcessNextBuffer();
                fstk_Current->lineNumber++;
            }
            parse_GetToken();
            return true;
        }
        case T_POP_MEXIT: {
            if (fstk_Current->type != CONTEXT_MACRO) {
                prj_Warn(WARN_MEXIT_OUTSIDE_MACRO);
            } else {
                fstk_ProcessNextBuffer();
            }
            parse_GetToken();
            return true;
        }
        case T_POP_SECTION: {
            int32_t loadAddress;
            char* name;
            char r[MAXSYMNAMELENGTH + 1];
            SSymbol* sym;
            string* pGroup;

            parse_GetToken();
            if ((name = stringExpression()) == NULL)
                return true;

            strcpy(r, name);
            mem_Free(name);

            if (!parse_ExpectChar(','))
                return sect_SwitchTo_NAMEONLY(r);

            if (lex_Current.token != T_ID) {
                prj_Error(ERROR_EXPECT_IDENTIFIER);
                return false;
            }

            pGroup = str_Create(lex_Current.value.string);
            sym = sym_FindSymbol(pGroup);
            str_Free(pGroup);

            if (sym->eType != SYM_GROUP) {
                prj_Error(ERROR_IDENTIFIER_GROUP);
                return true;
            }
            parse_GetToken();

            if (g_pConfiguration->bSupportBanks && lex_Current.token == ',') {
                int32_t bank;
                parse_GetToken();
                bank = parse_ExpectBankFixed();

                if (bank == -1)
                    return true;

                return sect_SwitchTo_BANK(r, sym, bank);
            } else if (lex_Current.token != '[') {
                return sect_SwitchTo(r, sym);
            }
            parse_GetToken();

            loadAddress = parse_ConstantExpression();
            if (!parse_ExpectChar(']'))
                return true;

            if (g_pConfiguration->bSupportBanks && lex_Current.token == ',') {
                int32_t bank;
                parse_GetToken();
                bank = parse_ExpectBankFixed();

                if (bank == -1)
                    return true;

                return sect_SwitchTo_LOAD_BANK(r, sym, loadAddress, bank);
            }

            return sect_SwitchTo_LOAD(r, sym, loadAddress);
        }
        case T_POP_ORG: {
            uint32_t orgAddress;

            parse_GetToken();
            orgAddress = (uint32_t) parse_ConstantExpression();

            sect_SetOrgAddress(orgAddress);

            return true;
        }
        case T_POP_PRINTT: {
            char* r;

            parse_GetToken();
            if ((r = stringExpression()) != NULL) {
                printf("%s", r);
                mem_Free(r);
                return true;
            }

            return false;
        }
        case T_POP_PRINTV: {
            parse_GetToken();
            printf("$%X", parse_ConstantExpression());
            return true;
        }
        case T_POP_PRINTF: {
            int32_t i;

            parse_GetToken();
            i = parse_ConstantExpression();
            if (i < 0) {
                printf("-");
                i = -i;
            }
            printf("%d.%05d", (uint32_t) i >> 16u, imuldiv((uint32_t) i & 0xFFFFu, 100000, 65536));

            return true;
        }
        case T_POP_IMPORT: {
            return parse_SymbolOp(parse_Import);
        }
        case T_POP_EXPORT: {
            return parse_SymbolOp(parse_Export);
        }
        case T_POP_GLOBAL: {
            return parse_SymbolOp(parse_Global);
        }
        case T_POP_PURGE: {
            bool r;

            tokens_expandStrings = false;
            r = parse_SymbolOp(sym_Purge);
            tokens_expandStrings = true;

            return r;
        }
        case T_POP_RSRESET: {
            string* pRS = str_Create("__RS");
            parse_GetToken();
            sym_CreateSET(pRS, 0);
            str_Free(pRS);
            return true;
        }
        case T_POP_RSSET: {
            string* pRS = str_Create("__RS");
            int32_t val;

            parse_GetToken();
            val = parse_ConstantExpression();
            sym_CreateSET(pRS, val);
            str_Free(pRS);
            return true;
        }
        case T_POP_RB: {
            parse_RS_Skip(parse_ConstantExpression());
            return true;
        }
        case T_POP_RW: {
            parse_RS_Skip(parse_ConstantExpression() * 2);
            return true;
        }
        case T_POP_RL: {
            parse_RS_Skip(parse_ConstantExpression() * 4);
            return true;
        }
        case T_POP_FAIL: {
            char* r;

            parse_GetToken();
            if ((r = stringExpression()) != NULL) {
                prj_Fail(WARN_USER_GENERIC, r);
                return true;
            } else {
                internalerror("String expression is NULL");
            }
        }
        case T_POP_WARN: {
            char* r;

            parse_GetToken();
            if ((r = stringExpression()) != NULL) {
                prj_Warn(WARN_USER_GENERIC, r);
                return true;
            } else {
                internalerror("String expression is NULL");
            }
        }
        case T_POP_EVEN: {
            parse_GetToken();
            sect_Align(2);
            return true;
        }
        case T_POP_CNOP: {
            parse_GetToken();
            int32_t offset = parse_ConstantExpression();
            if (offset < 0) {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }

            if (!parse_ExpectComma())
                return true;

            int32_t align = parse_ConstantExpression();
            if (align < 0) {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
            sect_Align((uint32_t) align);
            sect_SkipBytes((uint32_t) offset);
            return true;
        }
        case T_POP_DSB: {
            parse_GetToken();

            int32_t offset = parse_ConstantExpression();
            if (offset >= 0) {
                sect_SkipBytes((uint32_t) offset);
                return true;
            } else {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
        }
        case T_POP_DSW: {
            parse_GetToken();

            int32_t offset = parse_ConstantExpression();
            if (offset >= 0) {
                sect_SkipBytes((uint32_t) offset * 2);
                return true;
            } else {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
        }
        case T_POP_DSL: {
            parse_GetToken();

            int32_t offset = parse_ConstantExpression();
            if (offset >= 0) {
                sect_SkipBytes((uint32_t) offset * 4);
                return true;
            } else {
                prj_Error(ERROR_EXPR_POSITIVE);
                return true;
            }
        }
        case T_POP_DB: {
            SExpression* expr;
            char* s;

            do {
                parse_GetToken();
                if ((expr = parse_Expression(1)) != NULL) {
                    expr = expr_CheckRange(expr, -128, 255);
                    if (expr)
                        sect_OutputExpr8(expr);
                    else
                        prj_Error(ERROR_EXPRESSION_N_BIT, 8);
                } else if ((s = parse_StringExpressionRaw_Pri0()) != NULL) {
                    while (*s)
                        sect_OutputConst8((uint8_t) *s++);
                } else
                    sect_SkipBytes(1); //prj_Error(ERROR_INVALID_EXPRESSION);
            } while (lex_Current.token == ',');

            return true;
        }
        case T_POP_DW: {
            SExpression* expr;

            do {
                parse_GetToken();
                expr = parse_Expression(2);
                if (expr) {
                    expr = expr_CheckRange(expr, -32768, 65535);
                    if (expr)
                        sect_OutputExpr16(expr);
                    else
                        prj_Error(ERROR_EXPRESSION_N_BIT, 16);
                } else
                    sect_SkipBytes(2); // prj_Error(ERROR_INVALID_EXPRESSION);
            } while (lex_Current.token == ',');

            return true;
        }
        case T_POP_DL: {
            SExpression* expr;

            do {
                parse_GetToken();
                expr = parse_Expression(4);
                if (expr)
                    sect_OutputExpr32(expr);
                else
                    sect_SkipBytes(4); //prj_Error(ERROR_INVALID_EXPRESSION);
            } while (lex_Current.token == ',');

            return true;
        }
        case T_POP_INCLUDE: {
            SLexerBookmark mark;
            char* r;

            lex_Bookmark(&mark);
            parse_GetToken();
            if ((r = parse_StringExpressionRaw_Pri0()) == NULL) {
                size_t pEnd = 0;

                while (lex_PeekChar(pEnd) == ' ' || lex_PeekChar(pEnd) == '\t')
                    ++pEnd;

                while (!isspace((unsigned char) lex_PeekChar(pEnd)))
                    ++pEnd;

                r = mem_Alloc(pEnd + 1);
                fstk_Current->lineNumber += lex_GetChars(r, pEnd);
                r[pEnd] = 0;
                parse_GetToken();
            }

            if (r != NULL) {
                string* pFile = str_Create(r);
                fstk_ProcessIncludeFile(pFile);
                str_Free(pFile);
                mem_Free(r);
                return true;
            } else {
                prj_Error(ERROR_EXPR_STRING);
                return false;
            }
        }
        case T_POP_INCBIN: {
            char* r;

            parse_GetToken();
            if ((r = stringExpression()) != NULL) {
                string* pFile = str_Create(r);
                sect_OutputBinaryFile(pFile);
                mem_Free(r);
                str_Free(pFile);
                return true;
            }
            return false;
        }
        case T_POP_REPT: {
            parse_GetToken();
            int32_t reptCount = parse_ConstantExpression();

            size_t reptSize;
            char* reptBlock;
            if (copyReptBlock(&reptBlock, &reptSize)) {
                if (reptCount > 0) {
                    fstk_ProcessRepeatBlock(reptBlock, reptSize, (uint32_t) reptCount);
                } else if (reptCount < 0) {
                    prj_Error(ERROR_EXPR_POSITIVE);
                    mem_Free(reptBlock);
                } else {
                    mem_Free(reptBlock);
                }
                return true;
            } else {
                prj_Fail(ERROR_NEED_ENDR);
                return false;
            }
        }
        case T_POP_SHIFT: {
            SExpression* expr;

            parse_GetToken();
            expr = parse_Expression(4);
            if (expr) {
                if (expr_IsConstant(expr)) {
                    fstk_ShiftMacroArgs(expr->value.integer);
                    expr_Free(expr);
                    return true;
                } else {
                    prj_Fail(ERROR_EXPR_CONST);
                    return false;
                }
            } else {
                fstk_ShiftMacroArgs(1);
                return true;
            }
        }
        case T_POP_IFC: {
            char* s1;

            parse_GetToken();
            s1 = stringExpression();
            if (s1 != NULL) {
                if (parse_ExpectComma()) {
                    char* s2;

                    s2 = stringExpression();
                    if (s2 != NULL) {
                        if (strcmp(s1, s2) != 0)
                            skipToElse();

                        mem_Free(s1);
                        mem_Free(s2);
                        return true;
                    } else
                        prj_Error(ERROR_EXPR_STRING);
                }
                mem_Free(s1);
            } else
                prj_Error(ERROR_EXPR_STRING);

            return false;
        }
        case T_POP_IFNC: {
            char* s1;

            parse_GetToken();
            s1 = stringExpression();
            if (s1 != NULL) {
                if (parse_ExpectComma()) {
                    char* s2;

                    s2 = stringExpression();
                    if (s2 != NULL) {
                        if (strcmp(s1, s2) == 0)
                            skipToElse();

                        mem_Free(s1);
                        mem_Free(s2);
                        return true;
                    } else
                        prj_Error(ERROR_EXPR_STRING);
                }
                mem_Free(s1);
            } else
                prj_Error(ERROR_EXPR_STRING);

            return false;
        }
        case T_POP_IFD: {
            parse_GetToken();

            if (lex_Current.token == T_ID) {
                string* pName = str_Create(lex_Current.value.string);
                if (sym_IsDefined(pName)) {
                    parse_GetToken();
                } else {
                    parse_GetToken();
                    /* will continue parsing just after ELSE or just at ENDC keyword */
                    skipToElse();
                }
                str_Free(pName);
                return true;
            }
            prj_Error(ERROR_EXPECT_IDENTIFIER);
            return false;
        }
        case T_POP_IFND: {
            parse_GetToken();

            if (lex_Current.token == T_ID) {
                string* pName = str_Create(lex_Current.value.string);
                if (!sym_IsDefined(pName)) {
                    parse_GetToken();
                } else {
                    parse_GetToken();
                    /* will continue parsing just after ELSE or just at ENDC keyword */
                    skipToElse();
                }
                str_Free(pName);
                return true;
            }
            prj_Error(ERROR_EXPECT_IDENTIFIER);
            return false;
        }
        case T_POP_IF: {
            parse_GetToken();

            if (parse_ConstantExpression() == 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
                parse_GetToken();
            }
            return true;
        }
        case T_POP_IFEQ: {
            parse_GetToken();

            if (parse_ConstantExpression() != 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFGT: {
            parse_GetToken();

            if (parse_ConstantExpression() <= 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFGE: {
            parse_GetToken();

            if (parse_ConstantExpression() < 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFLT: {
            parse_GetToken();

            if (parse_ConstantExpression() >= 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_IFLE: {
            parse_GetToken();

            if (parse_ConstantExpression() > 0) {
                /* will continue parsing just after ELSE or just at ENDC keyword */
                skipToElse();
            }
            return true;
        }
        case T_POP_ELSE: {
            /* will continue parsing just at ENDC keyword */
            skipToEndc();
            parse_GetToken();
            return true;
        }
        case T_POP_ENDC: {
            parse_GetToken();
            return true;
        }
        case T_POP_PUSHO: {
            opt_Push();
            parse_GetToken();
            return true;
        }
        case T_POP_POPO: {
            opt_Pop();
            parse_GetToken();
            return true;
        }
        case T_POP_OPT: {
            lex_SetState(LEX_STATE_MACRO_ARGS);
            parse_GetToken();
            if (lex_Current.token == T_STRING) {
                opt_Parse(lex_Current.value.string);
                parse_GetToken();
                while (lex_Current.token == ',') {
                    parse_GetToken();
                    opt_Parse(lex_Current.value.string);
                    parse_GetToken();
                }
            }
            lex_SetState(LEX_STATE_NORMAL);
            return true;
        }
        default: {
            return false;
        }
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
parse_Do(void) {
    bool r = true;

    lex_GetNextToken();

    while (lex_Current.token && r) {
        if (!parse_TargetSpecific() && !parse_Symbol() && !parse_PseudoOp() && !parse_Misc()) {
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
