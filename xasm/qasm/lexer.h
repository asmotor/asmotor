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


typedef struct LexerContext {
	struct LexerContext* next;
	SLexerToken token;
	SLexerBuffer* buffer;
	size_t bufferLine;
} SLexerContext;


extern SLexerContext* lex_Context;

extern void
lex_Init(void);

extern SLexerContext*
lex_CreateContext(SLexerBuffer* buffer);

extern void
lex_Bookmark(SLexerContext* bookmark);

extern void
lex_Goto(SLexerContext* bookmark);

extern const SLexLine*
lex_NextLine(void);
