#pragma once

#include "types.h"

typedef enum {
	WARN_SYMBOL_WITH_RESERVED_NAME = 0,

    ERROR_INVALID_EXPRESSION = 100,
} EError;


extern bool
err_Warn(uint32_t errorNumber, ...);

extern bool
err_Error(uint32_t errorNumber, ...);
