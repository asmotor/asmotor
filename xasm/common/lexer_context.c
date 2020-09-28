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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// From util
#include "file.h"
#include "mem.h"
#include "strbuf.h"
#include "strcoll.h"

// From xasm
#include "xasm.h"
#include "dependency.h"
#include "errors.h"
#include "includes.h"
#include "filebuffer.h"
#include "lexer.h"
#include "lexer_context.h"
#include "symbol.h"
#include "tokens.h"


/* Internal structures */

#define MAX_INCLUDE_PATHS 16

/* Internal variables */

SLexerContext* lex_Context;

static vec_t* g_newMacroArguments;

static map_t* g_fileNameMap = NULL;


/* Private functions */

static SFileInfo*
getFileInfoFor(const string* fileName) {
	intptr_t value;
	if (strmap_Value(g_fileNameMap, fileName, &value)) {
		return (SFileInfo*) value;
	} else {
		return NULL;
	}
}

static void
freeFileNameInfo(intptr_t userData, intptr_t element) {
	mem_Free((void*) element);
}

static SFileInfo*
createFileInfo(string* fileName) {
	static uint32_t nextFileId = 0;

	intptr_t value;
	if (strmap_Value(g_fileNameMap, fileName, &value)) {
		return (SFileInfo*) value;
	} else {
		SFileInfo* entry = mem_Alloc(sizeof(SFileInfo));
		entry->fileName = str_Copy(fileName);
		entry->fileId = nextFileId++;
		entry->crc32 = 0;
		strmap_Insert(g_fileNameMap, fileName, (intptr_t) entry);

		return entry;
	}
}

static void
setNewContext(SLexerContext* context) {
	list_Insert(lex_Context, context);
}

static SLexerContext*
getThisContextMacro(SLexerContext* context) {
	if (context == NULL) {
		return NULL;
	} else if (context->type == CONTEXT_MACRO) {
		return context;
	} else if (context->type == CONTEXT_REPT) {
		return getThisContextMacro(context->pNext);
	} else {
		return NULL;
	}
}

static SLexerContext*
getMacroContext(void) {
	return getThisContextMacro(lex_Context);
}

static SFileInfo*
getMostRecentFileInfo(SLexerContext* stackEntry) {
	if (stackEntry == NULL)
		return NULL;

	switch (stackEntry->type) {
		case CONTEXT_FILE:
		case CONTEXT_MACRO:
			return stackEntry->fileInfo;
		case CONTEXT_REPT:
			return getMostRecentFileInfo(stackEntry->pNext);
		default:
			return 0;
	}
}

static uint32_t
getMostRecentLineNumber(SLexerContext* stackEntry) {
	if (stackEntry == NULL)
		return 0;

	switch (stackEntry->type) {
		case CONTEXT_FILE:
		case CONTEXT_MACRO:
			return stackEntry->lineNumber;
		case CONTEXT_REPT:
			return stackEntry->lineNumber + getMostRecentLineNumber(stackEntry->pNext);
		default:
			return 0;
	}
}

static void
copyFileInfo(intptr_t key, intptr_t value, intptr_t data) {
	SFileInfo* fileInfo = (SFileInfo*) value;
	SFileInfo** array = (SFileInfo**) data;
	array[fileInfo->fileId] = fileInfo;
}


/* Public functions */

extern size_t
lex_GetMacroArgumentCount(void) {
	SLexerContext* macro = getMacroContext();
	if (macro != NULL) {
		return strvec_Count(macro->lexBuffer->fileBuffer.arguments) - 1;
	}
	return 0;
}

extern void
lex_AddMacroArgument(string* str) {
	strvec_PushBack(g_newMacroArguments, str);
}

extern void
lex_SetMacroArgument0(string* str) {
	str_Free(strvec_StringAt(g_newMacroArguments, 0));
	strvec_SetAt(g_newMacroArguments, 0, str_Copy(str));
}

extern void
lex_ShiftMacroArgs(int32_t count) {
	SLexerContext* context = getMacroContext();
	if (context != NULL) {
		fbuf_ShiftArguments(&context->lexBuffer->fileBuffer, count);
	} else {
		err_Warn(WARN_SHIFT_MACRO);
	}
}

extern string*
lex_Dump(void) {
	string_buffer* buf = strbuf_Create();

	if (lex_Context == NULL) {
		strbuf_AppendStringZero(buf, "(From commandline) ");
	} else {
		SLexerContext* stack = lex_Context;
		while (list_GetNext(stack)) {
			stack = list_GetNext(stack);
		}

		while (stack != NULL) {
			if (stack->name != NULL) {
				strbuf_AppendFormat(buf, "%s:%d", str_String(stack->name), stack->lineNumber);
			}
			stack = list_GetPrev(stack);

			strbuf_AppendStringZero(buf, stack != NULL ? " -> " : ": ");
		}
	}

	string* str = strbuf_String(buf);
	strbuf_Free(buf);

	return str;
}

extern bool
lex_EndCurrentBuffer(void) {
	if (list_IsLast(lex_Context)) {
		return false;
	} else {
		SLexerContext* context = lex_Context;
		if (context->type == CONTEXT_REPT) {
			if (context->block.repeat.remaining-- > 0) {
				lex_Goto(&context->block.repeat.bookmark);
				context->lineNumber = 0;
				fbuf_RenewUniqueValue(&context->lexBuffer->fileBuffer);
				return true;
			} else {
				list_Remove(lex_Context, lex_Context);
				lex_CopyBuffer(lex_Context->lexBuffer, context->lexBuffer);
			}
		} else {
			list_Remove(lex_Context, lex_Context);
		}

		lex_FreeBuffer(context->lexBuffer);
		str_Free(context->name);

		mem_Free(context);
		lex_SetBuffer(lex_Context->lexBuffer);
		return true;
	}
}

extern void
lex_ProcessIncludeFile(string* fileName) {
	SLexerContext* newContext = mem_Alloc(sizeof(SLexerContext));
	list_Init(newContext);

	newContext->type = CONTEXT_FILE;
	newContext->name = inc_FindFile(fileName);

	FILE* fileHandle;
	if (newContext->name != NULL && (fileHandle = fopen(str_String(newContext->name), "rt")) != NULL) {
		newContext->fileInfo = createFileInfo(newContext->name);
		dep_AddDependency(newContext->name);
		if ((newContext->lexBuffer = lex_CreateFileBuffer(fileHandle, &newContext->fileInfo->crc32)) != NULL) {
			lex_SetBuffer(newContext->lexBuffer);
			lex_SetMode(LEXER_MODE_NORMAL);
			newContext->lineNumber = 0;
			setNewContext(newContext);
		}
		fclose(fileHandle);
	} else {
		err_Fail(ERROR_NO_FILE);
	}
}

extern void
lex_ProcessRepeatBlock(uint32_t count) {
	SLexerContext* newContext = mem_Alloc(sizeof(SLexerContext));
	list_Init(newContext);

	newContext->name = str_Create("REPT");
	newContext->fileInfo = NULL;
	newContext->type = CONTEXT_REPT;
	newContext->lineNumber = 0;
	newContext->block.repeat.remaining = count - 1;
	lex_Bookmark(&newContext->block.repeat.bookmark);

	if ((newContext->lexBuffer = lex_CreateBookmarkBuffer(&newContext->block.repeat.bookmark)) != NULL) {
		lex_SetBuffer(newContext->lexBuffer);
		lex_SetMode(LEXER_MODE_NORMAL);
		setNewContext(newContext);
	}
}

extern void
lex_ProcessMacro(string* macroName) {
	SSymbol* symbol = sym_GetSymbol(macroName);

	if (symbol != NULL) {
		SLexerContext* newContext = mem_Alloc(sizeof(SLexerContext));

		memset(newContext, 0, sizeof(SLexerContext));
		newContext->type = CONTEXT_MACRO;

		newContext->name = str_Copy(macroName);
		newContext->fileInfo = symbol->fileInfo;
		newContext->lexBuffer = lex_CreateMemoryBuffer(symbol->value.macro, g_newMacroArguments);

		g_newMacroArguments = strvec_Create();
		strvec_PushBack(g_newMacroArguments, NULL);

		lex_SetBuffer(newContext->lexBuffer);
		lex_SetMode(LEXER_MODE_NORMAL);
		newContext->lineNumber = 1;
		newContext->block.macro.symbol = symbol;

		setNewContext(newContext);
	} else {
		err_Error(ERROR_NO_MACRO);
	}
}

extern bool
lex_ContextInit(string* fileName) {
	g_newMacroArguments = strvec_Create();
	strvec_PushBack(g_newMacroArguments, NULL);

	g_fileNameMap = strmap_Create(freeFileNameInfo);
	createFileInfo(fileName);
	dep_AddDependency(fileName);

	string* symbolName = str_Create("__FILE");
	sym_CreateEqus(symbolName, fileName);
	str_Free(symbolName);

	lex_Context = mem_Alloc(sizeof(SLexerContext));
	memset(lex_Context, 0, sizeof(SLexerContext));
	lex_Context->type = CONTEXT_FILE;

	FILE* fileHandle;
	if ((lex_Context->name = inc_FindFile(fileName)) != NULL
		&& (fileHandle = fopen(str_String(lex_Context->name), "rt")) != NULL) {

		lex_Context->fileInfo = getFileInfoFor(lex_Context->name);
		lex_Context->lexBuffer = lex_CreateFileBuffer(fileHandle, &lex_Context->fileInfo->crc32);
		fclose(fileHandle);

		lex_SetBuffer(lex_Context->lexBuffer);
		lex_SetMode(LEXER_MODE_NORMAL);
		lex_Context->lineNumber = 1;
		return true;
	}

	err_Fail(ERROR_NO_FILE);
	return false;
}

extern void
lex_Cleanup(void) {
}

extern SFileInfo*
lex_CurrentFileInfo() {
	return getMostRecentFileInfo(lex_Context);
}

extern uint32_t
lex_CurrentLineNumber() {
	return getMostRecentLineNumber(lex_Context);
}

extern SFileInfo**
lex_GetFileInfo(size_t* totalFiles) {
	*totalFiles = map_Count(g_fileNameMap);
	assert (*totalFiles != 0);

	SFileInfo** result = mem_Alloc(*totalFiles * sizeof(SFileInfo*));
	map_ForEachKeyValue(g_fileNameMap, copyFileInfo, (intptr_t) result);
	return result;
}

