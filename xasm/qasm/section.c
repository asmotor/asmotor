#include "mem.h"
#include "strcoll.h"

#include "errors.h"
#include "options.h"
#include "section.h"
#include "symbol.h"


strmap_t* s_section_map = NULL;
static SSection* s_current_section = NULL;


static void sectFree(intptr_t userData, intptr_t element) {
	SSection* section = (SSection*) element;
	str_Free(section->name);
	mem_Free(section->data);
	mem_Free(section);
}


extern SSection*
allocateSection(const string* name, SSymbol* group) {
	SSection* section = (SSection*) mem_Alloc(sizeof(SSection));

	section->name = str_Copy(name);
	section->group = group;
	section->data = NULL;
	section->size = 0;
	section->allocated_bytes = 0;

	return section;
}


extern void
sect_Init(void) {
	s_section_map = strmap_Create(sectFree);
}


extern void
sect_Close(void) {
	strmap_Free(s_section_map);
}


extern SSection*
sect_CreateOrSwitchTo(const string* name, SSymbol* group) {
	if (s_current_section != NULL) {
		err_Error(ERROR_SECTION_OPEN, str_String(s_current_section->name));
		return NULL;
	}

	SSection* section = NULL;
	intptr_t sect_int;

	if (strmap_Value(s_section_map, name, &sect_int)) {
		section = (SSection*) sect_int;
		if (section->group != group) {
			err_Error(ERROR_SECTION_TYPE_MISMATCH);
			return NULL;
		}
	} else {
		section = allocateSection(name, group);
	}

	s_current_section = section;
	return section;
}


static void
reserve(size_t count) {
	size_t new_size = s_current_section->size + count;
	if (new_size > s_current_section->allocated_bytes) {
		size_t growth = s_current_section->allocated_bytes == 0 ? 16 : s_current_section->allocated_bytes >> 1;
		s_current_section->allocated_bytes += growth;
		s_current_section->data = realloc(s_current_section->data, s_current_section->allocated_bytes);
	}
}


extern void
sect_OutputData(const void* data, size_t count) {
	if (s_current_section == NULL) {
		err_Error(ERROR_NO_SECTION);
		return;
	}

	reserve(count);

	memcpy(s_current_section->data + s_current_section->size, data, count);
	s_current_section->size += count;
}


extern void
sect_OutputConst8(uint8_t value) {
	sect_OutputData(&value, sizeof(value));
}


extern void
sect_OutputConst16(uint16_t value) {
	if (opt_Current->endianness == ASM_BIG_ENDIAN) {
		sect_OutputConst8(value >> 8);
		sect_OutputConst8(value);
	} else {
		sect_OutputConst8(value);
		sect_OutputConst8(value >> 8);
	}
}


extern void
sect_OutputConst16At(uint16_t value, uint32_t offset) {
	reserve(s_current_section->size - offset + sizeof(uint16_t));
	if (opt_Current->endianness == ASM_BIG_ENDIAN) {
		sect_OutputConst8(value >> 8);
		sect_OutputConst8(value);
	} else {
		sect_OutputConst8(value);
		sect_OutputConst8(value >> 8);
	}
}


extern void
sect_OutputConst32(uint32_t value) {
	if (opt_Current->endianness == ASM_BIG_ENDIAN) {
		sect_OutputConst16(value >> 16);
		sect_OutputConst16(value);
	} else {
		sect_OutputConst16(value);
		sect_OutputConst16(value >> 16);
	}
}


void
sect_OutputFloat32(long double value) {
	float floatValue = (float) value;
	uint32_t intValue;
	assert(sizeof(floatValue) == sizeof(intValue));
	memcpy(&intValue, &floatValue, sizeof(uint32_t));
	sect_OutputConst32(intValue);
}


void
sect_OutputFloat64(long double value) {
	double floatValue = (double) value;
	uint64_t intValue;
	assert(sizeof(floatValue) == sizeof(intValue));
	memcpy(&intValue, &floatValue, sizeof(uint64_t));

	if (opt_Current->endianness == ASM_BIG_ENDIAN) {
		sect_OutputConst32((uint32_t) (intValue >> 32));
		sect_OutputConst32((uint32_t) intValue);
	} else {
		sect_OutputConst32((uint32_t) intValue);
		sect_OutputConst32((uint32_t) (intValue >> 32));
	}
}


extern void
sect_OutputExpr8(SExpression* expr) {
	if (expr != NULL && expr->isConstant) {
		sect_OutputConst8(expr->value.integer);
	} else {
		sect_Skip(1);
		// TODO: save expression as patch
	}
}


extern void
sect_OutputExpr16(SExpression* expr) {
	internalerror("sect_OutputExpr16 not implemented");
}


extern void
sect_OutputExpr32(SExpression* expr) {
	internalerror("sect_OutputExpr32 not implemented");
}


extern void
sect_Skip(int64_t bytes) {
	while (bytes--)
		sect_OutputConst8(0);
}
