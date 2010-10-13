/*  Copyright 2008 Carsten Sørensen

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
#include <string.h>
#include <memory.h>
#include <ctype.h>

#include "asmotor.h"
#include "lists.h"
#include "lexer.h"
#include "fstack.h"
#include "symbol.h"
#include "project.h"


/*	Internal defines*/

#define HASH(hash,key)			\
{								\
	hash = ((hash) << 1) + key;	\
	hash ^= hash >> 3;			\
	hash &= LEXHASHSIZE - 1;	\
}

#define TOKENSTACKSIZE	16
#define	SAFETYMARGIN	MAXSTRINGSYMBOLSIZE
#define	MAXFLOATS		32
#define	LEXHASHSIZE		1024
#define	BUF_REMAINING_CHARS	(g_pCurrentBuffer->pBufferStart + g_pCurrentBuffer->nBufferSize - g_pCurrentBuffer->pBuffer)




/*	Private structures*/

struct LexString
{
	char*	pszName;
	ULONG	Token;
	SLONG	NameLength;
	list_Data(struct LexString);
};
typedef	struct LexString SLexString;

struct FloatingChars
{
	ULONG Chars[256];
	list_Data(struct FloatingChars);
};
typedef	struct FloatingChars SFloatingChars;




/*	Private variables*/

static SLexFloat g_aLexFloat[MAXFLOATS];
static ULONG g_nNextFreeFloat;
static SLexString* g_aLexHash[LEXHASHSIZE];
static SLexBuffer* g_pCurrentBuffer;
static SLONG g_nLexTokenMaxLength;
static SFloatingChars* g_pFloatingChars;
static ULONG g_aFloatSuffix[256];
static ULONG g_nHasSuffix;



/*	Public variables*/

SLexToken	g_CurrentToken;




/*	Private routines*/

static SFloatingChars* lex_GetPointerToFloatingChar(SLONG charnumber)
{
	if(charnumber >= 0)
	{
		SFloatingChars* chars;

		if(g_pFloatingChars == NULL)
		{
			if((g_pFloatingChars = malloc(sizeof(SFloatingChars))) == NULL)
			{
				internalerror("Out of memory");
				return NULL;
			}
			memset(g_pFloatingChars, 0, sizeof(SFloatingChars));
		}

		chars = g_pFloatingChars;

		if(charnumber == 0)
		{
			while(list_GetNext(chars))
				chars = list_GetNext(chars);
		}
		else
		{
			while(--charnumber > 0)
			{
				if(list_GetNext(chars))
				{
					chars = list_GetNext(chars);
				}
				else
				{
					SFloatingChars* newchars;

					if((newchars = malloc(sizeof(SFloatingChars))) != NULL)
					{
						memset(newchars, 0, sizeof(SFloatingChars));
						list_InsertAfter(chars, newchars);
						chars = newchars;
					}
					else
					{
						internalerror("Out of memory");
					}
				}
			}
		}

		return chars;
	}
	else
	{
		internalerror("Negative argument not allowed");
		return NULL;
	}
}

static SLexFloat* lex_GetFloat(ULONG id)
{
    ULONG r = 0;
	ULONG mask = 1;

    if(id == 0)
		return NULL;

    while((id & mask) == 0)
    {
		mask <<= 1;
		r += 1;
    }

    return &g_aLexFloat[r];
}

static ULONG lex_CalcHash(char* s)
{
    ULONG r = 0;

    while(*s)
    {
		HASH(r, toupper(*s));
		++s;
    }

    return r;
}

static char* lex_ParseStringUntil(char* dst, char* src, char* stopchar, BOOL bAllowUndefinedSymbols)
{
	while(*src && strchr(stopchar, *src) == NULL)
	{
		char ch;

		if((ch = *src++) == '\\')
		{
			/*	Ahh, it's an escape sequence*/

			switch(ch = (*src++))
			{
				case 'n':
				{
					ch = ASM_CRLF;
					break;
				}
				case 't':
				{
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
				case '9':
				{
					char* marg;

					if((marg = fstk_GetMacroArgValue(ch))!=NULL)
					{
						while(*marg)
							*dst++ = *marg++;
					}

					ch = 0;
					break;
				}
				case '@':
				{
					char* marg;

					if((marg=fstk_GetMacroRunID())!=NULL)
					{
						while(*marg)
							*dst++ = *marg++;

						ch=0;
					}
					break;
				}
			}
		}
		else if(ch=='{')
		{
			char sym[MAXSYMNAMELENGTH];
			int i = 0;

			while(*src && (*src!='}') && (strchr(stopchar,*src)==NULL))
			{
				if((ch=*src++)=='\\')
				{
					switch(ch=(*src++))
					{
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
						{
							char* marg;

							if((marg=fstk_GetMacroArgValue(ch))!=NULL)
							{
								while(*marg)
								{
									sym[i++]=*marg++;
								}

								ch=0;
							}
							break;
						}
						case '@':
						{
							char* marg;

							if((marg=fstk_GetMacroRunID())!=NULL)
							{
								while(*marg)
								{
									sym[i++]=*marg++;
								}

								ch=0;
							}
							break;
						}
					}
				}
				else
				{
					sym[i++]=ch;
				}
			}

			sym[i] = 0;
			if(!bAllowUndefinedSymbols && !sym_isDefined(sym))
			{
				return NULL;
			}

			dst = sym_ConvertSymbolValueToString(dst, sym);
			if(*src == '}')
			{
				if(strchr(stopchar, *src++) != NULL)
					return src;
			}
			else
				prj_Fail(ERROR_CHAR_EXPECTED, '}');

			ch = 0;
		}

		if(ch != 0)
			*dst++ = ch;
	}

	*dst++ = 0;

	return src;
}




/*	Public routines*/

void lex_Bookmark(SLexBookmark* pBookmark)
{
	pBookmark->Buffer = *g_pCurrentBuffer;
	pBookmark->Token = g_CurrentToken;
}

void lex_Goto(SLexBookmark* pBookmark)
{
	*g_pCurrentBuffer = pBookmark->Buffer;
	g_CurrentToken = pBookmark->Token;
}

void lex_SkipBytes(ULONG count)
{
	if(g_pCurrentBuffer)
	{
		while(count > 0)
		{
			if(g_pCurrentBuffer->pBuffer[0] == '\n')
				++g_pFileContext->LineNumber;
			++g_pCurrentBuffer->pBuffer;
			--count;
		}
	}
	else
	{
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_RewindBytes(ULONG count)
{
	if(g_pCurrentBuffer)
	{
		g_pCurrentBuffer->pBuffer-=count;
	}
	else
	{
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_UnputChar(char c)
{
	if(g_pCurrentBuffer)
	{
		*(--(g_pCurrentBuffer->pBuffer)) = c;
	}
	else
	{
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void lex_UnputString(char* s)
{
	int i = (int)strlen(s) - 1;

	while(i >= 0)
	{
		lex_UnputChar(s[i--]);
	}
}

void lex_SetBuffer(SLexBuffer* buf)
{
	if(buf)
	{
		g_pCurrentBuffer = buf;
	}
	else
	{
		internalerror("Argument must not be NULL");
	}
}

void	lex_SetState(ELexerState i)
{
	if(g_pCurrentBuffer)
	{
		g_pCurrentBuffer->State=i;
	}
	else
	{
		internalerror("g_pCurrentBuffer not initialized");
	}
}

void	lex_FreeBuffer(SLexBuffer* buf)
{
	if(buf)
	{
		if(buf->pBufferStart)
		{
			free(buf->pBufferStart-SAFETYMARGIN);
		}
		else
		{
			internalerror("buf->pBufferStart not initialized");
		}
		free(buf);
	}
	else
	{
		internalerror("Argument must not be NULL");
	}
}

SLexBuffer* lex_CreateMemoryBuffer(char* mem, size_t size)
{
	SLexBuffer* pBuffer;

	if((pBuffer = (SLexBuffer* )malloc(sizeof(SLexBuffer))) != NULL)
	{
		memset(pBuffer, 0, sizeof(SLexBuffer));
		if((pBuffer->pBuffer = pBuffer->pBufferStart = (char*)malloc(size + 1 + SAFETYMARGIN)) != NULL)
		{
			pBuffer->pBuffer += SAFETYMARGIN;
			pBuffer->pBufferStart += SAFETYMARGIN;
			memcpy(pBuffer->pBuffer, mem, size);
			pBuffer->nBufferSize = size;
			pBuffer->bAtLineStart = TRUE;
			pBuffer->pBuffer[size] = 0;
			pBuffer->State = LEX_STATE_NORMAL;
			return pBuffer;
		}
	}
	internalerror("Out of memory!");
	return NULL;
}

SLexBuffer* lex_CreateFileBuffer(FILE* f)
{
	SLexBuffer* pBuffer;

	if((pBuffer = (SLexBuffer*)malloc(sizeof(SLexBuffer))) != NULL)
	{
	    size_t size;
		char strterm = 0;
		char* pFile;

		memset(pBuffer, 0, sizeof(SLexBuffer));

		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);

		if((pFile = (char*)malloc(size)) != NULL)
		{
			size = fread(pFile, sizeof(UBYTE), size, f);

			if((pBuffer->pBuffer = pBuffer->pBufferStart = (char*)malloc(size + 2 + SAFETYMARGIN)) != NULL)
			{
				char* mem;
				char* dest;
				BOOL bWasSpace = TRUE;

				pBuffer->pBuffer += SAFETYMARGIN;
				pBuffer->pBufferStart += SAFETYMARGIN;
				dest = pBuffer->pBuffer;

				mem = pFile;

				while((size_t)(mem - pFile) < size)
				{
					if(*mem == '"' || *mem == '\'')
					{
						strterm = *mem;
						*dest++ = *mem++;
						while(*mem && *mem != strterm)
						{
							if(*mem == '\\')
								*dest++ = *mem++;

							*dest++ = *mem++;
						}
						*dest++ = *mem++;
						bWasSpace = FALSE;
					}
					else if((mem[0]==10 && mem[1]==13) || (mem[0]==13 && mem[1]==10))
					{
						*dest++ = '\n';
						mem += 2;
						bWasSpace = TRUE;
					}
					else if(mem[0] == 10 || mem[0] == 13)
					{
						*dest++ = '\n';
						mem += 1;
						bWasSpace = TRUE;
					}
					else if(*mem==';' || (bWasSpace && mem[0] == '*'))
					{
						++mem;
						while(*mem && *mem != 13 && *mem != 10)
							++mem;
						bWasSpace = FALSE;
					}
					else
					{
						bWasSpace = isspace((UBYTE)*mem);
						*dest++ = *mem++;
					}
				}

				*dest++ = '\n';
				*dest++ = 0;
				pBuffer->nBufferSize = dest - pBuffer->pBufferStart;
				pBuffer->bAtLineStart = TRUE;

				free(pFile);
				return pBuffer;
			}
			free(pFile);
		}
		free(pBuffer);
	}

	internalerror("Out of memory!");
	return NULL;
}

ULONG lex_FloatAlloc(SLexFloat* tok)
{
	g_aLexFloat[g_nNextFreeFloat] = *tok;

	return 1 << g_nNextFreeFloat++;
}

void lex_FloatRemoveAll(ULONG id)
{
	SFloatingChars* chars = g_pFloatingChars;

	while(chars)
	{
		int c;

		for(c = 0; c < 256; c += 1)
			chars->Chars[c] &= ~id;

		chars = list_GetNext(chars);
	}
}

void lex_FloatAddRangeAndBeyond(ULONG id, UWORD start, UWORD end, SLONG charnumber)
{
	if(charnumber >= 0)
	{
		SFloatingChars* chars = lex_GetPointerToFloatingChar(charnumber);

		while(chars)
		{
			UWORD c;

			c = start;

		    while(c <= end)
				chars->Chars[c++] |= id;

			chars = list_GetNext(chars);
		}
	}
}

void lex_FloatAddRange(ULONG id, UWORD start, UWORD end, SLONG charnumber)
{
	if(charnumber >= 0)
	{
		SFloatingChars* chars = lex_GetPointerToFloatingChar(charnumber);

		while(start <= end)
			chars->Chars[start++] |= id;
	}
}

void lex_FloatSetSuffix(ULONG id, UBYTE ch)
{
	g_nHasSuffix |= id;
	g_aFloatSuffix[ch] |= id;
}

void lex_Init(void)
{
	int i;

	for(i = 0; i < LEXHASHSIZE; ++i)
		g_aLexHash[i] = NULL;

	g_nHasSuffix = 0;
	for(i = 0; i < 256; ++i)
		g_aFloatSuffix[i] = 0;

	g_pFloatingChars = NULL;

	g_nLexTokenMaxLength = 0;
	g_nNextFreeFloat = 0;
}

void lex_PrintMaxTokensPerHash(void)
{
	int nMax = 0;
	int i;
	int nInUse = 0;
	int nTotal = 0;

	for(i = 0; i < LEXHASHSIZE; ++i)
	{
		int n = 0;
		SLexString* p = g_aLexHash[i];
		if(p)
			++nInUse;
		while(p)
		{
			++nTotal;
			++n;
			p = list_GetNext(p);
		}
		if(n > nMax)
			nMax = n;
	}

	printf("Total strings %d, max %d strings with same hash, %d slots in use\n", nTotal, nMax, nInUse);
}

void lex_AddString(char* pszName, int nToken)
{
	SLexString** pHash = &g_aLexHash[lex_CalcHash(pszName)];
	SLexString* pPrev = *pHash;
	SLexString* pNew;

	/*printf("%s has hashvalue %d\n", lex->tzName, hash);*/

	if((pNew = (SLexString*)malloc(sizeof(SLexString))) != NULL)
	{
		memset(pNew, 0, sizeof(SLexString));
		if((pNew->pszName = (char*)_strdup(pszName)) != NULL)
		{
			pNew->NameLength = (SLONG)strlen(pszName);
			pNew->Token = nToken;

			_strupr(pNew->pszName);

			if(pNew->NameLength > g_nLexTokenMaxLength)
				g_nLexTokenMaxLength = pNew->NameLength;

			if(pPrev)
			{
				list_InsertAfter(pPrev, pNew);
			}
			else
			{
				*pHash = pNew;
			}

		}
		else
		{
			internalerror("Out of memory");
		}
	}
	else
	{
		internalerror("Out of memory");
	}
}

void lex_AddStrings(SLexInitString* lex)
{
    while(lex->pszName)
    {
		lex_AddString(lex->pszName, lex->nToken);
		++lex;
    }

	/*lex_PrintMaxTokensPerHash();*/
}

static ULONG lex_LexStateNormal()
{
	BOOL bLineStart = g_pCurrentBuffer->bAtLineStart;
	SLexString* pLongestFixed = NULL;

	g_pCurrentBuffer->bAtLineStart = FALSE;

	for(;;)
	{
		SLONG nFloatLen;
		ULONG nNewFloatMask;
		ULONG nFloatMask;
		SFloatingChars* pFloat;
		SLONG nMaxLen;
		ULONG nHash;
		SLexFloat* pFloatToken;
		unsigned char* s;

		while(isspace(g_pCurrentBuffer->pBuffer[0]) &&	g_pCurrentBuffer->pBuffer[0] != '\n')
		{
			bLineStart = 0;
			g_pCurrentBuffer->pBuffer += 1;
		}

		if(*(g_pCurrentBuffer->pBuffer) == 0)
		{
			if(fstk_RunNextBuffer())
			{
				bLineStart = g_pCurrentBuffer->bAtLineStart;
				g_pCurrentBuffer->bAtLineStart = FALSE;
				continue;
			}
			else
			{
				g_CurrentToken.ID.Token = 0;
				return 0;
			}
		}

		s = (unsigned char*)g_pCurrentBuffer->pBuffer;
		nFloatMask = nFloatLen = 0;
		pFloat = g_pFloatingChars;
		nNewFloatMask = pFloat->Chars[(int)(*s++)];
		while(nNewFloatMask && nFloatLen < BUF_REMAINING_CHARS)
		{
			if(list_GetNext(pFloat))
			{
				pFloat = list_GetNext(pFloat);
			}

			++nFloatLen;
			nFloatMask = nNewFloatMask;
			nNewFloatMask &= pFloat->Chars[(int)(*s++)];
		}

		if(g_nHasSuffix & nFloatMask)
		{
			nNewFloatMask = nFloatMask & g_aFloatSuffix[(int)s[-1]];
			if(nNewFloatMask)
			{
				++nFloatLen;
				nFloatMask = nNewFloatMask;
			}
			else
			{
				nFloatMask &= ~g_nHasSuffix;
			}
		}

		nMaxLen = (SLONG)BUF_REMAINING_CHARS;
		if(g_nLexTokenMaxLength < nMaxLen)
		{
			nMaxLen = g_nLexTokenMaxLength;
		}

		g_CurrentToken.TokenLength = 0;
		nHash = 0;
		s = (unsigned char*)g_pCurrentBuffer->pBuffer;
		while(g_CurrentToken.TokenLength < nMaxLen)
		{
			++g_CurrentToken.TokenLength;
			HASH(nHash, toupper(*s));
			++s;
			if(g_aLexHash[nHash])
			{
				SLexString* lex = g_aLexHash[nHash];
				while(lex)
				{
					if(lex->NameLength == g_CurrentToken.TokenLength
					&& 0 == _strnicmp(g_pCurrentBuffer->pBuffer, lex->pszName, g_CurrentToken.TokenLength))
					{
						pLongestFixed = lex;
					}
					lex = list_GetNext(lex);
				}
			}

		}

		if(nFloatLen == 0 && pLongestFixed == NULL )
		{
			if(*g_pCurrentBuffer->pBuffer == '"'
			|| *g_pCurrentBuffer->pBuffer == '\'')
			{
				char term[3];
				term[0] = *g_pCurrentBuffer->pBuffer;
				term[1] = '\n';
				term[2] = 0;
				g_pCurrentBuffer->pBuffer = lex_ParseStringUntil(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer+1, term, TRUE);
		 		if(*g_pCurrentBuffer->pBuffer != term[0])
				{
					prj_Fail(ERROR_STRING_TERM);
				}
				else
				{
					g_pCurrentBuffer->pBuffer += 1;
				}
				g_CurrentToken.ID.Token = T_STRING;
				return T_STRING;
			}
			else if(*g_pCurrentBuffer->pBuffer == '{')
			{
				char sym[MAXSYMNAMELENGTH];
				char* pNewBuf;

				pNewBuf = lex_ParseStringUntil(sym, g_pCurrentBuffer->pBuffer, "}\n", FALSE);
				if(pNewBuf)
				{
					g_pCurrentBuffer->pBuffer = pNewBuf;
					g_CurrentToken.ID.Token = T_STRING;
					strcpy(g_CurrentToken.Value.aString, sym);
					return T_STRING;
				}
			}
			if(*g_pCurrentBuffer->pBuffer == '\n')
			{
				g_pCurrentBuffer->bAtLineStart = TRUE;
			}

			g_CurrentToken.TokenLength = 1;
			g_CurrentToken.ID.Token = *(g_pCurrentBuffer->pBuffer);
			return* (g_pCurrentBuffer->pBuffer)++;
		}

		if(nFloatLen == 0)
		{
			g_CurrentToken.TokenLength = pLongestFixed->NameLength;
			g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
			g_CurrentToken.ID.Token = pLongestFixed->Token;
			return pLongestFixed->Token;
		}

		pFloatToken = lex_GetFloat(nFloatMask);
		if(pLongestFixed == NULL || nFloatLen > pLongestFixed->NameLength)
		{
			g_CurrentToken.TokenLength = nFloatLen;
			if(pFloatToken->pCallback)
			{
				if(!pFloatToken->pCallback(g_pCurrentBuffer->pBuffer, g_CurrentToken.TokenLength))
				{
					continue;
				}
			}

			if(pFloatToken->nToken == T_ID && bLineStart)
			{
				g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
				g_CurrentToken.ID.Token=T_LABEL;
				return T_LABEL;
			}
			else
			{
				g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
				g_CurrentToken.ID.Token = pFloatToken->nToken;
				return pFloatToken->nToken;
			}
		}
		else if(pFloatToken && pFloatToken->nToken == T_ID && bLineStart && g_pCurrentBuffer->pBuffer[nFloatLen] == ':')
		{
			g_CurrentToken.TokenLength = nFloatLen;
			if(pFloatToken->pCallback)
			{
				if(!(pFloatToken->pCallback(g_pCurrentBuffer->pBuffer,g_CurrentToken.TokenLength)))
				{
					continue;
				}
			}
			memcpy(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, g_CurrentToken.TokenLength);
			g_CurrentToken.Value.aString[g_CurrentToken.TokenLength] = 0;
			g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
			g_CurrentToken.ID.Token = T_LABEL;
			return T_LABEL;
		}
		else
		{
			g_CurrentToken.TokenLength = pLongestFixed->NameLength;
			memcpy(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, g_CurrentToken.TokenLength);
			g_CurrentToken.Value.aString[g_CurrentToken.TokenLength] = 0;
			g_pCurrentBuffer->pBuffer += g_CurrentToken.TokenLength;
			g_CurrentToken.ID.Token = pLongestFixed->Token;
			return pLongestFixed->Token;
		}
	}
}

ULONG	lex_GetNextToken(void)
{
	switch(g_pCurrentBuffer->State)
	{
		case LEX_STATE_NORMAL:
		{
			return lex_LexStateNormal();
			break;
		}
		case LEX_STATE_MACROARG0:
		{
			g_pCurrentBuffer->State = LEX_STATE_MACROARGS;

			if(g_pCurrentBuffer->pBuffer[0] == '.')
			{
				int i = 0;

				g_pCurrentBuffer->pBuffer += 1;
				while(!isspace(g_pCurrentBuffer->pBuffer[0]))
				{
					g_CurrentToken.Value.aString[i++] = g_pCurrentBuffer->pBuffer[0];
					g_pCurrentBuffer->pBuffer += 1;
				}
				g_CurrentToken.Value.aString[i++] = 0;
				return g_CurrentToken.ID.Token = T_MACROARG0;
			}

			// fall through
		}
		case LEX_STATE_MACROARGS:
		{
			BOOL  linestart = g_pCurrentBuffer->bAtLineStart;
			char* newbuf;
			ULONG index;

			while(isspace(g_pCurrentBuffer->pBuffer[0]) && g_pCurrentBuffer->pBuffer[0] != '\n')
			{
				linestart = 0;
				g_pCurrentBuffer->pBuffer += 1;
			}

			if(g_pCurrentBuffer->pBuffer[0] == '<')
			{
				g_pCurrentBuffer->pBuffer += 1;
				newbuf = lex_ParseStringUntil(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, ">\n", TRUE);
				index = (SLONG)(newbuf - g_pCurrentBuffer->pBuffer);
				if(newbuf[0] == '>')
					newbuf += 1;
			}
			else
			{
				newbuf = lex_ParseStringUntil(g_CurrentToken.Value.aString, g_pCurrentBuffer->pBuffer, ",\n", TRUE);
				index = (SLONG)(newbuf - g_pCurrentBuffer->pBuffer);
			}
			g_pCurrentBuffer->pBuffer = newbuf;

			if(index)
			{
				g_CurrentToken.TokenLength=index;
				if(*(g_pCurrentBuffer->pBuffer)=='\n')
				{
					while(g_CurrentToken.Value.aString[--index]==' ')
					{
						g_CurrentToken.Value.aString[index]=0;
						g_CurrentToken.TokenLength-=1;
					}
				}
				g_CurrentToken.ID.Token=T_STRING;
				return (T_STRING);
			}
			else if(*(g_pCurrentBuffer->pBuffer)=='\n')
			{
				g_pCurrentBuffer->pBuffer+=1;
				g_pCurrentBuffer->bAtLineStart = TRUE;
				g_CurrentToken.TokenLength = 1;
				g_CurrentToken.ID.Token='\n';
				return '\n';
			}
			else if(*(g_pCurrentBuffer->pBuffer)==',')
			{
				g_pCurrentBuffer->pBuffer+=1;
				g_CurrentToken.TokenLength = 1;
				g_CurrentToken.ID.Token=',';
				return ',';
			}
			else
			{
				g_CurrentToken.ID.Token=0;
				return 0;
			}
			break;
		}
	}

	internalerror("Weird error encountered");
	return 0;
}
