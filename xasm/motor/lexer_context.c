/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// From util
#include "file.h"
#include "lists.h"
#include "map.h"
#include "mem.h"
#include "str.h"
#include "strbuf.h"
#include "strcoll.h"
#include "util.h"
#include "vec.h"

// From xasm
#include "crc32.h"
#include "dependency.h"
#include "errors.h"
#include "includes.h"
#include "lexer_buffer.h"
#include "lexer_context.h"
#include "options.h"
#include "symbol.h"


/* Internal variables */

SLexerContext* lex_Context;

static vec_t* g_newMacroArguments;

static map_t* g_fileNameMap = NULL;


/* Private functions */

static SLexerContext*
createContext(void) {
	SLexerContext* ctx = (SLexerContext*) mem_Alloc(sizeof(SLexerContext));
	list_Init(ctx);
	return ctx;
}

static void
freeFileNameInfo(intptr_t userData, intptr_t element) {
	SFileInfo* fileInfo = (SFileInfo*) element;
	str_Free(fileInfo->fileName);
	mem_Free(fileInfo);
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
pushContext(SLexerContext* context) {
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
copyFileInfo(map_t* map, intptr_t key, intptr_t value, intptr_t data) {
	SFileInfo* fileInfo = (SFileInfo*) value;
	SFileInfo** array = (SFileInfo**) data;
	array[fileInfo->fileId] = fileInfo;
}


/* Public functions */

extern size_t
lexctx_GetMacroArgumentCount(void) {
	SLexerContext* macro = getMacroContext();
	if (macro != NULL) {
		return strvec_Count(macro->buffer.arguments) - 1;
	}
	return 0;
}

extern void
lexctx_AddMacroArgument(string* str) {
	strvec_PushBack(g_newMacroArguments, str);
}

extern void
lexctx_SetMacroArgument0(string* str) {
	str_Free(strvec_StringAt(g_newMacroArguments, 0));
	strvec_SetAt(g_newMacroArguments, 0, str);
}

extern void
lexctx_ShiftMacroArgs(int32_t count) {
	SLexerContext* context = getMacroContext();
	if (context != NULL) {
		lexbuf_ShiftArguments(&context->buffer, count);
	} else {
		err_Warn(WARN_SHIFT_MACRO);
	}
}

extern string*
lexctx_GetFilenameBreadcrumb(void) {
	string_buffer* buf = strbuf_Create();

	if (lex_Context == NULL) {
		strbuf_AppendStringZero(buf, "(From commandline) ");
	} else {
		SLexerContext* stack = lex_Context;
		while (list_GetNext(stack)) {
			stack = list_GetNext(stack);
		}

		while (stack != NULL) {
			if (stack->buffer.name != NULL) {
				strbuf_AppendFormat(buf, "%s:%d", str_String(stack->buffer.name), stack->lineNumber);
			}
			stack = list_GetPrev(stack);

			strbuf_AppendStringZero(buf, stack != NULL ? " -> " : ": ");
		}
	}

	string* str = strbuf_String(buf);
	strbuf_Free(buf);

	return str;
}


extern uint32_t
lexctx_GetMainFileLineNumber(void) {
	if (lex_Context == NULL)
		return 0;

	SLexerContext* stack = lex_Context;
	while (list_GetNext(stack)) {
		stack = list_GetNext(stack);
	}

	return stack->lineNumber;
}


static void
copyToken(SLexerToken* dest, const SLexerToken* src) {
	dest->id = src->id;
	dest->length = src->length;
	memcpy(&dest->value, &src->value, dest->length < 10 ? 10 : dest->length);
}

static void
continueFrom(SLexerContext* ctx) {
	copyToken(&lex_Context->token, &ctx->token);
	lex_Context->mode = ctx->mode;
	lex_Context->atLineStart = ctx->atLineStart;
	lex_Context->fileInfo = ctx->fileInfo;
	lex_Context->lineNumber = ctx->lineNumber;
	lexbuf_ContinueFrom(&lex_Context->buffer, &ctx->buffer);
}

extern bool
lexctx_EndReptBlock(void) {
	assert (lex_Context->type == CONTEXT_REPT);

	if (list_IsLast(lex_Context)) {
		return false;
	} else if (lex_Context->block.repeat.remaining-- > 0) {
		continueFrom(lex_Context->block.repeat.bookmark);
		lexbuf_RenewUniqueValue(&lex_Context->buffer);
		return false;
	} else {
		SLexerContext* context = lex_Context;
		list_Remove(lex_Context, lex_Context);
		continueFrom(context);
		lexctx_FreeContext(context);
		return true;
	}
}

extern bool
lexctx_EndCurrentBuffer(void) {
	if (lex_Context->type == CONTEXT_REPT) {
		err_Fail(ERROR_NEED_ENDR);
	}
	assert (lex_Context->type != CONTEXT_REPT);

	if (list_IsLast(lex_Context)) {
		return false;
	} else {
		SLexerContext* context = lex_Context;
		list_Remove(lex_Context, lex_Context);
		lexctx_FreeContext(context);
		return true;
	}
}

void
lexctx_Destroy(SLexerContext* context) {
	if (context != NULL) {
		if ((context->type == CONTEXT_FILE || context->type == CONTEXT_MACRO) && context->buffer.arguments != NULL) {
			strvec_Free(context->buffer.arguments);
		}

		lexbuf_Destroy(&context->buffer);
	} else {
		internalerror("Argument must not be NULL");
	}
}

void
lexctx_FreeContext(SLexerContext* context) {
	if (context != NULL) {
		lexctx_Destroy(context);
		mem_Free(context);
	} else {
		internalerror("Argument must not be NULL");
	}
}

SLexerContext*
lexctx_CreateMemoryContext(string* name, string* memory, vec_t* arguments) {
	SLexerContext* ctx = createContext();

	lexbuf_Init(&ctx->buffer, name, memory, arguments);
	ctx->atLineStart = true;
	ctx->mode = LEXER_MODE_NORMAL;
	return ctx;
}

SLexerContext*
lexctx_CreateFileContext(FILE* fileHandle, string* name) {
	SLexerContext* ctx = createContext();

	size_t size = fsize(fileHandle);
	string* fileContent = str_ReadFile(fileHandle, size);
	string* canonicalizedContent = str_CanonicalizeLineEndings(fileContent);
	str_Free(fileContent);

	lexbuf_Init(&ctx->buffer, name, canonicalizedContent, strvec_Create());
	ctx->type = CONTEXT_FILE;
	ctx->atLineStart = true;
	ctx->mode = LEXER_MODE_NORMAL;
	ctx->lineNumber = 1;
	ctx->fileInfo = createFileInfo(name);

	if (opt_Current->enableDebugInfo)
		ctx->fileInfo->crc32 = crc32((const uint8_t *)str_String(fileContent), size);

	str_Free(canonicalizedContent);

	return ctx;
}


extern void
lexctx_ProcessIncludeFile(string* name) {
	FILE* fileHandle;
	if (name != NULL && (fileHandle = fopen(str_String(name), "rb")) != NULL) {
		SLexerContext* newContext = lexctx_CreateFileContext(fileHandle, name);
		fclose(fileHandle);

		dep_AddDependency(newContext->buffer.name);
		pushContext(newContext);
	} else {
		err_Fail(ERROR_NO_FILE);
	}
	str_Free(name);
}

extern void
lexctx_ProcessRepeatBlock(uint32_t count) {
	SLexerContext* newContext = createContext();

	lexctx_Copy(newContext, lex_Context);
	lexbuf_RenewUniqueValue(&newContext->buffer);
	newContext->type = CONTEXT_REPT;
	newContext->mode = LEXER_MODE_NORMAL;
	newContext->block.repeat.remaining = count - 1;
	newContext->block.repeat.bookmark = lex_Context;

	pushContext(newContext);
}

extern void
lexctx_ProcessMacro(string* macroName) {
	SSymbol* symbol = sym_GetSymbol(macroName);

	if (symbol != NULL) {
		SLexerContext* newContext = lexctx_CreateMemoryContext(symbol->fileInfo->fileName, symbol->value.macro, g_newMacroArguments);

		newContext->type = CONTEXT_MACRO;

		newContext->fileInfo = symbol->fileInfo;
		newContext->mode = LEXER_MODE_NORMAL;

		g_newMacroArguments = strvec_Create();
		strvec_PushBack(g_newMacroArguments, NULL);

		newContext->lineNumber = symbol->lineNumber;
		newContext->block.macro.symbol = symbol;

		pushContext(newContext);
	} else {
		err_Error(ERROR_NO_MACRO);
	}
}

extern bool
lexctx_ContextInit(string* fileName) {
	g_newMacroArguments = strvec_Create();
	strvec_PushBack(g_newMacroArguments, NULL);

	g_fileNameMap = strmap_Create(freeFileNameInfo);
	createFileInfo(fileName);
	dep_AddDependency(fileName);

	string* symbolName = str_Create("__FILE");
	sym_CreateEqus(symbolName, fileName);
	str_Free(symbolName);

	string* name = inc_FindFile(fileName);
	if (name != NULL) {
		FILE* fileHandle = fopen(str_String(name), "rb");
		if (fileHandle != NULL) {
			lex_Context = lexctx_CreateFileContext(fileHandle, name);
			str_Free(name);
			fclose(fileHandle);
			return true;
		}
		str_Free(name);
	}

	err_Fail(ERROR_NO_FILE);
	return false;
}

extern void
lexctx_Cleanup(void) {
	SLexerContext* ctx = lex_Context;
	while (ctx != NULL) {
		SLexerContext* next = list_GetNext(ctx);
		lexctx_FreeContext(ctx);
		ctx = next;
	}

	if (g_fileNameMap != NULL)
		strmap_Free(g_fileNameMap);

	if (g_newMacroArguments != NULL)
		strvec_Free(g_newMacroArguments);
}

extern SFileInfo*
lexctx_TokenFileInfo(void) {
	return getMostRecentFileInfo(lex_Context);
}

extern uint32_t
lexctx_TokenLineNumber(void) {
	return getMostRecentLineNumber(lex_Context);
}

extern SFileInfo**
lexctx_GetFileInfo(size_t* totalFiles) {
	*totalFiles = map_Count(g_fileNameMap);
	assert (*totalFiles != 0);

	SFileInfo** result = mem_Alloc(*totalFiles * sizeof(SFileInfo*));
	map_ForEachKeyValue(g_fileNameMap, copyFileInfo, (intptr_t) result);
	return result;
}

extern void
lexctx_Copy(SLexerContext* dest, const SLexerContext* source) {
	copyToken(&dest->token, &source->token);
	dest->type = source->type;
	dest->mode = source->mode;
	lexbuf_Copy(&dest->buffer, &source->buffer);
	dest->atLineStart = source->atLineStart;
	dest->fileInfo = source->fileInfo;
	dest->lineNumber = source->lineNumber;
	dest->block = source->block;
}

extern void
lexctx_ShallowCopy(SLexerContext* dest, const SLexerContext* source) {
	*dest = *source;
	/*
	copyToken(&dest->token, &source->token);
	dest->token.tokenString = source->token.tokenString;
	dest->type = source->type;
	dest->mode = source->mode;
	lexbuf_ShallowCopy(&dest->buffer, &source->buffer);
	dest->atLineStart = source->atLineStart;
	dest->fileInfo = source->fileInfo;
	dest->lineNumber = source->lineNumber;
	dest->block = source->block;
	*/
}
