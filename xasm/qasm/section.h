#pragma once

#include "str.h"

typedef struct Section {
    string* name;
} SSection;

extern void
sect_OutputData(const void* data, size_t count);

extern void
sect_OutputConst8(uint8_t value);

extern void
sect_OutputConst16(uint16_t value);

extern void
sect_OutputConst16At(uint16_t value, uint32_t offset);

extern void
sect_OutputConst32(uint32_t value);

extern void
sect_OutputFloat32(long double value);

extern void
sect_OutputFloat64(long double value);

