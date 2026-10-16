/*  Copyright 2008-2026 Carsten Elton Sorensen and contributors

    This file is part of ASMotor.

    ASMotor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ASMotor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ASMotor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(__STDC_IEC_559__)
#if defined(_M_IX86) || defined(_M_X64) || defined(__x86_64) || defined(__ARM_ARCH)
#define __STDC_IEC_559__
#endif
#endif

#if !defined(__STDC_IEC_559__) && !defined(__GCC_IEC_559)
#error "Requires IEEE 754 floating point!"
#endif

#if defined(_DEBUG) && defined(_WIN32)
#include <crtdbg.h>
#endif

#include "cli.h"
#include "str.h"

#include "amigaobject.h"
#include "binaryobject.h"
#include "dependency.h"
#include "elf.h"
#include "errors.h"
#include "lexer.h"
#include "object.h"
#include "options.h"
#include "parse.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"
#include "tokens.h"
#include "xasm.h"

#include "amitime.h"
#include "mem.h"
#include "util.h"

uint32_t xasm_TotalLines = 0;
uint32_t xasm_TotalErrors = 0;
uint32_t xasm_TotalWarnings = 0;

const SConfiguration* xasm_Configuration = NULL;

static bool
writeOutput(char format, string* outputFilename, string* sourceFilename) {
	switch (format) {
		case 'x':
			return obj_Write(outputFilename);
		case 'e':
			return elf_Write(outputFilename, xasm_Configuration->defaultEndianness == ASM_BIG_ENDIAN, EM_68K);
		case 'b':
			return bin_Write(outputFilename);
		case 'v':
			return bin_WriteVerilog(outputFilename);
		case 'g':
			return ami_WriteExecutable(outputFilename);
		case 'h':
			return ami_WriteObject(outputFilename, sourceFilename);
		default:
			return false;
	}
}

/* Command-line state, shared by the option handlers and the teardown. */
static void
reportSuccess(clock_t startClock, bool verbose) {
	if (!verbose)
		return;

	clock_t endClock = clock();
	float timespent = ((float) (endClock - startClock)) / CLOCKS_PER_SEC;
	printf("Success! %u lines in %.02f seconds ", xasm_TotalLines, timespent);
	if (timespent == 0) {
		printf("\n");
	} else {
		printf("(%d lines/minute)\n", (int) (60 / timespent * xasm_TotalLines));
	}
	if (xasm_TotalWarnings != 0) {
		printf("Encountered %u warnings\n", xasm_TotalWarnings);
	}
}

/* Tear down the assembler subsystems in reverse order of init. */
static void
cleanupSession(void) {
	opt_Close();

	dep_Exit();
	sym_Exit();
	lex_Exit();
	sect_Exit();
	parse_Exit();

	mem_ShowLeaks();
}
extern int
xasm_Main(const SConfiguration* configuration, int argc, char* argv[]) {
	xasm_Configuration = configuration;

	int argn = 1;
	int rcode;

#if defined(_DEBUG) && defined(_WIN32) && !defined(__MINGW32__)
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF |
	               _CRTDBG_DELAY_FREE_MEM_DF);
	atexit(getchar);
#endif

	clock_t startClock = clock();

	argc -= 1;
	if (argc == 0)
		cli_PrintUsage();

	err_Init();
	sect_Init();
	sym_Init();
	opt_Open();

	SCLiState cliState;
	cli_Init(&cliState);

	cli_ParseOptions(&cliState, &argc, &argn, argv);

	if (xasm_TotalErrors == 0) {
		xasm_Configuration->defineSymbols();

		rcode = EXIT_SUCCESS;

		if (argc == 1) {
			string* sourcePath = str_Create(argv[argn]);

			if (lex_Init(sourcePath)) {
				tokens_Init(configuration->supportFloat);
				if (configuration->supportFloat) {
					assert(sizeof(float) == 4);
					assert(sizeof(double) == 8);
				}
				xasm_Configuration->defineTokens();
				opt_Updated();
				cli_EvaluateDefines(cliState.cliDefines);

				bool parseResult = parse_Do();

				if (parseResult) {
					patch_OptimizeAll();
					patch_BackPatch();

					sym_ErrorOnUndefined();
				}

				if (parseResult && xasm_TotalErrors == 0) {
					reportSuccess(startClock, cliState.verbose);

					if (cliState.outputFilename != NULL) {
						dep_SetMainOutput(cliState.outputFilename);
						dep_WriteDependencyFile();
						if (!writeOutput(cliState.format, cliState.outputFilename, sourcePath)) {
							dep_RemoveDependencyfile();
							remove(str_String(cliState.outputFilename));
						}
					}
				}
			}
			str_Free(sourcePath);
		} else if (argc > 1) {
			err_Error(ERROR_TOO_MANY_FILES, argv[argn]);
		}
	}

	err_PrintAll();
	if (xasm_TotalErrors > 0) {
		rcode = EXIT_FAILURE;
	}

	cli_Free(&cliState);
	cleanupSession();

	return rcode;
}
