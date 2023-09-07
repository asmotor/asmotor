#pragma once

#include "types.h"

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
	SLexerToken token;
} SLexerContext;


typedef struct {
    const char* name;
    uint32_t token;
} SLexConstantsWord;


extern SLexerContext* lex_Context;


extern void
lex_ConstantsDefineWords(const SLexConstantsWord* lex);

extern void
lex_Bookmark(SLexerContext* bookmark);

extern void
lex_Goto(SLexerContext* bookmark);

extern void
lex_ConstantsUndefineWords(const SLexConstantsWord* lex);


