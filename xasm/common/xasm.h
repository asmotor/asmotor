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

#ifndef	INCLUDE_XASM_H
#define	INCLUDE_XASM_H

#include "types.h"

#define	MAXSYMNAMELENGTH		256
#define	MAXSTRINGSYMBOLSIZE		256
#define	ASM_CRLF				10
#define	ASM_TAB					9

#define	MAXTOKENLENGTH			256

#if	defined(__GNUC__) && !defined(__DJGPP__)
extern void strupr(char* s);
extern void strlwr(char* s);
#endif

extern uint32_t g_nTotalLines;
extern uint32_t g_nTotalErrors;
extern uint32_t g_nTotalWarnings;

extern int xasm_Main(int argc, char* argv[]);

extern void loclexer_Init(void);
extern void locopt_PrintOptions(void);
extern struct MachineOptions* locopt_Alloc(void);
extern void locopt_Free(struct MachineOptions* pOptions);
extern void locopt_Copy(struct MachineOptions* pDest, struct MachineOptions* pSrc);

typedef	enum Endian
{
	ASM_LITTLE_ENDIAN = 0,
	ASM_BIG_ENDIAN = 1
} EEndian;

typedef struct Configuration
{
	char* pszExecutable;
	char* pszBackendVersion;
	uint32_t nMaxSectionSize;
	EEndian eDefaultEndianness;
	bool_t bSupportBanks;
	bool_t bSupportAmiga;
	char* pszNameRB;
	char* pszNameRW;
	char* pszNameRL;
	char* pszNameDB;
	char* pszNameDW;
	char* pszNameDL;
	char* pszNameDSB;
	char* pszNameDSW;
	char* pszNameDSL;
} SConfiguration;

extern SConfiguration* g_pConfiguration;

#endif	/*INCLUDE_XASM_H*/
