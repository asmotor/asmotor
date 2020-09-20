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

static string* g_newMacroArgument0;
static string** g_newMacroArguments;
static uint32_t g_newMacroArgumentsCount;

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

static string*
createUniqueId(void) {
    static uint32_t runId = 0;
    return str_CreateFormat("_%u", runId++);
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

extern string*
fstk_GetMacroArgValue(char argumentId) {
    if (argumentId == '@') {
        return fstk_GetMacroUniqueId();
    } else {
        SFileStackEntry* context = getMacroContext();
        if (context != NULL) {
            string* str = NULL;
            if (argumentId == '0') {
                str = context->block.macro.argument0;
            } else {
                uint_fast8_t argumentIndex = (uint8_t) (argumentId - '1');
                if (argumentIndex < context->block.macro.argumentCount) {
                    str = context->block.macro.arguments[argumentIndex];
                }
            }
            return str_Copy(str);
        }
    } 
    return NULL;
}

extern string*
fstk_GetMacroUniqueId(void) {
    return str_Copy(fstk_Current->uniqueId);
}

extern int32_t
fstk_GetMacroArgumentCount(void) {
    SFileStackEntry* entry = getMacroContext();
    return entry != NULL ? entry->block.macro.argumentCount : 0;
}

extern void
fstk_AddMacroArgument(const char* str) {
    g_newMacroArguments = (string**) mem_Realloc(g_newMacroArguments, sizeof(string*) * (g_newMacroArgumentsCount + 1));
    g_newMacroArguments[g_newMacroArgumentsCount++] = str_Create(str);
}

extern void
fstk_SetMacroArgument0(const char* str) {
    g_newMacroArgument0 = str_Create(str);
}

extern void
fstk_ShiftMacroArgs(int32_t count) {
    SFileStackEntry* context = getMacroContext();
    if (context != NULL) {
        while (count >= 1) {
            str_Free(context->block.macro.arguments[0]);

            for (uint32_t i = 1; i < context->block.macro.argumentCount; ++i) {
                context->block.macro.arguments[i - 1] = context->block.macro.arguments[i];
            }
            context->block.macro.argumentCount -= 1;
            context->block.macro.arguments = mem_Realloc(context->block.macro.arguments, context->block.macro.argumentCount * sizeof(string*));
            count -= 1;
        }
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
        SFileStackEntry* oldContext = fstk_Current;
        list_Remove(fstk_Current, fstk_Current);
        lex_FreeBuffer(oldContext->lexBuffer);
        str_Free(oldContext->name);

        switch (oldContext->type) {
            case CONTEXT_REPT: {
                if (oldContext->block.repeat.remaining > 0) {
                    fstk_ProcessRepeatBlock(oldContext->block.repeat.buffer, oldContext->block.repeat.size,
                                            oldContext->block.repeat.remaining);
                } else {
                    fstk_Current->lineNumber += oldContext->lineNumber;
                    lex_SetBuffer(fstk_Current->lexBuffer);
                    mem_Free(oldContext->block.repeat.buffer);
                }
                str_Free(oldContext->uniqueId);
                mem_Free(oldContext);
                break;
            }
            case CONTEXT_MACRO: {
                for (uint32_t i = 0; i < oldContext->block.macro.argumentCount; i += 1) {
                    str_Free(oldContext->block.macro.arguments[i]);
                }
                str_Free(oldContext->uniqueId);
                mem_Free(oldContext->block.macro.arguments);
                mem_Free(oldContext);
                lex_SetBuffer(fstk_Current->lexBuffer);
                break;
            }
            default: {
                mem_Free(oldContext);
                lex_SetBuffer(fstk_Current->lexBuffer);
                break;
            }
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
fstk_ProcessRepeatBlock(char* buffer, size_t size, uint32_t count) {
    SFileStackEntry* newContext = mem_Alloc(sizeof(SFileStackEntry));
    list_Init(newContext);

    newContext->name = str_Create("REPT");
    newContext->fileInfo = NULL;
    newContext->type = CONTEXT_REPT;

    if ((newContext->lexBuffer = lex_CreateMemoryBuffer(buffer, size)) != NULL) {
        lex_SetBuffer(newContext->lexBuffer);
        lex_SetMode(LEXER_MODE_NORMAL);
        newContext->lineNumber = 0;
        newContext->block.repeat.buffer = buffer;
        newContext->block.repeat.size = size;
        newContext->block.repeat.remaining = count - 1;
        newContext->uniqueId = createUniqueId();
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
        newContext->lexBuffer = lex_CreateMemoryBuffer(str_String(symbol->value.macro), str_Length(symbol->value.macro));

        lex_SetBuffer(newContext->lexBuffer);
        lex_SetMode(LEXER_MODE_NORMAL);
        newContext->lineNumber = 1;
        newContext->block.macro.symbol = symbol;
        newContext->block.macro.argument0 = g_newMacroArgument0;
        newContext->block.macro.arguments = g_newMacroArguments;
        newContext->block.macro.argumentCount = g_newMacroArgumentsCount;
        newContext->uniqueId = createUniqueId();
        g_newMacroArgumentsCount = 0;
        g_newMacroArguments = NULL;
        g_newMacroArgument0 = NULL;
        setNewContext(newContext);
    } else {
        err_Error(ERROR_NO_MACRO);
    }
}

extern bool
fstk_Init(string* fileName) {
    g_newMacroArgumentsCount = 0;
    g_newMacroArguments = NULL;

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

