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

#ifndef XASM_COMMON_FILESTACK_H_INCLUDED_
#define XASM_COMMON_FILESTACK_H_INCLUDED_

#include "lists.h"
#include "str.h"

#include "lexer.h"

typedef enum {
    CONTEXT_FILE,
    CONTEXT_REPT,
    CONTEXT_MACRO
} EContextType;

typedef struct FileInfo {
    string* fileName;
    uint32_t fileId;
    uint32_t crc32;
} SFileInfo;

struct Symbol;

typedef struct FileStackEntry {
    list_Data(struct FileStackEntry);

    EContextType type;
    string* name;
    SFileInfo* fileInfo;
    SLexerBuffer* lexBuffer;
    uint32_t lineNumber;

    string* uniqueId;    /*	The \@ symbol */
    union {
        struct {
            char* buffer;
            size_t size;
            uint32_t remaining;
        } repeat;
        struct {
            struct Symbol* symbol;
            string* argument0;
            string** arguments;
            uint32_t argumentCount;
        } macro;
    } block;
} SFileStackEntry;

extern string*
fstk_GetMacroUniqueId(void);

extern string*
fstk_GetMacroArgValue(char argumentId);

extern int32_t
fstk_GetMacroArgumentCount(void);

extern SFileInfo**
fstk_GetFileInfo(size_t* totalFileNames);

extern void
fstk_AddMacroArgument(const char* str);

extern void
fstk_SetMacroArgument0(const char* str);

extern void
fstk_ProcessMacro(string* macroName);

extern void
fstk_ProcessIncludeFile(string* filename);

extern void
fstk_ProcessRepeatBlock(char* buffer, size_t size, uint32_t count);

extern bool
fstk_EndCurrentBuffer(void);

extern bool
fstk_Init(string* filename);

extern void
fstk_Cleanup(void);

extern string*
fstk_Dump(void);

extern void
fstk_ShiftMacroArgs(int32_t count);

extern SFileInfo*
fstk_CurrentFileInfo();

extern uint32_t
fstk_CurrentLineNumber();

extern SFileStackEntry*
fstk_Current;

#endif /* XASM_COMMON_FILESTACK_H_INCLUDED_ */
