#pragma once

struct MachineOptions;

typedef enum {
    ASM_LITTLE_ENDIAN,
    ASM_BIG_ENDIAN
} EEndianness;

typedef struct Options {
	struct Options* prev;
	struct Options* next;
    struct MachineOptions* machineOptions;
} SOptions;

extern SOptions* opt_Current;


extern void
opt_Init(void);

extern void
opt_Close(void);
