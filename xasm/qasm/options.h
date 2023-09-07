#pragma once

struct MachineOptions;

typedef enum {
    ASM_LITTLE_ENDIAN,
    ASM_BIG_ENDIAN
} EEndianness;

typedef struct Options {
    struct MachineOptions* machineOptions;
} SOptions;

extern SOptions* opt_Current;