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

#include "elf.h"

// from util
#include "file.h"
#include "strbuf.h"

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_PAD 7
#define EI_NIDENT 16

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define EV_NONE 0
#define EV_CURRENT 1

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

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

#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

#define ELF_HDSIZE 52

static void (*fput_word)(uint32_t, FILE*);
static void (*fput_half)(uint32_t, FILE*);

#define fput_addr fput_word
#define fput_off fput_word

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


static string_buffer* g_stringTable = NULL;
static e_shdr* g_sectionHeaders = NULL;
static uint32_t g_totalSectionHeaders = 0;


static void
addSectionHeader(const e_shdr* header) {
	uint32_t index = g_totalSectionHeaders;
	g_totalSectionHeaders = realloc(g_sectionHeaders + 1, index * sizeof(e_shdr));
	g_sectionHeaders[g_totalSectionHeaders++] = *header;
	return index;
}

static void
addSectionHeaderZero(void) {
	e_shdr header_zero = {
		0, SHT_NULL, 0, 0, 0, 0, SHN_UNDEF, 0, 0, 0
	};
	addSectionHeader(&header_zero);
}

static uint32_t
addString(const string* str) {
	uint32_t offset = str->length;
	strbuf_AppendString(g_stringTable, str);
	strbuf_AppendChar(g_stringTable, 0);
	return offset;
}

static void
writeElfHeader(FILE* fileHandle, bool bigEndian, EElfArch arch) {
	uint8_t header_ident[EI_NIDENT] = {
		0x7F, 'E', 'L', 'F',
		ELFCLASS32, bigEndian ? ELFDATA2MSB : ELFDATA2LSB, EV_CURRENT, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};
	fwrite(header_ident, 1, sizeof(header_ident), fileHandle);

	fput_half(ET_REL, fileHandle);		// e_type
	fput_half(arch, fileHandle);		// e_machine
	fput_word(EV_CURRENT, fileHandle);	// e_version
	fput_addr(0, fileHandle);			// e_entry
	fput_off(0, fileHandle);			// e_phoff
	fput_off(0, fileHandle);			// e_shoff
	fput_word(0, fileHandle);			// e_flags
	fput_half(ELF_HDSIZE, fileHandle);	// e_ehsize
	fput_half(0, fileHandle);			// e_phentsize
	fput_half(0, fileHandle);			// e_phnum
	fput_half(0, fileHandle);			// e_shentsize
	fput_half(0, fileHandle);			// e_shnum
	fput_half(0, fileHandle);			// e_shstrndx
}




static void
writeSections(FILE* fileHandle) {

}

bool
elf_Write(const string* filename, bool bigEndian, EElfArch arch) {
	fput_half = bigEndian ? fputbw : fputlw;
	fput_word = bigEndian ? fputbl : fputll;
	g_stringTable = strbuf_Create();
	
    FILE* fileHandle;
    if ((fileHandle = fopen(str_String(filename), "wb")) != NULL) {
		writeElfHeader(fileHandle, bigEndian, arch);
		writeSections(fileHandle);

		fclose(fileHandle);
		return true;
	}
	return false;
}
