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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asmotor.h"
#include "mem.h"
#include "xasm.h"
#include "filestack.h"
#include "project.h"
#include "lexer.h"
#include "tokens.h"
#include "symbol.h"


/* Internal structures */

#define MAX_INCLUDE_PATHS 16

/* Internal variables */

SFileStackEntry* g_currentContext;

static string* g_includePaths[MAX_INCLUDE_PATHS];
static uint8_t g_totalIncludePaths = 0;

static string** g_newMacroArgs;
static string* g_newMacroArg0;
static uint32_t g_totalNewMacroArgs;

#if defined(WIN32)
#	define PATH_SEPARATOR '\\'
#	define PATH_REPLACE '/'
#else
#	define PATH_SEPARATOR '/'
#	define PATH_REPLACE '\\'
#endif

/* Private functions */

static string* createUniqueId(void) {
	static uint32_t runId = 0;

	char s[MAXSYMNAMELENGTH + 1];
	sprintf(s, "_%u", runId++);

	return str_Create(s);
}

static void setNewContext(SFileStackEntry* context) {
	list_Insert(g_currentContext, context);
}

static bool isContextMacro(void) {
	return g_currentContext != NULL && g_currentContext->Type == CONTEXT_MACRO;
}

/* Public routines */

string* fstk_GetMacroArgValue(char ch) {
	string* str = NULL;

	if (isContextMacro()) {
		if (ch == '0') {
			str = g_currentContext->BlockInfo.Macro.Arg0;
		} else {
			uint_fast8_t argumentIndex = (uint8_t) (ch - '1');
			if (argumentIndex >= g_currentContext->BlockInfo.Macro.ArgCount) {
				str = g_currentContext->BlockInfo.Macro.Args[argumentIndex];
			}
		}
	}

	return str_Copy(str);
}

string* fstk_GetMacroUniqueId(void) {
	return str_Copy(g_currentContext->uniqueId);
}

int32_t fstk_GetMacroArgCount(void) {
	return isContextMacro() ? g_currentContext->BlockInfo.Macro.ArgCount : 0;
}

void fstk_AddMacroArg(char* str) {
	g_newMacroArgs = (string**) mem_Realloc(g_newMacroArgs, sizeof(string*) * (g_totalNewMacroArgs + 1));
	g_newMacroArgs[g_totalNewMacroArgs++] = str_Create(str);
}

void fstk_SetMacroArg0(char* str) {
	g_newMacroArg0 = str_Create(str);
}

void fstk_ShiftMacroArgs(int32_t count) {
	if (isContextMacro()) {
		while (count >= 1) {
			str_Free(g_currentContext->BlockInfo.Macro.Args[0]);

			for (uint32_t i = 1; i < g_currentContext->BlockInfo.Macro.ArgCount; ++i) {
				g_currentContext->BlockInfo.Macro.Args[i - 1] = g_currentContext->BlockInfo.Macro.Args[i];
			}
			g_currentContext->BlockInfo.Macro.ArgCount -= 1;
			g_currentContext->BlockInfo.Macro.Args =
					mem_Realloc(g_currentContext->BlockInfo.Macro.Args, g_currentContext->BlockInfo.Macro.ArgCount * sizeof(string*));
			count -= 1;
		}
	} else {
		prj_Warn(WARN_SHIFT_MACRO);
	}
}

static string* fstk_FixSeparators(string* pFile) {
	return str_Replace(pFile, PATH_REPLACE, PATH_SEPARATOR);
}

static string* fstk_BuildPath(string* pPath, string* pFile) {
	const char* pSlash = str_String(pPath) + str_Length(pPath) - 1;
	string* p;
	string* r;

	while (pSlash > str_String(pPath) && *pSlash != '/' && *pSlash != '\\')
		--pSlash;

	if (pSlash == str_String(pPath))
		return str_Copy(pFile);

	p = str_Slice(pPath, 0, pSlash + 1 - str_String(pPath));
	r = str_Concat(p, pFile);
	str_Free(p);

	p = fstk_FixSeparators(r);
	str_Free(r);
	return p;
}

static bool fstk_FileExists(string* pFile) {
	FILE* f;

	if ((f = fopen(str_String(pFile), "rb")) != NULL) {
		fclose(f);
		return true;
	}

	return false;
}

string* fstk_FindFile(string* pFile) {
	string* pResult;
	uint32_t count;

	pFile = fstk_FixSeparators(pFile);

	if (g_currentContext->pName == NULL) {
		if (fstk_FileExists(pFile))
			return pFile;
	} else {
		pResult = fstk_BuildPath(g_currentContext->pName, pFile);
		if (pResult != NULL) {
			if (fstk_FileExists(pResult)) {
				str_Free(pFile);
				return pResult;
			}

			str_Free(pResult);
		}
	}

	for (count = 0; count < g_totalIncludePaths; count += 1) {
		pResult = str_Concat(g_includePaths[count], pFile);

		if (fstk_FileExists(pResult)) {
			str_Free(pFile);
			return pResult;
		}

		str_Free(pResult);
	}

	str_Free(pFile);

	return NULL;
}

void fstk_Dump(void) {
	SFileStackEntry* stack;

	if (g_currentContext == NULL) {
		printf("(From commandline) ");
		return;
	}

	stack = g_currentContext;
	while (list_GetNext(stack)) {
		stack = list_GetNext(stack);
	}

	while (stack) {
		if (stack != NULL && stack->pName != NULL) {
			printf("%s(%d)", str_String(stack->pName), stack->LineNumber);
		}
		stack = list_GetPrev(stack);

		printf(stack != NULL ? "->" : ": ");
	}
}

bool fstk_RunNextBuffer(void) {
	if (list_isLast(g_currentContext)) {
		return false;
	} else {
		SFileStackEntry* newcontext;
		SFileStackEntry* oldcontext;

		oldcontext = g_currentContext;
		newcontext = list_GetNext(g_currentContext);

		if (newcontext->Type == CONTEXT_MACRO)
			g_currentContext = newcontext;
		else
			g_currentContext = NULL;

		list_Remove(g_currentContext, g_currentContext);
		lex_FreeBuffer(oldcontext->pLexBuffer);
		str_Free(oldcontext->pName);

		switch (oldcontext->Type) {
			case CONTEXT_REPT: {
				if (oldcontext->BlockInfo.Rept.RemainingRuns > 0) {
					fstk_RunRept(oldcontext->BlockInfo.Rept.pOriginalBuffer, oldcontext->BlockInfo.Rept.OriginalSize,
								 oldcontext->BlockInfo.Rept.RemainingRuns);
				} else {
					mem_Free(oldcontext->BlockInfo.Rept.pOriginalBuffer);
					lex_SetBuffer(g_currentContext->pLexBuffer);
				}
				str_Free(oldcontext->uniqueId);
				mem_Free(oldcontext);
				break;
			}
			case CONTEXT_MACRO: {
				uint32_t i;

				for (i = 0; i < oldcontext->BlockInfo.Macro.ArgCount; i += 1) {
					mem_Free(oldcontext->BlockInfo.Macro.Args[i]);
				}
				str_Free(oldcontext->uniqueId);
				mem_Free(oldcontext->BlockInfo.Macro.Args);
				mem_Free(oldcontext);
				lex_SetBuffer(g_currentContext->pLexBuffer);
				break;
			}
			default: {
				mem_Free(oldcontext);
				lex_SetBuffer(g_currentContext->pLexBuffer);
				break;
			}
		}
		return true;
	}
}

void fstk_AddIncludePath(string* pFile) {
	char ch;

	if (g_totalIncludePaths == MAX_INCLUDE_PATHS) {
		prj_Fail(ERROR_INCLUDE_LIMIT);
		return;
	}

	ch = str_CharAt(pFile, str_Length(pFile) - 1);
	if (ch != '\\' && ch != '/') {
		string* pSlash = str_Create("/");
		g_includePaths[g_totalIncludePaths++] = str_Concat(pFile, pSlash);
		str_Free(pSlash);
	} else {
		g_includePaths[g_totalIncludePaths++] = str_Copy(pFile);
	}
}

void fstk_RunInclude(string* pFile) {
	FILE* f;
	SFileStackEntry* newcontext = mem_Alloc(sizeof(SFileStackEntry));

	memset(newcontext, 0, sizeof(SFileStackEntry));
	newcontext->Type = CONTEXT_FILE;

	newcontext->pName = fstk_FindFile(pFile);

	if (newcontext->pName != NULL && (f = fopen(str_String(newcontext->pName), "rt")) != NULL) {
		if ((newcontext->pLexBuffer = lex_CreateFileBuffer(f)) != NULL) {
			lex_SetBuffer(newcontext->pLexBuffer);
			lex_SetState(LEX_STATE_NORMAL);
			newcontext->LineNumber = 0;
			setNewContext(newcontext);
			return;
		}
		fclose(f);
	} else {
		prj_Fail(ERROR_NO_FILE);
	}
}

void fstk_RunRept(char* buffer, size_t size, uint32_t count) {
	SFileStackEntry* newcontext = mem_Alloc(sizeof(SFileStackEntry));

	memset(newcontext, 0, sizeof(SFileStackEntry));
	newcontext->Type = CONTEXT_REPT;

	newcontext->pName = str_Create("REPT");

	if ((newcontext->pLexBuffer = lex_CreateMemoryBuffer(buffer, size)) != NULL) {
		lex_SetBuffer(newcontext->pLexBuffer);
		lex_SetState(LEX_STATE_NORMAL);
		newcontext->LineNumber = 0;
		newcontext->BlockInfo.Rept.pOriginalBuffer = buffer;
		newcontext->BlockInfo.Rept.OriginalSize = size;
		newcontext->BlockInfo.Rept.RemainingRuns = count - 1;
		newcontext->uniqueId = createUniqueId();
		setNewContext(newcontext);
	}
}

void fstk_RunMacro(string* pName) {
	SSymbol* sym;

	if ((sym = sym_FindSymbol(pName)) != NULL) {
		SFileStackEntry* newcontext = mem_Alloc(sizeof(SFileStackEntry));

		memset(newcontext, 0, sizeof(SFileStackEntry));
		newcontext->Type = CONTEXT_MACRO;

		newcontext->pName = str_Copy(pName);
		newcontext->pLexBuffer = lex_CreateMemoryBuffer(str_String(sym->Value.pMacro), str_Length(sym->Value.pMacro));

		lex_SetBuffer(newcontext->pLexBuffer);
		lex_SetState(LEX_STATE_NORMAL);
		newcontext->LineNumber = -1;
		newcontext->BlockInfo.Macro.Arg0 = g_newMacroArg0;
		newcontext->BlockInfo.Macro.Args = g_newMacroArgs;
		newcontext->BlockInfo.Macro.ArgCount = g_totalNewMacroArgs;
		newcontext->uniqueId = createUniqueId();
		g_totalNewMacroArgs = 0;
		g_newMacroArgs = NULL;
		g_newMacroArg0 = NULL;
		setNewContext(newcontext);
	} else {
		prj_Fail(ERROR_NO_MACRO);
	}
}

bool fstk_Init(string* pFile) {
	FILE* f;
	string* pName;

	g_totalNewMacroArgs = 0;
	g_newMacroArgs = NULL;

	pName = str_Create("__FILE");
	sym_CreateEQUS(pName, pFile);
	str_Free(pName);

	g_currentContext = mem_Alloc(sizeof(SFileStackEntry));
	memset(g_currentContext, 0, sizeof(SFileStackEntry));
	g_currentContext->Type = CONTEXT_FILE;

	if ((g_currentContext->pName = fstk_FindFile(pFile)) != NULL
		&& (f = fopen(str_String(g_currentContext->pName), "rt")) != NULL) {
		g_currentContext->pLexBuffer = lex_CreateFileBuffer(f);
		fclose(f);
		lex_SetBuffer(g_currentContext->pLexBuffer);
		lex_SetState(LEX_STATE_NORMAL);
		g_currentContext->LineNumber = 1;
		return true;
	}

	prj_Fail(ERROR_NO_FILE);
	return false;
}

void fstk_Cleanup(void) {
}
