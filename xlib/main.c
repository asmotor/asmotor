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
#include <ctype.h>

#include "asmotor.h"
#include "types.h"
#include "library.h"


/*
 * Print out an errormessage
 *
 */

void fatalerror(char* s)
{
	printf("*ERROR* : %s\n", s);
	exit(EXIT_FAILURE);
}

/*
 * Print the usagescreen
 *
 */

void	PrintUsage(void)
{
	printf( "xLib v" LIB_VERSION " (part of ASMotor " ASMOTOR_VERSION ")\n\n"
			"Usage: xlib library command [module1 module2 ... modulen]\n"
			"Commands:\n\ta\tAdd/replace modules to library\n"
			"\td\tDelete modules from library\n"
			"\tl\tList library contents\n"
			"\tx\tExtract modules from library\n" );
	exit(0);
}

/*
 * The main routine
 *
 */

int	main(int argc, char* argv[])
{
	int32_t	argn=0;
	char* libname;

	argc-=1;
	argn+=1;

	if(argc>=2)
	{
		uint8_t		command;
		SLibrary* lib;

		lib=lib_Read(libname=argv[argn++]);
		argc-=1;

		if(strlen(argv[argn])==1)
		{
			command=argv[argn++][0];
			argc-=1;

			switch(tolower(command))
			{
				case	'a':
					while(argc)
					{
						lib=lib_AddReplace(lib, argv[argn++]);
						argc-=1;
					}
					lib_Write(lib, libname);
					lib_Free(lib);
					break;
				case	'd':
					while(argc)
					{
						lib=lib_DeleteModule(lib, argv[argn++]);
						argc-=1;
					}
					lib_Write(lib, libname);
					lib_Free(lib);
					break;
				case	'l':
					{
						SLibrary* l;

						l=lib;

						while(l)
						{
							printf("%10ld %s\n", l->nByteLength, l->tName);
							l=l->pNext;
						}
					}
					break;
				case	'x':
					while(argc)
					{
						SLibrary* l;

						l=lib_Find(lib, argv[argn]);
						if(l)
						{
							FILE* f;

							if((f = fopen(argv[argn],"wb")) != NULL)
							{
								fwrite(l->pData, sizeof(uint8_t), l->nByteLength, f);
								fclose(f);
								printf("Extracted module '%s'\n", argv[argn]);
							}
							else
								fatalerror("Unable to write module");
						}
						else
							fatalerror("Module not found");

						argn+=1;
						argc-=1;
					}
					lib_Free(lib);
					break;
				default:
					fatalerror("Invalid command");
					break;
			}

		}
		else
		{
			fatalerror("Invalid command");
		}
	}
	else
		PrintUsage();

	return 0;
}
