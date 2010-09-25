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
#include <stdarg.h>

void	Error(char* fmt, ...)
{
	va_list	list;
	char	temp[256];

	va_start(list, fmt);
	vsprintf(temp, fmt, list);

	printf("*ERROR* : %s\n", temp);
	va_end(list);
	exit(EXIT_FAILURE);
}

/*	Help text */

void    PrintUsage(void)
{
    printf(	"xlink v" LINK_VERSION " (part of ASMotor " ASMOTOR_VERSION ")\n"
			"\n"
			"Usage: xlink [options] [file1[file2[...[filen]]]]\n"
    		"Options: (a forward slash (/) can be used instead of the dash (-))\n"
			"\t-@ <file>\tRead <file> for options\n"
			"\t-h\t\tThis text\n"
			"\t-m <mapfile>\tWrite a mapfile\n"
			"\t-o <output>\tWrite output to file <output>\n"
    		"\t-s <symbol>\tPerform smart linking starting with <symbol>\n"
			"\t-t\t\tOutput target\n"
			"\t    -tg\t\tGameboy ROM image(default)\n"
			"\t    -ts\t\tGameboy small mode (32kB)\n"
			"\t    -tm <mach>\tUse file <mach>\n"
    		"\t-z <hx>\t\tSet the byte value (hex format) used for uninitialised\n"
			"\t\t\tdata (default is ? for random)\n"
			);
    exit(EXIT_SUCCESS);
}

int	GetLine(char* s, int size, FILE* f)
{
	int ch;
	int	read;

	ch = fgetc(f);
	read = 0;
	while(ch != EOF && ch != 10 && ch != 13 && read < size)
	{
		*s++ = (char)ch;
		ch = fgetc(f);
		++read;
	}

	*s = 0;

	return ch != EOF;
}

void	ProcessFileLine(char* s1, void(*func)(char* ))
{
	char* s2;
	int		ok;

	ok=1;

	while(ok)
	{
		while(isspace(*s1) && *s1!=0)
		{
			s1+=1;
		}

		if(*s1)
		{
			char* s3;

			s2=s1;
			while(*s2!=',' && *s2!=0)
			{
				s2+=1;
			}

			if(*s2==0)
			{
				ok=0;
			}

			*s2=0;
			s3=s2-1;
			while(isspace(*s3))
			{
				*s3--=0;
			}


			if(*s1=='\"')
			{
				s1+=1;
			}

			if(*s3=='\"')
			{
				*s3=0;
			}

			func(s1);

			s1=s2+1;
		}
		else
		{
			ok=0;
		}
	}
}

void	ParseResponseFile(char* filename)
{
	if(filename)
	{
		FILE* f;

		if((f=fopen(filename,"rb"))!=NULL)
		{
			char	text[1024];

			GetLine(text, 1024, f);

			do
			{
				if(text[0]!='#')
				{
					if(strnicmp(text,"files",5)==0)
					{
						ProcessFileLine(&text[5], obj_Read);
					}
					else if(strnicmp(text,"lib",3)==0)
					{
						ProcessFileLine(&text[3], obj_Read);
					}
					else if(strnicmp(text,"output",3)==0)
					{
						ProcessFileLine(&text[6], output_SetFilename);
					}
					else if(strnicmp(text,"mapfile",7)==0)
					{
						ProcessFileLine(&text[7], map_SetFilename);
					}
					/*
					else
					{
						Error("Invalid command \"%s\"", text);
					}
					*/
				}
			} while(GetLine(text,1024,f));
		}
	}
}


/*	This thing runs the show */

int	main(int argc, char* argv[])
{
	int		argn=1;
	int		target_defined=0;
	char* smartlink=NULL;

	argc-=1;
	if(argc==0)
	{
		PrintUsage();
	}

	while(argc && ((argv[argn][0]=='-')||(argv[argn][0]=='/')))
	{
		switch(tolower(argv[argn][1]))
		{
			case '?':
			case 'h':
				argn+=1;
				argc-=1;
				PrintUsage ();
				break;
			case '@':
				/* Response file */
				argn+=1;
				argc-=1;
				if(argc && argv[argn])
				{
					ParseResponseFile(argv[argn]);
					argn+=1;
					argc-=1;
				}
				else
				{
					Error("option \"@\" needs an argument");
				}
				break;
			case 'm':
				/* MapFile */
				argn+=1;
				argc-=1;
				if(argc && argv[argn])
				{
					map_SetFilename(argv[argn]);
					argn+=1;
					argc-=1;
				}
				else
				{
					Error("option \"m\" needs an argument");
				}
				break;
			case 'o':
				/* Output filename */
				argn+=1;
				argc-=1;
				if(argc && argv[argn])
				{
					output_SetFilename(argv[argn]);
					argn+=1;
					argc-=1;
				}
				else
				{
					Error("option \"o\" needs an argument");
				}
				break;
			case 's':
				/* Smart linking */
				argn+=1;
				argc-=1;
				if(argc && argv[argn])
				{
					smartlink=argv[argn];
					argn+=1;
					argc-=1;
				}
				else
				{
					Error("option \"s\" needs an argument");
				}
				break;
			case 't':
				/* Target */
				if(target_defined==0)
				{
					switch(tolower(argv[argn][2]))
					{
						case	'g':
						{
							/* Gameboy ROM image */
							group_SetupGameboy();
							target_defined=1;
							argn+=1;
							argc-=1;
							break;
						}
						case	's':
						{
							/* Gameboy small mode ROM image */
							group_SetupSmallGameboy();
							target_defined=1;
							argn+=1;
							argc-=1;
							break;
						}
						case	'm':
						{
							/* Use machine def file */
							target_defined=1;
							Error("option \"tm\" not implemented yet");
							++argn;
							--argc;
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
				break;
		}
	}

	while(argc && argv[argn])
	{
		obj_Read(argv[argn]);
		argn+=1;
		argc-=1;
	}

	smart_Process(smartlink);
	assign_Process();
	patch_Process();

	return EXIT_SUCCESS;
}