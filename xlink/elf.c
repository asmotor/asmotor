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
#include "xlink.h"
#include "elf.h"

typedef uint32_t e_word_t;
typedef uint32_t e_off_t;
typedef uint32_t e_addr_t;
typedef uint16_t e_half_t;


typedef struct {
	e_word_t name;
	e_addr_t value;
	uint8_t bind;
	uint8_t type;
	e_half_t symtable;
} Symbol;



typedef struct {
	size_t length;
	uint8_t data[];
} StringTable;


typedef struct {
	size_t length;
	uint8_t data[];
} ProgBits;


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

	union {
		StringTable* strings;
		ProgBits* progbits;
	} sh_data;
} SectionHeader;

static uint32_t (*fget_word)(FILE*);
static uint16_t (*fget_half)(FILE*);

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
	} else if (id_endian == ELFDATA2MSB) {
		fget_half = fgetbw;
		fget_word = fgetbl;
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


static bool
readSections(FILE* fileHandle, SectionHeader* headers, uint_fast16_t totalSections) {
	for (uint_fast16_t i = 0; i < totalSections; ++totalSections) {
		SectionHeader* header = &headers[i];
		switch (header->sh_type) {
			case SHT_NULL: {
				break;
			}
			case SHT_PROGBITS: {
				ProgBits* progbits = malloc(sizeof(ProgBits) + header->sh_size);

				if (header->sh_flags & SHF_ALLOC) {
					fseek(fileHandle, header->sh_offset, SEEK_SET);
					progbits->length = header->sh_size;
					fread(progbits->data, 1, progbits->length, fileHandle);
				} else {
					progbits->length = 0;
				}
				header->sh_data.progbits = progbits;
				break;
			}
			case SHT_STRTAB: {
				StringTable* table = malloc(sizeof(StringTable) + header->sh_size);

				fseek(fileHandle, header->sh_offset, SEEK_SET);
				table->length = header->sh_size;
				fread(table->data, 1, table->length, fileHandle);
				header->sh_data.strings = table;
				break;
			}
			case SHT_SYMTAB: {
				break;
			}
		}
	}

	return true;
}


bool
elf_Read(FILE* fileHandle) {
	e_off_t sectionHeadersOffset;
	e_half_t sectionHeaderEntrySize;
	e_half_t totalSectionHeaders;
	e_half_t stringSectionHeaderIndex;

	if (!readHeader(fileHandle, &sectionHeadersOffset, &sectionHeaderEntrySize, &totalSectionHeaders, &stringSectionHeaderIndex))
		return false;

	SectionHeader* sectionHeaders = readSectionHeaders(fileHandle, sectionHeadersOffset, sectionHeaderEntrySize, totalSectionHeaders);
	if (!readSections(fileHandle, sectionHeaders, totalSectionHeaders))
		return false;

	return false;
}
