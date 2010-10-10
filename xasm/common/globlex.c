/*  Copyright 2008 Carsten Sørensen

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

#include <stdlib.h>
#include <string.h>

#include "xasm.h"
#include "lexer.h"
#include "options.h"
#include "fstack.h"
#include "project.h"
#include "symbol.h"


BOOL g_bDontExpandStrings;
ULONG BinaryConstID;

/*	Private data */

static SLexInitString staticstrings[] =
{
    "||",	T_OP_LOGICOR,
    "&&",	T_OP_LOGICAND,
    "==",	T_OP_LOGICEQU,
    ">",	T_OP_LOGICGT,
    "<",	T_OP_LOGICLT,
    ">=",	T_OP_LOGICGE,
    "<=",	T_OP_LOGICLE,
    "~=",	T_OP_LOGICNE,
    "~!",	T_OP_LOGICNOT,
    "|",	T_OP_OR,
    "!",	T_OP_OR,
    "^",	T_OP_XOR,
    "&",	T_OP_AND,
    "<<",	T_OP_SHL,
    ">>",	T_OP_SHR,
    "+",	T_OP_ADD,
    "-",	T_OP_SUB,
    "*",	T_OP_MUL,
    "/",	T_OP_DIV,
    "~/",	T_OP_MOD,
    "~",	T_OP_NOT,

    "def",	T_FUNC_DEF,

#ifdef	HASBANKS
    "bank", T_FUNC_BANK,
#endif
    "div",	T_FUNC_FDIV,
    "mul",	T_FUNC_FMUL,
    "sin",	T_FUNC_SIN,
    "cos",	T_FUNC_COS,
    "tan",	T_FUNC_TAN,
    "asin",	T_FUNC_ASIN,
    "acos",	T_FUNC_ACOS,
    "atan",	T_FUNC_ATAN,
    "atan2",T_FUNC_ATAN2,

    "strcmp",	T_FUNC_STRCMP,
    "strin",	T_FUNC_STRIN,
    "strsub",	T_FUNC_STRSUB,
    "strlen",	T_FUNC_STRLEN,
    "strcat",	T_FUNC_STRCAT,
    "strupr",	T_FUNC_STRUPR,
    "strlwr",	T_FUNC_STRLWR,

    "printt",	T_POP_PRINTT,
    "printv",	T_POP_PRINTV,
    "printf",	T_POP_PRINTF,
    "export",	T_POP_EXPORT,
    "xdef",		T_POP_EXPORT,
    "import",	T_POP_IMPORT,
    "xref",		T_POP_IMPORT,
    "global",	T_POP_GLOBAL,

    "rsreset",	T_POP_RSRESET,
    "rsset",	T_POP_RSSET,

    "set",		T_POP_SET,
    "=",		T_POP_SET,

    "section",	T_POP_SECTION,
    "group",	T_POP_GROUP,
    "text",		T_GROUP_TEXT,
    "ram",		T_GROUP_BSS,

    "equ",		T_POP_EQU,
    "equs",		T_POP_EQUS,

	"purge",	T_POP_PURGE,

    "fail",		T_POP_FAIL,
    "warn",		T_POP_WARN,

    "include",	T_POP_INCLUDE,
	"incbin",	T_POP_INCBIN,

    "rept",		T_POP_REPT,
    "endr",		T_POP_ENDR,		/*	Not needed but we have it here just to protect the name */
	"rexit",	T_POP_REXIT,

    "if",		T_POP_IF,
    "ifc",		T_POP_IFC,
    "ifd",		T_POP_IFD,
    "ifnc",		T_POP_IFNC,
    "ifnd",		T_POP_IFND,
    "ifne",		T_POP_IF,
    "ifeq",		T_POP_IFEQ,
    "ifgt",		T_POP_IFGT,
    "ifge",		T_POP_IFGE,
    "iflt",		T_POP_IFLT,
    "ifle",		T_POP_IFLE,
    "else",		T_POP_ELSE,
    "endc",		T_POP_ENDC,

    "macro",	T_POP_MACRO,
    "endm",		T_POP_ENDM,		/*	Not needed but we have it here just to protect the name */
	"shift",	T_POP_SHIFT,
	"mexit",	T_POP_MEXIT,

	"pushs",	T_POP_PUSHS,
	"pops",		T_POP_POPS,
	"pusho",	T_POP_PUSHO,
	"popo",		T_POP_POPO,

	"opt",		T_POP_OPT,

	"lowlimit",		T_FUNC_LOWLIMIT,
	"highlimit",	T_FUNC_HIGHLIMIT,

	"even",		T_POP_EVEN,
	"cnop",		T_POP_CNOP,

	"end",		T_POP_END,

    NULL, 0
};

static SLONG binary2bin(char ch)
{
	SLONG i;

	for(i = 0; i <= 1; ++i)
	{
		if(g_pOptions->BinaryChar[i] == ch)
			return i;
	}

    return 0;
}

static SLONG char2bin(char ch)
{
	if(ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;

	if(ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;

	if(ch >= '0' && ch <= '9')
		return ch - '0';

	return 0;
}

typedef	SLONG (*x2bin)(char ch);

static SLONG ascii2bin(char* s)
{
	SLONG	radix = 10;
	SLONG   result = 0;
	x2bin	convertfunc=char2bin;

	switch (*s)
	{
		case '$':
			radix = 16;
			s += 1;
			convertfunc=char2bin;
			break;
		case '%':
			radix = 2;
			s += 1;
			convertfunc=binary2bin;
			break;
	}

	while(*s != '\0')
		result = result * radix + convertfunc(*s++);

	return (result);
}


static BOOL ParseNumber(char* s, ULONG size)
{
    char dest[256];

	if(size >= 256)
		size = 255;

    strncpy(dest, s, size);
    dest[size] = 0;
    g_CurrentToken.Value.nInteger = ascii2bin(dest);

    return TRUE;
}

static BOOL ParseSymbol(char* src, ULONG size)
{
	char dest[MAXSYMNAMELENGTH+1];
	int copied = 0;
	int size_backup = size;

	while(size && copied < MAXSYMNAMELENGTH)
	{
		if(*src == '\\')
		{
			char* marg;

			++src;
			--size;

			if(*src == '@')
				marg = fstk_GetMacroRunID();
			else if(*src >= '0' && *src <= '9')
				marg = fstk_GetMacroArgValue(*src);
			else
			{
				prj_Fail(ERROR_ID_MALFORMED);
				return FALSE;
			}

			++src;
			--size;

			if(marg)
			{
				while(*marg)
					dest[copied++] = *marg++;
			}
		}
		else
		{
			dest[copied++] = *src++;
			size -= 1;
		}
	}

	if(copied > MAXSYMNAMELENGTH)
		internalerror("Symbolname too long");

	dest[copied] = 0;

	if(g_bDontExpandStrings == 0 && sym_isString(dest))
	{
		char* s;

		lex_SkipBytes(size_backup);
		lex_UnputString(s = sym_GetStringValue(dest));

		while(*s)
		{
			if(*s++ == '\n')
				g_pFileContext->LineNumber -= 1;
		}
		return FALSE;
	}

	strcpy(g_CurrentToken.Value.aString, dest);
	return TRUE;
}

static BOOL ParseFixedPoint(char* s, ULONG size)
{
	char dest[256];
	ULONG i = 0;
	ULONG dot = 0;

	while(size && dot != 2)
	{
		if(s[i] == '.')
			dot+=1;

		if(dot < 2)
		{
			dest[i] = s[i];
			size -= 1;
			i += 1;
		}
	}

	dest[i] = 0;

	lex_RewindBytes(size);

	g_CurrentToken.Value.nInteger = (SLONG)(atof(s) * 65536);
	return TRUE;
}

BOOL ParseMacroArg(char* src, ULONG size)
{
	char* arg = fstk_GetMacroArgValue(src[1]);
    lex_SkipBytes(size);
	if(arg != NULL)
	    lex_UnputString(arg);
    return FALSE;
}

BOOL ParseUniqueArg(char* src, ULONG size)
{
    lex_SkipBytes(size);
    lex_UnputString(fstk_GetMacroRunID());
    return FALSE;
}

enum
{
    T_LEX_MACROARG = 3000,
    T_LEX_MACROUNIQUE
};

SLexFloat tMacroArgToken=
{
    ParseMacroArg,
    T_LEX_MACROARG
};

SLexFloat tMacroUniqueToken=
{
    ParseUniqueArg,
    T_LEX_MACROUNIQUE
};

SLexFloat	tFixedToken=
{
    ParseFixedPoint,
    T_NUMBER
};

SLexFloat	tNumberToken=
{
    ParseNumber,
    T_NUMBER
};

SLexFloat	tIDToken=
{
    ParseSymbol,
    T_ID
};

void	globlex_Init(void)
{
	ULONG	id;

	g_bDontExpandStrings=FALSE;

	lex_Init();

	lex_AddStrings(staticstrings);

	if(g_pConfiguration->pszNameRB)
		lex_AddString(g_pConfiguration->pszNameRB, T_POP_RB);
	if(g_pConfiguration->pszNameRW)
		lex_AddString(g_pConfiguration->pszNameRW, T_POP_RW);
	if(g_pConfiguration->pszNameRL)
		lex_AddString(g_pConfiguration->pszNameRL, T_POP_RL);

	if(g_pConfiguration->pszNameDSB)
		lex_AddString(g_pConfiguration->pszNameDSB, T_POP_DSB);
	if(g_pConfiguration->pszNameDSW)
		lex_AddString(g_pConfiguration->pszNameDSW, T_POP_DSW);
	if(g_pConfiguration->pszNameDSL)
		lex_AddString(g_pConfiguration->pszNameDSL, T_POP_DSL);

	if(g_pConfiguration->pszNameDB)
		lex_AddString(g_pConfiguration->pszNameDB, T_POP_DB);
	if(g_pConfiguration->pszNameDW)
		lex_AddString(g_pConfiguration->pszNameDW, T_POP_DW);
	if(g_pConfiguration->pszNameDL)
		lex_AddString(g_pConfiguration->pszNameDL, T_POP_DL);



	/* Local ID */

    id = lex_FloatAlloc(&tIDToken);
    lex_FloatAddRange(id, '.', '.', 1);
    lex_FloatAddRange(id, 'a', 'z', 2);
    lex_FloatAddRange(id, 'A', 'Z', 2);
    lex_FloatAddRange(id, '_', '_', 2);
    lex_FloatAddRange(id, '#', '#', 2);
    lex_FloatAddRange(id, '\\', '\\', 2);
    lex_FloatAddRangeAndBeyond(id, 'a', 'z', 3);
    lex_FloatAddRangeAndBeyond(id, 'A', 'Z', 3);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 3);
    lex_FloatAddRangeAndBeyond(id, '_', '_', 3);
    lex_FloatAddRangeAndBeyond(id, '\\', '\\', 3);
    lex_FloatAddRangeAndBeyond(id, '@', '@', 3);
    lex_FloatAddRangeAndBeyond(id, '#', '#', 3);

    id = lex_FloatAlloc(&tIDToken);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 1);
    lex_FloatAddRangeAndBeyond(id, '\\', '\\', 1);
    lex_FloatAddRangeAndBeyond(id, '@', '@', 2);
	lex_FloatSetSuffix(id, '$');

    /*      Macro arguments */

    id = lex_FloatAlloc(&tMacroArgToken);
    lex_FloatAddRange(id, '\\', '\\', 1);
    lex_FloatAddRange(id, '0', '9', 2);
    id = lex_FloatAlloc(&tMacroUniqueToken);
    lex_FloatAddRange(id, '\\', '\\', 1);
    lex_FloatAddRange(id, '@', '@', 2);

	/*      Decimal constants*/

    id = lex_FloatAlloc(&tNumberToken);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 1);

	/* Fixedpoint constants */
	/*
    id = lex_FloatAlloc(&tFixedToken);
    lex_FloatAddRange(id, '@', '@', 1);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 2);
    lex_FloatAddRangeAndBeyond(id, '.', '.', 2);
	*/

	/*      Hex constants*/

    id = lex_FloatAlloc(&tNumberToken);
    lex_FloatAddRange(id, '$', '$', 1);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 2);
    lex_FloatAddRangeAndBeyond(id, 'A', 'F', 2);
    lex_FloatAddRangeAndBeyond(id, 'a', 'f', 2);

	/*      Binary constants*/

    BinaryConstID = id = lex_FloatAlloc(&tNumberToken);
    lex_FloatAddRange(id, '%', '%', 1);
    lex_FloatAddRangeAndBeyond(id, '0', '1', 2);

    /* ID's */

    id = lex_FloatAlloc(&tIDToken);
    lex_FloatAddRange(id, 'a', 'z', 1);
    lex_FloatAddRange(id, 'A', 'Z', 1);
    lex_FloatAddRange(id, '_', '_', 1);
    lex_FloatAddRange(id, '@', '@', 1);
    lex_FloatAddRange(id, '\\', '\\', 1);
    lex_FloatAddRangeAndBeyond(id, 'a', 'z', 2);
    lex_FloatAddRangeAndBeyond(id, 'A', 'Z', 2);
    lex_FloatAddRangeAndBeyond(id, '0', '9', 2);
    lex_FloatAddRangeAndBeyond(id, '_', '_', 2);
    lex_FloatAddRangeAndBeyond(id, '\\', '\\', 2);
    lex_FloatAddRangeAndBeyond(id, '@', '@', 2);
    lex_FloatAddRangeAndBeyond(id, '#', '#', 2);
}
