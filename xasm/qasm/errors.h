#pragma once

#include "types.h"

typedef enum {
	WARN_SYMBOL_WITH_RESERVED_NAME = 0,
	WARN_MACHINE_UNKNOWN_OPTION,

	ERROR_LINE_TOO_LONG = 1000,
	ERROR_INVALID_LABEL,
	ERROR_INVALID_OPERATION,
	ERROR_OPERAND_RANGE,
	ERROR_EXPR_CONST
} EError;


extern bool
err_Warn(uint32_t errorNumber, ...);

extern bool
err_Error(uint32_t errorNumber, ...);
