/*  Copyright 2008-2015 Carsten Elton Sorensen

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

#include "xlink.h"
#include "asmotor.h"
#include <stdarg.h>


typedef enum
{
	TARGET_BINARY,
	TARGET_EXECUTABLE
} TargetType;


void Error(char* fmt, ...)
{
	va_list	list;

	va_start(list, fmt);

	printf("ERROR: ");
	vprintf(fmt, list);
	printf("\n");

	va_end(list);

	exit(EXIT_FAILURE);
}


static void printUsage(void)
{
    printf(	"xlink v" LINK_VERSION " (part of ASMotor " ASMOTOR_VERSION ")\n"
			"\n"
			"Usage: motorlink [options] file1 file2 ... filen\n"
    		"Options: (a forward slash (/) can be used instead of the dash (-))\n"
			"\t-h\t\tThis text\n"
//			"\t-m<mapfile>\tWrite a mapfile\n"
			"\t-o<output>\tWrite output to file <output>\n"
//			"\t-s<symbol>\tPerform smart linking starting with <symbol>\n"
			"\t-t\t\tOutput target\n"
			"\t    -ta\t\tAmiga executable\n"
			"\t    -tg\t\tGameboy ROM image\n"
			"\t    -ts\t\tGameboy small mode (32 KiB)\n"
//			"\t    -tm<mach>\tUse file <mach>\n"
			);
    exit(EXIT_SUCCESS);
}


int	main(int argc, char* argv[])
{
	int argn = 1;
	bool_t targetDefined = false;
	TargetType targetType;

	char* outputFilename = NULL;
	char* smartlink = NULL;

	if (argc == 1)
		printUsage();

	while (argn < argc && (argv[argn][0] == '-' || argv[argn][0] == '/'))
	{
		switch (tolower((unsigned char)argv[argn][1]))
		{
			case '?':
			case 'h':
				++argn;
				printUsage();
				break;
			case 'm':
				/* MapFile */
				if (argv[argn][2] != 0)
				{
					map_SetFilename(&argv[argn][2]);
					++argn;
				}
				else
				{
					Error("option \"m\" needs an argument");
				}
				break;
			case 'o':
				/* Output filename */
				if (argv[argn][2] != 0)
				{
					outputFilename = &argv[argn][2];
					++argn;
				}
				else
				{
					Error("option \"o\" needs an argument");
				}
				break;
			case 's':
				/* Smart linking */
				if (argv[argn][2] != 0)
				{
					smartlink = &argv[argn][2];
					++argn;
				}
				else
				{
					Error("option \"s\" needs an argument");
				}
				break;
			case 't':
				/* Target */
				if(argv[argn][2] != 0 && !targetDefined)
				{
					switch(tolower((unsigned char)argv[argn][2]))
					{
						case 'a':
						{
							/* Amiga executable */
							group_SetupAmigaExecutable();
							targetDefined = true;
							targetType = TARGET_EXECUTABLE;
							++argn;
							break;
						}
						case 'g':
						{
							/* Gameboy ROM image */
							group_SetupGameboy();
							targetDefined = true;
							targetType = TARGET_BINARY;
							++argn;
							break;
						}
						case 's':
						{
							/* Gameboy small mode ROM image */
							group_SetupSmallGameboy();
							targetDefined = true;
							targetType = TARGET_BINARY;
							++argn;
							break;
						}
#if 0
						case 'm':
						{
							/* Use machine def file */
							targetDefined = 1;
							Error("option \"tm\" not implemented yet");
							++argn;
							break;
						}
#endif
					}
				}
				else
				{
					Error("more than one target (option \"t\") defined");
				}

				break;
			default:
				printf("Unknown option \"%s\", ignored\n", argv[argn]);
				++argn;
				break;
		}
	}

	if (!targetDefined)
	{
		Error("No target format defined");
	}

	while (argn < argc && argv[argn])
	{
		obj_Read(argv[argn++]);
	}

	smart_Process(smartlink);

	if (targetType == TARGET_BINARY)
		assign_Process();
	else // if (targetType == TARGET_EXECUTABLE)
		reloc_Process();

	patch_Process();

	if (outputFilename)
		output_WriteImage(outputFilename);

	return EXIT_SUCCESS;
}
