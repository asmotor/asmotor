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
	"Line too long",					// ERROR_LINE_TOO_LONG
	"Invalid label",					// ERROR_INVALID_LABEL
	"Invalid operation",				// ERROR_INVALID_OPERATION
	"Operand out of range",				// ERROR_OPERAND_RANGE
	"Expression must be constant",		// ERROR_EXPR_CONST
	"Wrong number of arguments",		// ERROR_ARGUMENT_COUNT
	"Invalid identifier",				// ERROR_INVALID_IDENTIFIER
	"Expected %c here",					// ERROR_EXPECTED_CHAR,
	"Invalid expression",				// ERROR_INVALID_EXPRESSION
	"Characters after operation",		// ERROR_CHARACTERS_AFTER_OPERATION
	"\"%s\" already defined",			// ERROR_SYMBOL_EXISTS
	"\"%s\" is not a variable",			// ERROR_NOT_A_VARIABLE
	"Unterminated string",				// ERROR_UNTERMINATED_STRING
	"File \"%s\" not found",			// ERROR_FILE_NOT_FOUND
	"String required",					// ERROR_EXPR_STRING,
	"Expected group here",				// ERROR_GROUP_EXPECTED,
	"Section group mismatch",			// ERROR_SECTION_TYPE_MISMATCH
	"Symbol \"%s\" not an integer",		// ERROR_UNKNOWN_INTEGER_SYMBOL
	"Symbol \"%s\" unknown",			// ERROR_UNKNOWN_SYMBOL
	"Operator does not support different types",	// ERROR_OPERATOR_TYPES
	"Section \"%s\" not closed",		// ERROR_SECTION_OPEN
	"Data or code outside section",		// ERROR_NO_SECTION
    "When writing binary file only PC relative addressing must be used, or first section must be LOAD fixed.",	// ERROR_NEEDS_LINKER
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


static void
printErrorFile(const string* fileLine, bool isError, uint32_t errorNumber, va_list arguments) {
	string* errorStr = str_CreateFormat("%c%04d", isError ? 'E' : 'W', errorNumber);
	string* description = str_CreateArgs(getError(errorNumber), arguments);

	printf("%s: %s %s\n", str_String(fileLine), str_String(errorStr), str_String(description));

	str_Free(errorStr);
	str_Free(description);
}


static void
printError(bool isError, uint32_t errorNumber, va_list arguments) {
	string* fileLine = lex_CurrentFileAndLine();
	printErrorFile(fileLine, isError, errorNumber, arguments);
	str_Free(fileLine);
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


extern bool
err_ErrorFile(const char* filename, size_t lineNumber, uint32_t errorNumber, ...) {
	va_list args;

	va_start(args, errorNumber);
	string* fileLine = str_CreateFormat("%s:%ld", filename, lineNumber);
	printErrorFile(fileLine, true, errorNumber, args);
	va_end(args);

	return true;
}
