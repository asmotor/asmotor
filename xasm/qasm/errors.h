#pragma once

#include "types.h"

typedef enum {
	WARN_SYMBOL_WITH_RESERVED_NAME = 0,

    ERROR_INVALID_EXPRESSION = 1000,
	ERROR_INVALID_LABEL,
	ERROR_LINE_TOO_LONG
} EError;


extern bool
err_Warn(uint32_t errorNumber, ...);

extern bool
err_Error(uint32_t errorNumber, ...);
