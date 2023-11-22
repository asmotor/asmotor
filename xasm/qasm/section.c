#include "mem.h"
#include "strcoll.h"

#include "errors.h"
#include "section.h"
#include "symbol.h"


strmap_t* s_section_map = NULL;


static void sectFree(intptr_t userData, intptr_t element) {
	SSection* section = (SSection*) element;
	str_Free(section->name);
	mem_Free(section);
}


extern SSection*
allocateSection(const string* name, SSymbol* group) {
	SSection* section = (SSection*) mem_Alloc(sizeof(SSection));

	section->name = str_Copy(name);
	section->group = group;

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
	intptr_t sect_int;

	if (strmap_Value(s_section_map, name, &sect_int)) {
		SSection* section = (SSection*) sect_int;
		if (section->group == group) {
			return section;
		}
		err_Error(ERROR_SECTION_TYPE_MISMATCH);
		return NULL;
	} else {
		SSection* section = allocateSection(name, group);
		return section;
	}
}


extern void
sect_OutputData(const void* data, size_t count) {
	const uint8_t* p = data;

	while (count--) {
		printf("%02X ", *p);
	}
}


extern void
sect_OutputConst8(uint8_t value) {
	sect_OutputData(&value, sizeof(value));
}


extern void
sect_OutputConst16(uint16_t value) {
	sect_OutputData(&value, sizeof(value));
}


extern void
sect_OutputConst16At(uint16_t value, uint32_t offset) {
	sect_OutputData(&value, sizeof(value));
}


extern void
sect_OutputConst32(uint32_t value) {
	sect_OutputData(&value, sizeof(value));
}


extern void
sect_OutputFloat32(long double value) {
	sect_OutputData(&value, sizeof(value));
}


extern void
sect_OutputFloat64(long double value) {
	sect_OutputData(&value, sizeof(value));
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
