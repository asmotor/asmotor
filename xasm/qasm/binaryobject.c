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

#include <stdio.h>

#include "errors.h"
#include "section.h"


static bool
hasPatches(SSection* section, intptr_t user_data) {
	return section->patches != NULL;	
}


static bool
needsLinker() {
	return !sect_Find(hasPatches, 0);
}


typedef struct {
	FILE* file;
	uint64_t write_position;
} write_section_data_t;


static bool
internalWrite(string* filename, const char* opentype, void (*writeSection)(SSection*, intptr_t data)) {
	if (needsLinker()) {
		err_Error(ERROR_NEEDS_LINKER);
		return false;
	}

    FILE* file;
    if ((file = fopen(str_String(filename), opentype)) != NULL) {
		write_section_data_t data = {
			file,
			sect_First()->load_address
		};

		sect_ForEach(writeSection, (intptr_t) &data);
        fclose(file);
        return true;
    }

    return false;
}


static void
writeBinarySection(SSection* section, intptr_t data) {
	write_section_data_t* write_data = (write_section_data_t*) data;

    while (write_data->write_position < section->load_address) {
        write_data->write_position += 1;
        fputc(0, write_data->file);
    }

    fwrite(section->data, 1, section->size, write_data->file);
	write_data->write_position += section->size;
}


static void
writeVerilogSection(SSection* section, intptr_t data) {
	write_section_data_t* write_data = (write_section_data_t*) data;

    while (write_data->write_position < section->load_address) {
        write_data->write_position += 1;
        fprintf(write_data->file, "00\n");
    }

    for (uint32_t i = 0; i < section->size; ++i) {
        uint8_t b = (uint8_t) (section->data ? section->data[i] : 0u);
        fprintf(write_data->file, "%02X\n", b);
    }

	write_data->write_position += section->size;
}


bool
bin_Write(string* filename) {
    return internalWrite(filename, "wb", writeBinarySection);
}


bool
bin_WriteVerilog(string* filename) {
    return internalWrite(filename, "wt", writeVerilogSection);
}
