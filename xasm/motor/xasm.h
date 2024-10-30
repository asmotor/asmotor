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

#ifndef XASM_COMMON_XASM_H_INCLUDED_
#define XASM_COMMON_XASM_H_INCLUDED_

#include <stdbool.h>
#include <stdlib.h>

#include "types.h"

#include "expression.h"
#include "options.h"
#include "section.h"
#include "str.h"

#define MAX_SYMBOL_NAME_LENGTH 256
#define MAX_STRING_SYMBOL_SIZE 256
#define ASM_CRLF 10
#define ASM_TAB 9

#define MAX_TOKEN_LENGTH 256

extern uint32_t xasm_TotalLines;
extern uint32_t xasm_TotalErrors;
extern uint32_t xasm_TotalWarnings;

typedef enum {
    MINSIZE_8BIT = 1,
    MINSIZE_16BIT = 2,
    MINSIZE_32BIT = 4,
    MINSIZE_64BIT = 8,
} EMinimumWordSize;

typedef struct Configuration {
    const char* executableName;

    uint32_t maxSectionSize;
    EEndianness defaultEndianness;

    bool supportBanks;
    bool supportAmiga;
    bool supportFloat;
	bool supportELF;

    EMinimumWordSize minimumWordSize;

    uint32_t sectionAlignment;

    const char* literalGroup;

    const char* reserveByteName;
    const char* reserveWordName;
    const char* reserveLongName;
    const char* reserveDoubleName;

    const char* defineByteName;
    const char* defineWordName;
    const char* defineLongName;
    const char* defineDoubleName;

    const char* defineByteSpaceName;
    const char* defineWordSpaceName;
    const char* defineLongSpaceName;
    const char* defineDoubleSpaceName;

    const char* (*getMachineError)(size_t errorNumber);
    void (*defineTokens)(void);
    void (*defineSymbols)(void);

    struct MachineOptions* (*allocOptions)(void);
    void (*setDefaultOptions)(struct MachineOptions*);
    void (*copyOptions)(struct MachineOptions* dest, struct MachineOptions* src);
    bool (*parseOption)(const char* option);
    void (*onOptionsUpdated)(struct MachineOptions*);
    void (*printOptionUsage)(void);

	SExpression* (*parseFunction)(void);
    bool (*parseInstruction)(void);

	void (*assignSection)(SSection* section);

	bool (*isValidLocalName)(const string* name);
} SConfiguration;

extern const SConfiguration*
xasm_Configuration;

extern int
xasm_Main(const SConfiguration* configuration, int argc, char* argv[]);

#endif // XASM_COMMON_XASM_H_INCLUDED_
