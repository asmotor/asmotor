#pragma once

#include "types.h"

typedef struct {
    const char* name;
    uint32_t token;
} SLexConstantsWord;


extern void
lex_ConstantsDefineWord(const char* name, uint32_t token);

extern void
lex_ConstantsDefineWords(const SLexConstantsWord* lex);

extern void
lex_ConstantsUndefineWord(const char* name, uint32_t token);

extern void
lex_ConstantsUndefineWords(const SLexConstantsWord* lex);
