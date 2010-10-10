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

#include "xlink.h"
#include "asmotor.h"
#include <stdarg.h>

void Error(char* fmt, ...)
{
	va_list	list;
	char temp[256];

	va_start(list, fmt);
	vsprintf(temp, fmt, list);

	printf("*ERROR* : %s\n", temp);
	va_end(list);
	exit(EXIT_FAILURE);
}

/*	Help text */

void PrintUsage(void)
{
    printf(	"motorlink v" LINK_VERSION " (part of ASMotor " ASMOTOR_VERSION ")\n"
			"\n"
			"Usage: motorlink [options] file1 file2 ... filen\n"
    		"Options: (a forward slash (/) can be used instead of the dash (-))\n"
			"\t-h\t\tThis text\n"
			"\t-m<mapfile>\tWrite a mapfile\n"
			"\t-o<output>\tWrite output to file <output>\n"
    		"\t-s<symbol>\tPerform smart linking starting with <symbol>\n"
			"\t-t\t\tOutput target\n"
			"\t    -tg\t\tGameboy ROM image(default)\n"
			"\t    -ts\t\tGameboy small mode (32kB)\n"
			"\t    -tm<mach>\tUse file <mach>\n"
    		"\t-z<hx>\t\tSet the byte value (hex format) used for uninitialised\n"
			"\t\t\tdata (default is ? for random)\n"
			);
    exit(EXIT_SUCCESS);
}



int	main(int argc, char* argv[])
{
	int argn = 1;
	int target_defined = 0;
	char* smartlink = NULL;

	if(argc == 1)
		PrintUsage();

	while(argn < argc && (argv[argn][0] == '-' || argv[argn][0] == '/'))
	{
		switch(tolower(argv[argn][1]))
		{
			case '?':
			case 'h':
				++argn;
				PrintUsage();
				break;
			case 'm':
				/* MapFile */
				if(argv[argn][2] != 0)
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
				if(argv[argn][2] != 0)
				{
					output_SetFilename(&argv[argn][2]);
					++argn;
				}
				else
				{
					Error("option \"o\" needs an argument");
				}
				break;
			case 's':
				/* Smart linking */
				if(argv[argn][2] != 0)
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
				if(argv[argn][2] != 0 && target_defined == 0)
				{
					switch(tolower(argv[argn][2]))
					{
						case 'g':
						{
							/* Gameboy ROM image */
							group_SetupGameboy();
							target_defined = 1;
							++argn;
							break;
						}
						case 's':
						{
							/* Gameboy small mode ROM image */
							group_SetupSmallGameboy();
							target_defined = 1;
							++argn;
							break;
						}
						case 'm':
						{
							/* Use machine def file */
							target_defined = 1;
							Error("option \"tm\" not implemented yet");
							++argn;
							break;
						}
					}
				}
				else
				{
					Error("more than one target (option \"t\") defined");
				}

				break;
			case 'z':
				/* Fill byte */
				break;
			default:
				printf("Unknown option \"%s\", ignored\n", argv[argn]);
				++argn;
				break;
		}
	}

	while(argn < argc && argv[argn])
	{
		obj_Read(argv[argn]);
		++argn;
	}

	smart_Process(smartlink);
	assign_Process();
	patch_Process();
	output_WriteRomImage();

	return EXIT_SUCCESS;
}