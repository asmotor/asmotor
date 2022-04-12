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

#include <stdint.h>
#include <stdbool.h>

#include "lexer.h"
#include "lexer_constants.h"
#include "lexer_context.h"
#include "parse.h"
#include "parse_block.h"
#include "parse_expression.h"
#include "parse_string.h"
#include "parse_symbol.h"
#include "errors.h"
#include "symbol.h"


static SSymbol*
g_rsSymbol = NULL;

static uint32_t
colonCount(void) {
	if (lex_Context->token.id == ':') {
		parse_GetToken();
		if (lex_Context->token.id == ':') {
			parse_GetToken();
			return 2;
		}
	}
	return 1;
}

static SSymbol*
getRsSymbol() {
	if (g_rsSymbol == NULL) {
		string* rsName = str_Create("__RS");
		g_rsSymbol = sym_CreateSet(rsName, 0);
		str_Free(rsName);
	}
	return g_rsSymbol;
}

static void
createRsSymbol(string* symbolName, int32_t multiplier) {
	parse_GetToken();
	sym_CreateSet(symbolName, parse_GetRs(multiplier * parse_ConstantExpression()));
}

void
parse_SetRs(int32_t rsValue) {
	getRsSymbol()->value.integer = rsValue;
}

int32_t
parse_GetRs(int32_t size) {
	SSymbol* rsSymbol = getRsSymbol();
	int32_t rsValue = rsSymbol->value.integer;
	rsSymbol->value.integer += size;
	return rsValue;
}

bool
parse_SymbolDefinition(void) {
	bool r = false;

	if (lex_Context->token.id == T_LABEL) {
		if (lex_ConstantsMatchTokenString() != NULL) {
			err_Warn(WARN_SYMBOL_WITH_RESERVED_NAME);
		}

		string* symbolName = lex_TokenString();

		parse_GetToken();

		uint32_t totalColons = colonCount();

		if (lex_Context->token.id == T_SYM_MACRO) {
			if (totalColons != 1) {
			    err_Error(ERROR_SYMBOL_EXPORT, str_String(lex_Context->buffer.name), lex_Context->lineNumber);
				return false;
			} else {
				uint32_t lineNumber = lex_Context->lineNumber;

				string* block = parse_CopyMacroBlock();
				if (block != NULL) {
					sym_CreateMacro(symbolName, block, lineNumber);
					str_Free(block);
					parse_GetToken();
					r = true;
				} else {
					err_Fail(ERROR_NEED_ENDM, str_String(lex_Context->buffer.name), lineNumber);
					return false;
				}
			}
		} else {
			switch (lex_Context->token.id) {
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
					sym_CreateEqu(symbolName, parse_ConstantExpression());
					break;
				}
				case T_SYM_SET: {
					parse_GetToken();
					sym_CreateSet(symbolName, parse_ConstantExpression());
					break;
				}
				case T_SYM_EQUS: {
					parse_GetToken();

					string* value = parse_ExpectStringExpression();
					if (value != NULL) {
						sym_CreateEqus(symbolName, value);
						str_Free(value);
					}
					break;
				}
				case T_SYM_GROUP: {
					EGroupType groupType;

					parse_GetToken();
					switch (lex_Context->token.id) {
						case T_GROUP_TEXT:
							groupType = GROUP_TEXT;
							break;
						case T_GROUP_BSS:
							groupType = GROUP_BSS;
							break;
						default:
							err_Error(ERROR_EXPECT_TEXT_BSS);
							return false;
					}
					sym_CreateGroup(symbolName, groupType);
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
