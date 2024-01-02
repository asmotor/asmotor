#include <stdio.h>

#include "str.h"
#include "util.h"

#include "assemble.h"
#include "lexbuffer.h"
#include "lexer.h"
#include "options.h"
#include "qasm.h"

NORETURN(static void printUsage(void));

SConfiguration* qasm_Configuration = NULL;


static void
printUsage(void) {
	printf("qasm%s input\n", qasm_Configuration->executableName);
	exit(EXIT_FAILURE);
}


static void
printLexLine(const SLexLine* line) {
	if (line->label) {
		printf("\"%s\":", line->label);
		if (line->export)
			putchar(':');
	}
	putchar('\t');
	if (line->operation) {
		printf("\"%s\"", line->operation);
	}
	putchar('\t');
	for (size_t i = 0; i < line->totalArguments; ++i) {
		printf("\"%s\"", line->arguments[i]);
		if (i != line->totalArguments - 1) {
			putchar(',');
		}
	}
	putchar('\n');
}


static void
printLexBuffer(const SLexerBuffer* buffer) {
	for (size_t i = 0; i < buffer->totalLines; ++i) {
		printLexLine(&buffer->lines[i]);
	}
}


static int
parseArguments(int argc, char* argv[]) {
	int dest = 0;
	int argn = 0;
	bool options_done = false;

	while (argn < argc && !options_done) {
		if (argv[argn][0] == '-') {
			switch (argv[argn][1]) {
				case 0: {
					options_done = true;
					break;
				}
				default: {
					const char* option = argv[argn++] + 1;
					const char* argument = NULL;
					if (argn < argc)
						argument = argv[argn++];
					opt_Parse(option, argument);
					break;
				}
			}
		} else {
			argv[dest++] = argv[argn++];
		}
	}

	argv[argn] = NULL;

	return argn;
}


extern int
qasm_Main(SConfiguration* configuration, int argc, char* argv[]) {
	qasm_Configuration = configuration;

	if (argc < 2) {
		printUsage();
	}

	sym_Init();
	sect_Init();
	lex_Init();
	opt_Init();

	argc = 1 + parseArguments(argc - 1, argv + 1);
	if (argc >= 2) {
		string* input = str_Create(argv[1]);
		SLexerBuffer* buffer = buf_CreateFromFile(input);
		if (buffer) {
			printf("Lines: %ld\n----\n", buffer->totalLines);
			printLexBuffer(buffer);
			assemble(buffer);
		}
		str_Free(input);
	}

	opt_Close();
	lex_Close();
	sect_Close();
	sym_Close();

	return 0;
}
