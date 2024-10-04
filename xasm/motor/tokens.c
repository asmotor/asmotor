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

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

// From util
#include "util.h"
#include "fmath.h"
#include "mem.h"
#include "str.h"
#include "strbuf.h"

// From xasm
#include "xasm.h"
#include "errors.h"
#include "lexer_context.h"
#include "lexer_constants.h"
#include "options.h"
#include "parse.h"
#include "symbol.h"

#if defined(_MSC_VER)
#   define sscanf sscanf_s
#endif

// Private data

static SLexConstantsWord staticTokens[] = {
        {"||",        T_OP_BOOLEAN_OR},
        {"&&",        T_OP_BOOLEAN_AND},
        {"==",        T_OP_EQUAL},
        {">",         T_OP_GREATER_THAN},
        {"<",         T_OP_LESS_THAN},
        {">=",        T_OP_GREATER_OR_EQUAL},
        {"<=",        T_OP_LESS_OR_EQUAL},
        {"~=",        T_OP_NOT_EQUAL},
        {"~!",        T_OP_BOOLEAN_NOT},
        {"|",         T_OP_BITWISE_OR},
        {"!",         T_OP_BITWISE_OR},
        {"^",         T_OP_BITWISE_XOR},
        {"&",         T_OP_BITWISE_AND},
        {"<<",        T_OP_BITWISE_ASL},
        {">>",        T_OP_BITWISE_ASR},
        {"+",         T_OP_ADD},
        {"-",         T_OP_SUBTRACT},
        {"*",         T_OP_MULTIPLY},
        {"/",         T_OP_DIVIDE},
        {"~/",        T_OP_MODULO},
        {"~",         T_OP_BITWISE_NOT},
        {"//",        T_OP_FDIV},
        {"**",        T_OP_FMUL},

        {"DEF",       T_FUNC_DEF},
        {"ALIGN",     T_FUNC_ALIGN},
        {"ROOT",      T_FUNC_ROOT},

        {"SIN",       T_FUNC_SIN},
        {"COS",       T_FUNC_COS},
        {"TAN",       T_FUNC_TAN},
        {"ASIN",      T_FUNC_ASIN},
        {"ACOS",      T_FUNC_ACOS},
        {"ATAN",      T_FUNC_ATAN},
        {"ATAN2",     T_FUNC_ATAN2},
        {"ASFLOAT",   T_FUNC_ASFLOAT},

        {"COMPARETO", T_STR_MEMBER_COMPARETO},
        {"INDEXOF",   T_STR_MEMBER_INDEXOF},
        {"SLICE",     T_STR_MEMBER_SLICE},
        {"LENGTH",    T_STR_MEMBER_LENGTH},
        {"UPPER",     T_STR_MEMBER_UPPER},
        {"LOWER",     T_STR_MEMBER_LOWER},

        {"PRINTT",    T_DIRECTIVE_PRINTT},
        {"PRINTV",    T_DIRECTIVE_PRINTV},
        {"PRINTF",    T_DIRECTIVE_PRINTF},
        {"EXPORT",    T_DIRECTIVE_EXPORT},
        {"XDEF",      T_DIRECTIVE_EXPORT},
        {"IMPORT",    T_DIRECTIVE_IMPORT},
        {"XREF",      T_DIRECTIVE_IMPORT},
        {"GLOBAL",    T_DIRECTIVE_GLOBAL},

        {"RSRESET",   T_DIRECTIVE_RSRESET},
        {"RSSET",     T_DIRECTIVE_RSSET},
        {"RSEND",     T_DIRECTIVE_RSEND},

        {"SET",       T_SYM_SET},
        {"=",         T_SYM_SET},

        {"SECTION",   T_DIRECTIVE_SECTION},
        {"GROUP",     T_SYM_GROUP},
        {"TEXT",      T_GROUP_TEXT},
        {"RAM",       T_GROUP_BSS},
        {"ORG",       T_DIRECTIVE_ORG},
        {"ONCE",      T_INCLUDE_ONCE},

        {"EQU",       T_SYM_EQU},
        {"EQUF",      T_SYM_EQUF},
        {"EQUS",      T_SYM_EQUS},

        {"PURGE",     T_DIRECTIVE_PURGE},

        {"FAIL",      T_DIRECTIVE_FAIL},
        {"WARN",      T_DIRECTIVE_WARN},

        {"INCLUDE",   T_DIRECTIVE_INCLUDE},
        {"INCBIN",    T_DIRECTIVE_INCBIN},

        {"REPT",      T_DIRECTIVE_REPT},
        {"ENDR",      T_DIRECTIVE_ENDR},
        {"REXIT",     T_DIRECTIVE_REXIT},

        {"IF",        T_DIRECTIVE_IF},
        {"IFC",       T_DIRECTIVE_IFC},
        {"IFD",       T_DIRECTIVE_IFD},
        {"IFNC",      T_DIRECTIVE_IFNC},
        {"IFND",      T_DIRECTIVE_IFND},
        {"IFNE",      T_DIRECTIVE_IF},
        {"IFEQ",      T_DIRECTIVE_IFEQ},
        {"IFGT",      T_DIRECTIVE_IFGT},
        {"IFGE",      T_DIRECTIVE_IFGE},
        {"IFLT",      T_DIRECTIVE_IFLT},
        {"IFLE",      T_DIRECTIVE_IFLE},
        {"ELSE",      T_DIRECTIVE_ELSE},
        {"ENDC",      T_DIRECTIVE_ENDC},

        {"MACRO",     T_SYM_MACRO},
        {"SHIFT",     T_DIRECTIVE_SHIFT},
        {"MEXIT",     T_DIRECTIVE_MEXIT},
        {"ENDM",      T_DIRECTIVE_ENDM},

        {"PUSHS",     T_DIRECTIVE_PUSHS},
        {"POPS",      T_DIRECTIVE_POPS},
        {"PUSHO",     T_DIRECTIVE_PUSHO},
        {"POPO",      T_DIRECTIVE_POPO},

        {"OPT",       T_DIRECTIVE_OPT},

        {"LOWLIMIT",  T_FUNC_LOWLIMIT},
        {"HIGHLIMIT", T_FUNC_HIGHLIMIT},

        {"EVEN",      T_DIRECTIVE_EVEN},
        {"CNOP",      T_DIRECTIVE_CNOP},

        {"END",       T_POP_END},

        {NULL,        0}
};

void
tokens_Init(bool supportFloat) {
    parse_ExpandStrings = true;

    lex_ConstantsDefineWords(staticTokens);

    if (xasm_Configuration->minimumWordSize <= MINSIZE_8BIT)
        lex_ConstantsDefineWord("__RSB", T_DIRECTIVE_RB);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_16BIT)
        lex_ConstantsDefineWord("__RSW", T_DIRECTIVE_RW);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_32BIT)
        lex_ConstantsDefineWord("__RSL", T_DIRECTIVE_RL);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_64BIT)
        lex_ConstantsDefineWord("__RSD", T_DIRECTIVE_RD);

    if (xasm_Configuration->reserveByteName && xasm_Configuration->minimumWordSize <= MINSIZE_8BIT)
        lex_ConstantsDefineWord(xasm_Configuration->reserveByteName, T_DIRECTIVE_RB);
    if (xasm_Configuration->reserveWordName && xasm_Configuration->minimumWordSize <= MINSIZE_16BIT)
        lex_ConstantsDefineWord(xasm_Configuration->reserveWordName, T_DIRECTIVE_RW);
    if (xasm_Configuration->reserveLongName && xasm_Configuration->minimumWordSize <= MINSIZE_32BIT)
        lex_ConstantsDefineWord(xasm_Configuration->reserveLongName, T_DIRECTIVE_RL);
    if (xasm_Configuration->reserveDoubleName && xasm_Configuration->minimumWordSize <= MINSIZE_64BIT)
        lex_ConstantsDefineWord(xasm_Configuration->reserveDoubleName, T_DIRECTIVE_RD);

    if (xasm_Configuration->minimumWordSize <= MINSIZE_8BIT)
        lex_ConstantsDefineWord("__DSB", T_DIRECTIVE_DSB);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_16BIT)
        lex_ConstantsDefineWord("__DSW", T_DIRECTIVE_DSW);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_32BIT)
        lex_ConstantsDefineWord("__DSL", T_DIRECTIVE_DSL);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_64BIT)
        lex_ConstantsDefineWord("__DSD", T_DIRECTIVE_DSD);

    if (xasm_Configuration->defineByteSpaceName && xasm_Configuration->minimumWordSize <= MINSIZE_8BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineByteSpaceName, T_DIRECTIVE_DSB);
    if (xasm_Configuration->defineWordSpaceName && xasm_Configuration->minimumWordSize <= MINSIZE_16BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineWordSpaceName, T_DIRECTIVE_DSW);
    if (xasm_Configuration->defineLongSpaceName && xasm_Configuration->minimumWordSize <= MINSIZE_32BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineLongSpaceName, T_DIRECTIVE_DSL);
    if (xasm_Configuration->defineDoubleSpaceName && xasm_Configuration->minimumWordSize <= MINSIZE_64BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineDoubleSpaceName, T_DIRECTIVE_DSD);

    if (xasm_Configuration->minimumWordSize <= MINSIZE_8BIT)
        lex_ConstantsDefineWord("__DCB", T_DIRECTIVE_DB);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_16BIT)
        lex_ConstantsDefineWord("__DCW", T_DIRECTIVE_DW);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_32BIT)
        lex_ConstantsDefineWord("__DCL", T_DIRECTIVE_DL);
    if (xasm_Configuration->minimumWordSize <= MINSIZE_64BIT)
        lex_ConstantsDefineWord("__DCD", T_DIRECTIVE_DD);

    if (xasm_Configuration->defineByteName && xasm_Configuration->minimumWordSize <= MINSIZE_8BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineByteName, T_DIRECTIVE_DB);
    if (xasm_Configuration->defineWordName && xasm_Configuration->minimumWordSize <= MINSIZE_16BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineWordName, T_DIRECTIVE_DW);
    if (xasm_Configuration->defineLongName && xasm_Configuration->minimumWordSize <= MINSIZE_32BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineLongName, T_DIRECTIVE_DL);
    if (xasm_Configuration->defineDoubleName && xasm_Configuration->minimumWordSize <= MINSIZE_64BIT)
        lex_ConstantsDefineWord(xasm_Configuration->defineDoubleName, T_DIRECTIVE_DD);

    if (xasm_Configuration->supportBanks)
        lex_ConstantsDefineWord("BANK", T_FUNC_BANK);
}
