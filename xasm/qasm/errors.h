#pragma once

#include "types.h"

typedef enum {
	WARN_SYMBOL_WITH_RESERVED_NAME = 0,
	WARN_MACHINE_UNKNOWN_OPTION,

	ERROR_FIRST = 100,
	ERROR_LINE_TOO_LONG = ERROR_FIRST,
	ERROR_INVALID_LABEL,
	ERROR_INVALID_OPERATION,
	ERROR_OPERAND_RANGE,
	ERROR_EXPR_CONST,
	ERROR_ARGUMENT_COUNT,
	ERROR_INVALID_IDENTIFIER,
	ERROR_EXPECTED_CHAR,
	ERROR_INVALID_EXPRESSION,
	ERROR_CHARACTERS_AFTER_OPERATION,
	ERROR_SYMBOL_EXISTS,
} EError;


extern bool
err_Warn(uint32_t errorNumber, ...);

extern bool
err_Error(uint32_t errorNumber, ...);

extern bool
err_ErrorFile(const char* filename, size_t lineNumber, uint32_t errorNumber, ...);
