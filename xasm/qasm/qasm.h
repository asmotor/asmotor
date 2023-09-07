#pragma once

#include "str.h"

#include "expression.h"
#include "options.h"
#include "section.h"

typedef enum {
    MINSIZE_8BIT = 1,
    MINSIZE_16BIT = 2,
    MINSIZE_32BIT = 4,
    MINSIZE_64BIT = 8,
} EMinimumWordSize;

typedef struct Configuration {
    const char* executableName;
    const char* backendVersion;

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
