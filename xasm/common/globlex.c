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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "asmotor.h"
#include "xasm.h"
#include "mem.h"
#include "lexer.h"
#include "options.h"
#include "filestack.h"
#include "project.h"
#include "symbol.h"
#include "strbuf.h"


bool_t g_bDontExpandStrings;
uint32_t BinaryConstID;

/*	Private data */

static SLexInitString staticstrings[] =
{
    {"||",	T_OP_LOGICOR},
    {"&&",	T_OP_LOGICAND},
    {"==",	T_OP_LOGICEQU},
    {">",	T_OP_LOGICGT},
    {"<",	T_OP_LOGICLT},
    {">=",	T_OP_LOGICGE},
    {"<=",	T_OP_LOGICLE},
    {"~=",	T_OP_LOGICNE},
    {"~!",	T_OP_LOGICNOT},
    {"|",	T_OP_OR},
    {"!",	T_OP_OR},
    {"^",	T_OP_XOR},
    {"&",	T_OP_AND},
    {"<<",	T_OP_SHL},
    {">>",	T_OP_SHR},
    {"+",	T_OP_ADD},
    {"-",	T_OP_SUB},
    {"*",	T_OP_MUL},
    {"/",	T_OP_DIV},
    {"~/",	T_OP_MOD},
    {"~",	T_OP_NOT},

    {"def",	T_FUNC_DEF},

    {"//",	T_FUNC_FDIV},
    {"**",	T_FUNC_FMUL},

    {"sin",	T_FUNC_SIN},
    {"cos",	T_FUNC_COS},
    {"tan",	T_FUNC_TAN},
    {"asin",	T_FUNC_ASIN},
    {"acos",	T_FUNC_ACOS},
    {"atan",	T_FUNC_ATAN},
    {"atan2",	T_FUNC_ATAN2},

    {"compareto",	T_FUNC_COMPARETO},
    {"indexof",	T_FUNC_INDEXOF},
    {"slice",	T_FUNC_SLICE},
    {"length",	T_FUNC_LENGTH},
    {"toupper",	T_FUNC_TOUPPER},
    {"tolower",	T_FUNC_TOLOWER},

    {"printt",	T_POP_PRINTT},
    {"printv",	T_POP_PRINTV},
    {"printf",	T_POP_PRINTF},
    {"export",	T_POP_EXPORT},
    {"xdef",	T_POP_EXPORT},
    {"import",	T_POP_IMPORT},
    {"xref",	T_POP_IMPORT},
    {"global",	T_POP_GLOBAL},

    {"rsreset",	T_POP_RSRESET},
    {"rsset",	T_POP_RSSET},

    {"set",		T_POP_SET},
    {"=",		T_POP_SET},

    {"section",	T_POP_SECTION},
    {"group",	T_POP_GROUP},
    {"text",	T_GROUP_TEXT},
    {"ram",		T_GROUP_BSS},
    {"org",		T_POP_ORG},

    {"equ",		T_POP_EQU},
    {"equs",	T_POP_EQUS},

    {"purge",	T_POP_PURGE},

    {"fail",	T_POP_FAIL},
    {"warn",	T_POP_WARN},

    {"include",	T_POP_INCLUDE},
    {"incbin",	T_POP_INCBIN},

    {"rept",	T_POP_REPT},
    {"endr",	T_POP_ENDR},	/*	Not needed but we have it here just to protect the name */
    {"rexit",	T_POP_REXIT},

    {"if",		T_POP_IF},
    {"ifc",		T_POP_IFC},
    {"ifd",		T_POP_IFD},
    {"ifnc",	T_POP_IFNC},
    {"ifnd",	T_POP_IFND},
    {"ifne",	T_POP_IF},
    {"ifeq",	T_POP_IFEQ},
    {"ifgt",	T_POP_IFGT},
    {"ifge",	T_POP_IFGE},
    {"iflt",	T_POP_IFLT},
    {"ifle",	T_POP_IFLE},
    {"else",	T_POP_ELSE},
    {"endc",	T_POP_ENDC},

    {"macro",	T_POP_MACRO},
    {"endm",	T_POP_ENDM},	/*	Not needed but we have it here just to protect the name */
    {"shift",	T_POP_SHIFT},
    {"mexit",	T_POP_MEXIT},

    {"pushs",	T_POP_PUSHS},
    {"pops",	T_POP_POPS},
    {"pusho",	T_POP_PUSHO},
    {"popo",	T_POP_POPO},

    {"opt",		T_POP_OPT},

    {"lowlimit",	T_FUNC_LOWLIMIT},
    {"highlimit",	T_FUNC_HIGHLIMIT},

    {"even",	T_POP_EVEN},
    {"cnop",	T_POP_CNOP},

    {"end",	T_POP_END},

    {NULL, 0}
};

static int32_t binary2bin(char ch)
{
    int32_t i;

    for(i = 0; i <= 1; ++i)
    {
        if(g_pOptions->BinaryChar[i] == ch)
            return i;
    }

    return 0;
}

static int32_t char2bin(char ch)
{
    if(ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;

    if(ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;

    if(ch >= '0' && ch <= '9')
        return ch - '0';

    return 0;
}

typedef	int32_t (*x2bin)(char ch);

static int32_t ascii2bin(char* s, size_t len)
{
    int32_t radix = 10;
    int32_t result = 0;
    x2bin convertfunc = char2bin;

    switch(*s)
    {
        case '$':
            radix = 16;
            ++s;
            --len;
            convertfunc = char2bin;
            break;
        case '%':
            radix = 2;
            ++s;
            --len;
            convertfunc = binary2bin;
            break;
    }

    while(*s != '\0' && len-- > 0)
        result = result * radix + convertfunc(*s++);

    return result;
}


static bool_t ParseNumber(char* s, uint32_t size)
{
    g_CurrentToken.Value.nInteger = ascii2bin(s, size);
    return true;
}

static bool_t ParseDecimal(char* s, uint32_t size)
{
    uint32_t integer = 0;

    while(*s >= '0' && *s <= '9' && size-- > 0)
        integer = integer * 10 + *s++ - '0';

    if(*s == '.')
    {
        uint32_t fraction = 0;
        uint32_t d = 1;
        ++s;
        --size;
        while(*s >= '0' && *s <= '9' && size-- > 0)
        {
            fraction = fraction * 10 + *s++ - '0';
            d *= 10;
        }

        integer = (integer << 16) + imuldiv(fraction, 65536, d);
    }

    g_CurrentToken.TokenLength -= size;
    g_CurrentToken.Value.nInteger = integer;
    return true;
}

static bool_t ParseSymbol(char* src, uint32_t size)
{
    stringbuffer* pBuffer = strbuf_Create();
    string* pString;
    uint32_t i;
    bool_t r;

    for(i = 0; i < size; ++i)
    {
        char ch = *src++;
        
        if(ch == '\\')
        {
            if(i + 1 < size)
            {
                char* marg = NULL;

                ch = *src++;
                ++i;
                
                if(ch == '@')
                    marg = fstk_GetMacroRunID();
                else if(ch >= '0' && ch <= '9')
                    marg = fstk_GetMacroArgValue(ch);
                    
                if(marg)
                {
                    strbuf_AppendStringZero(pBuffer, marg);
                    continue;
                }
            }

            prj_Fail(ERROR_ID_MALFORMED);
            strbuf_Free(pBuffer);
            return false;
        }

        strbuf_AppendChar(pBuffer, ch);
    }

    pString = strbuf_String(pBuffer);
    strbuf_Free(pBuffer);
    
    if(g_bDontExpandStrings == 0 && sym_IsString(pString))
    {
        string* pValue = sym_GetStringValueByName(pString);
        int len = str_Length(pValue);
        int i;

        lex_SkipBytes(size);
        lex_UnputString(str_String(pValue));

        for(i = 0; i < len; ++i)
        {
            if(str_CharAt(pValue, i) == '\n')
                g_pFileContext->LineNumber -= 1;
        }
        
        r = false;
        str_Free(pValue);
    }
    else
    {
        strcpy(g_CurrentToken.Value.aString, str_String(pString));
        r = true;
    }
    
    str_Free(pString);
    return r;
}

bool_t ParseMacroArg(char* src, uint32_t size)
{
    char* arg = fstk_GetMacroArgValue(src[1]);
    lex_SkipBytes(size);
    if (arg != NULL)
        lex_UnputString(arg);
    return false;
}

bool_t ParseUniqueArg(char* src, uint32_t size)
{
    assert(src != NULL);

    lex_SkipBytes(size);
    lex_UnputString(fstk_GetMacroRunID());
    return false;
}

enum
{
    T_LEX_MACROARG = 3000,
    T_LEX_MACROUNIQUE
};

static SVariadicWordDefinition s_sMacroArgToken=
{
    ParseMacroArg,
    T_LEX_MACROARG
};

static SVariadicWordDefinition s_sMacroUniqueToken=
{
    ParseUniqueArg,
    T_LEX_MACROUNIQUE
};

static SVariadicWordDefinition s_sDecimal =
{
    ParseDecimal,
    T_NUMBER
};

static SVariadicWordDefinition s_sNumberToken=
{
    ParseNumber,
    T_NUMBER
};

static SVariadicWordDefinition s_sIDToken=
{
    ParseSymbol,
    T_ID
};

void globlex_Init(void)
{
    uint32_t id;

    g_bDontExpandStrings = false;

    lex_Init();

    lex_AddStrings(staticstrings);

    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT)
        lex_AddString("__RSB", T_POP_RB);
    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT)
        lex_AddString("__RSW", T_POP_RW);
    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT)
        lex_AddString("__RSL", T_POP_RL);
    
    if(g_pConfiguration->pszNameRB && g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT)
        lex_AddString(g_pConfiguration->pszNameRB, T_POP_RB);
    if(g_pConfiguration->pszNameRW && g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT)
        lex_AddString(g_pConfiguration->pszNameRW, T_POP_RW);
    if(g_pConfiguration->pszNameRL && g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT)
        lex_AddString(g_pConfiguration->pszNameRL, T_POP_RL);

    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT)
        lex_AddString("__DSB", T_POP_DSB);
    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT)
        lex_AddString("__DSW", T_POP_DSW);
    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT)
        lex_AddString("__DSL", T_POP_DSL);
    
    if(g_pConfiguration->pszNameDSB && g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT)
        lex_AddString(g_pConfiguration->pszNameDSB, T_POP_DSB);
    if(g_pConfiguration->pszNameDSW && g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT)
        lex_AddString(g_pConfiguration->pszNameDSW, T_POP_DSW);
    if(g_pConfiguration->pszNameDSL && g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT)
        lex_AddString(g_pConfiguration->pszNameDSL, T_POP_DSL);

    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT)
        lex_AddString("__DCB", T_POP_DB);
    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT)
        lex_AddString("__DCW", T_POP_DW);
    if(g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT)
        lex_AddString("__DCL", T_POP_DL);
    
    if(g_pConfiguration->pszNameDB && g_pConfiguration->eMinimumWordSize <= MINSIZE_8BIT)
        lex_AddString(g_pConfiguration->pszNameDB, T_POP_DB);
    if(g_pConfiguration->pszNameDW && g_pConfiguration->eMinimumWordSize <= MINSIZE_16BIT)
        lex_AddString(g_pConfiguration->pszNameDW, T_POP_DW);
    if(g_pConfiguration->pszNameDL && g_pConfiguration->eMinimumWordSize <= MINSIZE_32BIT)
        lex_AddString(g_pConfiguration->pszNameDL, T_POP_DL);

    if(g_pConfiguration->bSupportBanks)
        lex_AddString("bank", T_FUNC_BANK);

    /* Local ID */

    id = lex_VariadicCreateWord(&s_sIDToken);
    lex_FloatAddRange(id, '.', '.', 0);
    lex_FloatAddRange(id, 'a', 'z', 1);
    lex_FloatAddRange(id, 'A', 'Z', 1);
    lex_FloatAddRange(id, '_', '_', 1);
    lex_FloatAddRange(id, '#', '#', 1);
    lex_FloatAddRange(id, '\\', '\\', 1);
    lex_FloatAddRangeAndBeyond(id, 'a', 'z', 2);
    lex_FloatAddRangeAndBeyond(id, 'A', 'Z', 2);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 2);
    lex_FloatAddRangeAndBeyond(id, '_', '_', 2);
    lex_FloatAddRangeAndBeyond(id, '\\', '\\', 2);
    lex_FloatAddRangeAndBeyond(id, '@', '@', 2);
    lex_FloatAddRangeAndBeyond(id, '#', '#', 2);

    id = lex_VariadicCreateWord(&s_sIDToken);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 0);
    lex_FloatAddRangeAndBeyond(id, '\\', '\\', 0);
    lex_FloatAddRangeAndBeyond(id, '@', '@', 1);
    lex_FloatSetSuffix(id, '$');

    /* Macro arguments */

    id = lex_VariadicCreateWord(&s_sMacroArgToken);
    lex_FloatAddRange(id, '\\', '\\', 0);
    lex_FloatAddRange(id, '0', '9', 1);

    id = lex_VariadicCreateWord(&s_sMacroUniqueToken);
    lex_FloatAddRange(id, '\\', '\\', 0);
    lex_FloatAddRange(id, '@', '@', 1);

    /* Decimal constants */

    id = lex_VariadicCreateWord(&s_sDecimal);
    lex_FloatAddRange(id, '0', '9', 0);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 1);
    lex_FloatAddRangeAndBeyond(id, '.', '.', 1);

    /* Hex constants */

    id = lex_VariadicCreateWord(&s_sNumberToken);
    lex_FloatAddRange(id, '$', '$', 0);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 1);
    lex_FloatAddRangeAndBeyond(id, 'A', 'F', 1);
    lex_FloatAddRangeAndBeyond(id, 'a', 'f', 1);

    /* Binary constants */

    BinaryConstID = id = lex_VariadicCreateWord(&s_sNumberToken);
    lex_FloatAddRange(id, '%', '%', 0);
    lex_FloatAddRangeAndBeyond(id, '0', '1', 1);

    /* ID's */

    id = lex_VariadicCreateWord(&s_sIDToken);
    lex_FloatAddRange(id, 'a', 'z', 0);
    lex_FloatAddRange(id, 'A', 'Z', 0);
    lex_FloatAddRange(id, '_', '_', 0);
    lex_FloatAddRange(id, '@', '@', 0);
    lex_FloatAddRange(id, '\\', '\\', 0);
    lex_FloatAddRangeAndBeyond(id, 'a', 'z', 1);
    lex_FloatAddRangeAndBeyond(id, 'A', 'Z', 1);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 1);
    lex_FloatAddRangeAndBeyond(id, '_', '_', 1);
    lex_FloatAddRangeAndBeyond(id, '\\', '\\', 1);
    lex_FloatAddRangeAndBeyond(id, '@', '@', 1);
    lex_FloatAddRangeAndBeyond(id, '#', '#', 1);
}
