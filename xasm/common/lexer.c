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
#include "lists.h"
#include "lexer.h"
#include "filestack.h"
#include "symbol.h"
#include "project.h"


/* Internal defines */

#define HASH(hash, key)              \
{                                    \
	(hash) = ((hash) << 1u) + (key); \
	(hash) ^= (hash) >> 3u;          \
	(hash) &= WORDS_HASH_SIZE - 1u;  \
}

#define SAFETY_MARGIN MAXSTRINGSYMBOLSIZE
#define WORDS_HASH_SIZE 1024u
#define BUF_REMAINING_CHARS (g_pCurrentBuffer->pBufferStart + g_pCurrentBuffer->bufferSize - g_pCurrentBuffer->pBuffer)

/* Private structures */

struct ConstantWord {
	string* name;
	EToken token;
	list_Data(struct ConstantWord);
};
typedef struct ConstantWord SConstantWord;

/* Private variables */

static SConstantWord* g_wordsHashTable[WORDS_HASH_SIZE];
static size_t g_maxWordLength;

static SLexBuffer* g_pCurrentBuffer;

/* Public variables */

SLexToken g_CurrentToken;

/* Private functions */

static uint32_t hashString(const char* s) {
	uint32_t r = 0;

	while (*s) {
		HASH(r, toupper((uint8_t) *s));
		++s;
	}

	return r;
}

static char* lex_ParseStringUntil(char* dst, char* src, char* stopchar, bool bAllowUndefinedSymbols) {
	while (*src && strchr(stopchar, *src) == NULL) {
		char ch;

		if ((ch = *src++) == '\\') {
			/* Handle escape sequences */

			switch (ch = (*src++)) {
				default:
					break;
				case 'n': {
					ch = ASM_CRLF;
					break;
				}
				case 't': {
					ch = ASM_TAB;
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9': {
					char* marg;

					if ((marg = fstk_GetMacroArgValue(ch)) != NULL) {
						while (*marg)
							*dst++ = *marg++;
					}

					ch = 0;
					break;
				}
				case '@': {
					char* marg;

					if ((marg = fstk_GetMacroRunID()) != NULL) {
						while (*marg)
							*dst++ = *marg++;

						ch = 0;
					}
					break;
				}
			}
		} else if (ch == '{') {
			bool bSymDefined;
			string* pSymName;
			char sym[MAXSYMNAMELENGTH];
			int i = 0;

			while (*src && (*src != '}') && (strchr(stopchar, *src) == NULL)) {
				if ((ch = *src++) == '\\') {
					switch (ch = (*src++)) {
						default:
							break;
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9': {
							char* marg;

							if ((marg = fstk_GetMacroArgValue(ch)) != NULL) {
								while (*marg) {
									sym[i++] = *marg++;
								}

								ch = 0;
							}
							break;
						}
						case '@': {
							char* marg;

							if ((marg = fstk_GetMacroRunID()) != NULL) {
								while (*marg) {
									sym[i++] = *marg++;
								}

								ch = 0;
							}
							break;
						}
					}
				} else {
					sym[i++] = ch;
				}
			}

			sym[i] = 0;
			pSymName = str_Create(sym);
			bSymDefined = sym_IsDefined(pSymName);

			if (!bAllowUndefinedSymbols && !bSymDefined) {
				str_Free(pSymName);
				return NULL;
			}

			dst = sym_GetValueAsStringByName(dst, pSymName);
			str_Free(pSymName);

			if (*src == '}') {
				if (strchr(stopchar, *src++) != NULL)
					return src;
			} else
				prj_Fail(ERROR_CHAR_EXPECTED, '}');

			ch = 0;
		}

		if (ch != 0)
			*dst++ = ch;
	}

	*dst++ = 0;

	return src;
}

/*	Public routines*/

void lex_Bookmark(SLexBookmark* pBookmark) {
	pBookmark->Buffer = *g_pCurrentBuffer;
	pBookmark->Token = g_CurrentToken;
}

void lex_Goto(SLexBookmark* pBookmark) {
	*g_pCurrentBuffer = pBookmark->Buffer;
	g_CurrentToken = pBookmark->Token;
}

void lex_SkipBytes(size_t count) {
	if (g_pCurrentBuffer) {
		while (count > 0) {
			if (g_pCurrentBuffer->pBuffer[0] == '\n')
				++g_pFileContext->LineNumber;
			++g_pCurrentBuffer->pBuffer;
			--count;
		}
	} else {
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_RewindBytes(size_t count) {
	if (g_pCurrentBuffer) {
		g_pCurrentBuffer->pBuffer -= count;
	} else {
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_UnputChar(char c) {
	if (g_pCurrentBuffer) {
		*(--(g_pCurrentBuffer->pBuffer)) = c;
	} else {
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_UnputString(const char* s) {
	ssize_t i = strlen(s) - 1;

	while (i >= 0) {
		lex_UnputChar(s[i--]);
	}
}

void lex_SetBuffer(SLexBuffer* buf) {
	if (buf) {
		g_pCurrentBuffer = buf;
	} else {
		internalerror("Argument must not be NULL");
	}
}

void lex_SetState(ELexerState i) {
	if (g_pCurrentBuffer) {
		g_pCurrentBuffer->State = i;
	} else {
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_FreeBuffer(SLexBuffer* buf) {
	if (buf) {
		if (buf->pBufferStart) {
			mem_Free(buf->pBufferStart - SAFETY_MARGIN);
		} else {
			internalerror("buf->pBufferStart not initialized");
		}
		mem_Free(buf);
	} else {
		internalerror("Argument must not be NULL");
	}
}

SLexBuffer* lex_CreateMemoryBuffer(const char* mem, size_t size) {
	SLexBuffer* pBuffer = (SLexBuffer*) mem_Alloc(sizeof(SLexBuffer));
	memset(pBuffer, 0, sizeof(SLexBuffer));

	pBuffer->pBuffer = pBuffer->pBufferStart = (char*) mem_Alloc(size + 1 + SAFETY_MARGIN);

	pBuffer->pBuffer += SAFETY_MARGIN;
	pBuffer->pBufferStart += SAFETY_MARGIN;
	memcpy(pBuffer->pBuffer, mem, size);
	pBuffer->bufferSize = size;
	pBuffer->atLineStart = true;
	pBuffer->pBuffer[size] = 0;
	pBuffer->State = LEX_STATE_NORMAL;
	return pBuffer;
}

SLexBuffer* lex_CreateFileBuffer(FILE* f) {
	size_t size;
	char strterm = 0;
	char* pFile;
	char* mem;
	char* dest;
	bool bWasSpace = true;

	SLexBuffer* pBuffer = (SLexBuffer*) mem_Alloc(sizeof(SLexBuffer));
	memset(pBuffer, 0, sizeof(SLexBuffer));

	size = fsize(f);

	pFile = (char*) mem_Alloc(size);
	size = fread(pFile, sizeof(uint8_t), size, f);

	pBuffer->pBuffer = pBuffer->pBufferStart = (char*) mem_Alloc(size + 2 + SAFETY_MARGIN) + SAFETY_MARGIN;
	dest = pBuffer->pBuffer;

	mem = pFile;

	while ((size_t) (mem - pFile) < size) {
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
			bWasSpace = isspace((uint8_t) *mem);
			*dest++ = *mem++;
		}
	}

	*dest++ = '\n';
	*dest++ = 0;
	pBuffer->bufferSize = dest - pBuffer->pBufferStart;
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

void lex_RemoveString(const char* pszName, uint32_t nToken) {
	SConstantWord** pHash = &g_wordsHashTable[hashString(pszName)];
	SConstantWord* pToken = *pHash;

	while (pToken) {
		if (pToken->token == (uint32_t) nToken && str_EqualConst(pToken->name, pszName) == 0) {
			list_Remove(*pHash, pToken);
			str_Free(pToken->name);
			mem_Free(pToken);
			return;
		}
		pToken = list_GetNext(pToken);
	}
	internalerror("token not found");
}

void lex_RemoveStrings(SLexInitString* pLex) {
	while (pLex->name) {
		lex_RemoveString(pLex->name, pLex->token);
		++pLex;
	}
}

void lex_AddString(const char* pszName, uint32_t nToken) {
	SConstantWord** pHash = &g_wordsHashTable[hashString(pszName)];
	SConstantWord* pPrev = *pHash;
	SConstantWord* pNew;

	/*printf("%s has hashvalue %d\n", lex->tzName, hash);*/

	pNew = (SConstantWord*) mem_Alloc(sizeof(SConstantWord));
	memset(pNew, 0, sizeof(SConstantWord));

	pNew->name = str_Create(pszName);
	str_ToUpperReplace(&pNew->name);
	pNew->token = (EToken) nToken;

	if (str_Length(pNew->name) > g_maxWordLength)
		g_maxWordLength = str_Length(pNew->name);

	if (pPrev) {
		list_InsertAfter(pPrev, pNew);
	} else {
		*pHash = pNew;
	}
}

void lex_AddStrings(SLexInitString* lex) {
	while (lex->name) {
		lex_AddString(lex->name, lex->token);
		++lex;
	}

	/*lex_PrintMaxTokensPerHash();*/
}

static uint32_t lex_LexStateNormal() {
	bool bLineStart = g_pCurrentBuffer->atLineStart;
	SConstantWord* pLongestFixed = NULL;

	g_pCurrentBuffer->atLineStart = false;

	for (;;) {
		size_t variadicLength;
		SVariadicWordDefinition* variadicWord;
		size_t nMaxLen;
		uint32_t hashCode;
		unsigned char* s;

		while (isspace((unsigned char) g_pCurrentBuffer->pBuffer[0]) && g_pCurrentBuffer->pBuffer[0] != '\n') {
			bLineStart = 0;
			g_pCurrentBuffer->pBuffer += 1;
		}

		if (*(g_pCurrentBuffer->pBuffer) == 0) {
			if (fstk_RunNextBuffer()) {
				bLineStart = g_pCurrentBuffer->atLineStart;
				g_pCurrentBuffer->atLineStart = false;
				continue;
			} else {
				g_CurrentToken.Token = T_NONE;
				return 0;
			}
		}

		lex_VariadicMatchString(g_pCurrentBuffer->pBuffer, BUF_REMAINING_CHARS, &variadicLength, &variadicWord);

		nMaxLen = BUF_REMAINING_CHARS;
		if (g_maxWordLength < nMaxLen) {
			nMaxLen = g_maxWordLength;
		}

		g_CurrentToken.TokenLength = 0;
		hashCode = 0;
		s = (unsigned char*) g_pCurrentBuffer->pBuffer;
		while (g_CurrentToken.TokenLength < nMaxLen) {
			++g_CurrentToken.TokenLength;
			HASH(hashCode, toupper(*s));
			++s;
			if (g_wordsHashTable[hashCode]) {
				SConstantWord* lex = g_wordsHashTable[hashCode];
				while (lex) {
					if (str_Length(lex->name) == g_CurrentToken.TokenLength &&
						0 == _strnicmp(g_pCurrentBuffer->pBuffer, str_String(lex->name), g_CurrentToken.TokenLength)) {
						pLongestFixed = lex;
					}
					lex = list_GetNext(lex);
				}
			}

		}

		if (variadicLength == 0 && pLongestFixed == NULL) {
			if (*g_pCurrentBuffer->pBuffer == '"' || *g_pCurrentBuffer->pBuffer == '\'') {
				char term[3];
				term[0] = *g_pCurrentBuffer->pBuffer;
				term[1] = '\n';
				term[2] = 0;
				g_pCurrentBuffer->pBuffer = lex_ParseStringUntil(g_CurrentToken.Value.aString,
																 g_pCurrentBuffer->pBuffer + 1, term, true);
				if (*g_pCurrentBuffer->pBuffer != term[0]) {
					prj_Fail(ERROR_STRING_TERM);
				} else {
					g_pCurrentBuffer->pBuffer += 1;
				}
				g_CurrentToken.Token = T_STRING;
				return T_STRING;
			} else if (*g_pCurrentBuffer->pBuffer == '{') {
				char sym[MAXSYMNAMELENGTH];
				char* pNewBuf;

				pNewBuf = lex_ParseStringUntil(sym, g_pCurrentBuffer->pBuffer, "}\n", false);
				if (pNewBuf) {
					g_pCurrentBuffer->pBuffer = pNewBuf;
					g_CurrentToken.Token = T_STRING;
					strcpy(g_CurrentToken.Value.aString, sym);
					return T_STRING;
				}
			}
			if (*g_pCurrentBuffer->pBuffer == '\n') {
				g_pCurrentBuffer->atLineStart = true;
			}

			g_CurrentToken.TokenLength = 1;
			g_CurrentToken.Token = (EToken) *(g_pCurrentBuffer->pBuffer);
			return (uint32_t) *(g_pCurrentBuffer->pBuffer)++;
		}

		if (variadicLength == 0) {
			g_CurrentToken.TokenLength = str_Length(pLongestFixed->name);
			g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
			g_CurrentToken.Token = pLongestFixed->token;
			return pLongestFixed->token;
		}

		if (pLongestFixed == NULL || variadicLength > str_Length(pLongestFixed->name)) {
			g_CurrentToken.TokenLength = variadicLength;
			if (variadicWord->callback) {
				if (!variadicWord->callback(g_pCurrentBuffer->pBuffer, g_CurrentToken.TokenLength)) {
					continue;
				}
			}

			if (variadicWord->token == T_ID && bLineStart) {
				g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
				g_CurrentToken.Token = T_LABEL;
				return T_LABEL;
			} else {
				g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
				g_CurrentToken.Token = variadicWord->token;
				return variadicWord->token;
			}
		} else if (variadicWord && variadicWord->token == T_ID && bLineStart &&
				   g_pCurrentBuffer->pBuffer[variadicLength] == ':') {
			g_CurrentToken.TokenLength = variadicLength;
			if (variadicWord->callback) {
				if (!(variadicWord->callback(g_pCurrentBuffer->pBuffer, g_CurrentToken.TokenLength))) {
					continue;
				}
			}
			memcpy(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, g_CurrentToken.TokenLength);
			g_CurrentToken.Value.aString[g_CurrentToken.TokenLength] = 0;
			g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
			g_CurrentToken.Token = T_LABEL;
			return T_LABEL;
		} else {
			g_CurrentToken.TokenLength = str_Length(pLongestFixed->name);
			memcpy(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, g_CurrentToken.TokenLength);
			g_CurrentToken.Value.aString[g_CurrentToken.TokenLength] = 0;
			g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
			g_CurrentToken.Token = pLongestFixed->token;
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

			if (g_pCurrentBuffer->pBuffer[0] == '.') {
				int i = 0;

				g_pCurrentBuffer->pBuffer += 1;
				while (!isspace((unsigned char) g_pCurrentBuffer->pBuffer[0])) {
					g_CurrentToken.Value.aString[i++] = g_pCurrentBuffer->pBuffer[0];
					g_pCurrentBuffer->pBuffer += 1;
				}
				g_CurrentToken.Value.aString[i++] = 0;
				return g_CurrentToken.Token = T_MACROARG0;
			}

			// fall through
		}
		case LEX_STATE_MACRO_ARGS: {
			char* newbuf;
			size_t index;

			while (isspace((unsigned char) g_pCurrentBuffer->pBuffer[0]) && g_pCurrentBuffer->pBuffer[0] != '\n') {
				g_pCurrentBuffer->pBuffer += 1;
			}

			if (g_pCurrentBuffer->pBuffer[0] == '<') {
				g_pCurrentBuffer->pBuffer += 1;
				newbuf = lex_ParseStringUntil(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, ">\n", true);
				index = newbuf - g_pCurrentBuffer->pBuffer;
				if (newbuf[0] == '>')
					newbuf += 1;
			} else {
				newbuf = lex_ParseStringUntil(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, ",\n", true);
				index = newbuf - g_pCurrentBuffer->pBuffer;
			}
			g_pCurrentBuffer->pBuffer = newbuf;

			if (index) {
				g_CurrentToken.TokenLength = index;
				if (*(g_pCurrentBuffer->pBuffer) == '\n') {
					while (g_CurrentToken.Value.aString[--index] == ' ') {
						g_CurrentToken.Value.aString[index] = 0;
						g_CurrentToken.TokenLength -= 1;
					}
				}
				g_CurrentToken.Token = T_STRING;
				return T_STRING;
			} else if (*(g_pCurrentBuffer->pBuffer) == '\n') {
				g_pCurrentBuffer->pBuffer += 1;
				g_pCurrentBuffer->atLineStart = true;
				g_CurrentToken.TokenLength = 1;
				g_CurrentToken.Token = T_LINEFEED;
				return '\n';
			} else if (*(g_pCurrentBuffer->pBuffer) == ',') {
				g_pCurrentBuffer->pBuffer += 1;
				g_CurrentToken.TokenLength = 1;
				g_CurrentToken.Token = T_COMMA;
				return ',';
			} else {
				g_CurrentToken.Token = T_NONE;
				return 0;
			}
			break;
		}
	}

	internalerror("Weird error encountered");
	return 0;
}
