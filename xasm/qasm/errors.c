#include <stdarg.h>

#include "str.h"

#include "errors.h"
#include "lexer.h"
#include "qasm.h"


static const char* g_warnings[] = {
	"Symbol has reserved name",		// WARN_SYMBOL_WITH_RESERVED_NAME
	"Unknown machine option %s",	// WARN_MACHINE_UNKNOWN_OPTION,
};


static const char* g_errors[] = {
    "%s not implemented",				// ERROR_NOT_IMPLEMENTED
	"Line too long",					// ERROR_LINE_TOO_LONG
	"Invalid label",					// ERROR_INVALID_LABEL
	"Invalid operation",				// ERROR_INVALID_OPERATION
	"Operand out of range",				// ERROR_OPERAND_RANGE
	"Expression must be constant",		// ERROR_EXPR_CONST
	"Wrong number of arguments",		// ERROR_ARGUMENT_COUNT
	"Invalid identifier",				// ERROR_INVALID_IDENTIFIER
	"Value %ld out of range [%ld;%ld]",	// ERROR_INTEGER_RANGE,

};


static const char*
getError(size_t errorNumber) {
    if (errorNumber >= 1000)
        return qasm_Configuration->getMachineError(errorNumber);

    if (errorNumber < sizeof(g_warnings) / sizeof(char*)) {
        return g_warnings[errorNumber];
    } else {
        return g_errors[errorNumber - ERROR_FIRST];
    }
}


static bool
printError(bool isError, uint32_t errorNumber, va_list arguments) {
	string* fileLine = lex_CurrentFileAndLine();
	string* errorStr = str_CreateFormat("%c%04d", isError ? 'E' : 'W', errorNumber);
	string* description = str_CreateArgs(getError(errorNumber), arguments);

	printf("%s: %s %s\n", str_String(fileLine), str_String(errorStr), str_String(description));

	str_Free(fileLine);
	str_Free(errorStr);
	str_Free(description);

	return true;
}


extern bool
err_Warn(uint32_t errorNumber, ...) {
	va_list args;

	va_start(args, errorNumber);
	printError(false, errorNumber, args);
	va_end(args);

	return true;
}


extern bool
err_Error(uint32_t errorNumber, ...) {
	va_list args;

	va_start(args, errorNumber);
	printError(true, errorNumber, args);
	va_end(args);

	return true;
}
