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

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <string.h>

#include "asmotor.h"
#include "file.h"
#include "mem.h"
#include "strbuf.h"
#include "lists.h"
#include "lexer.h"
#include "filestack.h"
#include "symbol.h"
#include "project.h"


/* Private defines */

#define HASH(hash, key) {            \
	(hash) = ((hash) << 1u) + (key); \
	(hash) ^= (hash) >> 3u;          \
	(hash) &= WORDS_HASH_SIZE - 1u;  \
}

#define WORDS_HASH_SIZE 1024u


/* Private structures */

typedef struct ConstantWord {
	string* name;
	EToken token;
	list_Data(struct ConstantWord);
} SConstantWord;


/* Private variables */

static SConstantWord* g_wordsHashTable[WORDS_HASH_SIZE];
static size_t g_maxWordLength;

static SLexerBuffer* g_pCurrentBuffer;


/* Private functions */

static void unputChar(char c) {
	g_pCurrentBuffer->charStack.stack[g_pCurrentBuffer->charStack.count++] = c;
}

static uint32_t hashString(const char* s) {
	uint32_t r = 0;

	while (*s) {
		HASH(r, toupper((uint8_t) *s));
		++s;
	}

	return r;
}

static size_t appendAndFreeString(char* dst, string* str) {
	if (str != NULL) {
		size_t length = str_Length(str);
		memcpy(dst, str_String(str), length);
		str_Free(str);
		return length;
	}
	return 0;
}

static void skip(size_t count) {
	if (g_pCurrentBuffer->charStack.count > 0) {
		if (count >= g_pCurrentBuffer->charStack.count) {
			count -= g_pCurrentBuffer->charStack.count;
			g_pCurrentBuffer->charStack.count = 0;
		} else {
			g_pCurrentBuffer->charStack.count -= count;
			return;
		}
	}
	g_pCurrentBuffer->index += count;
	if (g_pCurrentBuffer->index > g_pCurrentBuffer->bufferSize)
		g_pCurrentBuffer->index = g_pCurrentBuffer->bufferSize;
}

static size_t charsAvailable(void) {
	return g_pCurrentBuffer->bufferSize - g_pCurrentBuffer->index + g_pCurrentBuffer->charStack.count;
}

static bool
expandMacroArguments(char** dst, char argument) {
	switch (argument) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': {
			*dst += appendAndFreeString(*dst, fstk_GetMacroArgValue(argument));
			return true;
		}
		case '@': {
			*dst += appendAndFreeString(*dst, fstk_GetMacroUniqueId());
			return true;
		}
	}	

	return false;
}

static bool
expandEscapeSequence(char** dst, char escapeSymbol) {
	switch (escapeSymbol) {
		default: {
			return expandMacroArguments(dst, escapeSymbol);
		}
		case 'n': {
			*(*dst)++ = ASM_CRLF;
			return true;
		}
		case 't': {
			*(*dst)++ = ASM_TAB;
			return true;
		}
	}

	return false;
}

static bool
expandSymbolRecursive(char** dst, size_t* index, bool allowUndefinedSymbols);

static void
expandSymbolName(char* dst, size_t* index, bool allowUndefinedSymbols) {
	for (;;) {
		char ch = lex_PeekChar((*index)++);
		if (ch == 0 || ch == '}') {
			*dst = 0;
			break;
		} else {
			if (ch == '\\') {
				expandEscapeSequence(&dst, lex_PeekChar(*index));
			} else if (ch == '{') {
				expandSymbolRecursive(&dst, index, allowUndefinedSymbols);
			} else {
				*dst++ = ch;
			}
		}
	}
}

static bool
expandSymbolRecursive(char** dst, size_t* index, bool allowUndefinedSymbols) {
	bool result = false;

	char symbolNameArray[MAXSYMNAMELENGTH];
	expandSymbolName(symbolNameArray, index, allowUndefinedSymbols);

	string* symbolName = str_Create(symbolNameArray);

	if (allowUndefinedSymbols || sym_IsDefined(symbolName)) {
		*dst = sym_GetValueAsStringByName(*dst, symbolName);
		result = true;
	}

	str_Free(symbolName);
	return result;
}

static bool
expandStringUntil(char* dst, char* stopChars, bool allowUndefinedSymbols) {
	size_t index = 0;
	for (;;) {
		char ch = lex_PeekChar(index++);
		if (ch == 0 || strchr(stopChars, ch) != NULL) {
			break;
		}

		if (ch == '\\') {
			expandEscapeSequence(&dst, lex_PeekChar(index++));
		} else if (ch == '{') {
			if (!expandSymbolRecursive(&dst, &index, allowUndefinedSymbols)) {
				break;
			}
		} else {
			*dst++ = ch;
		}
	}

	lex_SkipBytes(index);
	*dst = 0;

	return true;
}

static bool
expandSymbol(char* dst, size_t index, bool allowUndefinedSymbols) {
	if (expandSymbolRecursive(&dst, &index, allowUndefinedSymbols)) {
		lex_SkipBytes(index);
		return true;
	}

	return false;
}


/* Public variables */

SLexerToken lex_Current;


/*	Public functions */

char lex_PeekChar(size_t index) {
	if (index < g_pCurrentBuffer->charStack.count) {
		return g_pCurrentBuffer->charStack.stack[g_pCurrentBuffer->charStack.count - index - 1];
	} else {
		index -= g_pCurrentBuffer->charStack.count;
	}
	index += g_pCurrentBuffer->index;
	if (index < g_pCurrentBuffer->bufferSize) {
		return g_pCurrentBuffer->buffer[index];
	} else {
		return 0;
	}
}

char lex_GetChar(void) {
	if (g_pCurrentBuffer->charStack.count > 0) {
		return g_pCurrentBuffer->charStack.stack[--(g_pCurrentBuffer->charStack.count)];
	}
	if (g_pCurrentBuffer->index < g_pCurrentBuffer->bufferSize) {
		return g_pCurrentBuffer->buffer[g_pCurrentBuffer->index++];
	} else {
		return 0;
	}
}

size_t lex_GetChars(char* dest, size_t length) {
	size_t copiedLines = 0;
	for (size_t i = 0; i < length; ++i) {
		char ch = lex_GetChar();
		*dest++ = ch;
		if (ch == '\n')
			copiedLines += 1;
	}
	*dest = 0;
	return copiedLines;
}

bool lex_MatchChar(char ch) {
	if (lex_PeekChar(0) == ch) {
		lex_GetChar();
		return true;
	} else {
		return false;
	}
}

bool lex_CompareNoCase(size_t index, const char* str, size_t length) {
	for (size_t i = 0; i < length; ++i) {
		if (tolower(lex_PeekChar(index + i)) != tolower(str[i]))
			return false;
	}
	return true;
}

bool lex_StartsWithNoCase(const char* str, size_t length) {
	return lex_CompareNoCase(0, str, length);
}

bool lex_StartsWithStringNoCase(const string* str) {
	return lex_CompareNoCase(0, str_String(str), str_Length(str));
}

void lex_Bookmark(SLexerBookmark* bookmark) {
	bookmark->Buffer = *g_pCurrentBuffer;
	bookmark->Token = lex_Current;
}

void lex_Goto(SLexerBookmark* bookmark) {
	*g_pCurrentBuffer = bookmark->Buffer;
	lex_Current = bookmark->Token;
}

size_t lex_SkipBytes(size_t count) {
	size_t linesSkipped = 0;

	if (g_pCurrentBuffer) {
		for (size_t i = 0; i < count; ++i) {
			char ch = lex_GetChar();
			if (ch == 0) {
				break;
			} else if (ch == '\n') {
				linesSkipped += 1;
			}
		}
	} else {
		internalerror("g_pCurrentBuffer not initialized");
	}

	return linesSkipped;
}

void lex_UnputString(const char* str) {
	size_t length = strlen(str);
	str += length;
	for (size_t i = 0; i < length ; ++i) {
		unputChar(*(--str));
	}
}

void lex_SetBuffer(SLexerBuffer* buffer) {
	if (buffer) {
		g_pCurrentBuffer = buffer;
	} else {
		internalerror("Argument must not be NULL");
	}
}

void lex_SetState(ELexerState state) {
	if (g_pCurrentBuffer) {
		g_pCurrentBuffer->State = state;
	} else {
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_FreeBuffer(SLexerBuffer* buffer) {
	if (buffer) {
		if (buffer->buffer) {
			mem_Free(buffer->buffer);
		} else {
			internalerror("buf->pBufferStart not initialized");
		}
		mem_Free(buffer);
	} else {
		internalerror("Argument must not be NULL");
	}
}

SLexerBuffer* lex_CreateMemoryBuffer(const char* memory, size_t size) {
	SLexerBuffer* pBuffer = (SLexerBuffer*) mem_Alloc(sizeof(SLexerBuffer));

	pBuffer->buffer = (char*) mem_Alloc(size);
	memcpy(pBuffer->buffer, memory, size);
	pBuffer->charStack.count = 0;
	pBuffer->index = 0;
	pBuffer->bufferSize = size;
	pBuffer->atLineStart = true;
	pBuffer->State = LEX_STATE_NORMAL;
	return pBuffer;
}

SLexerBuffer* lex_CreateFileBuffer(FILE* f) {
	char strterm = 0;
	char* pFile;
	bool bWasSpace = true;

	SLexerBuffer* pBuffer = (SLexerBuffer*) mem_Alloc(sizeof(SLexerBuffer));
	memset(pBuffer, 0, sizeof(SLexerBuffer));

	size_t size = fsize(f);

	pFile = (char*) mem_Alloc(size);
	size = fread(pFile, sizeof(uint8_t), size, f);

	pBuffer->buffer = (char*) mem_Alloc(size);
	char* dest = pBuffer->buffer;

	char* mem = pFile;

	while (mem < pFile + size) {
		if (*mem == '"' || *mem == '\'') {
			strterm = *mem;
			*dest++ = *mem++;
			while (*mem && *mem != strterm) {
				if (*mem == '\\')
					*dest++ = *mem++;

				*dest++ = *mem++;
			}
			*dest++ = *mem++;
			bWasSpace = false;
		} else if ((mem[0] == 10 && mem[1] == 13) || (mem[0] == 13 && mem[1] == 10)) {
			*dest++ = '\n';
			mem += 2;
			bWasSpace = true;
		} else if (mem[0] == 10 || mem[0] == 13) {
			*dest++ = '\n';
			mem += 1;
			bWasSpace = true;
		} else if (*mem == ';' || (bWasSpace && mem[0] == '*')) {
			++mem;
			while (*mem && *mem != 13 && *mem != 10)
				++mem;
			bWasSpace = false;
		} else {
			bWasSpace = isspace((uint8_t) *mem) ? true : false;
			*dest++ = *mem++;
		}
	}

	*dest++ = '\n';
	pBuffer->bufferSize = dest - pBuffer->buffer;
	pBuffer->atLineStart = true;

	mem_Free(pFile);
	return pBuffer;
}

void lex_Init(void) {
	for (uint32_t i = 0; i < WORDS_HASH_SIZE; ++i)
		g_wordsHashTable[i] = NULL;

	g_maxWordLength = 0;

	lex_VariadicInit();
}

void lex_PrintMaxTokensPerHash(void) {
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

void lex_UndefineToken(const char* name, uint32_t token) {
	SConstantWord** pHash = &g_wordsHashTable[hashString(name)];
	SConstantWord* pToken = *pHash;

	while (pToken) {
		if (pToken->token == (uint32_t) token && str_EqualConst(pToken->name, name) == 0) {
			list_Remove(*pHash, pToken);
			str_Free(pToken->name);
			mem_Free(pToken);
			return;
		}
		pToken = list_GetNext(pToken);
	}
	internalerror("token not found");
}

void lex_UndefineTokens(SLexerTokenDefinition* lex) {
	while (lex->name) {
        lex_UndefineToken(lex->name, lex->token);
		++lex;
	}
}

void lex_DefineToken(const char* name, uint32_t token) {
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

void lex_DefineTokens(const SLexerTokenDefinition* lex) {
	while (lex->name) {
        lex_DefineToken(lex->name, lex->token);
		lex += 1;
	}

	/*lex_PrintMaxTokensPerHash();*/
}


static uint32_t lex_LexStateNormal() {
	bool bLineStart = g_pCurrentBuffer->atLineStart;
	SConstantWord* pLongestFixed = NULL;

	g_pCurrentBuffer->atLineStart = false;

	for (;;) {
		/* Skip whitespace but stop at line break */
		for (;;) {
			uint8_t ch = (uint8_t) lex_PeekChar(0);
			if (isspace(ch) && ch != '\n') {
				bLineStart = 0;
				lex_GetChar();
			} else {
				break;
			}
		}

		/* Check if we're done with this buffer */
		if (lex_PeekChar(0) == 0) {
			if (fstk_ProcessNextBuffer()) {
				bLineStart = g_pCurrentBuffer->atLineStart;
				g_pCurrentBuffer->atLineStart = false;
				continue;
			} else {
				lex_Current.token = T_NONE;
				return 0;
			}
		}

		size_t variadicLength;
		SVariadicWordDefinition* variadicWord;
		lex_VariadicMatchString(lex_PeekChar, charsAvailable(), &variadicLength, &variadicWord);

		size_t nMaxLen = charsAvailable();
		if (g_maxWordLength < nMaxLen) {
			nMaxLen = g_maxWordLength;
		}

		lex_Current.length = 0;
		uint32_t hashCode = 0;
		size_t s = 0;

		while (lex_Current.length < nMaxLen) {
			++lex_Current.length;
			HASH(hashCode, toupper(lex_PeekChar(s)));
			++s;
			if (g_wordsHashTable[hashCode]) {
				SConstantWord* lex = g_wordsHashTable[hashCode];
				while (lex) {
					if (str_Length(lex->name) == lex_Current.length && lex_StartsWithStringNoCase(lex->name)) {
						pLongestFixed = lex;
					}
					lex = list_GetNext(lex);
				}
			}
		}

		if (variadicLength == 0 && pLongestFixed == NULL) {
			char p = lex_PeekChar(0);
			if (p == '"' || p == '\'') {
				lex_GetChar();
				char term[3] = { p, '\n', 0 };
				expandStringUntil(lex_Current.value.string, term, true);
				lex_Current.token = T_STRING;
				lex_Current.length = strlen(lex_Current.value.string);
				return T_STRING;
			} else if (p == '{') {
				if (expandSymbol(lex_Current.value.string, 1, false)) {
					lex_Current.token = T_STRING;
					lex_Current.length = strlen(lex_Current.value.string);
					return T_STRING;
				}
			}

			uint8_t ch = (uint8_t) lex_GetChar();

			if (ch == '\n') {
				g_pCurrentBuffer->atLineStart = true;
			}

			lex_Current.length = 1;
			lex_Current.token = (EToken) ch;
			return (uint32_t) ch;
		}

		if (variadicLength == 0) {
			lex_Current.length = str_Length(pLongestFixed->name);
			skip(lex_Current.length);
			lex_Current.token = pLongestFixed->token;
			return pLongestFixed->token;
		}

		if (variadicWord && variadicWord->token == T_ID && bLineStart && lex_PeekChar(variadicLength) == ':') {
			pLongestFixed = NULL;
		}

		if (pLongestFixed == NULL || variadicLength > str_Length(pLongestFixed->name)) {
			lex_Current.length = variadicLength;
			if (variadicWord->callback && !variadicWord->callback(lex_Current.length)) {
				continue;
			}

			if (variadicWord->token == T_ID && bLineStart) {
				skip(lex_Current.length);
				lex_Current.token = T_LABEL;
				return T_LABEL;
			} else {
				skip(lex_Current.length);
				lex_Current.token = variadicWord->token;
				return variadicWord->token;
			}
		} else {
			lex_Current.length = str_Length(pLongestFixed->name);
			lex_GetChars(lex_Current.value.string, lex_Current.length);
			lex_Current.token = pLongestFixed->token;
			return pLongestFixed->token;
		}
	}
}

uint32_t lex_GetNextToken(void) {
	switch (g_pCurrentBuffer->State) {
		case LEX_STATE_NORMAL: {
			return lex_LexStateNormal();
			break;
		}
		case LEX_STATE_MACRO_ARG0: {
			g_pCurrentBuffer->State = LEX_STATE_MACRO_ARGS;

			if (lex_MatchChar('.')) {
				int i = 0;

				while (!isspace((unsigned char) lex_PeekChar(0))) {
					lex_Current.value.string[i++] = lex_GetChar();
				}
				lex_Current.value.string[i] = 0;
				return lex_Current.token = T_MACROARG0;
			}

			// fall through
		}
		case LEX_STATE_MACRO_ARGS: {
			while (isspace((unsigned char) lex_PeekChar(0)) && lex_PeekChar(0) != '\n') {
				lex_GetChar();
			}

			size_t tokenStart = g_pCurrentBuffer->index;

			if (lex_MatchChar('<')) {
				expandStringUntil(lex_Current.value.string, ">\n", true);
				lex_MatchChar('>');
			} else {
				expandStringUntil(lex_Current.value.string, ",\n", true);
			}

			if (g_pCurrentBuffer->index > tokenStart) {
				size_t length = g_pCurrentBuffer->index - tokenStart;
				if (lex_PeekChar(0) == '\n') {
					while (lex_Current.value.string[length - 1] == ' ') {
						lex_Current.value.string[--length] = 0;
					}
				}
				lex_Current.length = length;
				lex_Current.token = T_STRING;
				return T_STRING;
			} else if (lex_MatchChar('\n')) {
				g_pCurrentBuffer->atLineStart = true;
				lex_Current.length = 1;
				lex_Current.token = T_LINEFEED;
				return '\n';
			} else if (lex_MatchChar(',')) {
				lex_Current.length = 1;
				lex_Current.token = T_COMMA;
				return ',';
			} else {
				lex_Current.token = T_NONE;
				return 0;
			}
			break;
		}
	}

	internalerror("Abnormal error encountered");
	return 0;
}
