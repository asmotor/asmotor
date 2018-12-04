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

struct LexBuffer;

typedef enum {
	CONTEXT_FILE,
	CONTEXT_REPT,
	CONTEXT_MACRO
} EContextType;

typedef struct FileStackEntry {
	list_Data(struct FileStackEntry);

	EContextType Type;
	string* pName;
	struct LexBuffer* pLexBuffer;
	int32_t LineNumber;

	/*	This is for repeating block type stuff. Currently only REPT */
	string* uniqueId;    /*	For the \@ symbol */
	union {
		struct {
			char* pOriginalBuffer;
			uint32_t OriginalSize;
			uint32_t RemainingRuns;
		} Rept;
		struct {
			string* Arg0;
			string** Args;
			uint32_t ArgCount;
		} Macro;
	} BlockInfo;
} SFileStackEntry;

extern string* fstk_GetMacroUniqueId(void);
extern string* fstk_GetMacroArgValue(char argNumber);
extern int32_t fstk_GetMacroArgCount(void);
extern void fstk_AddMacroArg(char* str);
extern void fstk_SetMacroArg0(char* str);

extern void fstk_RunMacro(string* pName);
extern void fstk_RunInclude(string* pFile);
extern void fstk_RunRept(char* buffer, size_t size, uint32_t count);
extern bool fstk_RunNextBuffer(void);
extern bool fstk_Init(string* pFile);
extern void fstk_Cleanup(void);
extern void fstk_Dump(void);
extern string* fstk_FindFile(string* pFile);
extern void fstk_ShiftMacroArgs(int32_t count);
extern void fstk_AddIncludePath(string* pFile);

extern SFileStackEntry* g_currentContext;

#endif /* XASM_COMMON_FILESTACK_H_INCLUDED_ */
