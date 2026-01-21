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

// from xasm
#include "elf.h"
#include "section.h"
#include "xasm.h"

// from util
#include "errors.h"
#include "file.h"
#include "strbuf.h"
#include "symbol.h"

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

#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_NONE    0
#define EV_CURRENT 1

#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

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
#define SHF_MASKPROC  0xf0000000

#define SHN_UNDEF     0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC    0xff00
#define SHN_HIPROC    0xff1f
#define SHN_ABS       0xfff1
#define SHN_COMMON    0xfff2
#define SHN_HIRESERVE 0xffff

#define SYM_SIZE 16

#define STB_LOCAL  0
#define STB_GLOBAL 1
#define STB_WEAK   2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPROC  13
#define STT_HIPROC  15

#define ELF32_ST_BIND(i)    ((i) >> 4)
#define ELF32_ST_TYPE(i)    ((i) & 0xf)
#define ELF32_ST_INFO(b, t) (((b) << 4) + ((t) & 0xf))

#define RELA_SIZE 12

#define ELF32_R_SYM(i)     ((i) >> 8)
#define ELF32_R_TYPE(i)    ((unsigned char) (i))
#define ELF32_R_INFO(s, t) (((s) << 8) + (unsigned char) (t))

static void (*fput_word)(uint32_t, FILE*);
static void (*fput_half)(uint16_t, FILE*);

#define fput_addr fput_word
#define fput_off  fput_word

typedef uint32_t e_word_t;
typedef uint32_t e_off_t;
typedef uint32_t e_addr_t;
typedef uint16_t e_half_t;

typedef struct {
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
} e_shdr;

#define SHDR_NAME      0
#define SHDR_TYPE      4
#define SHDR_FLAGS     8
#define SHDR_ADDR      12
#define SHDR_OFFSET    16
#define SHDR_SIZE      20
#define SHDR_LINK      24
#define SHDR_INFO      28
#define SHDR_ADDRALIGN 32
#define SHDR_ENTSIZE   36
#define SHDR_SIZEOF    40

#define R_68K_NONE 0 /* No reloc */
#define R_68K_32   1 /* Direct 32 bit  */
#define R_68K_16   2 /* Direct 16 bit  */
#define R_68K_8    3 /* Direct 8 bit  */
#define R_68K_PC32 4 /* PC relative 32 bit */
#define R_68K_PC16 5 /* PC relative 16 bit */
#define R_68K_PC8  6 /* PC relative 8 bit */

static string_buffer* g_stringTable = NULL;
static e_shdr* g_sectionHeaders = NULL;
static uint32_t g_totalSectionHeaders = 0;

static void
alignFile(FILE* fileHandle, uint32_t alignment) {
	uint32_t pad = ftell(fileHandle) % alignment;
	while (pad > 0 && pad < alignment) {
		fputc(0, fileHandle);
		pad += 1;
	}
}

static void
align4(FILE* fileHandle) {
	alignFile(fileHandle, 4);
}

static uint32_t
addSectionHeader(const e_shdr* header) {
	uint32_t index = g_totalSectionHeaders;
	g_sectionHeaders = realloc(g_sectionHeaders, (g_totalSectionHeaders + 1) * sizeof(e_shdr));
	g_sectionHeaders[g_totalSectionHeaders++] = *header;
	return index;
}

static void
addSectionHeaderZero(void) {
	e_shdr header_zero = {0, SHT_NULL, 0, 0, 0, 0, SHN_UNDEF, 0, 0, 0};
	addSectionHeader(&header_zero);
}

static uint32_t
addStringChars(const char* str) {
	uint32_t offset = (uint32_t) g_stringTable->size;
	strbuf_AppendChars(g_stringTable, str, strlen(str) + 1); // include terminating zero
	return offset;
}

static uint32_t
addString(const string* str) {
	uint32_t offset = (uint32_t) g_stringTable->size;
	strbuf_AppendChars(g_stringTable, str_String(str), str_Length(str) + 1); // include terminating zero
	return offset;
}

static void
writeElfHeader(FILE* fileHandle, bool bigEndian, EElfArch arch) {
	uint8_t header_ident[EI_NIDENT] = {
	    0x7F, 'E', 'L', 'F', ELFCLASS32, bigEndian ? ELFDATA2MSB : ELFDATA2LSB, EV_CURRENT, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	fwrite(header_ident, 1, sizeof(header_ident), fileHandle);

	fput_half(ET_REL, fileHandle);      // e_type
	fput_half(arch, fileHandle);        // e_machine
	fput_word(EV_CURRENT, fileHandle);  // e_version
	fput_addr(0, fileHandle);           // e_entry
	fput_off(0, fileHandle);            // e_phoff
	fput_off(0, fileHandle);            // e_shoff
	fput_word(0, fileHandle);           // e_flags
	fput_half(ELF_HD_SIZE, fileHandle); // e_ehsize
	fput_half(0, fileHandle);           // e_phentsize
	fput_half(0, fileHandle);           // e_phnum
	fput_half(SHDR_SIZEOF, fileHandle); // e_shentsize
	fput_half(0, fileHandle);           // e_shnum
	fput_half(0, fileHandle);           // e_shstrndx
}

static void
writeSymbolRaw(e_word_t name, e_addr_t value, uint8_t info, e_half_t sectionIndex, FILE* fileHandle) {
	fput_word(name, fileHandle);
	fput_addr(value, fileHandle);
	fput_word(0, fileHandle);
	fputc(info, fileHandle);
	fputc(0, fileHandle);
	fput_half(sectionIndex, fileHandle);
}

static void
writeSymbol(const string* name, e_addr_t value, uint8_t bind, uint8_t type, e_half_t sectionIndex, FILE* fileHandle) {
	writeSymbolRaw(addString(name), value, ELF32_ST_INFO(bind, type), sectionIndex, fileHandle);
}

static uint32_t
writeGlobalSymbols(FILE* fileHandle, uint32_t symbolIndex) {
	for (uint16_t i = 0; i < SYMBOL_HASH_SIZE; ++i) {
		for (SSymbol* symbol = sym_hashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
			if ((symbol->id == 0) &&
			    (symbol->type == SYM_LABEL || symbol->type == SYM_EQU || symbol->type == SYM_IMPORT ||
			     symbol->type == SYM_GLOBAL) &&
			    ((symbol->flags & (SYMF_RELOC | SYMF_USED | SYMF_EXPORT)) != 0)) {

				e_half_t sectionIndex = symbol->flags & SYMF_CONSTANT //
				                            ? SHN_ABS
				                            : symbol->type == SYM_GLOBAL || symbol->type == SYM_IMPORT //
				                                  ? SHN_UNDEF                                          //
				                                  : symbol->section->id;

				writeSymbol(symbol->name, symbol->value.integer, STB_GLOBAL, STT_NOTYPE, sectionIndex, fileHandle);
				symbol->id = symbolIndex++;
			}
		}
	}
	return symbolIndex;
}

static uint32_t
writeLocalSymbols(FILE* fileHandle, uint32_t symbolIndex) {
	for (uint16_t i = 0; i < SYMBOL_HASH_SIZE; ++i) {
		for (SSymbol* symbol = sym_hashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
			if ((symbol->id == 0) &&                                                    //
			    (symbol->type == SYM_LABEL || symbol->type == SYM_EQU)                  //
			    && ((symbol->flags & (SYMF_RELOC | SYMF_USED | SYMF_FILE_EXPORT)) != 0) //
			    && ((symbol->flags & SYMF_EXPORT) == 0)) {

				e_half_t sectionIndex = symbol->flags & SYMF_CONSTANT ? SHN_ABS : symbol->section->id;

				writeSymbol(symbol->name, symbol->value.integer, STB_LOCAL, STT_NOTYPE, sectionIndex, fileHandle);
				symbol->id = symbolIndex++;
			}
		}
	}
	return symbolIndex;
}

static void
writeSymbolSection(FILE* fileHandle, uint32_t symbolSection, uint32_t stringSection) {
	align4(fileHandle);

	off_t symbolTableLocation = ftello(fileHandle);

	writeSymbolRaw(0, 0, 0, SHN_UNDEF, fileHandle); // symbol #0
	uint32_t symbolIndex = 1;
	symbolIndex = writeLocalSymbols(fileHandle, symbolIndex);
	uint32_t totalLocals = symbolIndex;
	symbolIndex = writeGlobalSymbols(fileHandle, symbolIndex);

	g_sectionHeaders[symbolSection].sh_offset = (e_off_t) symbolTableLocation;
	g_sectionHeaders[symbolSection].sh_size = (e_off_t) (ftello(fileHandle) - symbolTableLocation);
	g_sectionHeaders[symbolSection].sh_link = stringSection;
	g_sectionHeaders[symbolSection].sh_info = totalLocals;
}

static void
writeSection(SSection* section, FILE* fileHandle) {
	section->id = UINT32_MAX;

	if (section->flags & SECTF_LOADFIXED && section->imagePosition == 0) {
		err_Error(ERROR_ELF_LOAD_ZERO);
		return;
	}

	e_word_t sh_type = 0;
	e_word_t sh_flags = 0;

	if (section->group->value.groupType == GROUP_TEXT) {
		sh_type = SHT_PROGBITS;
		sh_flags = section->group->flags & SYMF_DATA ? SHF_ALLOC | SHF_WRITE : SHF_ALLOC | SHF_EXECINSTR;
	} else {
		sh_type = SHT_NOBITS;
		sh_flags = SHF_ALLOC | SHF_WRITE;
	}

	uint32_t align = section->flags & SECTF_ALIGNED ? section->align : xasm_Configuration->sectionAlignment;
	alignFile(fileHandle, align);

	e_shdr header = {
	    addString(section->name),
	    sh_type,
	    sh_flags,
	    section->flags & SECTF_LOADFIXED ? section->imagePosition : 0,
	    sh_type & SHT_PROGBITS ? ftell(fileHandle) : 0,
	    section->usedSpace,
	    SHN_UNDEF,
	    0,
	    align,
	    0,
	};

	section->id = g_totalSectionHeaders;

	addSectionHeader(&header);
	if (sh_type & SHT_PROGBITS) {
		for (SPatch* patch = section->patches; patch != NULL; patch = patch->pNext) {
			for (int i = 0; i < 4; ++i)
				section->data[patch->offset + i] = 0;
		}
		fwrite(section->data, 1, section->usedSpace, fileHandle);
	}
}

typedef struct {
	e_addr_t offset;
	e_word_t info;
	e_word_t addend;
} e_rela;

static int
compare_rela(const void* lhs, const void* rhs) {
	return ((const e_rela*) lhs)->offset - ((const e_rela*) rhs)->offset;
}

static bool
writeReloc(SSection* section, uint32_t symbolSection, FILE* fileHandle) {
	if (section->id == UINT32_MAX)
		return true;

	align4(fileHandle);
	off_t sectionLocation = ftello(fileHandle);

	e_rela* relocs = NULL;
	int32_t total_relocs = 0;
	for (SPatch* patch = section->patches; patch != NULL; patch = list_GetNext(patch)) {
		SSymbol* symbol;
		uint32_t addend;
		if (expr_GetSymbolOffset(&addend, &symbol, patch->expression)) {
			relocs = realloc(relocs, sizeof(e_rela) * (total_relocs + 1));
			relocs[total_relocs].offset = patch->offset;
			relocs[total_relocs].info = ELF32_R_INFO(symbol->id, R_68K_32);
			relocs[total_relocs].addend = addend;
			total_relocs += 1;
		} else {
			err_PatchError(patch, ERROR_OBJECTFILE_PATCH);
			return false;
		}
	}

	qsort(relocs, total_relocs, sizeof(e_rela), compare_rela);
	for (int i = 0; i < total_relocs; ++i) {
		fput_addr(relocs[i].offset, fileHandle);
		fput_word(relocs[i].info, fileHandle);
		fput_word(relocs[i].addend, fileHandle);
	}
	free(relocs);

	string* reloc = str_Create(".reloc");
	string* sectionName = str_Concat(section->name, reloc);

	e_shdr header = {
	    addString(sectionName),
	    SHT_RELA,
	    0,
	    0,
	    (e_off_t) sectionLocation,
	    (e_off_t) (ftello(fileHandle) - sectionLocation),
	    symbolSection,
	    section->id,
	    0,
	    RELA_SIZE,
	};

	str_Free(reloc);
	str_Free(sectionName);

	addSectionHeader(&header);
	return true;
}

static void
writeStrings(FILE* fileHandle, uint32_t stringSection) {
	align4(fileHandle);
	off_t stringsLocation = ftello(fileHandle);
	fwrite(g_stringTable->data, 1, g_stringTable->size, fileHandle);

	g_sectionHeaders[stringSection].sh_offset = (e_off_t) stringsLocation;
	g_sectionHeaders[stringSection].sh_size = (e_word_t) g_stringTable->size;

	fseek(fileHandle, ELF_HD_SHSTRNDX, SEEK_SET);
	fput_half(stringSection, fileHandle);
	fseek(fileHandle, 0, SEEK_END);
}

static void
writeSectionHeaders(FILE* fileHandle, uint32_t stringsSection) {
	align4(fileHandle);
	off_t headersLocation = ftello(fileHandle);

	for (uint32_t i = 0; i < g_totalSectionHeaders; ++i) {
		e_shdr* header = &g_sectionHeaders[i];
		fput_word(header->sh_name, fileHandle);
		fput_word(header->sh_type, fileHandle);
		fput_word(header->sh_flags, fileHandle);
		fput_addr(header->sh_addr, fileHandle);
		fput_off(header->sh_offset, fileHandle);
		fput_word(header->sh_size, fileHandle);
		fput_word(header->sh_link, fileHandle);
		fput_word(header->sh_info, fileHandle);
		fput_word(header->sh_addralign, fileHandle);
		fput_word(header->sh_entsize, fileHandle);
	}

	fseek(fileHandle, ELF_HD_SHOFF, SEEK_SET);
	fput_off((e_off_t) headersLocation, fileHandle);
	fseek(fileHandle, ELF_HD_SHNUM, SEEK_SET);
	fput_half(g_totalSectionHeaders, fileHandle);
	fseek(fileHandle, 0, SEEK_END);
}

static bool
writeSections(FILE* fileHandle) {
	e_shdr symbolHeader = {addStringChars(".symtab"), SHT_SYMTAB, 0, 0, 0, 0, 0, 0, 0, SYM_SIZE};
	uint32_t symbolSection = addSectionHeader(&symbolHeader);

	e_shdr stringHeader = {addStringChars(".strtab"), SHT_STRTAB, 0, 0, 0, 0, 0, 0, 0, 0};
	uint32_t stringSection = addSectionHeader(&stringHeader);

	for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
		writeSection(section, fileHandle);
	}

	writeSymbolSection(fileHandle, symbolSection, stringSection);

	for (SSection* section = sect_Sections; section != NULL; section = list_GetNext(section)) {
		if (!writeReloc(section, symbolSection, fileHandle))
			return false;
	}

	writeStrings(fileHandle, stringSection);
	writeSectionHeaders(fileHandle, stringSection);

	return true;
}

static void
prepareSymbols(void) {
	// Reset all symbols id's
	for (uint16_t i = 0; i < SYMBOL_HASH_SIZE; ++i) {
		for (SSymbol* symbol = sym_hashedSymbols[i]; symbol != NULL; symbol = list_GetNext(symbol)) {
			symbol->id = 0;
		}
	}
}

bool
elf_Write(const string* filename, bool bigEndian, EElfArch arch) {
	prepareSymbols();

	fput_half = bigEndian ? fputbw : fputlw;
	fput_word = bigEndian ? fputbl : fputll;
	g_stringTable = strbuf_Create();
	addStringChars("");

	addSectionHeaderZero();

	FILE* fileHandle;
	if ((fileHandle = fopen(str_String(filename), "w+b")) != NULL) {
		writeElfHeader(fileHandle, bigEndian, arch);
		bool success = writeSections(fileHandle);
		fclose(fileHandle);

		if (!success)
			remove(str_String(filename));

		return success;
	}
	return false;
}
