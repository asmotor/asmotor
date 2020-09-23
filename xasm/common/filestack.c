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
#include "filestack.h"
#include "includes.h"
#include "lexer.h"
#include "symbol.h"
#include "tokens.h"


/* Internal structures */

#define MAX_INCLUDE_PATHS 16

/* Internal variables */

SFileStackEntry* fstk_Current;

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
setNewContext(SFileStackEntry* context) {
	list_Insert(fstk_Current, context);
}

static SFileStackEntry*
getThisContextMacro(SFileStackEntry* context) {
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

static SFileStackEntry*
getMacroContext(void) {
	return getThisContextMacro(fstk_Current);
}

static SFileInfo*
getMostRecentFileInfo(SFileStackEntry* stackEntry) {
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
getMostRecentLineNumber(SFileStackEntry* stackEntry) {
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
fstk_GetMacroArgumentCount(void) {
	SFileStackEntry* macro = getMacroContext();
	if (macro != NULL) {
		return strvec_Count(macro->lexBuffer->fileBuffer.arguments) - 1;
	}
	return 0;
}

extern void
fstk_AddMacroArgument(string* str) {
	strvec_PushBack(g_newMacroArguments, str);
}

extern void
fstk_SetMacroArgument0(string* str) {
	str_Free(strvec_StringAt(g_newMacroArguments, 0));
	strvec_SetAt(g_newMacroArguments, 0, str_Copy(str));
}

extern void
fstk_ShiftMacroArgs(int32_t count) {
	SFileStackEntry* context = getMacroContext();
	if (context != NULL) {
		fbuf_ShiftArguments(&context->lexBuffer->fileBuffer, count);
	} else {
		err_Warn(WARN_SHIFT_MACRO);
	}
}

extern string*
fstk_Dump(void) {
	string_buffer* buf = strbuf_Create();

	if (fstk_Current == NULL) {
		strbuf_AppendStringZero(buf, "(From commandline) ");
	} else {
		SFileStackEntry* stack = fstk_Current;
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
fstk_EndCurrentBuffer(void) {
	if (list_IsLast(fstk_Current)) {
		return false;
	} else {
		SFileStackEntry* context = fstk_Current;
		if (context->type == CONTEXT_REPT) {
			if (context->block.repeat.remaining > 0) {
				lex_Goto(&context->block.repeat.bookmark);
				context->lineNumber = 0;
				fbuf_RenewUniqueValue(&context->lexBuffer->fileBuffer);
			}
			return true;
		} else {
			list_Remove(fstk_Current, fstk_Current);
			lex_FreeBuffer(context->lexBuffer);
			str_Free(context->name);

			mem_Free(context);
			lex_SetBuffer(fstk_Current->lexBuffer);
		}
		return true;
	}
}

extern void
fstk_ProcessIncludeFile(string* fileName) {
	SFileStackEntry* newContext = mem_Alloc(sizeof(SFileStackEntry));
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
fstk_ProcessRepeatBlock(uint32_t count) {
	SFileStackEntry* newContext = mem_Alloc(sizeof(SFileStackEntry));
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
fstk_ProcessMacro(string* macroName) {
	SSymbol* symbol = sym_GetSymbol(macroName);

	if (symbol != NULL) {
		SFileStackEntry* newContext = mem_Alloc(sizeof(SFileStackEntry));

		memset(newContext, 0, sizeof(SFileStackEntry));
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
fstk_Init(string* fileName) {
	g_newMacroArguments = strvec_Create();
	strvec_PushBack(g_newMacroArguments, NULL);

	g_fileNameMap = strmap_Create(freeFileNameInfo);
	createFileInfo(fileName);
	dep_AddDependency(fileName);

	string* symbolName = str_Create("__FILE");
	sym_CreateEqus(symbolName, fileName);
	str_Free(symbolName);

	fstk_Current = mem_Alloc(sizeof(SFileStackEntry));
	memset(fstk_Current, 0, sizeof(SFileStackEntry));
	fstk_Current->type = CONTEXT_FILE;

	FILE* fileHandle;
	if ((fstk_Current->name = inc_FindFile(fileName)) != NULL
		&& (fileHandle = fopen(str_String(fstk_Current->name), "rt")) != NULL) {

		fstk_Current->fileInfo = getFileInfoFor(fstk_Current->name);
		fstk_Current->lexBuffer = lex_CreateFileBuffer(fileHandle, &fstk_Current->fileInfo->crc32);
		fclose(fileHandle);

		lex_SetBuffer(fstk_Current->lexBuffer);
		lex_SetMode(LEXER_MODE_NORMAL);
		fstk_Current->lineNumber = 1;
		return true;
	}

	err_Fail(ERROR_NO_FILE);
	return false;
}

extern void
fstk_Cleanup(void) {
}

extern SFileInfo*
fstk_CurrentFileInfo() {
	return getMostRecentFileInfo(fstk_Current);
}

extern uint32_t
fstk_CurrentLineNumber() {
	return getMostRecentLineNumber(fstk_Current);
}

extern SFileInfo**
fstk_GetFileInfo(size_t* totalFiles) {
	*totalFiles = map_Count(g_fileNameMap);
	assert (*totalFiles != 0);

	SFileInfo** result = mem_Alloc(*totalFiles * sizeof(SFileInfo*));
	map_ForEachKeyValue(g_fileNameMap, copyFileInfo, (intptr_t) result);
	return result;
}

