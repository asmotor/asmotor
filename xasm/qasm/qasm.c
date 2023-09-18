#include <stdio.h>

#include "str.h"
#include "util.h"

#include "lexbuffer.h"
#include "options.h"
#include "qasm.h"

NORETURN(static void printUsage(void));

const SConfiguration* qasm_Configuration = NULL;


static void
printUsage(void) {
	printf("qasm%s input\n", qasm_Configuration->executableName);
	exit(EXIT_FAILURE);
}


extern int
xasm_Main(const SConfiguration* configuration, int argc, char* argv[]) {
	qasm_Configuration = configuration;

	if (argc < 2) {
		printUsage();
	}

	string* input = str_Create(argv[1]);
	SLexBuffer* buffer = buf_CreateFromFile(input);

	printf("Lines: %ld", buffer->totalLines);

	str_Free(input);
	return 0;
}
