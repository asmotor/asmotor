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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// from util
#include "file.h"
#include "mem.h"
#include "str.h"

// from xlink
#include "elf.h"
#include "object.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"
#include "xlink.h"

// A good source of information on the ELF format is the glib.c elf.h header

static Group g_codeGroup = {"CODE", GROUP_TEXT, 0};
static Group g_dataGroup = {"DATA", GROUP_TEXT, GROUP_FLAG_DATA};
static Group g_bssGroup = {"BSS", GROUP_BSS, 0};

typedef uint32_t e_word_t;
typedef uint32_t e_off_t;
typedef uint32_t e_addr_t;
typedef uint16_t e_half_t;

struct ElfSectionHeader;

typedef struct {
	SSection* section;
	uint32_t symbolIndex;
} SSymbolHandle;

typedef struct {
	const char* name;
	const struct ElfSectionHeader* section;
	SSymbolHandle xlinkSymbol;

	e_word_t st_nameindex;
	e_addr_t st_value;
	e_half_t st_shndx;

	uint8_t bind;
	uint8_t type;
} ElfSymbol;

typedef struct ElfStringSection {
	size_t length;
	char data[];
} ElfStringSection;

typedef struct {
	const ElfStringSection* stringSection;

	size_t totalSymbols;
	ElfSymbol data[];
} ElfSymbolSection;

typedef struct {
	SSection* xlinkSection;
	uint32_t allocatedSymbols;
	size_t length;
	uint8_t data[];
} ElfProgBitsSection;

typedef struct {
	ElfSymbol* symbol;

	e_addr_t r_offset;

	e_word_t symbolIndex;
	e_word_t type;
} ElfRelocEntry;

typedef struct {
	ElfSymbolSection* symbols;
	struct ElfSectionHeader* section;

	size_t totalRelocations;
	ElfRelocEntry data[];
} ElfRelocSection;

typedef struct {
	ElfSymbol* symbol;

	e_addr_t r_offset;
	int32_t r_addend;

	e_word_t symbolIndex;
	e_word_t type;
} ElfRelocAddendEntry;

typedef struct {
	const ElfSymbolSection* symbols;
	const struct ElfSectionHeader* section;

	size_t totalRelocations;
	ElfRelocAddendEntry data[];
} ElfRelocAddendSection;

typedef struct ElfSectionHeader {
	e_word_t sh_name;
	e_word_t sh_type;
	e_word_t sh_flags;
	e_addr_t sh_addr;
	e_off_t sh_offset;
	e_word_t sh_size;
	e_word_t sh_link;
	e_word_t sh_info;
	e_word_t sh_addralign;
	e_word_t sh_entsize;

	const char* name;

	union {
		ElfStringSection* strings;
		ElfProgBitsSection* progbits;
		ElfSymbolSection* symbols;
		ElfRelocSection* relocations;
		ElfRelocAddendSection* relocationAddends;
	} data;
} ElfSectionHeader;

typedef struct {
	uint32_t totalSectionHeaders;
	e_half_t stringSectionHeaderIndex;
	ElfSectionHeader* headers;
} ElfHeader;

static SSymbol*
symbolOfHandle(const SSymbolHandle* handle) {
	return &handle->section->symbols[handle->symbolIndex];
}

static uint32_t
read_word_b(const uint8_t* mem, size_t offset) {
	uint32_t r;

	mem += offset;

	r = *mem++ << 24u;
	r |= *mem++ << 16u;
	r |= *mem++ << 8u;
	r |= *mem++;

	return r;
}

static uint32_t
read_word_l(const uint8_t* mem, size_t offset) {
	uint32_t r;

	mem += offset;
	r = *mem++;
	r |= *mem++ << 8u;
	r |= *mem++ << 16u;
	r |= *mem++ << 24u;

	return r;
}

static uint16_t
read_half_b(const uint8_t* mem, size_t offset) {
	uint16_t r;

	mem += offset;
	r = *mem++ << 8u;
	r |= *mem++;

	return r;
}

static uint16_t
read_half_l(const uint8_t* mem, size_t offset) {
	uint16_t r;

	mem += offset;
	r = *mem++;
	r |= *mem++ << 8u;

	return r;
}

static uint32_t (*fget_word)(FILE*);
static uint16_t (*fget_half)(FILE*);

static uint32_t (*read_word)(const uint8_t*, size_t);
static uint16_t (*read_half)(const uint8_t*, size_t);

#define fget_addr fget_word
#define fget_off  fget_word

#define EI_MAG0         0
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6
#define EI_PAD          7
#define EI_NIDENT       16
#define ELF_HD_SHOFF    32
#define ELF_HD_SHNUM    48
#define ELF_HD_SHSTRNDX 50
#define ELF_HD_SIZE     52

#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_68K 4

#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_NONE    0
#define EV_CURRENT 1

#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_HASH     5
#define SHT_DYNAMIC  6
#define SHT_NOTE     7
#define SHT_NOBITS   8
#define SHT_REL      9
#define SHT_SHLIB    10
#define SHT_DYNSYM   11
#define SHT_LOPROC   0x70000000
#define SHT_HIPROC   0x7fffffff
#define SHT_LOUSER   0x80000000
#define SHT_HIUSER   0xffffffff

#define SHF_WRITE     0x1
#define SHF_ALLOC     0x2
#define SHF_EXECINSTR 0x4
#define SHF_MERGE     0x10
#define SHF_STRINGS   0x20
#define SHF_MASKPROC  0xf0000000

// Symbol tab bind field
#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STB_NUM    3
#define STB_LOPROC 13
#define STB_HIPROC 15

// Symbol tab type field
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3

#define SHN_UNDEF     0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC    0xff00
#define SHN_HIPROC    0xff1f
#define SHN_ABS       0xfff1
#define SHN_COMMON    0xfff2
#define SHN_HIRESERVE 0xffff

#define ELF32_ST_BIND(i)    ((i) >> 4)
#define ELF32_ST_TYPE(i)    ((i) & 0xf)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

#define ELF32_R_SYM(i)     ((i) >> 8)
#define ELF32_R_TYPE(i)    ((unsigned char) (i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (unsigned char) (t))

#define R_68K_NONE 0 /* No reloc */
#define R_68K_32   1 /* Direct 32 bit  */
#define R_68K_16   2 /* Direct 16 bit  */
#define R_68K_8    3 /* Direct 8 bit  */
#define R_68K_PC32 4 /* PC relative 32 bit */
#define R_68K_PC16 5 /* PC relative 16 bit */
#define R_68K_PC8  6 /* PC relative 8 bit */

static bool
readHeader(FILE* fileHandle, e_off_t* sectionHeadersOffset, e_half_t* sectionHeaderEntrySize, e_half_t* totalSectionHeaders,
           e_half_t* stringSectionHeaderIndex) {
	fseek(fileHandle, EI_CLASS, SEEK_SET);
	uint8_t id_class = fgetc(fileHandle);
	uint8_t id_endian = fgetc(fileHandle);
	uint8_t id_version = fgetc(fileHandle);

	if (id_class != ELFCLASS32 || id_version != EV_CURRENT)
		return false;

	if (id_endian == ELFDATA2LSB) {
		fget_half = fgetlw;
		fget_word = fgetll;
		read_half = read_half_l;
		read_word = read_word_l;
	} else if (id_endian == ELFDATA2MSB) {
		fget_half = fgetbw;
		fget_word = fgetbl;
		read_half = read_half_b;
		read_word = read_word_b;
	} else {
		return false;
	}

	fseek(fileHandle, EI_NIDENT, SEEK_SET);

	e_half_t type = fget_half(fileHandle);                         // e_type = ET_REL
	e_half_t arch = fget_half(fileHandle);                         // e_machine
	e_word_t version = fget_word(fileHandle);                      // e_version = EV_CURRENT
	/* e_addr_t entry = */ fget_addr(fileHandle);                  // e_entry
	/* e_off_t programHeaderOffset = */ fget_off(fileHandle);      // e_phoff
	*sectionHeadersOffset = fget_off(fileHandle);                  // e_shoff
	/* e_word_t flags = */ fget_word(fileHandle);                  // e_flags
	e_half_t hdsize = fget_half(fileHandle);                       // e_ehsize
	/* e_half_t programHeaderEntrySize = */ fget_half(fileHandle); // e_phentsize
	/* e_half_t totalProgramHeaders = */ fget_half(fileHandle);    // e_phnum
	*sectionHeaderEntrySize = fget_half(fileHandle);               // e_shentsize = SHDR_SIZEOF
	*totalSectionHeaders = fget_half(fileHandle);                  // e_shnum
	*stringSectionHeaderIndex = fget_half(fileHandle);             // e_shstrndx

	return type == ET_REL && version == EV_CURRENT && arch == EM_68K && hdsize == ELF_HD_SIZE;
}

static ElfSectionHeader*
readSectionHeaders(FILE* fileHandle, e_off_t sectionHeadersOffset, e_half_t sectionHeaderEntrySize, e_half_t totalSectionHeaders) {
	ElfSectionHeader* headers = mem_Alloc(sizeof(ElfSectionHeader) * totalSectionHeaders);
	for (uint16_t i = 0; i < totalSectionHeaders; ++i) {
		fseek(fileHandle, sectionHeadersOffset + i * sectionHeaderEntrySize, SEEK_SET);

		ElfSectionHeader* header = &headers[i];
		header->sh_name = fget_word(fileHandle);
		header->sh_type = fget_word(fileHandle);
		header->sh_flags = fget_word(fileHandle);
		header->sh_addr = fget_addr(fileHandle);
		header->sh_offset = fget_off(fileHandle);
		header->sh_size = fget_word(fileHandle);
		header->sh_link = fget_word(fileHandle);
		header->sh_info = fget_word(fileHandle);
		header->sh_addralign = fget_word(fileHandle);
		header->sh_entsize = fget_word(fileHandle);
	}

	return headers;
}

static void
readProgBitsSection(FILE* fileHandle, ElfSectionHeader* header) {
	ElfProgBitsSection* progbits = mem_Alloc(sizeof(ElfProgBitsSection) + header->sh_size);
	progbits->allocatedSymbols = 0;
	progbits->xlinkSection = NULL;

	if (header->sh_flags & SHF_ALLOC) {
		fseek(fileHandle, header->sh_offset, SEEK_SET);
		progbits->length = header->sh_size;
		if (progbits->length != fread(progbits->data, 1, progbits->length, fileHandle))
			error("readProgBitsSection short file");
	} else {
		progbits->length = 0;
	}
	header->data.progbits = progbits;
}

static void
readNoBitsSection(FILE* fileHandle, ElfSectionHeader* header) {
	ElfProgBitsSection* bits = mem_Alloc(sizeof(ElfProgBitsSection));
	bits->allocatedSymbols = 0;
	bits->xlinkSection = NULL;
	bits->length = 0;
	bits->xlinkSection = NULL;

	header->data.progbits = bits;
}

static void
readStrTabSection(FILE* fileHandle, ElfSectionHeader* header) {
	ElfStringSection* table = mem_Alloc(sizeof(ElfStringSection) + header->sh_size);

	fseek(fileHandle, header->sh_offset, SEEK_SET);
	table->length = header->sh_size;
	if (table->length != fread(table->data, 1, table->length, fileHandle))
		error("readStrTabSection short file");
	header->data.strings = table;
}

static void
readSymTabSection(FILE* fileHandle, ElfSectionHeader* header) {
	/* e_word_t stringSection = header->sh_link; */
	e_word_t totalSymbols = header->sh_size / header->sh_entsize;

	ElfSymbolSection* table = mem_Alloc(sizeof(ElfSymbolSection) + totalSymbols * sizeof(ElfSymbol));
	table->totalSymbols = totalSymbols;

	for (e_word_t i = 0; i < totalSymbols; ++i) {
		ElfSymbol* symbol = &table->data[i];
		fseek(fileHandle, header->sh_offset + i * header->sh_entsize, SEEK_SET);
		symbol->st_nameindex = fget_word(fileHandle);
		symbol->st_value = fget_addr(fileHandle);
		/* size = */ fget_word(fileHandle);
		uint8_t info = fgetc(fileHandle);
		symbol->bind = ELF32_ST_BIND(info);
		symbol->type = ELF32_ST_TYPE(info);
		/* other = */ fgetc(fileHandle);
		symbol->st_shndx = fget_half(fileHandle);

		symbol->name = NULL;
		symbol->section = NULL;
		symbol->xlinkSymbol.section = NULL;
		symbol->xlinkSymbol.symbolIndex = UINT32_MAX;
	}

	header->data.symbols = table;
}

static void
readRelASection(FILE* fileHandle, ElfSectionHeader* header) {
	/* e_word_t stringSection = header->sh_link; */
	e_word_t totalRelocs = header->sh_size / header->sh_entsize;

	ElfRelocAddendSection* table = mem_Alloc(sizeof(ElfRelocAddendSection) + totalRelocs * sizeof(ElfRelocAddendEntry));
	table->totalRelocations = totalRelocs;

	for (e_word_t i = 0; i < totalRelocs; ++i) {
		ElfRelocAddendEntry* reloc = &table->data[i];
		fseek(fileHandle, header->sh_offset + i * header->sh_entsize, SEEK_SET);
		reloc->r_offset = fget_addr(fileHandle);
		e_word_t info = fget_word(fileHandle);
		reloc->symbolIndex = ELF32_R_SYM(info);
		reloc->symbol = NULL;
		reloc->type = ELF32_R_TYPE(info);
		reloc->r_addend = fget_word(fileHandle);
	}

	header->data.relocationAddends = table;
}

static void
readRelSection(FILE* fileHandle, ElfSectionHeader* header) {
	/* e_word_t stringSection = header->sh_link; */
	e_word_t totalRelocs = header->sh_info;

	ElfRelocSection* table = mem_Alloc(sizeof(ElfRelocSection) + totalRelocs * sizeof(ElfRelocEntry));
	table->totalRelocations = totalRelocs;

	for (e_word_t i = 0; i < totalRelocs; ++i) {
		ElfRelocEntry* reloc = &table->data[i];
		fseek(fileHandle, header->sh_offset + i * header->sh_entsize, SEEK_SET);
		reloc->r_offset = fget_addr(fileHandle);
		e_word_t info = fget_word(fileHandle);
		reloc->symbolIndex = ELF32_R_SYM(info);
		reloc->symbol = NULL;
		reloc->type = ELF32_R_TYPE(info);
	}

	header->data.relocations = table;
}

static void
readSections(FILE* fileHandle, ElfSectionHeader* headers, uint_fast16_t totalSections) {
	for (uint_fast16_t i = 0; i < totalSections; ++i) {
		ElfSectionHeader* header = &headers[i];
		switch (header->sh_type) {
			case SHT_PROGBITS:
				readProgBitsSection(fileHandle, header);
				break;
			case SHT_NOBITS:
				readNoBitsSection(fileHandle, header);
				break;
			case SHT_STRTAB:
				readStrTabSection(fileHandle, header);
				break;
			case SHT_SYMTAB:
				readSymTabSection(fileHandle, header);
				break;
			case SHT_RELA:
				readRelASection(fileHandle, header);
				break;
			case SHT_REL:
				readRelSection(fileHandle, header);
				break;
			default:
				break;
		}
	}
}

static const ElfSectionHeader*
getSectionHeader(const ElfHeader* elf, uint32_t index) {
	if (index == SHN_ABS)
		return NULL;

	if (index >= elf->totalSectionHeaders)
		error("getSectionHeader read outside range (%04X)", index);

	return index == 0 ? NULL : &elf->headers[index];
}

static const ElfStringSection*
getStringTable(const ElfHeader* elf, uint32_t index) {
	const ElfSectionHeader* header = getSectionHeader(elf, index);

	if (header->sh_type != SHT_STRTAB)
		error("getStringTable expected string section (%d)", index);

	return header->data.strings;
}

static const ElfSymbolSection*
getSymbolTable(const ElfHeader* elf, uint32_t index) {
	const ElfSectionHeader* header = getSectionHeader(elf, index);
	if (header->sh_type != SHT_SYMTAB)
		error("getSymbolTable expected symbol section (%d)", index);

	return header->data.symbols;
}

static ElfSymbol*
getSymbol(const ElfSymbolSection* symbols, uint32_t index) {
	if (index >= symbols->totalSymbols)
		error("getSymbol read outside range (%d)", index);

	return index == 0 ? NULL : (ElfSymbol*) &symbols->data[index];
}

static const char*
getString(const ElfStringSection* table, uint32_t index) {
	if (index >= table->length)
		error("getString read outside range (%d)", index);

	return &table->data[index];
}

static void
resolveSectionNames(ElfHeader* elf) {
	const ElfStringSection* names = getStringTable(elf, elf->stringSectionHeaderIndex);
	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		ElfSectionHeader* header = &elf->headers[i];
		header->name = getString(names, header->sh_name);
	}
}

static void
resolveSymTabSection(const ElfHeader* elf, ElfSectionHeader* header) {
	ElfSymbolSection* symbols = header->data.symbols;
	symbols->stringSection = getStringTable(elf, header->sh_link);

	for (uint32_t i = 0; i < symbols->totalSymbols; ++i) {
		ElfSymbol* symbol = &symbols->data[i];
		symbol->section = getSectionHeader(elf, symbol->st_shndx);
		if (symbol->type == STT_SECTION && symbol->st_nameindex == 0) {
			symbol->name = symbol->section->name;
		} else {
			symbol->name = getString(symbols->stringSection, symbol->st_nameindex);
		}
	}
}

static void
resolveRelASection(ElfHeader* elf, ElfSectionHeader* header) {
	ElfRelocAddendSection* relocs = header->data.relocationAddends;
	relocs->section = getSectionHeader(elf, header->sh_info);
	relocs->symbols = getSymbolTable(elf, header->sh_link);

	for (uint32_t i = 0; i < relocs->totalRelocations; ++i) {
		ElfRelocAddendEntry* reloc = &relocs->data[i];
		reloc->symbol = getSymbol(relocs->symbols, reloc->symbolIndex);
	}
}

static int32_t
readData(const ElfSectionHeader* text, uint32_t offset, uint32_t type) {
	switch (type) {
		case R_68K_32:
		case R_68K_PC32:
			return read_word(text->data.progbits->data, offset);
		case R_68K_16:
		case R_68K_PC16:
			return read_half(text->data.progbits->data, offset);
		default:
			error("readData unsupported type (%d)", type);
	}
}

static void
resolveRelSection(ElfHeader* elf, ElfSectionHeader* header) {
	const ElfSectionHeader* textSection = getSectionHeader(elf, header->sh_info);

	ElfRelocSection* sourceTable = header->data.relocations;
	ElfRelocAddendSection* destTable =
	    mem_Alloc(sizeof(ElfRelocAddendSection) + sourceTable->totalRelocations * sizeof(ElfRelocAddendEntry));

	for (uint32_t i = 0; i < sourceTable->totalRelocations; ++i) {
		ElfRelocEntry* source = &sourceTable->data[i];
		ElfRelocAddendEntry* dest = &destTable->data[i];

		dest->r_offset = source->r_offset;
		dest->symbolIndex = source->symbolIndex;
		dest->type = source->type;
		dest->r_addend = readData(textSection, source->r_offset, source->type);
	}

	header->sh_type = SHT_RELA;
	header->data.relocationAddends = destTable;
	resolveRelASection(elf, header);
}

static void
resolveNamesAndIndices(ElfHeader* elf) {
	resolveSectionNames(elf);

	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		ElfSectionHeader* header = &elf->headers[i];
		switch (header->sh_type) {
			case SHT_SYMTAB:
				resolveSymTabSection(elf, header);
				break;
			case SHT_RELA:
				resolveRelASection(elf, header);
				break;
			case SHT_REL:
				resolveRelSection(elf, header);
				break;
			default:
				break;
		}
	}
}

static SSymbolHandle
addElfSymbolToProgbits(ElfProgBitsSection* symbolSectionProgBits, const ElfSymbol* elfSymbol) {
	SSection* xlinkSymbolSection = symbolSectionProgBits->xlinkSection;
	if (xlinkSymbolSection->totalSymbols == symbolSectionProgBits->allocatedSymbols) {
		if (symbolSectionProgBits->allocatedSymbols == 0) {
			symbolSectionProgBits->allocatedSymbols = 16;
		} else {
			symbolSectionProgBits->allocatedSymbols += symbolSectionProgBits->allocatedSymbols >> 1;
		}
		xlinkSymbolSection->symbols =
		    realloc(xlinkSymbolSection->symbols, sizeof(SSymbol) * symbolSectionProgBits->allocatedSymbols);
	}

	SSymbol* xlinkSymbol = &xlinkSymbolSection->symbols[xlinkSymbolSection->totalSymbols];
	strncpy(xlinkSymbol->name, elfSymbol->name, MAX_SYMBOL_NAME_LENGTH - 1);
	xlinkSymbol->resolved = false;
	xlinkSymbol->section = xlinkSymbolSection;
	xlinkSymbol->value = elfSymbol->st_value;

	SSymbolHandle r = {xlinkSymbolSection, xlinkSymbolSection->totalSymbols++};
	return r;
}

static void
symbolsToXlink(const ElfHeader* elf, const ElfSectionHeader* header) {
	ElfSymbolSection* symbols = header->data.symbols;

	// Local symbols
	for (uint32_t i = 1; i < header->sh_info; ++i) {
		ElfSymbol* elfSymbol = &symbols->data[i];
		const ElfSectionHeader* elfSymbolSection = elfSymbol->section;

		if (elfSymbolSection == NULL)
			continue;
		if (elfSymbolSection->data.progbits->xlinkSection == NULL)
			continue;
		if (elfSymbol->st_nameindex == 0 && elfSymbol->type != STT_SECTION)
			continue;

		elfSymbol->xlinkSymbol = addElfSymbolToProgbits(elfSymbolSection->data.progbits, elfSymbol);

		if (elfSymbol->st_shndx == SHN_UNDEF) {
			SSymbol* xlinkSymbol = symbolOfHandle(&elfSymbol->xlinkSymbol);
			xlinkSymbol->type = SYM_LOCALIMPORT;
		} else if (elfSymbol->st_shndx == SHN_ABS) {
			SSymbol* xlinkSymbol = symbolOfHandle(&elfSymbol->xlinkSymbol);
			xlinkSymbol->type = SYM_LOCALEXPORT;
			xlinkSymbol->resolved = true;
		} else if (elfSymbol->type == STT_SECTION) {
			SSymbol* xlinkSymbol = symbolOfHandle(&elfSymbol->xlinkSymbol);
			xlinkSymbol->type = SYM_LOCALEXPORT;
			xlinkSymbol->resolved = true;
		} else {
			SSymbol* xlinkSymbol = symbolOfHandle(&elfSymbol->xlinkSymbol);
			xlinkSymbol->type = SYM_LOCALEXPORT;
		}
	}

	// Global symbols
	for (uint32_t i = header->sh_info; i < symbols->totalSymbols; ++i) {
		ElfSymbol* elfSymbol = &symbols->data[i];
		if (elfSymbol->st_nameindex == 0)
			continue;

		if (elfSymbol->st_shndx == SHN_UNDEF) {
			// Ignore imported symbols for now. They will be added when processing relocs
		} else {
			const ElfSectionHeader* elfSymbolSection = elfSymbol->section;
			elfSymbol->xlinkSymbol = addElfSymbolToProgbits(elfSymbolSection->data.progbits, elfSymbol);

			if (elfSymbol->st_shndx == SHN_ABS) {
				SSymbol* xlinkSymbol = symbolOfHandle(&elfSymbol->xlinkSymbol);
				xlinkSymbol->type = SYM_EXPORT;
				xlinkSymbol->resolved = true;
			} else {
				SSymbol* xlinkSymbol = symbolOfHandle(&elfSymbol->xlinkSymbol);
				xlinkSymbol->type = SYM_EXPORT;
			}
		}
	}
}

static uint8_t g_relocExpr[] = {
    OBJ_SYMBOL,   0, 0, 0, 0, //
    OBJ_CONSTANT, 0, 0, 0, 0, //
    OBJ_OP_ADD,               //
    OBJ_CONSTANT, 0, 0, 0, 0, //
    OBJ_PC_REL                //
};

#define EXPR_SIMPLE_SIZE 11
#define EXPR_PCREL_SIZE  16

#define EXPR_SYMBOL_OFFSET   1
#define EXPR_CONSTANT_OFFSET 6

static void
relocsToXlink(const ElfHeader* elf, const ElfSectionHeader* relocs) {
	ElfRelocAddendSection* relas = relocs->data.relocationAddends;
	ElfProgBitsSection* progbits = relas->section->data.progbits;
	SSection* xlinkSection = relas->section->data.progbits->xlinkSection;
	xlinkSection->patches = patch_Alloc((uint32_t) relas->totalRelocations);
	for (uint32_t i = 0; i < relas->totalRelocations; ++i) {
		ElfRelocAddendEntry* rela = &relas->data[i];
		SPatch* patch = &xlinkSection->patches->patches[i];
		int exprSize = 0;

		switch (rela->type) {
			case R_68K_32:
				patch->type = PATCH_BE_32;
				exprSize = EXPR_SIMPLE_SIZE;
				break;
			case R_68K_PC32:
				patch->type = PATCH_BE_32;
				exprSize = EXPR_PCREL_SIZE;
				break;
			default:
				error("relocsToXlink unknown relocation type (%d)", rela->type);
		}

		SSymbolHandle handle = rela->symbol->xlinkSymbol;
		bool validSymbol = handle.section != NULL;

		if (!validSymbol && rela->symbol->st_shndx == SHN_UNDEF) {
			handle = rela->symbol->xlinkSymbol = addElfSymbolToProgbits(progbits, rela->symbol);
			symbolOfHandle(&handle)->type = SYM_IMPORT;
		} else if (!validSymbol && (rela->symbol->section == relas->section)) {
			handle = rela->symbol->xlinkSymbol = addElfSymbolToProgbits(progbits, rela->symbol);
			symbolOfHandle(&handle)->type = SYM_LOCALIMPORT;
		} else if (rela->symbol->section != relas->section) {
			handle = addElfSymbolToProgbits(progbits, rela->symbol);
			symbolOfHandle(&handle)->type = rela->symbol->section == NULL ? SYM_IMPORT : SYM_LOCALIMPORT;
		}

		size_t xlinkSymbolIndex = handle.symbolIndex;
		if (xlinkSymbolIndex >= xlinkSection->totalSymbols)
			error("relocsToXlink error illegal symbol index (%d)", xlinkSymbolIndex);

		patch->valueSection = NULL;
		patch->valueSymbol = NULL;

		patch->offset = rela->r_offset;
		patch->expressionSize = exprSize;
		patch->expression = mem_Alloc(patch->expressionSize);
		memcpy(patch->expression, g_relocExpr, exprSize);
		patch->expression[EXPR_SYMBOL_OFFSET + 0] = xlinkSymbolIndex & 0xFF;
		patch->expression[EXPR_SYMBOL_OFFSET + 1] = (xlinkSymbolIndex >> 8) & 0xFF;
		patch->expression[EXPR_SYMBOL_OFFSET + 2] = (xlinkSymbolIndex >> 16) & 0xFF;
		patch->expression[EXPR_SYMBOL_OFFSET + 3] = (xlinkSymbolIndex >> 24) & 0xFF;
		patch->expression[EXPR_CONSTANT_OFFSET + 0] = rela->r_addend & 0xFF;
		patch->expression[EXPR_CONSTANT_OFFSET + 1] = (rela->r_addend >> 8) & 0xFF;
		patch->expression[EXPR_CONSTANT_OFFSET + 2] = (rela->r_addend >> 16) & 0xFF;
		patch->expression[EXPR_CONSTANT_OFFSET + 3] = (rela->r_addend >> 24) & 0xFF;
	}
}

static void
sectionToXlink(const ElfHeader* elf, const ElfSectionHeader* header, uint32_t sectionId, uint32_t fileId) {
	Group* group = NULL;

	if (header->sh_type == SHT_PROGBITS) {
		if ((header->sh_flags & (SHF_ALLOC | SHF_EXECINSTR)) == (SHF_ALLOC | SHF_EXECINSTR))
			group = &g_codeGroup;
		else if ((header->sh_flags & SHF_ALLOC) == SHF_ALLOC)
			group = &g_dataGroup;
		else if (header->sh_flags & SHF_STRINGS)
			return;
		else if (header->sh_flags == 0)
			return;
		else
			error("sectionToXlink unknown group type (\"%s\" %08X)", header->name, header->sh_flags);
	} else if (header->sh_type == SHT_NOBITS) {
		if ((header->sh_flags & (SHF_ALLOC | SHF_WRITE)) == (SHF_ALLOC | SHF_WRITE))
			group = &g_bssGroup;
		else
			error("sectionToXlink unknown group type (\"%s\" %08X)", header->name, header->sh_flags);
	} else {
		return;
	}

	SSection* section = sect_CreateNew();

	header->data.progbits->xlinkSection = section;

	section->group = group;
	section->fileId = fileId;
	section->data = NULL;

	section->cpuByteLocation = header->sh_addr == 0 ? -1 : (int32_t) header->sh_addr;
	section->cpuBank = -1;
	section->cpuLocation = section->cpuByteLocation;
	section->imageLocation = section->cpuByteLocation;
	section->minimumWordSize = 1;
	section->byteAlign = header->sh_addralign >= 2 ? (int32_t) header->sh_addralign : -1;
	section->page = -1;
	section->root = false;
	strncpy(section->name, header->name, MAX_SYMBOL_NAME_LENGTH - 1);

	section->totalSymbols = 0;
	section->symbols = NULL;

	section->totalLineMappings = 0;
	section->lineMappings = NULL;

	section->size = header->sh_size;
	if (header->sh_type == SHT_PROGBITS) {
		if (section->size > 0) {
			section->data = mem_Alloc(section->size);
			memcpy(section->data, header->data.progbits->data, section->size);
		} else {
			section->data = NULL;
		}
	}

	section->patches = NULL;
}

static void
elfToXlink(const ElfHeader* elf, uint32_t fileId) {
	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		const ElfSectionHeader* header = &elf->headers[i];
		sectionToXlink(elf, header, i, fileId);
	}

	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		const ElfSectionHeader* header = &elf->headers[i];
		if (header->sh_type == SHT_SYMTAB) {
			symbolsToXlink(elf, header);
		}
	}

	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		const ElfSectionHeader* header = &elf->headers[i];
		if (header->sh_type == SHT_RELA) {
			const ElfSectionHeader* patchSection = header->data.relocationAddends->section;
			if ((patchSection->sh_type == SHT_PROGBITS || patchSection->sh_type == SHT_NOBITS) &&
			    (patchSection->sh_flags & SHF_ALLOC))
				relocsToXlink(elf, header);
		}
	}
}

void
elf_Read(FILE* fileHandle, const char* filename, uint32_t fileId) {
	SFileInfo* fileInfo = obj_AllocateFileInfo(1);
	fileInfo->fileName = str_Create(filename);

	e_off_t sectionHeadersOffset;
	e_half_t sectionHeaderEntrySize;
	e_half_t totalSectionHeaders;
	e_half_t stringSectionHeaderIndex;

	if (!readHeader(fileHandle, &sectionHeadersOffset, &sectionHeaderEntrySize, &totalSectionHeaders, &stringSectionHeaderIndex))
		error("Unsupported ELF file");

	ElfSectionHeader* sectionHeaders =
	    readSectionHeaders(fileHandle, sectionHeadersOffset, sectionHeaderEntrySize, totalSectionHeaders);

	ElfHeader elf = {totalSectionHeaders, stringSectionHeaderIndex, sectionHeaders};

	readSections(fileHandle, sectionHeaders, totalSectionHeaders);
	resolveNamesAndIndices(&elf);

	elfToXlink(&elf, fileId);
}
