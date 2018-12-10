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

#include <memory.h>

// From util
#include "asmotor.h"
#include "lists.h"
#include "fmath.h"
#include "mem.h"
#include "types.h"

// From xasm
#include "lexervariadics.h"

/* Internal defines */

#define MAX_VARIADIC 32

/* Private structures */

struct VariadicWordsPerChar {
	uint32_t idBits[UINT8_MAX + 1];
	list_Data(struct VariadicWordsPerChar);
};
typedef struct VariadicWordsPerChar SVariadicWordsPerChar;

/* Private variables */

static SVariadicWordDefinition g_variadicWordDefinitions[MAX_VARIADIC];
static SVariadicWordsPerChar* g_variadicWordsPerChar;
static uint32_t g_nextVariadicId;
static uint32_t g_variadicSuffix[UINT8_MAX];
static uint32_t g_variadicHasSuffixFlags;


static SVariadicWordsPerChar* allocVariadicWordsPerChar() {
	SVariadicWordsPerChar* words = mem_Alloc(sizeof(SVariadicWordsPerChar));
	memset(words->idBits, 0, sizeof(words->idBits));
	list_Init(words);

	return words;
}

static SVariadicWordsPerChar* variadicWordsAt(uint32_t charIndex) {
	SVariadicWordsPerChar** chars = &g_variadicWordsPerChar;

	for(;;) {
		if (*chars == NULL) {
			*chars = allocVariadicWordsPerChar();
		}
		if (charIndex-- == 0) {
			return *chars;
		}
		chars = &list_GetNext(*chars);
	}
}

SVariadicWordDefinition* variadicWordByMask(uint32_t mask) {
	if (mask == 0)
		return NULL;

	return &g_variadicWordDefinitions[log2n(mask)];
}

/* Public functions */

void lex_VariadicInit(void) {
	int i;

	g_variadicHasSuffixFlags = 0;
	for (i = 0; i < 256; ++i)
		g_variadicSuffix[i] = 0;

	g_variadicWordsPerChar = NULL;

	g_nextVariadicId = 0;
}

uint32_t lex_VariadicCreateWord(SVariadicWordDefinition* tok) {
	g_variadicWordDefinitions[g_nextVariadicId] = *tok;

	return 1U << g_nextVariadicId++;
}

void lex_VariadicRemoveAll(uint32_t id) {
	SVariadicWordsPerChar* chars = g_variadicWordsPerChar;

	while (chars) {
		int c;

		for (c = 0; c < 256; c += 1)
			chars->idBits[c] &= ~id;

		chars = list_GetNext(chars);
	}
}

void lex_VariadicAddCharRangeRepeating(uint32_t id, uint8_t start, uint8_t end, uint32_t charNumber) {
	if (charNumber >= 0) {
		SVariadicWordsPerChar* chars = variadicWordsAt(charNumber);

		while (chars) {
			uint16_t c = start;

			while (c <= end)
				chars->idBits[c++] |= id;

			chars = list_GetNext(chars);
		}
	}
}

void lex_VariadicAddCharRange(uint32_t id, uint8_t start, uint8_t end, uint32_t charNumber) {
	if (charNumber >= 0) {
		SVariadicWordsPerChar* chars = variadicWordsAt(charNumber);

		while (start <= end)
			chars->idBits[start++] |= id;
	}
}

void lex_VariadicAddSuffix(uint32_t id, uint8_t ch) {
	g_variadicHasSuffixFlags |= id;
	g_variadicSuffix[ch] |= id;
}

void lex_VariadicMatchString(char (*peek)(size_t), size_t bufferLength, size_t* length, SVariadicWordDefinition** variadicWord) {
	*length = 0;

	SVariadicWordsPerChar* chars = g_variadicWordsPerChar;
	uint32_t mask = 0;
	uint32_t nextMask = UINT32_MAX;
	while ((nextMask &= chars->idBits[(uint8_t) peek(*length)]) && *length < bufferLength) {
		*length += 1;
		mask = nextMask;

		if (list_GetNext(chars)) {
			chars = list_GetNext(chars);
		}
	}

	if (g_variadicHasSuffixFlags & mask) {
		nextMask = mask & g_variadicSuffix[(uint8_t) peek(*length)];
		if (nextMask) {
			*length += 1;
			mask = nextMask;
		} else {
			mask &= ~g_variadicHasSuffixFlags;
		}
	}

	if (*length == 0) {
		*variadicWord = NULL;
	} else {
		*variadicWord = variadicWordByMask(mask);
	}
}
