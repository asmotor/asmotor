#pragma once

#include "types.h"

typedef enum {
	WARN_SYMBOL_WITH_RESERVED_NAME = 0,
	WARN_MACHINE_UNKNOWN_OPTION,

	ERROR_NOT_IMPLEMENTED = 1000,
	ERROR_LINE_TOO_LONG,
	ERROR_INVALID_LABEL,
	ERROR_INVALID_OPERATION,
	ERROR_OPERAND_RANGE,
	ERROR_EXPR_CONST,
	ERROR_ARGUMENT_COUNT
} EError;


extern bool
err_Warn(uint32_t errorNumber, ...);

extern bool
err_Error(uint32_t errorNumber, ...);
