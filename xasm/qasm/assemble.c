#include "assemble.h"
#include "errors.h"
#include "lexer.h"
#include "parse.h"
#include "parse_expression.h"
#include "tokens.h"
#include "qasm.h"


static bool
assembleLanguageSymbol(const SLexLine* line) {
	if (line->label) {
		string* name = str_Create(line->label);
		int op = lex_TokenOf(line->operation);

		// Is it a constant or variable?
		if (op == T_SYM_CONSTANT || op == T_SYM_VARIABLE || op == T_OP_EQUAL) {
			if (line->totalArguments != 1) {
				err_Error(ERROR_ARGUMENT_COUNT);
			} else {
				lex_GotoArgument(0);
				int64_t v = parse_ConstantExpression(sizeof(int64_t));

				if (op == T_SYM_CONSTANT) {
					sym_CreateConstant(name, v);
				} else if (op == T_SYM_VARIABLE) {
					sym_CreateVariable(name, v);
				} else if (op == T_OP_EQUAL) {
					sym_UpdateVariable(name, v);
				}
			}
			return true;
		} else if (op == T_SYM_MACRO) {
			internalerror("Macros not implemented");
		}
	}

	return false;
}


static bool
assembleLabel(const SLexLine* line) {
	if (line->label) {
		sym_CreateLabel(str_Create(line->label));
	}
	return true;
}


static bool
assembleLanguageOperation(const SLexLine* line) {
	if (lex_Context->token.id == T_DIRECTIVE_SECTION) {
		if (line->totalArguments < 1) {
			err_Error(ERROR_ARGUMENT_COUNT);
		} else {
			lex_GotoArgument(0);
			string* name = parse_StringExpression(8);
			if (name != NULL) {
				SSymbol* group = NULL;
				if (lex_Context->token.id == ',') {
					parse_GetToken();
					group = parse_Symbol();
					if (group == NULL || group->type != SYMBOL_GROUP) {
						err_Error(ERROR_GROUP_EXPECTED);
					}
				}

				sect_CreateOrSwitchTo(name, group);
				return true;
			}
		}
	}
	return false;
}


static bool
assembleOperation(const SLexLine* line) {
	return qasm_Configuration->parseInstruction();
}


static bool
assembleCurrentLine(void) {
	const SLexLine* line = lex_CurrentLine();

	if (line) {
		if (assembleLanguageSymbol(line))
			return true;

		if (!assembleLabel(line))
			return false;

		if (line->operation == NULL)
			return true;

		lex_GotoOperation();

		if (assembleLanguageOperation(line) || assembleOperation(line)) {
			if (lex_Context->token.id == '\n')
				return true;

			err_Error(ERROR_CHARACTERS_AFTER_OPERATION);
		}
	}

	return false;
}


extern void
assemble(SLexerBuffer* buffer) {
	SLexerContext* context = lex_CreateContext(buffer);
	lex_PushContext(context);

	while (assembleCurrentLine()) {
		lex_NextLine();
	}
}
