/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

// from util
#include "file.h"

// from xlink
#include "elf.h"
#include "group.h"
#include "section.h"
#include "xlink.h"

static Group g_codeGroup = { "CODE", GROUP_TEXT, 0 };
static Group g_dataGroup = { "DATA", GROUP_TEXT, GROUP_FLAG_DATA };
static Group g_bssGroup = { "BSS", GROUP_BSS, 0 };

typedef uint32_t e_word_t;
typedef uint32_t e_off_t;
typedef uint32_t e_addr_t;
typedef uint16_t e_half_t;

struct StringTable;
struct SectionHeader;


typedef struct {
	const char* name;
	const struct SectionHeader* section;
	SSymbol* xlinkSymbol;

	e_word_t nameindex;
	e_addr_t value;
	uint8_t bind;
	uint8_t type;
	e_half_t sectionindex;
} Symbol;

typedef struct {
	const struct StringTable* stringSection;

	size_t totalSymbols;
	Symbol data[];
} SymbolTable;

typedef struct StringTable {
	size_t length;
	char data[];
} StringTable;

typedef struct {
	size_t length;
	uint8_t data[];
} ProgBits;

typedef struct {
	Symbol* symbol;

	e_addr_t offset;
	e_word_t symbolIndex;
	e_word_t type;
} Reloc;

typedef struct {
	SymbolTable* symbols;
	struct SectionHeader* section;

	size_t totalRelocations;
	Reloc data[];
} RelocTable;

typedef struct {
	const Symbol* symbol;

	e_addr_t offset;
	int32_t addend;
	e_word_t symbolIndex;
	e_word_t type;
} RelocAddend;

typedef struct {
	const SymbolTable* symbols;
	const struct SectionHeader* section;

	size_t totalRelocations;
	RelocAddend data[];
} RelocAddendTable;

typedef struct SectionHeader {
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
		StringTable* strings;
		ProgBits* progbits;
		SymbolTable* symbols;
		RelocTable* relocations;
		RelocAddendTable* relocationAddends;
	} sh_data;
} SectionHeader;

typedef struct {
	uint32_t totalSectionHeaders;
	e_half_t stringSectionHeaderIndex;
	SectionHeader* headers;
} ElfHeader;


static uint32_t
read_word_b(const uint8_t* mem, size_t offset) {
	uint32_t r;

	mem += offset;
	r  = *mem++ << 24u;
	r |= *mem++ << 16u;
	r |= *mem++ << 8u;
	r |= *mem++;

	return r;
}

static uint32_t
read_word_l(const uint8_t* mem, size_t offset) {
	uint32_t r;

	mem += offset;
	r  = *mem++;
	r |= *mem++ << 8u;
	r |= *mem++ << 16u;
	r |= *mem++ << 24u;

	return r;
}

static uint16_t
read_half_b(const uint8_t* mem, size_t offset) {
	uint16_t r;

	mem += offset;
	r  = *mem++ << 8u;
	r |= *mem++;

	return r;
}

static uint16_t
read_half_l(const uint8_t* mem, size_t offset) {
	uint16_t r;

	mem += offset;
	r  = *mem++;
	r |= *mem++ << 8u;

	return r;
}


static uint32_t (*fget_word)(FILE*);
static uint16_t (*fget_half)(FILE*);

static uint32_t (*read_word)(const uint8_t*, size_t);
static uint16_t (*read_half)(const uint8_t*, size_t);

#define fget_addr fget_word
#define fget_off fget_word

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7
#define EI_NIDENT 16
#define ELF_HD_SHOFF 32
#define ELF_HD_SHNUM 48
#define ELF_HD_SHSTRNDX 50
#define ELF_HD_SIZE 52

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

#define EM_68K 4

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_NONE 0
#define EV_CURRENT 1

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKPROC 0xf0000000

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

#define R_68K_NONE 0	/* No reloc */
#define R_68K_32 1		/* Direct 32 bit  */
#define R_68K_16 2		/* Direct 16 bit  */
#define R_68K_8 3		/* Direct 8 bit  */
#define R_68K_PC32 4	/* PC relative 32 bit */
#define R_68K_PC16 5	/* PC relative 16 bit */
#define R_68K_PC8 6		/* PC relative 8 bit */


static bool
readHeader(FILE* fileHandle, e_off_t* sectionHeadersOffset, e_half_t* sectionHeaderEntrySize, e_half_t* totalSectionHeaders, e_half_t* stringSectionHeaderIndex) {
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

	e_half_t type = fget_half(fileHandle);		// e_type = ET_REL
	e_half_t arch = fget_half(fileHandle);		// e_machine
	e_word_t version = fget_word(fileHandle);	// e_version = EV_CURRENT
	/* e_addr_t entry = */ fget_addr(fileHandle);		// e_entry
	/* e_off_t programHeaderOffset = */ fget_off(fileHandle);	// e_phoff
	*sectionHeadersOffset = fget_off(fileHandle);	// e_shoff
	/* e_word_t flags = */ fget_word(fileHandle);			// e_flags
	e_half_t hdsize = fget_half(fileHandle);	// e_ehsize
	/* e_half_t programHeaderEntrySize = */ fget_half(fileHandle);	// e_phentsize
	/* e_half_t totalProgramHeaders = */ fget_half(fileHandle);		// e_phnum
	*sectionHeaderEntrySize = fget_half(fileHandle);	// e_shentsize = SHDR_SIZEOF
	*totalSectionHeaders = fget_half(fileHandle);		// e_shnum
	*stringSectionHeaderIndex = fget_half(fileHandle);	// e_shstrndx

	return type == ET_REL && version == EV_CURRENT && arch == EM_68K && hdsize == ELF_HD_SIZE;
}


static SectionHeader*
readSectionHeaders(FILE* fileHandle, e_off_t sectionHeadersOffset, e_half_t sectionHeaderEntrySize, e_half_t totalSectionHeaders) {
	SectionHeader* headers = malloc(sizeof(SectionHeader) * totalSectionHeaders);
	for (uint16_t i = 0; i < totalSectionHeaders; ++i) {
		fseek(fileHandle, sectionHeadersOffset + i * sectionHeaderEntrySize, SEEK_SET);

		SectionHeader* header = &headers[i];
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
readProgBits(FILE* fileHandle, SectionHeader* header) {
	ProgBits* progbits = malloc(sizeof(ProgBits) + header->sh_size);

	if (header->sh_flags & SHF_ALLOC) {
		fseek(fileHandle, header->sh_offset, SEEK_SET);
		progbits->length = header->sh_size;
		if (progbits->length != fread(progbits->data, 1, progbits->length, fileHandle))
			error("readProgBits short file");
	} else {
		progbits->length = 0;
	}
	header->sh_data.progbits = progbits;
}


static void
readStrTab(FILE* fileHandle, SectionHeader* header) {
	StringTable* table = malloc(sizeof(StringTable) + header->sh_size);

	fseek(fileHandle, header->sh_offset, SEEK_SET);
	table->length = header->sh_size;
	if (table->length != fread(table->data, 1, table->length, fileHandle))
		error("readStrTab short file");
	header->sh_data.strings = table;
}


static void
readSymTab(FILE* fileHandle, SectionHeader* header) {
	/* e_word_t stringSection = header->sh_link; */
	e_word_t totalSymbols = header->sh_size / header->sh_entsize;

	SymbolTable* table = malloc(sizeof(SymbolTable) + totalSymbols * sizeof(Symbol));
	table->totalSymbols = totalSymbols;

	for (e_word_t i = 0; i < totalSymbols; ++i) {
		Symbol* symbol = &table->data[i];
		fseek(fileHandle, header->sh_offset + i * header->sh_entsize, SEEK_SET);
		symbol->nameindex = fget_word(fileHandle);
		symbol->value = fget_addr(fileHandle);
		/* size = */ fget_word(fileHandle);
		uint8_t info = fgetc(fileHandle);
		symbol->bind = ELF32_ST_BIND(info);
		symbol->type = ELF32_ST_TYPE(info);
		/* other = */ fgetc(fileHandle);
		symbol->sectionindex = fget_half(fileHandle);
	}

	header->sh_data.symbols = table;
}


static void
readRelA(FILE* fileHandle, SectionHeader* header) {
	/* e_word_t stringSection = header->sh_link; */
	e_word_t totalRelocs = header->sh_size / header->sh_entsize;

	RelocAddendTable* table = malloc(sizeof(RelocAddendTable) + totalRelocs * sizeof(RelocAddend));
	table->totalRelocations = totalRelocs;

	for (e_word_t i = 0; i < totalRelocs; ++i) {
		RelocAddend* reloc = &table->data[i];
		fseek(fileHandle, header->sh_offset + i * header->sh_entsize, SEEK_SET);
		reloc->offset = fget_addr(fileHandle);
		e_word_t info = fget_word(fileHandle);
		reloc->symbolIndex = ELF32_R_SYM(info);
		reloc->type = ELF32_R_TYPE(info);
		reloc->addend = fget_word(fileHandle);
	}

	header->sh_data.relocationAddends = table;
}


static void
readRel(FILE* fileHandle, SectionHeader* header) {
	/* e_word_t stringSection = header->sh_link; */
	e_word_t totalRelocs = header->sh_info;

	RelocTable* table = malloc(sizeof(RelocTable) + totalRelocs * sizeof(Reloc));
	table->totalRelocations = totalRelocs;

	for (e_word_t i = 0; i < totalRelocs; ++i) {
		Reloc* reloc = &table->data[i];
		fseek(fileHandle, header->sh_offset + i * header->sh_entsize, SEEK_SET);
		reloc->offset = fget_addr(fileHandle);
		e_word_t info = fget_word(fileHandle);
		reloc->symbolIndex = ELF32_R_SYM(info);
		reloc->type = ELF32_R_TYPE(info);
	}

	header->sh_data.relocations = table;
}


static void
readSections(FILE* fileHandle, SectionHeader* headers, uint_fast16_t totalSections) {
	for (uint_fast16_t i = 0; i < totalSections; ++i) {
		SectionHeader* header = &headers[i];
		switch (header->sh_type) {
			case SHT_PROGBITS:
				readProgBits(fileHandle, header);
				break;
			case SHT_STRTAB:
				readStrTab(fileHandle, header);
				break;
			case SHT_SYMTAB:
				readSymTab(fileHandle, header);
				break;
			case SHT_RELA:
				readRelA(fileHandle, header);
				break;
			case SHT_REL:
				readRel(fileHandle, header);
				break;
			default:
				break;
		}
	}
}


static const SectionHeader*
getSectionHeader(const ElfHeader* elf, uint32_t index) {
	if (index >= elf->totalSectionHeaders)
		error("getSectionHeader read outside range (%d)", index);

	return &elf->headers[index];
}


static const StringTable*
getStringTable(const ElfHeader* elf, uint32_t index) {
	const SectionHeader* header = getSectionHeader(elf, index);

	if (header->sh_type != SHT_STRTAB)
		error("getStringTable expected string section (%d)", index);
	
	return header->sh_data.strings;
}


static const SymbolTable*
getSymbolTable(const ElfHeader* elf, uint32_t index) {
	const SectionHeader* header = getSectionHeader(elf, index);
	if (header->sh_type != SHT_SYMTAB)
		error("getSymbolTable expected symbol section (%d)", index);

	return header->sh_data.symbols;
}


static const Symbol*
getSymbol(const SymbolTable* symbols, uint32_t index) {
	if (index >= symbols->totalSymbols)
		error("getSymbol read outside range (%d)", index);

	return &symbols->data[index];
}


static const char*
getString(const StringTable* table, uint32_t index) {
	if (index >= table->length)
		error("getString read outside range (%d)", index);

	return &table->data[index];
}

static void
resolveSectionNames(ElfHeader* elf) {
	const StringTable* names = getStringTable(elf, elf->stringSectionHeaderIndex);
	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		SectionHeader* header = &elf->headers[i];
		header->name = getString(names, header->sh_name);
	}
}


static void
resolveSymTab(const ElfHeader* elf, SectionHeader* header) {
	SymbolTable* symbols = header->sh_data.symbols;
	symbols->stringSection = getStringTable(elf, header->sh_link);

	for (uint32_t i = 0; i < symbols->totalSymbols; ++i) {
		Symbol* symbol = &symbols->data[i];
		symbol->name = getString(symbols->stringSection, symbol->nameindex);
		symbol->section = getSectionHeader(elf, symbol->sectionindex);
	}
}


static void
resolveRelA(ElfHeader* elf, SectionHeader* header) {
	RelocAddendTable* relocs = header->sh_data.relocationAddends;
	relocs->section = getSectionHeader(elf, header->sh_info);
	relocs->symbols = getSymbolTable(elf, header->sh_link);

	for (uint32_t i = 0; i < relocs->totalRelocations; ++i) {
		RelocAddend* reloc = &relocs->data[i];
		reloc->symbol = getSymbol(relocs->symbols, reloc->symbolIndex);
	}
}


static int32_t
readData(const SectionHeader* text, uint32_t offset, uint32_t type) {
	switch (type) {
		case R_68K_32:
		case R_68K_PC32:
			return read_word(text->sh_data.progbits->data, offset);
		case R_68K_16:
		case R_68K_PC16:
			return read_half(text->sh_data.progbits->data, offset);
		default:
			error("readData unsupported type (%d)", type);
	}
}


static void
resolveRel(ElfHeader* elf, SectionHeader* header) {
	const SectionHeader* textSection = getSectionHeader(elf, header->sh_info);

	RelocTable* sourceTable = header->sh_data.relocations;
	RelocAddendTable* destTable = malloc(sizeof(RelocAddendTable) + sourceTable->totalRelocations * sizeof(RelocAddend));

	for (uint32_t i = 0; i < sourceTable->totalRelocations; ++i) {
		Reloc* source = &sourceTable->data[i];
		RelocAddend* dest = &destTable->data[i];

		dest->offset = source->offset;
		dest->symbolIndex = source->symbolIndex;
		dest->type = source->type;
		dest->addend = readData(textSection, source->offset, source->type);
	}

	header->sh_type = SHT_RELA;
	header->sh_data.relocationAddends = destTable;
	resolveRelA(elf, header);
}


static void
resolveNamesAndIndices(ElfHeader* elf) {
	resolveSectionNames(elf);

	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		SectionHeader* header = &elf->headers[i];
		switch (header->sh_type) {
			case SHT_SYMTAB:
				resolveSymTab(elf, header);
				break;
			case SHT_RELA:
				resolveRelA(elf, header);
				break;
			case SHT_REL:
				resolveRel(elf, header);
				break;
			default:
				break;
		}
	}
}


static void
symbolsToXlink(const ElfHeader* elf, const SectionHeader* header, SSection* section) {
	uint32_t allocatedSymbols = 16;

	section->symbols = malloc(sizeof(SSymbol) * allocatedSymbols);
	section->totalSymbols = 0;

	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		SectionHeader* symbols = &elf->headers[i];

		if (symbols->sh_type == SHT_SYMTAB) {
			for (uint32_t i = 0; i < symbols->sh_data.symbols->totalSymbols; ++i) {
				Symbol* elfSymbol = &symbols->sh_data.symbols->data[i];

				if (elfSymbol->section == header) {
					if (section->totalSymbols == allocatedSymbols) {
						allocatedSymbols += allocatedSymbols >> 1;
						section->symbols = realloc(section->symbols, sizeof(SSymbol) * allocatedSymbols);
					}

					SSymbol* xlinkSymbol = &section->symbols[section->totalSymbols++];
					elfSymbol->xlinkSymbol = xlinkSymbol;

					strcpy(xlinkSymbol->name, elfSymbol->name);
					xlinkSymbol->resolved = false;
					xlinkSymbol->section = section;
					xlinkSymbol->value = 0;

					if (i < symbols->sh_info) {
						// local symbols
						if (elfSymbol->sectionindex == SHN_UNDEF) {
							xlinkSymbol->type = SYM_LOCALIMPORT;
						} else if (elfSymbol->sectionindex == SHN_ABS) {
							xlinkSymbol->type = SYM_LOCALEXPORT;
							xlinkSymbol->resolved = true;
						} else {
							xlinkSymbol->type = SYM_LOCAL;
						}
					} else {
						if (elfSymbol->sectionindex == SHN_UNDEF) {
							xlinkSymbol->type = SYM_IMPORT;
						} else if (elfSymbol->sectionindex == SHN_ABS) {
							xlinkSymbol->type = SYM_EXPORT;
							xlinkSymbol->resolved = true;
						} else {
							xlinkSymbol->type = SYM_EXPORT;
						}
					}
				}
			}
		}
	}
}


static uint8_t
g_relocExpr[] = {
	OBJ_OP_ADD,
		OBJ_SYMBOL, 0, 0, 0, 0,
		OBJ_CONSTANT, 0, 0, 0, 0
};

#define EXPR_SYMBOL_OFFSET 2
#define EXPR_CONSTANT_OFFSET 7

static void
relocsToXlink(const ElfHeader* elf, const SectionHeader* header, SSection* section) {
	section->patches = NULL;

	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		SectionHeader* relocs = &elf->headers[i];
		if (relocs->sh_type == SHT_RELA) {
			RelocAddendTable* relas = relocs->sh_data.relocationAddends;
			if (relas->section == header) {
				section->patches = patch_Alloc(relas->totalRelocations);
				for (uint32_t i = 0; i < relas->totalRelocations; ++i) {
					RelocAddend* rela = &relas->data[i];
					SSymbol* xlinkSymbol = rela->symbol->xlinkSymbol;
					SPatch* patch = &section->patches->patches[i];

					switch (rela->type) {
						case R_68K_32:
							patch->type = PATCH_BE_32;
							break;
						default:
							error("relocsToXlink unknown relocation type (%d)", rela->type);
					}

					uint32_t xlinkSymbolIndex = (xlinkSymbol - section->symbols) / sizeof(SSymbol);
					if (xlinkSymbolIndex > section->totalSymbols)
						error("relocsToXlink error illegal symbol index (%d)", xlinkSymbolIndex);

					patch->valueSection = NULL;
					patch->valueSymbol = NULL;

					patch->offset = rela->offset;
					patch->expressionSize = sizeof(g_relocExpr);
					patch->expression = malloc(patch->expressionSize);
					memcpy(patch->expression, g_relocExpr, sizeof(g_relocExpr));
					patch->expression[EXPR_SYMBOL_OFFSET + 0] = xlinkSymbolIndex & 0xFF;
					patch->expression[EXPR_SYMBOL_OFFSET + 1] = (xlinkSymbolIndex >> 8) & 0xFF;
					patch->expression[EXPR_SYMBOL_OFFSET + 2] = (xlinkSymbolIndex >> 16) & 0xFF;
					patch->expression[EXPR_SYMBOL_OFFSET + 3] = (xlinkSymbolIndex >> 24) & 0xFF;
					patch->expression[EXPR_CONSTANT_OFFSET + 0] = rela->addend & 0xFF;
					patch->expression[EXPR_CONSTANT_OFFSET + 1] = (rela->addend >> 8) & 0xFF;
					patch->expression[EXPR_CONSTANT_OFFSET + 2] = (rela->addend >> 16) & 0xFF;
					patch->expression[EXPR_CONSTANT_OFFSET + 3] = (rela->addend >> 24) & 0xFF;
				}
			}
		}
	}
}


static void
sectionToXlink(const ElfHeader* elf, const SectionHeader* header, uint32_t sectionId, uint32_t fileId)  {
	Group* group = NULL;

	if (header->sh_type == SHT_PROGBITS) {
		if ((header->sh_flags & (SHF_ALLOC | SHF_WRITE)) == (SHF_ALLOC | SHF_WRITE))
			group = &g_dataGroup;
		else if ((header->sh_flags & (SHF_ALLOC | SHF_EXECINSTR)) == (SHF_ALLOC | SHF_EXECINSTR))
			group = &g_codeGroup;
		else
			error("sectionToXlink unknown group type (%08X)", header->sh_flags);
	} else if (header->sh_type == SHT_NOBITS) {
		if ((header->sh_flags & (SHF_ALLOC | SHF_WRITE)) == (SHF_ALLOC | SHF_WRITE))
			group = &g_bssGroup;
		else
			error("sectionToXlink unknown group type (%08X)", header->sh_flags);
	} else {
		return;
	}

	SSection* section = sect_CreateNew();
	section->group = group;
	section->fileId = fileId;
	section->data = NULL;

	section->cpuByteLocation = header->sh_addr == 0 ? -1 : (int32_t)header->sh_addr;
	section->cpuBank = -1;
	section->cpuLocation = section->cpuByteLocation;
	section->imageLocation = section->cpuByteLocation;
	section->minimumWordSize = 1;
	section->byteAlign = header->sh_addralign >= 2 ? (int32_t) header->sh_addralign : -1;
	section->root = false;
	strcpy(section->name, header->name);

	symbolsToXlink(elf, header, section);

	section->totalLineMappings = 0;
	section->lineMappings = NULL;

	section->size = header->sh_size;
	if (header->sh_type == SHT_PROGBITS) {
		section->data = malloc(section->size);
		memcpy(section->data, header->sh_data.progbits->data, section->size);
	}

	relocsToXlink(elf, header, section);
}


static void
elfToXlink(const ElfHeader* elf, uint32_t fileId) {
	for (uint32_t i = 0; i < elf->totalSectionHeaders; ++i) {
		SectionHeader* header = &elf->headers[i];
		sectionToXlink(elf, header, i, fileId);
	}

}


void
elf_Read(FILE* fileHandle, uint32_t fileId) {
	e_off_t sectionHeadersOffset;
	e_half_t sectionHeaderEntrySize;
	e_half_t totalSectionHeaders;
	e_half_t stringSectionHeaderIndex;

	if (!readHeader(fileHandle, &sectionHeadersOffset, &sectionHeaderEntrySize, &totalSectionHeaders, &stringSectionHeaderIndex))
		error("Unsupported ELF file");

	SectionHeader* sectionHeaders = readSectionHeaders(fileHandle, sectionHeadersOffset, sectionHeaderEntrySize, totalSectionHeaders);

	ElfHeader elf = {
		totalSectionHeaders,
		stringSectionHeaderIndex,
		sectionHeaders
	};

	readSections(fileHandle, sectionHeaders, totalSectionHeaders);
	resolveNamesAndIndices(&elf);

	elfToXlink(&elf, fileId);
}
