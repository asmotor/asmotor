// -w1008 -oStartup.o -fh -iC:\Users\cso\UAE\HD2\Asm\Include -iC:\Users\cso\UAE\HD2\Projects\DemoSys2  C:\Users\cso\UAE\HD2\Projects\Demosys2\Startup.S

/*  Copyright 2008 Carsten Sørensen

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

#include "asmotor.h"
#include "amitime.h"
#include "section.h"
#include "symbol.h"
#include "globlex.h"
#include "options.h"
#include "project.h"
#include "fstack.h"
#include "parse.h"
#include "patch.h"
#include "object.h"
#include "binobj.h"
#include "xasm.h"




/*	Some global variables*/

ULONG g_nTotalLines = 0;
ULONG g_nTotalErrors = 0;
ULONG g_nTotalWarnings = 0;




/*	Various system kludges*/

#if	defined(__GNUC__) && !defined(__DJGPP__)
void strupr(char* s)
{
	while(*s)
	{
		*s = toupper(*s);
		s += 1;
	}
}

void strlwr(char* s)
{
	while(*s)
	{
		*s = tolower(*s);
		s += 1;
	}
}
#endif




/*	Help text*/

void PrintUsage(void)
{
    printf(
		"%s v%s, ASMotor v" ASMOTOR_VERSION ")\n\nUsage: %s [options] asmfile\n"
   		"Options:\n"
   		"    -b<AS>  Change the two characters used for binary constants\n"
		"            (default is 01)\n"
   		"    -e(l|b) Change endianness (CAUTION!)\n"
		"    -f<f>   Output format, one of\n"
		"                x - xobj (default)\n"
		"                b - binary file \n"
#ifdef LOCAL_SUPPORT_AMIGA
		"                g - Amiga executable file\n"
		"                h - Amiga object file\n"
#endif
		"    -h      This text\n"
   		"    -i<dir> Extra include path (can appear more than once)\n"
   		"    -o<f>   Write assembly output to <file>\n"
		"    -v      Verbose text output\n"
		"    -w<d>   Disable warning <d> (four digits)\n"
   		"    -z<XX>  Set the byte value (hex format) used for uninitialised\n"
		"            data (default is ? for random)\n"
		"\n"
		"Machine specific options:\n",
		g_pConfiguration->pszExecutable, g_pConfiguration->pszBackendVersion, g_pConfiguration->pszExecutable);
	locopt_PrintOptions();
    exit(EXIT_SUCCESS);
}




/*	This thing runs the show*/

extern int xasm_Main(int argc, char* argv[])
{
	char format = 'x';
	int	argn = 1;
	int	rcode;
	clock_t	StartClock;
	clock_t EndClock;
	char* outname = NULL;
	BOOL debuginfo = FALSE;
	BOOL verbose = FALSE;

#if defined(_DEBUG)
	atexit(getchar);
#endif

    StartClock = clock();

	argc -= 1;
	if(argc == 0)
		PrintUsage();

	sect_Init();
	sym_Init();
	globlex_Init();
	loclexer_Init();

	opt_Open();

	while(argc && argv[argn][0] == '-')
	{
		switch(argv[argn][1])
		{
			case '?':
			case 'h':
				PrintUsage();
				break;
			case 'g':
				debuginfo = TRUE;
				break;
			case 'v':
				verbose = TRUE;
				break;
			case 'f':
				if(strlen(argv[argn]) > 2)
				{
					switch(argv[argn][2])
					{
						case 'x':
						case 'b':
#ifdef LOCAL_SUPPORT_AMIGA
						case 'g':
						case 'h':
#endif
							format = argv[argn][2];
							break;
						default:
							prj_Warn(WARN_OPTION, argv[argn]);
							break;
					}
				}
				break;
			case 'o':
				outname = &argv[argn][2];
				break;
			case 'i':
			case 'e':
			case 'm':
			case 'b':
			case 'w':
			case 'z':
				opt_Parse(&argv[argn][1]);
				break;
			default:
				prj_Warn(WARN_OPTION, argv[argn]);
				break;
		}
		++argn;
		--argc;
	}

	rcode = EXIT_SUCCESS;

	if(argc == 1)
	{
		char* source = argv[argn];
		if(fstk_Init(source))
		{
			BOOL b = parse_Do();

			if(b)
			{
				patch_OptimizeAll();
				patch_BackPatch();
			}

			if(b && g_nTotalErrors == 0)
			{
				float timespent;
				BOOL wr = FALSE;

				if(verbose)
				{
					EndClock = clock();

					timespent = ((float)(EndClock - StartClock))/CLOCKS_PER_SEC;
					printf("Success! %lu lines in %.02f seconds ", g_nTotalLines, timespent);
					if(timespent == 0)
					{
						printf("\n");
					}
					else
					{
						printf("(%d lines/minute)\n", (int)(60/timespent*g_nTotalLines));
					}
					if(g_nTotalWarnings != 0)
					{
						printf("Encountered %lu warnings\n", g_nTotalWarnings);
					}
				}

				if(outname != NULL)
				{
					switch(format)
					{
						case 'x':
							wr = obj_Write(outname);
							break;
						case 'b':
							wr = bin_Write(outname);
							break;
#ifdef LOCAL_SUPPORT_AMIGA
						case 'g':
							wr = ami_WriteExecutable(outname, debuginfo);
							break;
						case 'h':
							wr = ami_WriteObject(outname, source, debuginfo);
							break;
#endif
					}
					if(!wr)
					{
						remove(outname);
					}
				}
			}
			else
			{
				if(verbose)
				{
					printf("Encountered %lu error%s", g_nTotalErrors, g_nTotalErrors > 1 ? "s" : "");
					if(g_nTotalWarnings != 0)
						printf(" and %lu warning%s\n", g_nTotalWarnings, g_nTotalWarnings > 1 ? "s" : "");
					else
						printf("\n");
				}
				rcode = EXIT_FAILURE;
			}
			fstk_Cleanup();
		}
	}

	opt_Close();

	return rcode;
}