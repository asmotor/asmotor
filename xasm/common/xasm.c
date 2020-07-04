/*  Copyright 2008-2017 Carsten Elton Sorensen

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_DEBUG) && defined(_WIN32)
#include <crtdbg.h>
#endif

#include "xasm.h"
#include "asmotor.h"
#include "amitime.h"
#include "amigaobject.h"
#include "binaryobject.h"
#include "dependency.h"
#include "errors.h"
#include "filestack.h"
#include "object.h"
#include "options.h"
#include "parse.h"
#include "patch.h"
#include "section.h"
#include "symbol.h"
#include "tokens.h"

uint32_t xasm_TotalLines = 0;
uint32_t xasm_TotalErrors = 0;
uint32_t xasm_TotalWarnings = 0;

const SConfiguration* xasm_Configuration = NULL;


static void
printUsage(void) {
    printf("%s v%s, ASMotor v" ASMOTOR_VERSION "\n\nUsage: %s [options] asmfile\n"
           "Options:\n"
           "    -a<n>    Section alignment when writing binary file (default is %d bytes)\n"
           "    -b<AS>   Change the two characters used for binary constants\n"
           "             (default is 01)\n"
           "    -d<FILE> Output dependency file for GNU Make\n"
           "    -D<NAME> Define EQU symbol with the value 1\n"
           "    -e(l|b)  Change endianness (CAUTION!)\n"
           "    -fF      Output format, one of\n"
           "                 x - xobj (default)\n"
           "                 b - binary file\n"
           "                 v - verilog readmemh file\n", xasm_Configuration->executableName,
           xasm_Configuration->backendVersion, xasm_Configuration->executableName, xasm_Configuration->sectionAlignment);

    if (xasm_Configuration->supportAmiga) {
        printf("                 g - Amiga executable file\n"
               "                 h - Amiga object file\n");
    }

    printf("    -g       Include debug information\n"
           "    -h       This text\n"
           "    -i<dir>  Extra include path (can appear more than once)\n"
           "    -o<f>    Write assembly output to <file>\n"
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

#if defined(_DEBUG) && defined(_WIN32)
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG)| _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF);
    atexit(getchar);
#endif

    clock_t startClock = clock();

    argc -= 1;
    if (argc == 0)
        printUsage();

    err_Init();
    sect_Init();
    sym_Init();
    tokens_Init();
    xasm_Configuration->defineTokens();

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
                        case 'x':
                        case 'b':
                        case 'v':
                            format = argv[argn][2];
                            break;
                        case 'g':
                        case 'h':
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
                            if (xasm_Configuration->supportAmiga) {
                                format = argv[argn][2];
                                break;
                            }
#pragma GCC diagnostic pop 
                            // Fall through                          
                        default:
                            err_Warn(WARN_OPTION, argv[argn]);
                            break;
                    }
                }
                break;
            case 'o':
                outputFilename = str_Create(&argv[argn][2]);
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

    rcode = EXIT_SUCCESS;

    if (argc == 1) {
        string* source = str_Create(argv[argn]);
        if (fstk_Init(source)) {
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
                    if (writeOutput(format, outputFilename, source)) {
                        dep_WriteDependencyFile();
                    } else  {
                        remove(str_String(outputFilename));
                    }
                }
            } 

            if (xasm_TotalErrors > 0) {
                err_PrintAll();

                if (verbose) {
                    printf("Encountered %u error%s", xasm_TotalErrors, xasm_TotalErrors > 1 ? "s" : "");
                    if (xasm_TotalWarnings != 0)
                        printf(" and %u warning%s\n", xasm_TotalWarnings, xasm_TotalWarnings > 1 ? "s" : "");
                    else
                        printf("\n");
                }
                rcode = EXIT_FAILURE;
            }
            fstk_Cleanup();
        }
        str_Free(source);
    } else if (argc > 1) {
        err_Error(ERROR_TOO_MANY_FILES);
    }

    str_Free(outputFilename);
    opt_Close();

    return rcode;
}
