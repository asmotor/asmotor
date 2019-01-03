/*  Copyright 2008-2017 Carsten Elton Sorensen

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

#include <ctype.h>

// From xasm
#include "lexer_constants.h"
#include "lexer.h"
#include "tokens.h"

// From util
#include "lists.h"
#include "mem.h"
#include "str.h"


/* Private defines */

#define HASH(hash, key) {            \
    (hash) = ((hash) << 1u) + (key); \
    (hash) ^= (hash) >> 3u;          \
    (hash) &= WORDS_HASH_SIZE - 1u;  \
}

#define WORDS_HASH_SIZE 1024u

/* Private structures */

/* Private variables */

static SConstantWord* g_wordsHashTable[WORDS_HASH_SIZE];
static size_t g_maxWordLength;

static uint32_t
hashString(const char* str) {
    uint32_t result = 0;

    while (*str) {
        HASH(result, toupper((uint8_t) *str));
        ++str;
    }

    return result;
}

SConstantWord*
lex_ConstantsMatchWord(size_t maxWordLength) {
    if (g_maxWordLength < maxWordLength) {
        maxWordLength = g_maxWordLength;
    }

    SConstantWord* result = NULL;
    uint32_t hashCode = 0;
    size_t s = 0;

    while (s < maxWordLength) {
        HASH(hashCode, toupper(lex_PeekChar(s)));
        ++s;
        for (SConstantWord* lex = g_wordsHashTable[hashCode]; lex != NULL; lex = list_GetNext(lex)) {
            if (str_Length(lex->name) == s && lex_StartsWithStringNoCase(lex->name)) {
                result = lex;
            }
        }
    }
    return result;
}



void
lex_PrintMaxTokensPerHash(void) {
    int nMax = 0;
    int nInUse = 0;
    int nTotal = 0;

    for (uint32_t i = 0; i < WORDS_HASH_SIZE; ++i) {
        int n = 0;
        SConstantWord* p = g_wordsHashTable[i];
        if (p)
            ++nInUse;
        while (p) {
            ++nTotal;
            ++n;
            p = list_GetNext(p);
        }
        if (n > nMax)
            nMax = n;
    }

    printf("Total strings %d, max %d strings with same hash, %d slots in use\n", nTotal, nMax, nInUse);
}

void
lex_ConstantsUndefineWord(const char* name, uint32_t token) {
    SConstantWord** pHash = &g_wordsHashTable[hashString(name)];

    for (SConstantWord* pToken = *pHash; pToken != NULL; pToken = list_GetNext(pToken)) {
        if (pToken->token == token && str_EqualConst(pToken->name, name) == 0) {
            list_Remove(*pHash, pToken);
            str_Free(pToken->name);
            mem_Free(pToken);
            return;
        }
    }
    internalerror("token not found");
}

void
lex_ConstantsUndefineWords(SLexConstantsWord* lex) {
    while (lex->name) {
        lex_ConstantsUndefineWord(lex->name, lex->token);
        ++lex;
    }
}

void
lex_ConstantsDefineWord(const char* name, uint32_t token) {
    SConstantWord** pHash = &g_wordsHashTable[hashString(name)];
    SConstantWord* pPrev = *pHash;
    SConstantWord* pNew;

    /*printf("%s has hashvalue %d\n", lex->tzName, hash);*/

    pNew = (SConstantWord*) mem_Alloc(sizeof(SConstantWord));
    memset(pNew, 0, sizeof(SConstantWord));

    pNew->name = str_Create(name);
    str_ToUpperReplace(&pNew->name);
    pNew->token = (EToken) token;

    if (str_Length(pNew->name) > g_maxWordLength)
        g_maxWordLength = str_Length(pNew->name);

    if (pPrev) {
        list_InsertAfter(pPrev, pNew);
    } else {
        *pHash = pNew;
    }
}

void
lex_ConstantsDefineWords(const SLexConstantsWord* lex) {
    while (lex->name) {
        lex_ConstantsDefineWord(lex->name, lex->token);
        lex += 1;
    }

    /*lex_PrintMaxTokensPerHash();*/
}

void
lex_ConstantsInit(void) {
    for (uint32_t i = 0; i < WORDS_HASH_SIZE; ++i)
        g_wordsHashTable[i] = NULL;

    g_maxWordLength = 0;
}
