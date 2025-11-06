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
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "section.h"
#include "set.h"
#include "str.h"
#include "fmath.h"

#include "lexer.h"
#include "lexer_context.h"
#include "options.h"
#include "parse.h"
#include "errors.h"
#include "strcoll.h"
#include "symbol.h"

#include "includes.h"
#include "parse_block.h"
#include "parse_directive.h"
#include "parse_expression.h"
#include "parse_float_expression.h"
#include "parse_string.h"
#include "parse_symbol.h"
#include "tokens.h"


static set_t*
includeOnceFilenames = NULL;

static bool
mayIncludeFile(string* filename) {
	if (includeOnceFilenames == NULL) {
		includeOnceFilenames = strset_Create();
		return true;
	}

	bool included = strset_Exists(includeOnceFilenames, filename);
	return !included;
}

static bool
handleImport(string *name) {
	return sym_Import(name) != NULL;
}

static bool
handleExport(string *name) {
	return sym_Export(name) != NULL;
}

static bool
handleGlobal(string *name) {
	return sym_Global(name) != NULL;
}

static bool
modifySymbol(intptr_t intModification) {
	bool (*modification)(string *) = (bool (*)(string *))intModification;

	parse_GetToken();
	while (lex_Context->token.id == T_ID) {
		string *symbolName = lex_TokenString();
		modification(symbolName);
		str_Free(symbolName);

		parse_GetToken();

		if (lex_Context->token.id != ',')
			break;

		parse_GetToken();

		if (lex_Context->token.id != T_ID) {
			err_Error(ERROR_EXPECT_IDENTIFIER);
			break;
		}
	}

	return true;
}

static bool
purgeSymbol(intptr_t intModification) {
	parse_ExpandStrings = false;
	bool result = modifySymbol(intModification);
	parse_ExpandStrings = true;
	return result;
}

static bool
parseBracketedConstant(uint32_t* result) {
	if (lex_Context->token.id == '[') {
		parse_GetToken();

		*result = parse_ConstantExpression();
		return parse_ExpectChar(']');
	}

	*result = UINT32_MAX;
	return true;
}

static bool
handleEndr(intptr_t _) {
	if (lex_Context->type == CONTEXT_REPT) {
		lexctx_EndReptBlock();
	} else {
		err_Warn(WARN_REXIT_OUTSIDE_REPT);
	}

	if (lex_Context->token.id == T_DIRECTIVE_ENDR)
		parse_GetToken();

	return true;
}

static bool
handleRexit(intptr_t _) {
	if (lex_Context->type == CONTEXT_REPT) {
		parse_SkipPastEndr();
		lex_Context->block.repeat.remaining = 0;
		lexctx_EndReptBlock();
	} else {
		err_Warn(WARN_REXIT_OUTSIDE_REPT);
		parse_GetToken();
	}

	return true;
}

static bool
handleMexit(intptr_t _) {
	if (lex_Context->type == CONTEXT_MACRO) {
		lexctx_EndCurrentBuffer();
	} else {
		err_Warn(WARN_MEXIT_OUTSIDE_MACRO);
	}
	return true;
}

static bool
handleSection(intptr_t _) {
	parse_GetToken();

	string *name = parse_ExpectStringExpression();
	if (name == NULL)
		return true;

	if (!parse_ExpectChar(',')) {
		sect_SwitchTo_NAMEONLY(name);
		str_Free(name);
		return true;
	}

	if (lex_Context->token.id != T_ID) {
		err_Error(ERROR_EXPECT_IDENTIFIER);
		str_Free(name);
		return false;
	}

	string *groupName = lex_TokenString();
	SSymbol *groupSymbol = sym_GetSymbol(groupName);
	str_Free(groupName);

	if (groupSymbol->type != SYM_GROUP) {
		err_Error(ERROR_IDENTIFIER_GROUP);
		str_Free(name);
		return true;
	}
	parse_GetToken();

	uint32_t flags = 0;
	uint32_t loadAddress;
	if (!parseBracketedConstant(&loadAddress))
		return false;

	if (loadAddress != UINT32_MAX)
		flags |= SECTF_LOADFIXED;

	uint32_t bank = UINT32_MAX;
	uint32_t align = UINT32_MAX;
	uint32_t page = UINT32_MAX;

	while (lex_Context->token.id == ',') {
		parse_GetToken();
		switch (lex_Context->token.id) {
			case T_FUNC_BANK: {
				parse_GetToken();
				if (!xasm_Configuration->supportBanks || bank != UINT32_MAX || !parseBracketedConstant(&bank))
					return false;
				flags |= SECTF_BANKFIXED;
				break;
			}
			case T_FUNC_ALIGN: {
				parse_GetToken();
				if (align != UINT32_MAX || !parseBracketedConstant(&align))
					return false;
				flags |= SECTF_ALIGNED;
				break;
			}
			case T_FUNC_PAGE: {
				parse_GetToken();
				if (page != UINT32_MAX || !parseBracketedConstant(&page))
					return false;
				flags |= SECTF_PAGED;
				break;
			}
			case T_FUNC_ROOT: {
				parse_GetToken();
				flags |= SECTF_ROOT;
				break;
			}
			default:
				return false;
		}
	}

	if ((flags & SECTF_ALIGNED) && (flags & SECTF_LOADFIXED)) {
		if (loadAddress % align != 0) {
			err_Error(ERROR_SECT_FLAGS_COMBINATION);
			return false;
		}

		align = UINT32_MAX;
		flags &= ~SECTF_ALIGNED;
	}

	if (flags == 0) {
		sect_SwitchTo(name, groupSymbol);
	} else {
		sect_SwitchTo_KIND(name, groupSymbol, flags, loadAddress, bank, align, page);
	}

	str_Free(name);
	return true;
}

static bool
handleOrg(intptr_t _) {
	parse_GetToken();
	sect_SetOriginAddress((uint32_t)parse_ConstantExpression());

	return true;
}

static bool
handlePrintt(intptr_t _) {
	parse_GetToken();

	string *result = parse_ExpectStringExpression();
	if (result != NULL) {
		printf("%s", str_String(result));
		str_Free(result);
		return true;
	}

	return false;
}

static bool
handlePrintv(intptr_t _) {
	parse_GetToken();
	printf("$%X", parse_ConstantExpression());
	return true;
}

static bool
handlePrintf(intptr_t _) {
	parse_GetToken();
 
	int32_t i = parse_ConstantExpression();
	if (i < 0) {
		printf("-");
		i = -i;
	}
	printf("%d.%05d", (uint32_t)i >> 16u, imuldiv((uint32_t)i & 0xFFFFu, 100000, 65536));

	return true;
}

static bool
defineSpace(intptr_t multiplier) {
	parse_GetToken();

	int32_t offset = parse_ConstantExpression();
	if (offset >= 0) {
		sect_SkipBytes((uint32_t)offset * (uint32_t)multiplier);
		return true;
	} else {
		err_Error(ERROR_EXPR_POSITIVE);
		return true;
	}
}

static bool
handleRsreset(intptr_t _) {
	parse_GetToken();
	parse_SetRs(0);
	return true;
}

static bool
handleRsset(intptr_t _) {
	parse_GetToken();
	int32_t val = parse_ConstantExpression();
	parse_SetRs(val);
	return true;
}

static bool
handleRsend(intptr_t _) {
	parse_GetToken();
	sym_EndStructure();

	return true;
}

static bool
handleUserError(intptr_t intFail) {
	bool (*fail)(int, ...) = (bool (*)(int, ...))intFail;

	parse_GetToken();

	string *result = parse_ExpectStringExpression();
	if (result != NULL) {
		fail(WARN_USER_GENERIC, str_String(result));
	}
	str_Free(result);
	return true;
}

static bool
handleAssert(intptr_t intFail) {
	bool (*fail)(int, ...) = (bool (*)(int, ...))intFail;

	parse_GetToken();

	int32_t success = parse_ConstantExpression();
	string* message = NULL;

	if (parse_ExpectComma() && (message = parse_ExpectStringExpression()) != NULL && !success) {
		fail(WARN_USER_GENERIC, str_String(message));
	}
	str_Free(message);
	return true;
}

static bool
handleEven(intptr_t _) {
	parse_GetToken();
	sect_Align(2);
	return true;
}

static bool
handleCnop(intptr_t _) {
	parse_GetToken();

	int32_t offset = parse_ConstantExpression();
	if (offset >= 0) {
		if (!parse_ExpectComma())
			return false;

		int32_t align = parse_ConstantExpression();
		if (align >= 0) {
			sect_Align((uint32_t)align);
			sect_SkipBytes((uint32_t)offset);
		} else {
			err_Error(ERROR_EXPR_POSITIVE);
		}
	} else {
		err_Error(ERROR_EXPR_POSITIVE);
	}
	return true;
}

static bool
handleDb(intptr_t _) {
	do {
		parse_GetToken();

		SExpression *expr;
		string *str;

		if ((expr = parse_Expression(1)) != NULL) {
			if (expr == NULL && lex_Context->token.id != ',')
				err_Error(ERROR_EXPECT_EXPR);

			expr = expr_CheckRange(expr, -128, 255);
			if (expr != NULL) {
				sect_OutputExpr8(expr);
			} else {
				err_Error(ERROR_EXPRESSION_N_BIT, 8);
			}
		} else if ((str = parse_StringExpression()) != NULL) {
			const char *s = str_String(str);
			while (*s) {
				sect_OutputConst8((uint8_t)*s++);
			}

			str_Free(str);
		} else {
			sect_SkipBytes(1);
		}
	} while (lex_Context->token.id == ',');

	return true;
}

static bool
handleDw(intptr_t _) {
	do {
		parse_GetToken();

		SExpression *expr = parse_Expression(2);
		if (expr == NULL && lex_Context->token.id != ',')
			err_Error(ERROR_EXPECT_EXPR);

		if (expr != NULL) {
			expr = expr_CheckRange(expr, -32768, 65535);
			if (expr != NULL) {
				sect_OutputExpr16(expr);
			} else {
				err_Error(ERROR_EXPRESSION_N_BIT, 16);
			}
		} else {
			sect_SkipBytes(2);
		}
	} while (lex_Context->token.id == ',');

	return true;
}

static bool
handleDl(intptr_t _) {
	do {
		parse_GetToken();

		SExpression *expr = parse_Expression(4);
		if (expr == NULL && lex_Context->token.id != ',')
			err_Error(ERROR_EXPECT_EXPR);

		if (expr != NULL) {
			sect_OutputExpr32(expr);
		} else {
			long double result;
			if (xasm_Configuration->supportFloat && parse_TryFloatExpression(4, &result)) {
				sect_OutputFloat32(result);
			} else {
				sect_SkipBytes(4); //err_Error(ERROR_INVALID_EXPRESSION);
			}
		}
	} while (lex_Context->token.id == ',');

	return true;
}

static bool
handleDd(intptr_t _) {
	do {
		parse_GetToken();

		long double result;
		if (parse_TryFloatExpression(8, &result)) {
			sect_OutputFloat64(result);
		} else {
			sect_SkipBytes(8); //err_Error(ERROR_INVALID_EXPRESSION);
		}
	} while (lex_Context->token.id == ',');

	return true;
}

static string *getFilename(void) {
	string *filename = parse_StringExpression();

	if (filename == NULL) {
		lex_SetMode(LEXER_MODE_MACRO_ARGUMENT);
		parse_GetToken();
		if (lex_Context->token.id == T_STRING) {
			filename = lex_TokenString();
		}
		lex_SetMode(LEXER_MODE_NORMAL);
	}

	return filename != NULL ? inc_FindFile(filename) : NULL;
}

static void
includeFile(string* name) {
	lexctx_ProcessIncludeFile(name);
	parse_GetToken();
}

static bool
handleFileCore(intptr_t intProcess) {
	void (*process)(string *) = (void (*)(string *))intProcess;

	string *filename = getFilename();
	if (filename != NULL) {
		if (mayIncludeFile(filename)) {
			process(filename);
		}
		return true;
	}

	err_Error(ERROR_NO_FILE);
	return false;
}

static bool
handleFile(intptr_t intProcess) {
	parse_GetToken();
	return handleFileCore(intProcess);
}

static bool
handleInclude(intptr_t intProcess) {
	parse_GetToken();

	if (lex_Context->token.id == T_INCLUDE_ONCE) {
		parse_GetToken();
		strset_Insert(includeOnceFilenames, lex_Context->fileInfo->fileName);
		return true;
	} else {
		return handleFileCore(intProcess);
	}
}

static bool
handleRept(intptr_t _) {
	parse_GetToken();
	int32_t reptCount = parse_ConstantExpression();

	if (reptCount > 0) {
		lexctx_ProcessRepeatBlock((uint32_t)reptCount);
	} else if (reptCount < 0) {
		err_Error(ERROR_EXPR_POSITIVE);
	} else if (!parse_SkipPastEndr()) {
		err_Fail(ERROR_NEED_ENDR);
		return false;
	}
	return true;
}

static bool
handleShift(intptr_t _) {
	parse_GetToken();

	SExpression *expr = parse_Expression(4);
	if (expr != NULL) {
		if (expr_IsConstant(expr)) {
			lexctx_ShiftMacroArgs(expr->value.integer);
			expr_Free(expr);
			return true;
		} else {
			err_Error(ERROR_EXPR_CONST);
			return false;
		}
	} else {
		lexctx_ShiftMacroArgs(1);
		return true;
	}
}

static bool
handleIfStrings(intptr_t intPredicate) {
	bool (*predicate)(const string *, const string *) = (bool (*)(const string *, const string *))intPredicate;

	parse_GetToken();

	string *s1 = parse_ExpectStringExpression();
	if (s1 != NULL) {
		if (parse_ExpectComma()) {
			string *s2 = parse_ExpectStringExpression();
			if (s2 != NULL) {
				if (!predicate(s1, s2)) {
					parse_SkipPastTrueBranch();
				}

				str_Free(s1);
				str_Free(s2);
				return true;
			} else {
				err_Error(ERROR_EXPR_STRING);
			}
		}
		str_Free(s1);
	} else {
		err_Error(ERROR_EXPR_STRING);
	}

	return false;
}

static bool
handleIfSymbol(intptr_t intPredicate) {
	bool (*predicate)(const string *) = (bool (*)(const string *))intPredicate;

	parse_GetToken();

	if (lex_Context->token.id == T_ID) {
		string *symbolName = lex_TokenString();
		if (predicate(symbolName)) {
			parse_GetToken();
		} else {
			parse_GetToken();
			parse_SkipPastTrueBranch();
		}
		str_Free(symbolName);
		return true;
	}
	err_Error(ERROR_EXPECT_IDENTIFIER);
	return false;
}

static bool
handleIfExpr(intptr_t intPredicate) {
	bool (*predicate)(int32_t) = (bool (*)(int32_t))intPredicate;

	parse_GetToken();

	if (!predicate(parse_ConstantExpression())) {
		uint32_t lineNumber = lex_Context->lineNumber;
		if (!parse_SkipPastTrueBranch()) {
			err_Fail(ERROR_NEED_ENDC, str_String(lex_Context->buffer.name), lineNumber);
		}
	}
	return true;
}

static bool
valueEqualsZero(int32_t value) {
	return value == 0;
}

static bool
valueNotEqualsZero(int32_t value) {
	return value != 0;
}

static bool
valueGreaterThanZero(int32_t value) {
	return value > 0;
}

static bool
valueGreaterOrEqualsZero(int32_t value) {
	return value >= 0;
}

static bool
valueLessThanZero(int32_t value) {
	return value < 0;
}

static bool
valueLessOrEqualsZero(int32_t value) {
	return value <= 0;
}

static bool
handleElse(intptr_t _) {
	parse_SkipPastEndc();
	return true;
}

static bool
handleEndc(intptr_t _) {
	parse_GetToken();
	return true;
}

static bool
handlePusho(intptr_t _) {
	opt_Push();
	parse_GetToken();
	return true;
}

static bool
handlePopo(intptr_t _) {
	opt_Pop();
	parse_GetToken();
	return true;
}

static bool
handleOpt(intptr_t _) {
	lex_SetMode(LEXER_MODE_MACRO_ARGUMENT);
	parse_GetToken();
	if (lex_Context->token.id == T_STRING) {
		lex_Context->token.value.string[lex_Context->token.length] = 0;
		opt_Parse(lex_Context->token.value.string);
		parse_GetToken();
		while (lex_Context->token.id == ',') {
			parse_GetToken();
			opt_Parse(lex_Context->token.value.string);
			parse_GetToken();
		}
	}
	lex_SetMode(LEXER_MODE_NORMAL);
	opt_Updated();
	return true;
}

static bool
handlePushs(intptr_t _) {
	parse_GetToken();
	sect_Push();
	return true;
}

static bool
handlePops(intptr_t _) {
	parse_GetToken();
	if (!sect_Pop())
		err_Error(ERROR_SECTION_MISSING);
	return true;
}

static bool
handleRs(intptr_t multiplier) {
	parse_GetToken();
	parse_IncrementRs(parse_ConstantExpression() * (int32_t)multiplier);
	return true;
}

static bool
handleRandseed(intptr_t _) {
	parse_GetToken();
	s_randseed = (uint32_t) parse_ConstantExpression();
	return true;
}

typedef struct Directive {
	bool (*handler)(intptr_t);
	intptr_t userData;
} SDirective;

#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4113)
#endif

static SDirective
	g_Directives[T_DIRECTIVE_LAST - T_DIRECTIVE_FIRST + 1] = {
		{handleRsreset, 0},
		{handleRsset, 0},
		{handleRsend, 0},
		{handleRs, 1},
		{handleRs, 2},
		{handleRs, 4},
		{handleRs, 8},
		{handlePrintt, 0},
		{handlePrintv, 0},
		{handlePrintf, 0},
		{modifySymbol, (intptr_t)handleExport},
		{modifySymbol, (intptr_t)handleImport},
		{modifySymbol, (intptr_t)handleGlobal},
		{purgeSymbol, (intptr_t)sym_Purge},
		{handleUserError, (intptr_t)err_Fail},
		{handleUserError, (intptr_t)err_Warn},
		{handleAssert, (intptr_t)err_Fail},
		{handleAssert, (intptr_t)err_Warn},
		{handleInclude, (intptr_t)includeFile},
		{handleFile, (intptr_t)sect_OutputBinaryFile},
		{defineSpace, 1},
		{defineSpace, 2},
		{defineSpace, 4},
		{defineSpace, 8},
		{handleDb, 0},
		{handleDw, 0},
		{handleDl, 0},
		{handleDd, 0},
		{handleSection, 0},
		{handleOrg, 0},
		{handleShift, 0},
		{handleMexit, 0},
		{handleMexit, 0},
		{handleRept, 0},
		{handleRexit, 0},
		{handleEndr, 0},
		{handleIfStrings, (intptr_t)str_Equal},
		{handleIfStrings, (intptr_t)str_NotEqual},
		{handleIfSymbol, (intptr_t)sym_IsDefined},
		{handleIfSymbol, (intptr_t)sym_IsNotDefined},
		{handleIfExpr, (intptr_t)valueNotEqualsZero},
		{handleIfExpr, (intptr_t)valueEqualsZero},
		{handleIfExpr, (intptr_t)valueGreaterThanZero},
		{handleIfExpr, (intptr_t)valueGreaterOrEqualsZero},
		{handleIfExpr, (intptr_t)valueLessThanZero},
		{handleIfExpr, (intptr_t)valueLessOrEqualsZero},
		{handleElse, 0},
		{handleEndc, 0},
		{handleEven, 0},
		{handleCnop, 0},
		{handlePusho, 0},
		{handlePopo, 0},
		{handleOpt, 0},
		{handlePushs, 0},
		{handlePops, 0},
		{handleRandseed, 0},
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

bool parse_Directive(void) {
	if (lex_Context->token.id >= T_DIRECTIVE_FIRST && lex_Context->token.id <= T_DIRECTIVE_LAST) {
		SDirective *directive = &g_Directives[lex_Context->token.id - T_DIRECTIVE_FIRST];
		return directive->handler(directive->userData);
	}
	return false;
}
