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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mem.h"
#include "str.h"
#include "strbuf.h"

#include "xasm.h"
#include "errors.h"
#include "lexer_context.h"
#include "options.h"
#include "patch.h"

static string* g_lastErrorString = NULL;

static char* g_warnings[] = {
    "Cannot \"PURGE\" undefined symbol",
    "Error in option %s, ignored",
    "Cannot pop options from an empty stack",
    "%s",
    "\"SHIFT\" used outside MACRO, ignored",
    "\"MEXIT\" used outside MACRO, ignored",
    "Not inside REPT block, ignored",
    "Error in machine option %s",
	"Symbol has reserved name",
};

static char* g_errors[] = {
    "%c expected",
    "Expression must be %d bit",
    "Invalid expression",
    "Invalid source operand",
    "Invalid destination operand",
    "Invalid first operand",
    "Invalid second operand",
    "Invalid operand",
    "Expression expected",
    "Operand out of range",
    "Cannot modify symbol",
    "Label before SECTION",
    "Cannot export symbol",
    "SECTION cannot contain initialised data",
    "Cannot import already defined symbol",
    "Available SECTION space exhausted",
    "No SECTION defined",
    "Expression is neither constant nor relocatable",
    "Expression must be a power of two",
    "Expression must be constant",
    "Expression must be relocatable",
    "Invalid string expression",
    "Bad expression",
    "BANK expected",
    "TEXT or BSS expected",
    "Identifier must be a GROUP symbol",
    "Identifier expected",
    "Expression must be positive",
    "Syntax error",
    "Unknown instruction \"%s\"",
    "When writing binary file only PC relative addressing must be used, or first section must be LOAD fixed.",
    "Section \"%s\" cannot be placed at $%X",
    "Symbol must be constant",
    "Symbol must be a string value",
    "Symbol must be a floating point value",
    "SECTION already exist in a different GROUP",
    "Read error",
    "File not found",
    "SECTION already exists but it's not LOAD fixed to the same address",
    "SECTION already exists but it's not BANK fixed to the same bank",
    "SECTION already exists but it's not LOAD/BANK fixed to the same address/bank",
    "SECTION does not exist",
    "Divide by zero",
    "Symbol cannot be used in an expression",
    "DEF() needs a symbol",
    "BANK() needs a symbol",
    "Unterminated MACRO block (started at %s, line %d)",
    "Unterminated REPT block",
    "Unexpected end of file reached",
    "Unterminated string",
    "Malformed identifier",
    "Maximum number of include paths reached",
    "MACRO doesn't exist",
    "Symbol %s is undefined",
    "Object file does not support expression",
    "Invalid MACRO argument",
    "Not a string member function returning int",
    "Too many files specified on command line",
    "Extended precision floating point not supported"
};

static const char*
getError(size_t errorNumber) {
    if (errorNumber >= 1000)
        return xasm_Configuration->getMachineError(errorNumber);

    if (errorNumber < sizeof(g_warnings) / sizeof(char*)) {
        return g_warnings[errorNumber];
    } else {
        return g_errors[errorNumber - 100];
    }
}

typedef struct Messages {
    uint32_t totalMessages;
    string** messages;
} SMessages;

typedef struct SuspendedErrors {
    struct SuspendedErrors* next;
    SMessages errors;
    SMessages warnings;
} SSuspendedErrors;

static SMessages 
g_allMessages;

static SSuspendedErrors* 
g_suspendedErrors = NULL;



static void
initializeMessages(SMessages* msg) {
    msg->totalMessages = 0;
    msg->messages = NULL;
}


static bool
matchesLastMessage(string* str) {
    if (str_Equal(str, g_lastErrorString)) {
        return true;
    }

    STR_ASSIGN(g_lastErrorString, str);
    return false;
}

static int
compareStrings(const void* s1, const void* s2) {
    return str_Compare(*(const string**) s1, *(const string**) s2);
}


static void
sortMessages(SMessages* messages) {
    qsort(messages->messages, messages->totalMessages, sizeof(string*), compareStrings);
}


static void
addMessage(SMessages* msg, string* str) {
    msg->totalMessages += 1;
    msg->messages = (string**) mem_Realloc(msg->messages, msg->totalMessages * sizeof(string*));
    msg->messages[msg->totalMessages - 1] = str_Copy(str);
}


static void
freeMessages(SMessages* msg) {
    for (uint32_t i = 0; i < msg->totalMessages; ++i) {
        str_Free(msg->messages[i]);
    }
    mem_Free(msg->messages);
}


void
err_Suspend(void) {
    SSuspendedErrors* errors = (SSuspendedErrors*) mem_Alloc(sizeof(SSuspendedErrors));

    errors->next = g_suspendedErrors;
    initializeMessages(&errors->errors);
    initializeMessages(&errors->warnings);

    g_suspendedErrors = errors;
}

void
err_Discard(void) {
    assert(g_suspendedErrors != NULL);

    freeMessages(&g_suspendedErrors->errors);
    freeMessages(&g_suspendedErrors->warnings);

    SSuspendedErrors* block = g_suspendedErrors;
    g_suspendedErrors = block->next;
    mem_Free(block);    
}

static void
printMessages(const SMessages* messages, uint32_t* total) {
    for (uint32_t i = 0; i < messages->totalMessages; ++i) {
        addMessage(&g_allMessages, messages->messages[i]);
    }
    *total += messages->totalMessages;
}

void
err_Accept(void) {
    assert(g_suspendedErrors != NULL);
    
    printMessages(&g_suspendedErrors->warnings, &xasm_TotalWarnings);
    printMessages(&g_suspendedErrors->errors, &xasm_TotalErrors);

    err_Discard();
}

static void
printError(const SPatch* patch, const SSymbol* symbol, char severity, size_t errorNumber, uint32_t* count, va_list args) {
    string_buffer* buf = strbuf_Create();

    if (patch != NULL) {
        strbuf_AppendFormat(buf, "%s:%d: ", str_String(patch->filename), patch->lineNumber);
    } else if (symbol != NULL) {
        strbuf_AppendFormat(buf, "%s:%d: ", str_String(symbol->fileInfo->fileName), symbol->lineNumber);
    } else {
        string* stack = lexctx_Dump();
        strbuf_AppendString(buf, stack);
        str_Free(stack);
    }
    strbuf_AppendFormat(buf, "%c%04d ", severity, (int) errorNumber);

    strbuf_AppendArgs(buf, getError(errorNumber), args);

    string* str = strbuf_String(buf);
    strbuf_Free(buf);

    if (g_suspendedErrors == NULL) {
        addMessage(&g_allMessages, str);
        *count += 1;
    } else {
        SMessages* msg = severity == 'W' ? &g_suspendedErrors->warnings : &g_suspendedErrors->errors;
        addMessage(msg, str);
    }
    str_Free(str);
}

static bool
warningEnabled(uint32_t errorNumber) {
    for (uint32_t i = 0; i < opt_Current->disabledWarningsCount; ++i) {
        if (opt_Current->disabledWarnings[i] == errorNumber)
            return false;
    }
    return true;
}

bool
err_Warn(uint32_t errorNumber, ...) {
    if (warningEnabled(errorNumber)) {
        va_list args;

        va_start(args, errorNumber);
        printError(NULL, NULL, 'W', errorNumber, &xasm_TotalWarnings, args);
        va_end(args);
    }
    return true;
}

bool
err_Error(uint32_t n, ...) {
    va_list args;

    va_start(args, n);
    printError(NULL, NULL, 'E', n, &xasm_TotalErrors, args);
    va_end(args);

    return false;
}

bool
err_PatchError(const SPatch* patch, uint32_t n, ...) {
    va_list args;

    va_start(args, n);
    printError(patch, NULL, 'E', n, &xasm_TotalErrors, args);
    va_end(args);

    ++xasm_TotalErrors;
    return false;
}

bool
err_SymbolError(const SSymbol* symbol, uint32_t n, ...) {
    va_list args;

    va_start(args, n);
    printError(NULL, symbol, 'E', n, &xasm_TotalErrors, args);
    va_end(args);

    ++xasm_TotalErrors;
    return false;
}

bool
err_Fail(uint32_t n, ...) {
    va_list args;

    va_start(args, n);
    printError(NULL, NULL, 'F', n, &xasm_TotalErrors, args);
    va_end(args);

    err_PrintAll();

    printf("Bailing out.\n");
    exit(EXIT_FAILURE);
}

bool
err_PatchFail(const SPatch* patch, uint32_t n, ...) {
    va_list args;

    va_start(args, n);
    printError(patch, NULL, 'F', n, &xasm_TotalErrors, args);
    va_end(args);

    err_PrintAll();

    printf("Bailing out.\n");
    exit(EXIT_FAILURE);
}


void
err_Init(void) {
    initializeMessages(&g_allMessages);
}


void
err_PrintAll(void) {
    sortMessages(&g_allMessages);
    for (uint32_t i = 0; i < g_allMessages.totalMessages; ++i) {
        string* str = g_allMessages.messages[i];
        if (!matchesLastMessage(str)) {
            printf("%s\n", str_String(str));
        }
    }
	freeMessages(&g_allMessages);
	str_Free(g_lastErrorString);
}
