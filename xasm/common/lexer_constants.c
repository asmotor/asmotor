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

#include <assert.h>
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

typedef struct ConstantWord {
    list_Data(struct ConstantWord);
    SLexConstantsWord definition;
    size_t nameLength;
} SConstantWord;

/* Private variables */

static SConstantWord* g_wordsHashTable[WORDS_HASH_SIZE];
static size_t g_maxWordLength;

/* Private functions */

static bool
isNotLowerCase(const char* str) {
    while (*str != 0) {
        if (islower(*str++))
            return false;
    }

    return true;
}

static uint32_t
hashString(const char* str) {
    uint32_t result = 0;

    while (*str) {
        HASH(result, toupper((uint8_t) *str));
        ++str;
    }

    return result;
}

static bool
doesNotExist(const char* name) {
    for (SConstantWord* word = g_wordsHashTable[hashString(name)]; word != NULL; word = list_GetNext(word)) {
        if (strcmp(name, word->definition.name) == 0)
            return false;
    }
    return true;
}

/* Public functions */

void
lex_ConstantsMatchWord(size_t maxWordLength, size_t* length, const SLexConstantsWord** word) {
    if (g_maxWordLength < maxWordLength) {
        maxWordLength = g_maxWordLength;
    }

    uint32_t hashCode = 0;
    size_t index = 0;
    const SConstantWord* result = NULL;

    while (index < maxWordLength) {
        HASH(hashCode, toupper(lex_PeekChar(index)));
        ++index;
        for (SConstantWord* lex = g_wordsHashTable[hashCode]; lex != NULL; lex = list_GetNext(lex)) {
            if (lex->nameLength == index && lex_StartsWithNoCase(lex->definition.name, lex->nameLength)) {
                result = lex;
            }
        }
    }

    if (result == NULL) {
        *length = 0;
        *word = NULL;
    } else {
        *length = result->nameLength;
        *word = &result->definition;
    }
}



void
lex_PrintMaxTokensPerHash(void) {
    int maxWithSameHash = 0;
    int slotsInUse = 0;
    int totalStrings = 0;

    for (uint32_t i = 0; i < WORDS_HASH_SIZE; ++i) {
        int wordsInList = 0;
        const SConstantWord* word = g_wordsHashTable[i];
        if (word != NULL) {
            ++slotsInUse;
            while (word != NULL) {
                ++totalStrings;
                ++wordsInList;
                word = list_GetNext(word);
            }
        }
        if (wordsInList > maxWithSameHash)
            maxWithSameHash = wordsInList;
    }

    printf("Total strings %d, max %d strings with same hash, %d slots in use\n", totalStrings, maxWithSameHash, slotsInUse);
}

void
lex_ConstantsUndefineWord(const char* name, uint32_t token) {
    SConstantWord** hashTableEntry = &g_wordsHashTable[hashString(name)];

    for (SConstantWord* word = *hashTableEntry; word != NULL; word = list_GetNext(word)) {
        if (word->definition.token == token && strcmp(word->definition.name, name) == 0) {
            list_Remove(*hashTableEntry, word);
            mem_Free(word);
            return;
        }
    }
    internalerror("token not found");
}

void
lex_ConstantsUndefineWords(const SLexConstantsWord* words) {
    while (words->name) {
        lex_ConstantsUndefineWord(words->name, words->token);
        ++words;
    }
}

void
lex_ConstantsDefineWord(const char* name, uint32_t token) {
    assert(isNotLowerCase(name));
    assert(doesNotExist(name));

    /*printf("%s has hashvalue %d\n", lex->tzName, hash);*/

    SConstantWord* pNew = (SConstantWord*) mem_Alloc(sizeof(SConstantWord));
    list_Init(pNew);

    pNew->definition.name = name;
    pNew->definition.token = (EToken) token;
    pNew->nameLength = strlen(name);

    if (pNew->nameLength > g_maxWordLength)
        g_maxWordLength = pNew->nameLength;

    SConstantWord** hashTableEntry = &g_wordsHashTable[hashString(name)];
    list_Insert(*hashTableEntry, pNew);
}

void
lex_ConstantsDefineWords(const SLexConstantsWord* lex) {
    while (lex->name) {
        lex_ConstantsDefineWord(lex->name, lex->token);
        lex += 1;
    }

    // lex_PrintMaxTokensPerHash();
}

void
lex_ConstantsInit(void) {
    for (uint32_t i = 0; i < WORDS_HASH_SIZE; ++i)
        g_wordsHashTable[i] = NULL;

    g_maxWordLength = 0;
}
