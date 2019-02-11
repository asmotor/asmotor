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

// From util
#include "asmotor.h"
#include "file.h"
#include "mem.h"

// From xasm
#include "xasm.h"
#include "filestack.h"
#include "errors.h"
#include "lexer.h"
#include "tokens.h"
#include "symbol.h"


/* Internal structures */

#define MAX_INCLUDE_PATHS 16

/* Internal variables */

SFileStackEntry* fstk_Current;

static string* g_includePaths[MAX_INCLUDE_PATHS];
static uint8_t g_totalIncludePaths = 0;

static string* g_newMacroArgument0;
static string** g_newMacroArguments;
static uint32_t g_newMacroArgumentsCount;

#if defined(WIN32)
# define PATH_SEPARATOR '\\'
# define PATH_REPLACE '/'
#else
# define PATH_SEPARATOR '/'
# define PATH_REPLACE '\\'
#endif

/* Private functions */

static string*
createUniqueId(void) {
    static uint32_t runId = 0;

    char symbolName[MAX_SYMBOL_NAME_LENGTH + 1];
    sprintf(symbolName, "_%u", runId++);

    return str_Create(symbolName);
}

static void
setNewContext(SFileStackEntry* context) {
    list_Insert(fstk_Current, context);
}

static bool
isContextMacro(void) {
    return fstk_Current != NULL && fstk_Current->type == CONTEXT_MACRO;
}

static string*
fixPathSeparators(string* filename) {
    return str_Replace(filename, PATH_REPLACE, PATH_SEPARATOR);
}

static string*
replaceFileComponent(string* fullPath, string* filename) {
    if (fullPath == NULL)
        return filename;

    const char* lastSlash = str_String(fullPath) + str_Length(fullPath) - 1;

    while (lastSlash > str_String(fullPath) && *lastSlash != '/' && *lastSlash != '\\')
        --lastSlash;

    if (lastSlash == str_String(fullPath))
        return str_Copy(filename);

    string* basePath = str_Slice(fullPath, 0, lastSlash + 1 - str_String(fullPath));
    string* newFullPath = str_Concat(basePath, filename);
    str_Free(basePath);

    string* fixedPath = fixPathSeparators(newFullPath);
    str_Free(newFullPath);
    return fixedPath;
}

/* Public functions */

string*
fstk_GetMacroArgValue(char argumentId) {
    string* str = NULL;

    if (isContextMacro()) {
        if (argumentId == '0') {
            str = fstk_Current->block.macro.argument0;
        } else {
            uint_fast8_t argumentIndex = (uint8_t) (argumentId - '1');
            if (argumentIndex < fstk_Current->block.macro.argumentCount) {
                str = fstk_Current->block.macro.arguments[argumentIndex];
            }
        }
    }

    return str_Copy(str);
}

string*
fstk_GetMacroUniqueId(void) {
    return str_Copy(fstk_Current->uniqueId);
}

int32_t
fstk_GetMacroArgumentCount(void) {
    return isContextMacro() ? fstk_Current->block.macro.argumentCount : 0;
}

void
fstk_AddMacroArgument(const char* str) {
    g_newMacroArguments = (string**) mem_Realloc(g_newMacroArguments, sizeof(string*) * (g_newMacroArgumentsCount + 1));
    g_newMacroArguments[g_newMacroArgumentsCount++] = str_Create(str);
}

void
fstk_SetMacroArgument0(const char* str) {
    g_newMacroArgument0 = str_Create(str);
}

void
fstk_ShiftMacroArgs(int32_t count) {
    if (isContextMacro()) {
        while (count >= 1) {
            str_Free(fstk_Current->block.macro.arguments[0]);

            for (uint32_t i = 1; i < fstk_Current->block.macro.argumentCount; ++i) {
                fstk_Current->block.macro.arguments[i - 1] = fstk_Current->block.macro.arguments[i];
            }
            fstk_Current->block.macro.argumentCount -= 1;
            fstk_Current->block.macro.arguments = mem_Realloc(fstk_Current->block.macro.arguments,
                                                                  fstk_Current->block.macro.argumentCount
                                                                  * sizeof(string*));
            count -= 1;
        }
    } else {
        err_Warn(WARN_SHIFT_MACRO);
    }
}

string*
fstk_FindFile(string* filename) {
    string* fullPath = NULL;
    filename = fixPathSeparators(filename);

    if (fstk_Current->name == NULL) {
        if (fexists(str_String(filename))) {
            fullPath = str_Copy(filename);
        }
    }

    if (fullPath == NULL) {
        string* candidate = replaceFileComponent(fstk_Current->name, filename);
        if (candidate != NULL) {
            if (fexists(str_String(candidate))) {
                STR_MOVE(fullPath, candidate);
            }
        }
    }

    if (fullPath == NULL) {
        for (uint32_t count = 0; count < g_totalIncludePaths; count += 1) {
            string* candidate = str_Concat(g_includePaths[count], filename);

            if (fexists(str_String(candidate))) {
                STR_MOVE(fullPath, candidate);
                break;
            }
        }
    }

    str_Free(filename);
    return fullPath;
}

void
fstk_Dump(void) {
    SFileStackEntry* stack;

    if (fstk_Current == NULL) {
        printf("(From commandline) ");
        return;
    }

    stack = fstk_Current;
    while (list_GetNext(stack)) {
        stack = list_GetNext(stack);
    }

    while (stack != NULL) {
        if (stack->name != NULL) {
            printf("%s(%d)", str_String(stack->name), stack->lineNumber);
        }
        stack = list_GetPrev(stack);

        printf(stack != NULL ? "->" : ": ");
    }
}

bool
fstk_ProcessNextBuffer(void) {
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
                    lex_SetBuffer(fstk_Current->lexBuffer);
                    mem_Free(oldContext->block.repeat.buffer);
                }
                str_Free(oldContext->uniqueId);
                mem_Free(oldContext);
                break;
            }
            case CONTEXT_MACRO: {
                for (uint32_t i = 0; i < oldContext->block.macro.argumentCount; i += 1) {
                    mem_Free(oldContext->block.macro.arguments[i]);
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

void
fstk_AddIncludePath(string* pathname) {
    char ch;

    if (g_totalIncludePaths == MAX_INCLUDE_PATHS) {
        err_Fail(ERROR_INCLUDE_LIMIT);
        return;
    }

    ch = str_CharAt(pathname, str_Length(pathname) - 1);
    if (ch != '\\' && ch != '/') {
        string* pSlash = str_Create("/");
        g_includePaths[g_totalIncludePaths++] = str_Concat(pathname, pSlash);
        str_Free(pSlash);
    } else {
        g_includePaths[g_totalIncludePaths++] = str_Copy(pathname);
    }
}

void
fstk_ProcessIncludeFile(string* filename) {
    SFileStackEntry* newContext = mem_Alloc(sizeof(SFileStackEntry));
    list_Init(newContext);

    newContext->type = CONTEXT_FILE;
    newContext->name = fstk_FindFile(filename);

    FILE* fileHandle;
    if (newContext->name != NULL && (fileHandle = fopen(str_String(newContext->name), "rt")) != NULL) {
        if ((newContext->lexBuffer = lex_CreateFileBuffer(fileHandle)) != NULL) {
            lex_SetBuffer(newContext->lexBuffer);
            lex_SetState(LEX_STATE_NORMAL);
            newContext->lineNumber = 0;
            setNewContext(newContext);
            return;
        }
        fclose(fileHandle);
    } else {
        err_Fail(ERROR_NO_FILE);
    }
}

void
fstk_ProcessRepeatBlock(char* buffer, size_t size, uint32_t count) {
    SFileStackEntry* newContext = mem_Alloc(sizeof(SFileStackEntry));
    list_Init(newContext);

    newContext->name = str_Create("REPT");
    newContext->type = CONTEXT_REPT;

    if ((newContext->lexBuffer = lex_CreateMemoryBuffer(buffer, size)) != NULL) {
        lex_SetBuffer(newContext->lexBuffer);
        lex_SetState(LEX_STATE_NORMAL);
        newContext->lineNumber = 0;
        newContext->block.repeat.buffer = buffer;
        newContext->block.repeat.size = size;
        newContext->block.repeat.remaining = count - 1;
        newContext->uniqueId = createUniqueId();
        setNewContext(newContext);
    }
}

void
fstk_ProcessMacro(string* macroName) {
    SSymbol* sym;

    if ((sym = sym_GetSymbol(macroName)) != NULL) {
        SFileStackEntry* newContext = mem_Alloc(sizeof(SFileStackEntry));

        memset(newContext, 0, sizeof(SFileStackEntry));
        newContext->type = CONTEXT_MACRO;

        newContext->name = str_Copy(macroName);
        newContext->lexBuffer = lex_CreateMemoryBuffer(str_String(sym->value.macro), str_Length(sym->value.macro));

        lex_SetBuffer(newContext->lexBuffer);
        lex_SetState(LEX_STATE_NORMAL);
        newContext->lineNumber = UINT32_MAX;
        newContext->block.macro.argument0 = g_newMacroArgument0;
        newContext->block.macro.arguments = g_newMacroArguments;
        newContext->block.macro.argumentCount = g_newMacroArgumentsCount;
        newContext->uniqueId = createUniqueId();
        g_newMacroArgumentsCount = 0;
        g_newMacroArguments = NULL;
        g_newMacroArgument0 = NULL;
        setNewContext(newContext);
    } else {
        err_Fail(ERROR_NO_MACRO);
    }
}

bool
fstk_Init(string* filename) {
    g_newMacroArgumentsCount = 0;
    g_newMacroArguments = NULL;

    string* symbolName = str_Create("__FILE");
    sym_CreateEqus(symbolName, filename);
    str_Free(symbolName);

    fstk_Current = mem_Alloc(sizeof(SFileStackEntry));
    memset(fstk_Current, 0, sizeof(SFileStackEntry));
    fstk_Current->type = CONTEXT_FILE;

    FILE* fileHandle;
    if ((fstk_Current->name = fstk_FindFile(filename)) != NULL
        && (fileHandle = fopen(str_String(fstk_Current->name), "rt")) != NULL) {

        fstk_Current->lexBuffer = lex_CreateFileBuffer(fileHandle);
        fclose(fileHandle);

        lex_SetBuffer(fstk_Current->lexBuffer);
        lex_SetState(LEX_STATE_NORMAL);
        fstk_Current->lineNumber = 1;
        return true;
    }

    err_Fail(ERROR_NO_FILE);
    return false;
}

void
fstk_Cleanup(void) {
}

SFileStackEntry*
fstk_GetLastStackEntry(void) {
    SFileStackEntry* stackEntry = fstk_Current;
    while (!list_IsLast(stackEntry)) {
        stackEntry = list_GetNext(stackEntry);
    }
    return stackEntry;
}
