#include <assert.h>
#include <ctype.h>

#include "str.h"
#include "strcoll.h"
#include "strpmap.h"

#include "errors.h"
#include "lexer.h"
#include "lexer_constants.h"
#include "qasm.h"
#include "tokens.h"



static strpmap_t* s_operations = NULL;
static const char* s_operatorChars = "-!()[]{}*/\\&#^+<=>|~";
SLexerContext* lex_Context = NULL;


static void
setNewlineToken(void) {
	lex_Context->argument = -1;
	lex_Context->argumentChar = NULL;
	lex_Context->token.id = '\n';
	lex_Context->token.length = 1;
}


static void
freeOperationValue(intptr_t userData, intptr_t element) {
	// dummy no-op
}


static char*
skipWhitespace(const char* s) {
	while (isspace(*s))
		++s;

	return (char*) s;
}

static bool
parseInteger(int base) {
	lex_Context->token.length = 0;
	lex_Context->token.value.integer = 0;

	char ch;
	while ((ch = *lex_Context->argumentChar) != 0) {
		ch = toupper(ch);
		if (isdigit(ch))
			ch -= '0';
		else if (ch >= 'A' && ch <= 'F')
			ch -= 'A' - 10;
		else
			break;

		if (ch < base) {
			lex_Context->token.value.integer = lex_Context->token.value.integer * base + ch;
			++lex_Context->token.length;
			++lex_Context->argumentChar;
		}
	}

	if (lex_Context->token.length != 0) {
		lex_Context->token.id = T_NUMBER;
		return true;
	}

	return false;
}


static void
popContext(void) {
	SLexerContext* next = lex_Context->next;
	free(lex_Context);
	lex_Context = next;
}


extern void
lex_Init(void) {
	s_operations = strpmap_CreateI(freeOperationValue);
	tokens_Define();
	qasm_Configuration->defineTokens();
}


extern void
lex_Close(void) {
}


extern void
lex_ConstantsDefineWords(const SLexConstantsWord* lex) {
	while (lex->name) {
		strpmap_Insert(s_operations, lex->name, lex->token);
		++lex;
	}
}


extern void
lex_ConstantsUndefineWords(const SLexConstantsWord* lex) {
	internalerror("lex_ConstantsUndefineWords Not implemented");
}


extern SLexerContext*
lex_CreateContext(SLexerBuffer* buffer) {
	SLexerContext* context = mem_Alloc(sizeof(SLexerContext));
	context->next = NULL;
	context->token.id = T_NONE;
	context->token.length = 0;
	context->token.value.integer = 0;
	context->buffer = buffer;
	context->bufferLine = 0;
	context->argument = LEX_ARGUMENT_NONE;

	return context;
}


extern void
lex_PushContext(SLexerContext* context) {
	context->next = lex_Context;
	lex_Context = context;
}


extern void
lex_Bookmark(SLexerContext* bookmark) {
	*bookmark = *lex_Context;
}


extern void
lex_Goto(SLexerContext* bookmark) {
	*lex_Context = *bookmark;
}


extern void
lex_GotoArgument(size_t argument) {
	const SLexLine* line = lex_CurrentLine();

	if (argument < line->totalArguments) {
		lex_Context->argument = argument;
		lex_Context->argumentChar = line->arguments[argument];

		lex_NextToken();
	} else {
		lex_Context->token.id = '\n';
		lex_Context->token.length = 1;
	}
}


extern void
lex_GotoOperation(void) {
	const SLexLine* line = lex_CurrentLine();

	if (line->operation != NULL) {
		lex_Context->token.id = lex_TokenOf(line->operation);

		if (line->totalArguments > 0) {
			lex_Context->argument = 0;
			lex_Context->argumentChar = lex_CurrentLine()->arguments[0];
		} else {
			lex_Context->argument = -1;
			lex_Context->argumentChar = NULL;
		}
	} else {
		lex_GotoArgument(0);
	}
}


extern int
lex_TokenOf(const char* str) {
	intptr_t token;
	if (strpmap_Value(s_operations, str, &token)) {
		return token;
	}

	return T_NONE;
}


extern const SLexLine*
lex_CurrentLine(void) {
	if (lex_Context->bufferLine >= lex_Context->buffer->totalLines)
		return NULL;

	return &lex_Context->buffer->lines[lex_Context->bufferLine];
}


extern void
lex_NextLine(void) {
	assert(lex_Context != NULL);

	if (lex_Context->bufferLine == lex_Context->buffer->totalLines) {
		popContext();
	}

	++lex_Context->bufferLine;
}


extern string*
lex_CurrentFileAndLine(void) {
	if (lex_Context == NULL)
		return str_Create("[shell]");

	return str_CreateFormat("%s:%ld", str_String(lex_Context->buffer->name), lex_Context->bufferLine + 1);
}


extern void
lex_NextToken(void) {
	if (lex_Context->argument == -1) {
		setNewlineToken();
		return;
	}

	if (*lex_Context->argumentChar == 0) {
		const SLexLine* line = lex_CurrentLine();
		lex_Context->argument += 1;
		if (lex_Context->argument != (ssize_t) line->totalArguments) {
			lex_Context->argumentChar = line->arguments[lex_Context->argument];

			lex_Context->token.id = ',';
			lex_Context->token.length = 1;
		} else {
			setNewlineToken();
		}

		return;
	}

	assert(lex_Context->argumentChar != NULL);

	lex_Context->argumentChar = skipWhitespace(lex_Context->argumentChar);
	if (lex_Context->argumentChar) {
		// Try operator
		size_t opLen = 0;
		while (lex_Context->argumentChar[opLen] != 0 && strchr(s_operatorChars, lex_Context->argumentChar[opLen]) != NULL) {
			lex_Context->token.value.string[opLen] = lex_Context->argumentChar[opLen];
			opLen += 1;
		}

		if (opLen > 0) {
			while (opLen > 0) {
				lex_Context->token.value.string[opLen] = 0;
				int token = lex_TokenOf(lex_Context->token.value.string);
				if (token != T_NONE) {
					lex_Context->token.id = token;
					lex_Context->token.length = opLen;
					lex_Context->argumentChar += opLen;
					return;
				}
				opLen -= 1;
			}
			lex_Context->token.id = *lex_Context->argumentChar++;
			lex_Context->token.length = 1;
			return;
		}

		// Try string literal
		if (*lex_Context->argumentChar == '"') {
			int len = 0;
			lex_Context->argumentChar += 1;
			while (*lex_Context->argumentChar != '"') {
				switch (*lex_Context->argumentChar) {
					case 0: {
						err_Error(ERROR_UNTERMINATED_STRING);
						lex_Context->token.id = T_NONE;
						return;
					}
					case '\\': {
						switch (*++lex_Context->argumentChar) {
							case 0: {
								break;
							}
							case 'n': {
								lex_Context->token.value.string[len++] = '\n';
								++lex_Context->argumentChar;
								break;
							}
							default: {
								lex_Context->token.value.string[len++] = *lex_Context->argumentChar++;
								break;
							}
						}
						break;
					}
					default: {
						lex_Context->token.value.string[len++] = *lex_Context->argumentChar++;
						break;
					}
				}
			}
			lex_Context->argumentChar += 1;
			lex_Context->token.value.string[len] = 0;
			lex_Context->token.length = len;
			lex_Context->token.id = T_STRING;
			return;
		}

		// Try numbers
		if (*lex_Context->argumentChar == '$') {
			++lex_Context->argumentChar;

			if (parseInteger(16))
				return;
		}

		if (*lex_Context->argumentChar == '%') {
			++lex_Context->argumentChar;

			if (parseInteger(2))
				return;
		}

		// Try numeric local identifiers
		if (isdigit(*lex_Context->argumentChar)) {
			int len = 1;
			while (isdigit(lex_Context->argumentChar[len])) {
				++len;
			}

			if (lex_Context->argumentChar[len] == '$') {
				// local numeric label
				++len;
				lex_Context->token.id = T_ID;
				lex_Context->token.length = len;
				memcpy(lex_Context->token.value.string, lex_Context->argumentChar, len);
				lex_Context->argumentChar += len;
			} else {
				// decimal integer
				parseInteger(10);
			}
			return;
		}

		// Try identifiers
		if (sym_IsLabelStartChar(*lex_Context->argumentChar)) {
			lex_Context->token.value.string[0] = *lex_Context->argumentChar;

			int len = 1;
			while (sym_IsLabelChar(lex_Context->argumentChar[len])) {
				lex_Context->token.value.string[len] = lex_Context->argumentChar[len];
				++len;
			}
			lex_Context->token.id = T_ID;
			lex_Context->token.value.string[len] = 0;
			lex_Context->token.length = len;
			lex_Context->argumentChar += len;
			return;
		}

		// No token found
		err_Error(ERROR_INVALID_IDENTIFIER);
		lex_Context->token.id = T_NONE;
	}
}


extern string*
lex_TokenString(void) {
	return str_CreateLength(lex_Context->token.value.string, lex_Context->token.length);
}
