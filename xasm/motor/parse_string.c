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
#include <ctype.h>

/* From xasm */
#include "errors.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include "parse_string.h"

/* From util */
#include "fmath.h"
#include "str.h"
#include "strbuf.h"

/* Internal functions */

static const char*
determineCodeEnd(const char* literal);

static const char*
determineStringEnd(const char* literal, char endChar) {
    for (char ch = *literal; ch != 0 && ch != endChar; ch = *(++literal)) {
        if (ch == '\\' && literal[1] != 0) {
            ++literal;
        } else if (ch == '{') {
            literal = determineCodeEnd(literal + 1);
        }
    }
    return literal;
}

static const char*
determineCodeEnd(const char* literal) {
    for (char ch = *literal; ch != 0 && ch != '}'; ch = *(++literal)) {
        if (ch == '\'' || ch == '"') {
            literal = determineStringEnd(literal + 1, ch);
        }
    }
    return literal;
}

static bool
parseFormatSpecifier(char* format, uint32_t* precision) {
    if (lex_Context->token.id == ':') {
        char f = lex_GetChar();
        if (isalpha(f)) {
            *format = toupper(f);
            parse_GetToken();

            SExpression* expression = parse_Expression(4);
            if (expression != NULL && expr_IsConstant(expression)) {
                *precision = expression->value.integer;
            } else {
                *precision = 0;
            }
            expr_Free(expression);
            return true;
        }
		lex_UnputChar(f);
    }

    *format = 'D';
    *precision = 0;
    return 0;
}

static string*
formatValue(int32_t value, char format, int32_t precision) {
    string_buffer* buf = strbuf_Create();
    switch (format) {
        case 'D': {
            if (value < 0) {
                strbuf_AppendChar(buf, '-');
                value = -value;
            }
            strbuf_AppendFormat(buf, "%0*d", precision < 0 ? 0 : precision, value);
            break;
        }
        case 'F': {
            if (precision == 0)
                precision = 5;

            uint32_t low = imuldiv(value & 0xFFFF, 100000, 65536);
            char fraction[8];
            snprintf(fraction, sizeof(fraction), "%05d", low);
            fraction[precision] = 0;

            int32_t high = asr(value, 16);
            strbuf_AppendFormat(buf, "%d.%s", high, fraction);
            break;
        }
        case 'X': {
            char formatted[10];
            snprintf(formatted, sizeof(formatted), "%08X", value);
            if (precision > 0)
                memcpy(formatted, formatted + 8 - precision, precision + 1);
            strbuf_AppendStringZero(buf, formatted);
            break;
        }
        case 'C': {
            strbuf_AppendChar(buf, value & 0xFF);
            break;
        }
    }

    string *str = strbuf_String(buf);
    strbuf_Free(buf);
    return str;
}

static int32_t
parseAlignment(void) {
    if (lex_Context->token.id == ',') {
        parse_GetToken();
        return parse_ConstantExpression();
    }

    return 0;
}

static string*
parseIntegerExpressionAndFormat(void) {
    SExpression* expression = parse_Expression(4);
    if (expression != NULL && expr_IsConstant(expression)) {
        int32_t value = expression->value.integer;
        expr_Free(expression);

        int32_t alignment = parseAlignment();

        char format;
        uint32_t precision;
        parseFormatSpecifier(&format, &precision);

        string* formatted = formatValue(value, format, precision);
        string* aligned = str_Align(formatted, alignment);
        str_Free(formatted);
        return aligned;
    }

    return NULL;
}

static string*
parseStringExpressionAndFormat(void) {
    string* substring = parse_StringExpression();
    if (substring != NULL) {
        int32_t alignment = parseAlignment();

        string* aligned = str_Align(substring, alignment);
        str_Free(substring);
        return aligned;
    }

    return NULL;
}

static string*
parseSubstring(void) {
    SLexerContext bookmark;
    lex_Bookmark(&bookmark);

    string* substr;
    if ((substr = parseStringExpressionAndFormat()) != NULL) {
        return substr;
    }

    lex_Goto(&bookmark);
    
    if ((substr = parseIntegerExpressionAndFormat()) != NULL) {
        return substr;
    }

    lex_Goto(&bookmark);

    return NULL;
} 

static const char*
parseEmbeddedExpression(string_buffer* resultBuffer, const char* literal) {
    const char* codeEnd = determineCodeEnd(literal) + 1;
    lex_UnputStringLength(literal, codeEnd - literal);

    parse_GetToken();

    string* substr = parseSubstring();
    if (substr != NULL)
        strbuf_AppendString(resultBuffer, substr);
    str_Free(substr);

    return codeEnd;
}

static char
unescapeCharacter(char ch) {
    switch (ch) {
        case 'n': return '\n';
        case 't': return '\t';
        default: return ch;
    }
}

static string*
interpolateString(const char* literal) {
    string_buffer* resultBuffer = strbuf_Create();
    char ch;

    while ((ch = *literal++) != 0) {
        if (ch == '\\') {
            ch = *literal++;
            if (ch == 0)
                break;

            strbuf_AppendChar(resultBuffer, unescapeCharacter(ch));
        } else if (ch == '{') {
            literal = parseEmbeddedExpression(resultBuffer, literal);
        } else {
            strbuf_AppendChar(resultBuffer, ch);
        }
    }

    string* result = strbuf_String(resultBuffer);
    strbuf_Free(resultBuffer);

    parse_GetToken();

    return result;
}

static string*
stringExpressionPri2(void) {
    SLexerContext bm;
    lex_Bookmark(&bm);

    switch (lex_Context->token.id) {
        case T_STRING: {
            string* literal = lex_TokenString();
            string* result = interpolateString(str_String(literal));
            str_Free(literal);

            return result;
        }
        case T_OP_BITWISE_OR: {
            parse_ExpandStrings = false;
            parse_GetToken();
            parse_ExpandStrings = true;

            if (lex_Context->token.id == T_ID) {
                string* result = NULL;

                string* symbol = lex_TokenString();
                parse_GetToken();
                if (lex_Context->token.id == T_OP_BITWISE_OR) {
                    result = sym_GetSymbolValueAsStringByName(symbol);
                    parse_GetToken();
                }
                str_Free(symbol);
                return result;
            }
            break;
        }
        case (EToken) '(': {
            parse_GetToken();

            string* r = parse_StringExpression();
            if (r != NULL) {
                if (lex_Context->token.id == ')') {
                    parse_GetToken();
                    return r;
                }
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

    SLexerContext bm;
    for (lex_Bookmark(&bm); parse_IsDot(); lex_Bookmark(&bm)) {
        switch (lex_Context->token.id) {
            case T_STR_MEMBER_SLICE: {
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
                if (lex_Context->token.id == ',') {
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
            case T_STR_MEMBER_UPPER: {
                parse_GetToken();
                str_ToUpperReplace(&t);

                break;
            }
            case T_STR_MEMBER_LOWER: {
                parse_GetToken();
                str_ToLowerReplace(&t);

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

    while (lex_Context->token.id == T_OP_ADD) {
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
        err_Error(ERROR_EXPR_STRING);

    return s;
}
