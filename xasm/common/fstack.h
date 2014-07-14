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

#ifndef	INCLUDE_FSTACK_H
#define	INCLUDE_FSTACK_H

#include "lists.h"
#include "str.h"

struct LexBuffer;

typedef	enum
{
	CONTEXT_FILE,
	CONTEXT_REPT,
	CONTEXT_MACRO
} EContextType;

struct FileStack
{
	list_Data(struct FileStack);
	string*			pName;
	struct LexBuffer* pLexBuffer;
	int32_t			LineNumber;
	EContextType	Type;
	char*			RunID;	/*	For the \@ symbol */

	/*	This is for repeating block type stuff. Currently only REPT */
	union
	{
		struct
		{
			char*	pOriginalBuffer;
			uint32_t	OriginalSize;
			uint32_t	RemainingRuns;
		} Rept;
		struct
		{
			char*	Arg0;
			char**	Args;
			uint32_t	ArgCount;
		} Macro;
	} BlockInfo;
};
typedef struct FileStack SFileStack;

extern void fstk_RunMacro(string* pName);
extern void fstk_RunInclude(string* pFile);
extern void fstk_RunRept(char* buffer, uint32_t size, uint32_t count);
extern bool_t fstk_RunNextBuffer(void);
extern bool_t fstk_Init(string* pFile);
extern void fstk_Cleanup(void);
extern void fstk_Dump(void);
extern string* fstk_FindFile(string* pFile);
extern char* fstk_GetMacroArgValue(char ch);
extern char* fstk_GetMacroRunID(void);
extern void fstk_AddMacroArg(char* s);
extern void fstk_SetMacroArg0(char* s);
extern void fstk_ShiftMacroArgs(int32_t count);
extern int32_t fstk_GetMacroArgCount(void);
extern void fstk_AddIncludePath(string* pFile);

extern SFileStack* g_pFileContext;

#endif	/*INCLUDE_FSTACK_H*/
