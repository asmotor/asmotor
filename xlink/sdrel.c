/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

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

/*
 * xLink - SDREL.C
 *
 *	Reader for SDCC/sdas ASxxxx RREL (.rel) text object files.
 *
 *	The format is a line-oriented text format produced by the sdas
 *	assembler family and consumed by the aslink/sdld linkers; see
 *	sdas/doc/format.txt. This reader implements the version 3 subset
 *	actually emitted by the sdcc rc8 backend:
 *
 *	- Magic "XL2": hex radix, little-endian, two-byte addresses
 *	- Areas (A lines) become sections; the area -> xlink group
 *	  mapping is port-specific (rc8 table below)
 *	- Symbols (S lines): Def -> SYM_EXPORT (absolute when defined
 *	  before the first area), Ref -> SYM_IMPORT
 *	- Text/relocation record pairs (T/R lines). Each R record starts
 *	  with an implicit word relocation of the record's own two-byte
 *	  offset field to the area base, which also identifies the area
 *	  the record belongs to. It is followed by relocation groups:
 *	      <mode> <byte index into the T record, offset field incl.>
 *	      <area/symbol index lo> <hi>
 *	- Byte relocations with bit3 (BYTX) carry a two-byte object of
 *	  which only the selected (LSB or MSB) byte is emitted; the other
 *	  byte is padding that the linker drops. Section data and patch
 *	  offsets are therefore computed in the compacted, emitted
 *	  address space, matching sdld's rtflg[] output compaction.
 *
 *	PC-relative patches use the OBJ_PC_REL operator; sdld computes
 *	target - (object address + 1) for bytes and + 2 for words, and
 *	the rc8 backend additionally bakes the +1 bias of relative
 *	branches into the object bytes, so the expression constant is
 *	objectValue - pcrAdjust + selectedBytePosition.
 *
 * Anything outside the accepted subset (radix, endianness, address
 * size, paged/overlay areas, unknown area names or relocation modes,
 * P/G/B lines) is refused loudly rather than guessed at.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// from util
#include "mem.h"
#include "str.h"

// from xlink
#include "object.h"
#include "patch.h"
#include "sdrel.h"
#include "section.h"
#include "symbol.h"
#include "xlink.h"

/* R-line relocation mode bits as emitted by sdas (asxxxx.h) and
 * interpreted by the version 3 relocation code (aslink.h R3_*). */
#define R_BYTE 0x01  /* 0 = 16-bit word object, 1 = 8-bit byte object */
#define R_SYM 0x02   /* 0 = area reference, 1 = symbol reference */
#define R_PCR 0x04   /* PC relative relocation */
#define R_BYTX 0x08  /* byte object stored in two bytes, one selected */
#define R_MSB 0x80   /* select the high byte of a BYTX object */
#define R_ESCAPE_MASK 0xF0

/* Area flags (asxxxx.h); rc8 emits only A_CON|A_REL (0xC00) and
 * A_CON|A_ABS (0xC08). */
#define A_WORDLENGTH 0x0003u
#define A_OVR 0x0404u
#define A_ABS 0x0808u
#define A_PAG 0x1010u
#define A_FLAGS_KNOWN 0x1C0Cu

static Group g_homeGroup = {"HOME", GROUP_TEXT, 0};
static Group g_codeGroup = {"CODE", GROUP_TEXT, 0};
static Group g_dataGroup = {"DATA", GROUP_TEXT, GROUP_FLAG_DATA};

static bool
sdrel_StartsWith(const char* name, const char* prefix) {
	return strncmp(name, prefix, strlen(prefix)) == 0;
}

/*
 * Map an sdas area name onto an xlink group for the rc8 port.
 * Code-space areas keep sdas' declaration order relative to CODE so
 * the default placement matches sdld; RAM areas go to DATA. The
 * rc8 backend also emits numbered code areas (CODE0..., CABS0...).
 */
static Group*
areaGroup(const char* name) {
	if (strcmp(name, "HOME") == 0)
		return &g_homeGroup;
	if (sdrel_StartsWith(name, "CODE") || sdrel_StartsWith(name, "CABS") || strcmp(name, "_CODE") == 0 ||
	    strcmp(name, "GSINIT") == 0 || strcmp(name, "GSFINAL") == 0 || strcmp(name, "CONST") == 0 ||
	    strcmp(name, "INITIALIZED") == 0 || strcmp(name, "INITIALIZER") == 0)
		return &g_codeGroup;
	if (strcmp(name, "DATA") == 0 || strcmp(name, "SSEG") == 0 || strcmp(name, "DABS") == 0)
		return &g_dataGroup;
	return NULL;
}

struct SRelGlobalArea {
	struct SRelGlobalArea* next;
	char name[MAX_SYMBOL_NAME_LENGTH];
	SSection* section;     /* single section shared by all modules declaring the area */
	bool absolute;
	uint32_t totalSize;    /* accumulated size, exported as l_<AREA> */
	uint32_t sizeSymbol;   /* index of l_<AREA> in the size section; ~0 if none */
};

typedef struct {
	char name[MAX_SYMBOL_NAME_LENGTH];
	struct SRelGlobalArea* global;
	uint32_t declaredSize;   /* this module's declared size for the area */
	uint32_t base;         /* section size when this module declared the area */
	uint32_t writtenEnd;   /* highest base-relative emitted end, this module */
	uint32_t totalPatches;
	uint32_t allocatedPatches;
	SPatch* patches;
} SRelArea;

typedef struct {
	char name[MAX_SYMBOL_NAME_LENGTH];
	bool defined;
	SSection* section;     /* where the definition was exported, NULL for imports */
} SRelSymbol;

typedef struct {
	const char* filename;
	uint32_t fileId;
	char module[MAX_SYMBOL_NAME_LENGTH];

	SRelArea* areas;         /* one entry per distinct area name */
	uint32_t totalAreas;
	uint32_t allocatedAreas;
	uint32_t totalAreaRecords; /* A records seen, including re-declarations */
	uint32_t* areaIndex;     /* A-record ordinal -> area entry */

	SRelSymbol* symbols;
	uint32_t totalSymbols;
	uint32_t allocatedSymbols;

	SSection* absoluteSection; // holds absolute symbols and imports, created on demand
	int currentArea;

	uint8_t* tdata; // raw bytes of the preceding T record, offset field included
	uint32_t tcount;
} SRelModule;

static NORETURN(void sdrel_Error(SRelModule* module, const char* fmt, ...));

static void
sdrel_Error(SRelModule* module, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "%s: ?RREL-", module->filename);
	vfprintf(stderr, fmt, args);
	fputc('\n', stderr);
	va_end(args);
	exit(EXIT_FAILURE);
}

/*
 * Line input and tokenization
 */

static char*
sdrel_ReadLine(char** buffer, size_t* capacity, FILE* fileHandle) {
	size_t length = 0;

	for (;;) {
		if (length + 1 >= *capacity) {
			*capacity = *capacity == 0 ? 1024 : *capacity * 2;
			*buffer = mem_Realloc(*buffer, *capacity);
		}
		if (fgets(*buffer + length, (int) (*capacity - length), fileHandle) == NULL)
			return length != 0 ? *buffer : NULL;
		length += strlen(*buffer + length);
		if (length > 0 && (*buffer)[length - 1] == '\n') {
			(*buffer)[length - 1] = 0;
			return *buffer;
		}
	}
}

static char*
sdrel_NextToken(char** cursor) {
	char* p = *cursor;
	char* start;

	while (*p == ' ' || *p == '\t')
		++p;
	if (*p == 0)
		return NULL;

	start = p;
	while (*p != 0 && !isspace((unsigned char) *p))
		++p;
	if (*p != 0)
		*p++ = 0;
	*cursor = p;
	return start;
}

static unsigned long
sdrel_ParseHex(SRelModule* module, const char* token) {
	char* end;
	unsigned long value = strtoul(token, &end, 16);

	if (end == token || *end != 0)
		sdrel_Error(module, "invalid hex value \"%s\"", token);
	return value;
}

/*
 * Symbol and patch construction
 */

static SSymbol*
sdrel_AddSymbol(SSection* section, const char* name, ESymbolType type, int32_t value, bool resolved) {
	section->symbols = mem_Realloc(section->symbols, sizeof(SSymbol) * (section->totalSymbols + 1));
	SSymbol* symbol = &section->symbols[section->totalSymbols++];

	strncpy(symbol->name, name, MAX_SYMBOL_NAME_LENGTH - 1);
	symbol->name[MAX_SYMBOL_NAME_LENGTH - 1] = 0;
	symbol->type = type;
	symbol->value = value;
	symbol->resolved = resolved;
	symbol->fileInfoIndex = UINT32_MAX;
	symbol->lineNumber = 0;
	symbol->section = NULL;
	symbol->expression = NULL;

	return symbol;
}

/*
 * A relocation in a section cannot reference a symbol in another
 * section directly; OBJ_SYMBOL indexes the symbols of the patched
 * section. For cross-section references a stub symbol is placed in
 * the patched section: SYM_LOCALIMPORT resolves to a definition in
 * the same file (an area anchor or a locally defined global), while
 * SYM_IMPORT resolves through the global symbol table.
 */
static uint32_t
sdrel_EnsureStub(SSection* section, const char* name, ESymbolType type) {
	for (uint32_t i = 0; i < section->totalSymbols; ++i) {
		SSymbol* symbol = &section->symbols[i];
		if (symbol->type == type && strcmp(symbol->name, name) == 0)
			return i;
	}

	sdrel_AddSymbol(section, name, type, 0, false);
	return section->totalSymbols - 1;
}

static void
sdrel_AddPatch(SRelModule* module, SRelArea* area, uint32_t offset, EPatchType type, const uint8_t* expression,
               uint32_t expressionSize) {
	(void) module;

	if (area->totalPatches == area->allocatedPatches) {
		area->allocatedPatches = area->allocatedPatches == 0 ? 8 : area->allocatedPatches * 2;
		area->patches = mem_Realloc(area->patches, sizeof(SPatch) * area->allocatedPatches);
	}

	SPatch* patch = &area->patches[area->totalPatches++];
	patch->type = type;
	patch->valueSymbol = NULL;
	patch->valueSection = NULL;
	patch->offset = offset;
	patch->expressionSize = expressionSize;
	patch->expression = mem_Alloc(expressionSize);
	memcpy(patch->expression, expression, expressionSize);
}

static void
sdrel_AppendOpLong(uint8_t** p, uint8_t op, int32_t value) {
	*(*p)++ = op;
	*(*p)++ = value & 0xFF;
	*(*p)++ = (value >> 8) & 0xFF;
	*(*p)++ = (value >> 16) & 0xFF;
	*(*p)++ = (value >> 24) & 0xFF;
}

/*
 * Build the patch expression for a relocation mode.
 *
 * sdld computes the patched value as
 *	anchor + object - (pcAdjust + objectAddress)   (PC relative)
 *	anchor + object                                  (otherwise)
 * where object is the little-endian object bytes at the relocation
 * index (the assembler's addend) and only the selected byte of a
 * BYTX object is emitted. The anchor of an area reference is the
 * *module's own* placement inside the merged area, so the module's
 * base is folded into the object constant. OBJ_PC_REL subtracts the
 * section base plus the patch offset, i.e. the final address of the
 * patched byte, so the constant folds in -pcAdjust + selectedBytePosition.
 * The byte selection (mask or shift) reproduces sdld's per-byte addition.
 */
static uint32_t
sdrel_BuildExpression(uint8_t* expression, int mode, uint32_t symbolIndex, int32_t objectValue, int32_t areaBase) {
	bool isByte = (mode & R_BYTE) != 0;
	bool isMsb = (mode & R_MSB) != 0;
	uint8_t* p = expression;

	/* Area references resolve to the declaring module's contribution
	 * start; symbol references already carry that offset. */
	sdrel_AppendOpLong(&p, OBJ_SYMBOL, (int32_t) symbolIndex);
	sdrel_AppendOpLong(&p, OBJ_CONSTANT, objectValue + ((mode & R_SYM) ? 0 : areaBase));
	*p++ = OBJ_OP_ADD;

	if (mode & R_PCR) {
		int32_t pcrAdjust = isByte ? 1 : 2;
		int32_t selPos = isMsb ? 1 : 0;
		sdrel_AppendOpLong(&p, OBJ_CONSTANT, selPos - pcrAdjust);
		*p++ = OBJ_PC_REL;
	}

	if (isByte) {
		if (isMsb) {
			sdrel_AppendOpLong(&p, OBJ_CONSTANT, 8);
			*p++ = OBJ_OP_ASR;
		}
		sdrel_AppendOpLong(&p, OBJ_CONSTANT, 0xFF);
		*p++ = OBJ_OP_AND;
	}

	return (uint32_t) (p - expression);
}

/*
 * Area registry
 */

static struct SRelGlobalArea* g_globalAreas; /* areas merged across modules, as sdld does */
static SSection* g_sizeSection;              /* absolute l_<AREA> symbols, created on demand */

static SSection*
sdrel_GetSizeSection(void) {
	if (g_sizeSection == NULL) {
		SSection* section = sect_CreateNew();
		section->group = NULL; /* special exported EQU symbol section */
		strncpy(section->name, "__area_sizes__", MAX_SYMBOL_NAME_LENGTH - 1);
		section->name[MAX_SYMBOL_NAME_LENGTH - 1] = 0;
		section->minimumWordSize = 1;
		section->cpuByteLocation = -1;
		section->cpuBank = -1;
		section->cpuLocation = -1;
		section->imageLocation = -1;
		section->byteAlign = -1;
		section->page = -1;
		g_sizeSection = section;
	}
	return g_sizeSection;
}

/*
 * Find or create the linker-wide area of a given name. sdld merges all
 * modules' contributions to a same-named area into one contiguous area;
 * the single section plays the same role. Absolute areas are placed at
 * their own address and must agree across modules.
 */
static struct SRelGlobalArea*
sdrel_FindGlobalArea(SRelModule* module, const char* name, bool absolute, uint32_t addr) {
	for (struct SRelGlobalArea* area = g_globalAreas; area != NULL; area = area->next) {
		if (strcmp(area->name, name) == 0) {
			if (absolute && (uint32_t) area->section->cpuLocation != addr)
				sdrel_Error(module, "absolute area \"%s\" has conflicting addresses", name);
			return area;
		}
	}

	struct SRelGlobalArea* area = mem_Alloc(sizeof(struct SRelGlobalArea));
	area->next = g_globalAreas;
	g_globalAreas = area;
	strncpy(area->name, name, MAX_SYMBOL_NAME_LENGTH - 1);
	area->name[MAX_SYMBOL_NAME_LENGTH - 1] = 0;
	area->absolute = absolute;
	area->totalSize = 0;
	area->sizeSymbol = UINT32_MAX;

	Group* group = areaGroup(name);
	if (group == NULL)
		sdrel_Error(module, "area \"%s\" is not supported by this port", name);

	SSection* section = sect_CreateNew();
	section->group = group;
	section->fileId = module->fileId;
	strncpy(section->name, name, MAX_SYMBOL_NAME_LENGTH - 1);
	section->name[MAX_SYMBOL_NAME_LENGTH - 1] = 0;
	section->cpuByteLocation = absolute ? (int32_t) addr : -1;
	section->cpuBank = -1;
	section->cpuLocation = absolute ? (int32_t) addr : -1;
	section->imageLocation = -1;
	section->minimumWordSize = 1;
	section->byteAlign = -1;
	section->page = -1;
	section->root = false;
	section->totalSymbols = 0;
	section->symbols = NULL;
	section->totalLineMappings = 0;
	section->lineMappings = NULL;
	section->size = 0;
	section->data = NULL;
	section->patches = NULL;
	area->section = section;

	char anchorName[MAX_SYMBOL_NAME_LENGTH + 16];
	snprintf(anchorName, sizeof(anchorName), "__area_%s", name);
	sdrel_AddSymbol(section, anchorName, SYM_EXPORT, 0, false);

	/* sdld synthesizes s_<AREA> (the area start address) and l_<AREA>
	 * (the area length) as globals once areas are placed; the start is
	 * simply an offset-0 export of the section, the size accumulates
	 * in the absolute size section as modules are read. */
	char symName[MAX_SYMBOL_NAME_LENGTH + 4];
	snprintf(symName, sizeof(symName), "s_%s", name);
	sdrel_AddSymbol(section, symName, SYM_EXPORT, 0, false);
	snprintf(symName, sizeof(symName), "l_%s", name);
	SSection* sizeSection = sdrel_GetSizeSection();
	sdrel_AddSymbol(sizeSection, symName, SYM_EXPORT, 0, true);
	area->sizeSymbol = sizeSection->totalSymbols - 1;

	return area;
}

/*
 * Record processing
 */

static void
sdrel_ProcessTLine(SRelModule* module, char* line) {
	char* token;
	uint32_t count = 0;
	uint32_t allocated = 0;

	while ((token = sdrel_NextToken(&line)) != NULL) {
		unsigned long value = sdrel_ParseHex(module, token);
		if (value > 0xFF)
			sdrel_Error(module, "T record byte value out of range");
		if (count == allocated) {
			allocated = allocated == 0 ? 64 : allocated * 2;
			module->tdata = mem_Realloc(module->tdata, allocated);
		}
		module->tdata[count++] = (uint8_t) value;
	}

	if (count < 2)
		sdrel_Error(module, "T record without offset field");

	module->tcount = count;
}

static bool
sdrel_ModeSupported(int mode) {
	switch (mode) {
		case 0x00: /* word, area reference */
		case 0x02: /* word, symbol reference */
		case 0x09: /* byte, BYTX, low byte, area reference */
		case 0x0B: /* byte, BYTX, low byte, symbol reference */
		case 0x0D: /* byte, BYTX, low byte, PC relative, area reference */
		case 0x89: /* byte, BYTX, high byte, area reference */
		case 0x8B: /* byte, BYTX, high byte, symbol reference */
			return true;
		default:
			return false;
	}
}

/*
 * Resolve an A-record ordinal (as used by R records) to this module's
 * area entry. Several A records may describe one area (re-entry).
 */
static SRelArea*
sdrel_AreaByIndex(SRelModule* module, int index) {
	if (index < 0 || (uint32_t) index >= module->totalAreaRecords)
		sdrel_Error(module, "area index %d out of range", index);
	return &module->areas[module->areaIndex[index]];
}

static void
sdrel_ProcessRLine(SRelModule* module, char* line) {
	typedef struct {
		int mode;
		int rtp;
		int index;
	} SGroup;
	SGroup* groups = NULL;
	uint32_t totalGroups = 0;
	uint32_t allocatedGroups = 0;
	char* token;

	/* Read the 4-token groups, unescaping modes as needed. */
	while ((token = sdrel_NextToken(&line)) != NULL) {
		unsigned long mode = sdrel_ParseHex(module, token);
		int rtp, index;

		if ((mode & R_ESCAPE_MASK) == R_ESCAPE_MASK) {
			token = sdrel_NextToken(&line);
			if (token == NULL)
				sdrel_Error(module, "truncated escaped relocation mode");
			mode = ((mode & ~R_ESCAPE_MASK) << 8) | sdrel_ParseHex(module, token);
		}

		token = sdrel_NextToken(&line);
		if (token == NULL)
			sdrel_Error(module, "truncated relocation group");
		rtp = (int) sdrel_ParseHex(module, token);

		token = sdrel_NextToken(&line);
		if (token == NULL)
			sdrel_Error(module, "truncated relocation group");
		index = (int) sdrel_ParseHex(module, token);
		token = sdrel_NextToken(&line);
		if (token == NULL)
			sdrel_Error(module, "truncated relocation group");
		index |= (int) sdrel_ParseHex(module, token) << 8;

		if (totalGroups == allocatedGroups) {
			allocatedGroups = allocatedGroups == 0 ? 8 : allocatedGroups * 2;
			groups = mem_Realloc(groups, sizeof(SGroup) * allocatedGroups);
		}
		groups[totalGroups++] = (SGroup) {
		    (int) mode, rtp, index,
		};
	}

	if (totalGroups == 0)
		sdrel_Error(module, "R record without area anchor");

	/* The first group must be the implicit anchor: a word relocation
	 * of the T record offset field (index 0) against the area the
	 * record belongs to. */
	if (groups[0].mode != 0x00 || groups[0].rtp != 0)
		sdrel_Error(module, "R record does not start with an area anchor");

	SRelArea* area = sdrel_AreaByIndex(module, groups[0].index);
	SSection* section = area->global->section;
	uint32_t rawCount = module->tcount;
	uint32_t logicalOffset = module->tdata[0] | (module->tdata[1] << 8);

	/* sdas record offsets count emitted (compacted) bytes, so the record
	 * lands at the module's base plus its offset. */
	uint32_t recordStart = area->base + logicalOffset;
	uint32_t recordDrops = 0;

	/* Mark the padding byte of every BYTX object; only the selected
	 * byte survives into the output (sdld rtflg[] compaction). */
	uint8_t* dropped = mem_Alloc(rawCount);
	memset(dropped, 0, rawCount);
	dropped[0] = dropped[1] = 1;

	for (uint32_t g = 1; g < totalGroups; ++g) {
		int mode = groups[g].mode;
		if (!sdrel_ModeSupported(mode))
			sdrel_Error(module, "unsupported relocation mode 0x%X", mode);
		uint32_t span = (mode & R_BYTE) ? ((mode & R_BYTX) ? 2 : 1) : 2;
		if (groups[g].rtp < 2 || (uint32_t) groups[g].rtp + span > rawCount)
			sdrel_Error(module, "relocation index %d outside T record", groups[g].rtp);
		if ((mode & R_BYTE) && (mode & R_BYTX)) {
			int selPos = (mode & R_MSB) ? 1 : 0;
			dropped[groups[g].rtp + (1 - selPos)] = 1;
		}
	}

	/* Append the compacted record bytes, checking contiguity against
	 * the relocated record offset. */
	uint32_t cursor = section->size;
	if (recordStart < cursor)
		sdrel_Error(module, "overlapping text record");
	while (recordStart > cursor) {
		section->data = mem_Realloc(section->data, cursor + 1);
		section->data[cursor] = 0;
		++cursor;
	}
	for (uint32_t k = 2; k < rawCount; ++k) {
		if (dropped[k]) {
			++recordDrops;
			continue;
		}
		section->data = mem_Realloc(section->data, cursor + 1);
		section->data[cursor] = module->tdata[k];
		++cursor;
	}
	section->size = cursor;
		uint32_t end = logicalOffset + (rawCount - 2) - recordDrops;
	if (end > area->writtenEnd)
		area->writtenEnd = end;

	/* Emit the patches with offsets in the compacted address space. */
	for (uint32_t g = 1; g < totalGroups; ++g) {
		int mode = groups[g].mode;
		bool isByte = (mode & R_BYTE) != 0;
		bool isMsb = (mode & R_MSB) != 0;
		uint32_t span = isByte ? ((mode & R_BYTX) ? 2 : 1) : 2;
		int selPos = (isByte && isMsb) ? 1 : 0;
		uint32_t objectValue = 0;

		for (uint32_t b = 0; b < span; ++b)
			objectValue |= (uint32_t) module->tdata[groups[g].rtp + b] << (8 * b);

		uint32_t symbolIndex;
		if (mode & R_SYM) {
			if ((uint32_t) groups[g].index >= module->totalSymbols)
				sdrel_Error(module, "symbol index %d out of range", groups[g].index);
			const SRelSymbol* symbol = &module->symbols[groups[g].index];
			symbolIndex = sdrel_EnsureStub(section, symbol->name,
			                               symbol->defined && symbol->section != NULL &&
			                                       symbol->section->fileId == section->fileId
			                                           ? SYM_LOCALIMPORT
			                                           : SYM_IMPORT);
		} else {
			SRelArea* refArea = sdrel_AreaByIndex(module, groups[g].index);
			char anchor[MAX_SYMBOL_NAME_LENGTH + 16];
			snprintf(anchor, sizeof(anchor), "__area_%s", refArea->name);
			symbolIndex = sdrel_EnsureStub(section, anchor, SYM_IMPORT);
		}

		uint32_t patchOffset = recordStart + groups[g].rtp + selPos - 2;
		for (uint32_t k = 2; k < groups[g].rtp + (uint32_t) selPos; ++k) {
			if (dropped[k])
				--patchOffset;
		}

		uint8_t expression[32];
		SRelArea* target = (mode & R_SYM) ? NULL : sdrel_AreaByIndex(module, groups[g].index);
		uint32_t expressionSize = sdrel_BuildExpression(expression, mode, symbolIndex, (int32_t) objectValue,
		                                                target != NULL ? (int32_t) target->base : 0);
		sdrel_AddPatch(module, area, patchOffset, isByte ? PATCH_8 : PATCH_LE_16, expression, expressionSize);
	}

	mem_Free(groups);
	mem_Free(dropped);
}

/*
 * Header records
 */

static void
sdrel_ProcessALine(SRelModule* module, char* line) {
	char* token = sdrel_NextToken(&line);
	if (token == NULL)
		sdrel_Error(module, "A record without area name");

	char name[MAX_SYMBOL_NAME_LENGTH];
	strncpy(name, token, MAX_SYMBOL_NAME_LENGTH - 1);
	name[MAX_SYMBOL_NAME_LENGTH - 1] = 0;

	uint32_t size = 0;
	uint32_t flags = 0;
	uint32_t addr = 0;

	while ((token = sdrel_NextToken(&line)) != NULL) {
		if (strcmp(token, "size") == 0 || strcmp(token, "flags") == 0 || strcmp(token, "addr") == 0) {
			char* value = sdrel_NextToken(&line);
			if (value == NULL)
				sdrel_Error(module, "A record field \"%s\" without value", token);
			uint32_t parsed = (uint32_t) sdrel_ParseHex(module, value);
			if (strcmp(token, "size") == 0)
				size = parsed;
			else if (strcmp(token, "flags") == 0)
				flags = parsed;
			else
				addr = parsed;
		} else {
			sdrel_Error(module, "unknown A record field \"%s\"", token);
		}
	}

	if (flags & ~A_FLAGS_KNOWN)
		sdrel_Error(module, "area \"%s\" has unsupported flags 0x%X", name, flags);
	if (flags & A_WORDLENGTH)
		sdrel_Error(module, "area \"%s\" has a non-byte word length", name);
	if ((flags & A_PAG) == A_PAG)
		sdrel_Error(module, "paged area \"%s\" is not supported", name);
	if ((flags & A_OVR) == A_OVR)
		sdrel_Error(module, "overlay area \"%s\" is not supported", name);

	bool absolute = (flags & A_ABS) == A_ABS;

	/* sdas re-emits the A record every time the module re-enters an
	 * area, each time carrying the area's then-current size; all such
	 * records describe one area of this module. */
	uint32_t entry;
	bool found = false;
	for (entry = 0; entry < module->totalAreas; ++entry) {
		if (strcmp(module->areas[entry].name, name) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		if (module->totalAreas == module->allocatedAreas) {
			module->allocatedAreas = module->allocatedAreas == 0 ? 16 : module->allocatedAreas * 2;
			module->areas = mem_Realloc(module->areas, sizeof(SRelArea) * module->allocatedAreas);
		}
		entry = module->totalAreas++;
		SRelArea* area = &module->areas[entry];
		strncpy(area->name, name, MAX_SYMBOL_NAME_LENGTH - 1);
		area->name[MAX_SYMBOL_NAME_LENGTH - 1] = 0;
		area->global = sdrel_FindGlobalArea(module, name, absolute, addr);
		area->declaredSize = size;
		area->base = (uint32_t) area->global->section->size;
		area->writtenEnd = 0;
		area->totalPatches = 0;
		area->allocatedPatches = 0;
		area->patches = NULL;
	} else if (size > module->areas[entry].declaredSize) {
		module->areas[entry].declaredSize = size;
	}

	module->areaIndex = mem_Realloc(module->areaIndex, sizeof(uint32_t) * (module->totalAreaRecords + 1));
	module->areaIndex[module->totalAreaRecords++] = entry;

	module->currentArea = (int) entry;
}

static SSection*
sdrel_GetAbsoluteSection(SRelModule* module) {
	if (module->absoluteSection == NULL) {
		SSection* section = sect_CreateNew();
		section->group = NULL; /* special exported EQU symbol section */
		section->fileId = module->fileId;
		strncpy(section->name, "__absolute__", MAX_SYMBOL_NAME_LENGTH - 1);
		section->minimumWordSize = 1;
		section->cpuByteLocation = -1;
		section->cpuBank = -1;
		section->cpuLocation = -1;
		section->imageLocation = -1;
		section->byteAlign = -1;
		section->page = -1;
		module->absoluteSection = section;
	}
	return module->absoluteSection;
}

static void
sdrel_ProcessSLine(SRelModule* module, char* line) {
	char* name = sdrel_NextToken(&line);
	char* def = sdrel_NextToken(&line);

	if (name == NULL || def == NULL)
		sdrel_Error(module, "malformed S record");

	bool defined = strncmp(def, "Def", 3) == 0;
	if (!defined && strncmp(def, "Ref", 3) != 0)
		sdrel_Error(module, "S record symbol \"%s\" is neither Def nor Ref", name);

	uint32_t value = (uint32_t) sdrel_ParseHex(module, def + 3);

	if (module->totalSymbols == module->allocatedSymbols) {
		module->allocatedSymbols = module->allocatedSymbols == 0 ? 16 : module->allocatedSymbols * 2;
		module->symbols = mem_Realloc(module->symbols, sizeof(SRelSymbol) * module->allocatedSymbols);
	}
	SRelSymbol* symbol = &module->symbols[module->totalSymbols++];
	strncpy(symbol->name, name, MAX_SYMBOL_NAME_LENGTH - 1);
	symbol->name[MAX_SYMBOL_NAME_LENGTH - 1] = 0;
	symbol->defined = defined;
	symbol->section = NULL;

	if (defined && module->currentArea >= 0) {
		SRelArea* area = &module->areas[module->currentArea];
		symbol->section = area->global->section;
		sdrel_AddSymbol(symbol->section, name, SYM_EXPORT, (int32_t) (value + area->base), false);
		return;
	}
	if (defined) {
		symbol->section = sdrel_GetAbsoluteSection(module);
		sdrel_AddSymbol(symbol->section, name, SYM_EXPORT, (int32_t) value, true);
	} else
		sdrel_AddSymbol(sdrel_GetAbsoluteSection(module), name, SYM_IMPORT, 0, false);
}

/*
 * Main entry point
 */

void
sdrel_Read(FILE* fileHandle, const char* filename, uint32_t fileId) {
	SRelModule module = {
	    .filename = filename,
	    .fileId = fileId,
	    .module = {0},
	    .areas = NULL,
	    .totalAreas = 0,
	    .allocatedAreas = 0,
	    .totalAreaRecords = 0,
	    .areaIndex = NULL,
	    .symbols = NULL,
	    .totalSymbols = 0,
	    .allocatedSymbols = 0,
	    .absoluteSection = NULL,
	    .currentArea = -1,
	    .tdata = NULL,
	    .tcount = 0,
	};
	SFileInfo* fileInfo = obj_AllocateFileInfo(1);
	fileInfo->fileName = str_Create(filename);

	char* lineBuffer = NULL;
	size_t capacity = 0;

	char* line = sdrel_ReadLine(&lineBuffer, &capacity, fileHandle);
	if (line == NULL)
		sdrel_Error(&module, "empty object file");

	if (strlen(line) < 3 || line[0] != 'X' || line[1] != 'L' || line[2] != '2')
		sdrel_Error(&module,
		            "unsupported RREL magic \"%.3s\" (only \"XL2\": hex radix, little-endian, 16-bit addresses)",
		            line);

	while ((line = sdrel_ReadLine(&lineBuffer, &capacity, fileHandle)) != NULL) {
		if (line[0] == 0)
			continue;

		switch (line[0]) {
			case 'H': /* header: area and symbol counts */
			case 'O': /* options */
				break;
			case 'M': {
				char* p = line + 1;
				char* token = sdrel_NextToken(&p);
				if (token != NULL) {
					strncpy(module.module, token, MAX_SYMBOL_NAME_LENGTH - 1);
					module.module[MAX_SYMBOL_NAME_LENGTH - 1] = 0;
				}
				break;
			}
			case 'A':
				sdrel_ProcessALine(&module, line + 1);
				break;
			case 'S':
				sdrel_ProcessSLine(&module, line + 1);
				break;
			case 'T':
				sdrel_ProcessTLine(&module, line + 1);
				break;
			case 'R':
				sdrel_ProcessRLine(&module, line + 1);
				break;
			case 'P':
				sdrel_Error(&module, "paging (P record) is not supported");
				break;
			case 'G':
			case 'B':
				sdrel_Error(&module, "version 4 records (G/B lines) are not supported");
				break;
			default:
				sdrel_Error(&module, "unknown record type '%c'", line[0]);
				break;
		}
	}

	/* Finalize the areas: pad reserved space, accumulate the merged
	 * area sizes, and hand the patches to the section. */
	for (uint32_t i = 0; i < module.totalAreas; ++i) {
		SRelArea* area = &module.areas[i];
		SSection* section = area->global->section;
		uint32_t contribution = area->writtenEnd > area->declaredSize ? area->writtenEnd : area->declaredSize;

		while ((uint32_t) section->size < area->base + contribution) {
			section->data = mem_Realloc(section->data, section->size + 1);
			section->data[section->size] = 0;
			++section->size;
		}
		area->global->totalSize += contribution;
		if (area->global->sizeSymbol != UINT32_MAX)
			sdrel_GetSizeSection()->symbols[area->global->sizeSymbol].value = (int32_t) area->global->totalSize;

		if (area->totalPatches != 0) {
			uint32_t old = section->patches == NULL ? 0 : section->patches->totalPatches;
			SPatches* fresh = patch_Alloc(old + area->totalPatches);
			if (old != 0) {
				memcpy(fresh->patches, section->patches->patches, sizeof(SPatch) * old);
				mem_Free(section->patches);
			}
			memcpy(fresh->patches + old, area->patches, sizeof(SPatch) * area->totalPatches);
			section->patches = fresh;
		}
	}
}
