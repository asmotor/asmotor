#include "section.h"


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
