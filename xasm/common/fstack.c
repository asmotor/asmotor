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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asmotor.h"
#include "mem.h"
#include "xasm.h"
#include "fstack.h"
#include "project.h"
#include "lexer.h"
#include "tokens.h"
#include "symbol.h"


/* Internal structures */

#define	MAXINCLUDEPATHS	16




/* Internal variables */

SFileStack*	g_pFileContext;

static string* g_aIncludePaths[MAXINCLUDEPATHS];
static uint32_t g_nTotalIncludePaths = 0;
static SFileStack* g_pMacroContext;
static char* g_pszCurrentRunId;

static char** g_ppszNewMacroArgs;
static char* g_pszNewMacroArg0;
static uint32_t g_nTotalNewMacroArgs;




/* Private routines */

static char* fstk_CreateNewRunID(void)
{
	char s[MAXSYMNAMELENGTH + 1];
	char* r;
	static uint32_t runid = 0;

	sprintf(s, "_%u", runid++);
	r = mem_Alloc(strlen(s) + 1);
	strcpy(r, s);
	return r;
}

static void fstk_SetNewContext(SFileStack* newcontext)
{
	list_Insert(g_pFileContext, newcontext);
	g_pFileContext = newcontext;
	g_pszCurrentRunId = g_pFileContext->RunID;
}

/* Public routines */

char* fstk_GetMacroArgValue(char ch)
{
	uint32_t ct;

	if(ch == '0')
		return g_pMacroContext->BlockInfo.Macro.Arg0;

	ct = ch - '1';
	if(g_pMacroContext == NULL
	|| g_pMacroContext->Type != CONTEXT_MACRO
	|| ct >= g_pMacroContext->BlockInfo.Macro.ArgCount)
	{
		return NULL;
	}

	return g_pMacroContext->BlockInfo.Macro.Args[ct];
}

char* fstk_GetMacroRunID(void)
{
	return g_pszCurrentRunId;
}

int32_t fstk_GetMacroArgCount(void)
{
	if(g_pMacroContext == NULL
	|| g_pMacroContext->Type != CONTEXT_MACRO)
	{
		return 0;
	}

	return g_pMacroContext->BlockInfo.Macro.ArgCount;
}

void fstk_AddMacroArg(char* s)
{
	char* r = mem_Alloc(strlen(s) + 1);
	strcpy(r, s);

	g_ppszNewMacroArgs = (char**)mem_Realloc(g_ppszNewMacroArgs, sizeof(char*) * (g_nTotalNewMacroArgs+1));
	g_ppszNewMacroArgs[g_nTotalNewMacroArgs++] = r;
}

void fstk_SetMacroArg0(char* s)
{
	g_pszNewMacroArg0 = mem_Alloc(strlen(s) + 1);
	strcpy(g_pszNewMacroArg0, s);
}

void fstk_ShiftMacroArgs(int32_t count)
{
	if(g_pMacroContext)
	{
		while(count >= 1)
		{
			uint32_t i;

			mem_Free(g_pMacroContext->BlockInfo.Macro.Args[0]);
			i = 1;
			while(i < g_pMacroContext->BlockInfo.Macro.ArgCount)
			{
				g_pMacroContext->BlockInfo.Macro.Args[i - 1] = g_pMacroContext->BlockInfo.Macro.Args[i];
				i += 1;
			}
			g_pMacroContext->BlockInfo.Macro.ArgCount -= 1;
			g_pMacroContext->BlockInfo.Macro.Args = mem_Realloc(g_pMacroContext->BlockInfo.Macro.Args, g_pMacroContext->BlockInfo.Macro.ArgCount * sizeof(char*));
			count -= 1;
		}
	}
	else
	{
		prj_Warn(WARN_SHIFT_MACRO);
	}
}

static string* fstk_BuildPath(string* pPath, string* pFile)
{
	char* pSlash = str_String(pPath) + str_Length(pPath) - 1;
	string* p;
	string* r;

	while(pSlash > str_String(pPath) && *pSlash != '/' && *pSlash != '\\')
		--pSlash;

	if(pSlash == str_String(pPath))
		return NULL;

	p = str_Slice(pPath, 0, pSlash + 1 - str_String(pPath));
	r = str_Concat(p, pFile);
	str_Free(p);

	return r;
}

static bool_t fstk_FileExists(string* pFile)
{
	FILE* f;
	if((f = fopen(str_String(pFile), "rb")) != NULL)
	{
		fclose(f);
		return true;
	}

	return false;
}

string* fstk_FindFile(string* pFile)
{
	string* pResult;
	uint32_t count;

	if(fstk_FileExists(pFile))
		return str_Copy(pFile);

	pResult = fstk_BuildPath(g_pFileContext->pName, pFile);
	if(pResult != NULL)
	{
		if(fstk_FileExists(pResult))
			return pResult;

		str_Free(pResult);
	}

	for(count = 0; count < g_nTotalIncludePaths; count += 1)
	{
		FILE* f;
		pResult = str_Concat(g_aIncludePaths[count], pFile);
		
		if((f = fopen(str_String(pResult), "rt")) != NULL)
		{
			fclose(f);
			return pResult;
		}
		str_Free(pResult);
	}
	
	return NULL;
}

void fstk_Dump(void)
{
	SFileStack* stack;

	if(g_pFileContext == NULL)
	{
		printf("(From commandline) ");
		return;
	}

	stack = g_pFileContext;
	while(list_GetNext(stack))
	{
		stack = list_GetNext(stack);
	}

	while(stack)
	{
		printf("%s(%d)", str_String(stack->pName), stack->LineNumber);
		stack = list_GetPrev(stack);
		if(stack)
		{
			printf("->");
		}
		else
		{
			printf(": ");
		}
	}
}

bool_t fstk_RunNextBuffer(void)
{
	if(list_isLast(g_pFileContext))
	{
		return false;
	}
	else
	{
		SFileStack* newcontext;
		SFileStack* oldcontext;

		oldcontext = g_pFileContext;
		newcontext = list_GetNext(g_pFileContext);

		if(newcontext->Type == CONTEXT_MACRO)
			g_pMacroContext = newcontext;
		else
			g_pMacroContext = NULL;

		if(newcontext->Type == CONTEXT_MACRO
		|| newcontext->Type == CONTEXT_REPT)
		{
			g_pszCurrentRunId = newcontext->RunID;
		}

		list_Remove(g_pFileContext, g_pFileContext);
		lex_FreeBuffer(oldcontext->pLexBuffer);
		mem_Free(oldcontext->pName);

		switch(oldcontext->Type)
		{
			case CONTEXT_REPT:
			{
				if(oldcontext->BlockInfo.Rept.RemainingRuns > 0)
				{
					fstk_RunRept(oldcontext->BlockInfo.Rept.pOriginalBuffer, oldcontext->BlockInfo.Rept.OriginalSize, oldcontext->BlockInfo.Rept.RemainingRuns);
				}
				else
				{
					mem_Free(oldcontext->BlockInfo.Rept.pOriginalBuffer);
					lex_SetBuffer(g_pFileContext->pLexBuffer);
				}
				mem_Free(oldcontext->RunID);
				mem_Free(oldcontext);
				break;
			}
			case CONTEXT_MACRO:
			{
				uint32_t i;

				for(i = 0; i < oldcontext->BlockInfo.Macro.ArgCount; i += 1)
				{
					mem_Free(oldcontext->BlockInfo.Macro.Args[i]);
				}
				mem_Free(oldcontext->RunID);
				mem_Free(oldcontext->BlockInfo.Macro.Args);
				mem_Free(oldcontext);
				lex_SetBuffer(g_pFileContext->pLexBuffer);
				break;
			}
			default:
			{
				mem_Free(oldcontext);
				lex_SetBuffer(g_pFileContext->pLexBuffer);
				break;
			}
		}
		return true;
	}
}

void fstk_AddIncludePath(string* pFile)
{
	char ch;
	
	if(g_nTotalIncludePaths == MAXINCLUDEPATHS)
	{
		prj_Fail(ERROR_INCLUDE_LIMIT);
		return;
	}

	ch = str_CharAt(pFile, str_Length(pFile) - 1);
	if(ch != '\\' && ch != '/')
	{
		string* pSlash = str_Create("/");
		g_aIncludePaths[g_nTotalIncludePaths++] = str_Concat(pFile, pSlash);
		str_Free(pSlash);
	}
	else
	{
		g_aIncludePaths[g_nTotalIncludePaths++] = str_Copy(pFile);
	}
}

void fstk_RunInclude(string* pFile)
{
	FILE* f;
	SFileStack* newcontext = mem_Alloc(sizeof(SFileStack));

	memset(newcontext, 0, sizeof(SFileStack));
	newcontext->Type = CONTEXT_FILE;

	newcontext->pName = fstk_FindFile(pFile);

    if(newcontext->pName != NULL
	&&(f = fopen(str_String(newcontext->pName), "rt")) != NULL)
    {
		if((newcontext->pLexBuffer = lex_CreateFileBuffer(f)) != NULL)
		{
			lex_SetBuffer(newcontext->pLexBuffer);
			lex_SetState(LEX_STATE_NORMAL);
			newcontext->LineNumber = 0;
			fstk_SetNewContext(newcontext);
			return;
		}
		fclose(f);
    }
	else
	{
		prj_Fail(ERROR_NO_FILE);
	}
}

void fstk_RunRept(char* buffer, uint32_t size, uint32_t count)
{
	SFileStack* newcontext = mem_Alloc(sizeof(SFileStack));

	memset(newcontext, 0, sizeof(SFileStack));
	newcontext->Type = CONTEXT_REPT;

	newcontext->pName = str_Create("REPT");

	if((newcontext->pLexBuffer = lex_CreateMemoryBuffer(buffer,size)) != NULL)
	{
		lex_SetBuffer(newcontext->pLexBuffer);
		lex_SetState(LEX_STATE_NORMAL);
		newcontext->LineNumber = 0;
		newcontext->BlockInfo.Rept.pOriginalBuffer = buffer;
		newcontext->BlockInfo.Rept.OriginalSize = size;
		newcontext->BlockInfo.Rept.RemainingRuns = count-1;
		newcontext->RunID = fstk_CreateNewRunID();
		fstk_SetNewContext(newcontext);
	}
}

void fstk_RunMacro(string* pName)
{
	SSymbol	*sym;

	if((sym = sym_FindSymbol(pName)) != NULL)
	{
		SFileStack* newcontext = mem_Alloc(sizeof(SFileStack));

		memset(newcontext, 0, sizeof(SFileStack));
		newcontext->Type = CONTEXT_MACRO;

		newcontext->pName = str_Copy(pName);
		newcontext->pLexBuffer = lex_CreateMemoryBuffer(str_String(sym->Value.pMacro), str_Length(sym->Value.pMacro));

		lex_SetBuffer(newcontext->pLexBuffer);
		lex_SetState(LEX_STATE_NORMAL);
		newcontext->LineNumber = -1;
		newcontext->BlockInfo.Macro.Arg0 = g_pszNewMacroArg0;
		newcontext->BlockInfo.Macro.Args = g_ppszNewMacroArgs;
		newcontext->BlockInfo.Macro.ArgCount = g_nTotalNewMacroArgs;
		newcontext->RunID = fstk_CreateNewRunID();
		g_nTotalNewMacroArgs = 0;
		g_ppszNewMacroArgs = NULL;
		g_pszNewMacroArg0 = NULL;
		g_pMacroContext = newcontext;
		fstk_SetNewContext(newcontext);
	}
	else
	{
		prj_Fail(ERROR_NO_MACRO);
	}
}

bool_t fstk_Init(string* pFile)
{
	FILE* f;
	string* pName;

	g_nTotalNewMacroArgs = 0;
	g_ppszNewMacroArgs = NULL;
	g_pMacroContext = NULL;
	g_pszCurrentRunId = NULL;

	pName = str_Create("__FILE");
    sym_CreateEQUS(pName, pFile);
	str_Free(pName);

	g_pFileContext = mem_Alloc(sizeof(SFileStack));
	memset(g_pFileContext, 0, sizeof(SFileStack));
	g_pFileContext->Type = CONTEXT_FILE;

    if((g_pFileContext->pName = fstk_FindFile(pFile)) != NULL
	&& (f = fopen(str_String(g_pFileContext->pName), "rt")) != NULL)
    {
		g_pFileContext->pLexBuffer = lex_CreateFileBuffer(f);
		fclose(f);
		lex_SetBuffer(g_pFileContext->pLexBuffer);
		lex_SetState(LEX_STATE_NORMAL);
		g_pFileContext->LineNumber = 1;
		return true;
    }

	prj_Fail(ERROR_NO_FILE);
	return false;
}

void fstk_Cleanup(void)
{
}
