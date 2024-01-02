#pragma once

struct MachineOptions;

typedef enum {
    ASM_LITTLE_ENDIAN,
    ASM_BIG_ENDIAN
} endianness_t;

#define EEndianness endianness_t

typedef struct Options {
	struct Options* next;
    struct MachineOptions* machineOptions;

	endianness_t endianness;

} SOptions;

extern SOptions* opt_Current;


extern void
opt_Init(void);

extern void
opt_Close(void);

extern void
opt_Parse(const char* option, const char* argument);

