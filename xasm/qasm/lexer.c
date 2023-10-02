#include "strcoll.h"

#include "lexer.h"
#include "lexer_constants.h"
#include "tokens.h"

static strmap_t* s_operations = NULL;

SLexerContext* lex_Context = NULL;

static void
freeOperationValue(intptr_t userData, intptr_t element) {
}


extern void
popContext(void) {
	SLexerContext* next = lex_Context->next;
	free(lex_Context);
	lex_Context = next;
}


extern void
lex_Init(void) {
	s_operations = strmap_Create(freeOperationValue);
}


extern void
lex_ConstantsDefineWords(const SLexConstantsWord* lex) {
	while (lex->name) {
		strmap_Insert(s_operations, str_Create(lex->name), lex->token);
		++lex;
	}
}


extern void
lex_ConstantsUndefineWords(const SLexConstantsWord* lex) {
}


extern SLexerContext*
lex_CreateContext(SLexerBuffer* buffer) {
	SLexerContext* context = malloc(sizeof(SLexerContext));
	context->next = NULL;
	context->token.id = T_NONE;
	context->token.length = 0;
	context->token.value.integer = 0;
	context->buffer = buffer;
	context->bufferLine = 0;

	return context;
}


extern void
lex_Bookmark(SLexerContext* bookmark) {
}


extern void
lex_Goto(SLexerContext* bookmark) {
	lex_Context = bookmark;
}


extern const SLexLine*
lex_NextLine(void) {
	if (lex_Context->bufferLine == lex_Context->buffer->totalLines) {
		popContext();
	}

	if (lex_Context) {
		return &lex_Context->buffer->lines[lex_Context->bufferLine++];
	}

	return NULL;
}
