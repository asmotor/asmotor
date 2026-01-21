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
#include "util.h"

uint32_t xasm_TotalLines = 0;
uint32_t xasm_TotalErrors = 0;
uint32_t xasm_TotalWarnings = 0;

const SConfiguration* xasm_Configuration = NULL;

static void
printUsage(void) {
	printf("%s v" ASMOTOR_VERSION "\n\nUsage: %s [options] asmfile\n"
	       "Options:\n"
	       "    -a<n>    Section alignment when writing binary file (default is %d bytes)\n"
	       "    -b<AS>   Change the two characters used for binary constants\n"
	       "             (default is 01)\n"
	       "    -d<FILE> Output dependency file for GNU Make\n"
	       "    -D<NAME> Define EQU symbol with the value 1\n"
	       "    -e(l|b)  Change endianness\n"
	       "    -f<F>    Output format, one of\n"
	       "                 x - xobj (default)\n"
	       "                 e - ELF object file\n"
	       "                 b - binary file\n"
	       "                 v - verilog readmemh file\n",
	       xasm_Configuration->executableName, xasm_Configuration->executableName, xasm_Configuration->sectionAlignment);

	if (xasm_Configuration->supportAmiga) {
		printf("                 g - Amiga executable file\n"
		       "                 h - Amiga object file\n");
	}

	printf("    -g       Include debug information\n"
	       "    -h       This text\n"
	       "    -i<dir>  Extra include path (can appear more than once)\n"
	       "    -o<f>    Write assembly output to <file>\n"
	       "    -s<file> Use section types from machine definition file\n"
	       "    -v       Verbose text output\n"
	       "    -w<d>    Disable warning <d> (four digits)\n"
	       "    -z<XX>   Set the byte value (hex format) used for uninitialised\n"
	       "             data (default is FF)\n"
	       "\n"
	       "Machine specific options:\n");
	xasm_Configuration->printOptionUsage();
	exit(EXIT_SUCCESS);
}

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
		printUsage();

	err_Init();
	sect_Init();
	sym_Init();
	opt_Open();

	char format = 'x';
	string* outputFilename = NULL;
	bool verbose = false;
	while (argc && argv[argn][0] == '-') {
		switch (argv[argn][1]) {
			case '?':
			case 'h':
				printUsage();
				break;
			case 'd':
				dep_Initialize(&argv[argn][2]);
				break;
			case 'D': {
				string* name = str_Create(&argv[argn][2]);
				sym_CreateEqu(name, 1);
				str_Free(name);
				break;
			}
			case 'f':
				if (strlen(argv[argn]) > 2) {
					switch (argv[argn][2]) {
						case 'e':
							if (xasm_Configuration->supportELF) {
								format = argv[argn][2];
								break;
							}
							err_Warn(WARN_OPTION, argv[argn]);
							break;
						case 'x':
						case 'b':
						case 'v':
							format = argv[argn][2];
							break;
						case 'g':
						case 'h':
							if (xasm_Configuration->supportAmiga) {
								format = argv[argn][2];
								break;
							}
							err_Warn(WARN_OPTION, argv[argn]);
							break;
						default:
							err_Warn(WARN_OPTION, argv[argn]);
							break;
					}
				}
				break;
			case 'o':
				outputFilename = str_Create(&argv[argn][2]);
				break;
			case 's':
				if (sym_ReadMachineDefinitionFile(&argv[argn][2])) {
					opt_Current->createGroups = false;
				}
				break;
			case 'v':
				verbose = true;
				break;
			case 'a':
			case 'b':
			case 'e':
			case 'g':
			case 'i':
			case 'm':
			case 'w':
			case 'z':
				opt_Parse(&argv[argn][1]);
				break;
			default:
				err_Warn(WARN_OPTION, argv[argn]);
				break;
		}
		++argn;
		--argc;
	}

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

				bool parseResult = parse_Do();

				if (parseResult) {
					patch_OptimizeAll();
					patch_BackPatch();

					sym_ErrorOnUndefined();
				}

				if (parseResult && xasm_TotalErrors == 0) {
					if (verbose) {
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

					if (outputFilename != NULL) {
						dep_SetMainOutput(outputFilename);
						dep_WriteDependencyFile();
						if (!writeOutput(format, outputFilename, sourcePath)) {
							dep_RemoveDependencyfile();
							remove(str_String(outputFilename));
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

	str_Free(outputFilename);
	opt_Close();

	dep_Exit();
	sym_Exit();
	lex_Exit();
	sect_Exit();

	//	mem_ShowLeaks();

	return rcode;
}
