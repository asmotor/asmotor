#pragma once

#include "types.h"

#include "lexbuffer.h"

#define MAX_TOKEN_LENGTH 256

typedef struct {
    uint32_t id;
    size_t length;
    union {
        char string[MAX_TOKEN_LENGTH + 1];
        int32_t integer;
        long double floating;
    } value;
} SLexerToken;


#define LEX_ARGUMENT_NONE -1

typedef struct LexerContext {
	struct LexerContext* next;
	SLexerToken token;
	SLexerBuffer* buffer;
	size_t bufferLine;
	ssize_t argument;
	const char* argumentChar;
} SLexerContext;


extern SLexerContext* lex_Context;

extern void
lex_Init(void);

extern void
lex_Close(void);

extern SLexerContext*
lex_CreateContext(SLexerBuffer* buffer);

extern void
lex_PushContext(SLexerContext* context);

extern void
lex_Bookmark(SLexerContext* bookmark);

extern void
lex_Goto(SLexerContext* bookmark);

extern void
lex_GotoArgument(size_t argument);

extern void
lex_GotoOperation(void);

extern const SLexLine*
lex_CurrentLine(void);

extern void
lex_NextLine(void);

extern int
lex_TokenOf(const char* str);

extern string*
lex_CurrentFileAndLine(void);

extern void
lex_NextToken(void);

extern string*
lex_TokenString(void);

