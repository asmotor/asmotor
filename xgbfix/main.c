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

#define XGBFIX_VERSION "1.0.0"

#define POS_NINTENDO_LOGO	0x0104L
#define POS_CARTRIDGE_TITLE 0x0134L
#define POS_CARTRIDGE_TYPE	0x0147L
#define POS_ROM_SIZE		0x0148L
#define POS_COMP_CHECKSUM	0x014DL
#define POS_CHECKSUM		0x014EL


/*
 * Option defines
 *
 */

#define OPTF_DEBUG		0x01L
#define OPTF_PAD		0x02L
#define OPTF_VALIDATE	0x04L
#define OPTF_TITLE		0x08L

int g_nOptions;

int IsOptionSet(int nOption)
{
	return g_nOptions & nOption ? 1 : 0;
}


/*
 * Misc. variables
 *
 */

unsigned char g_NintendoChar[48]=
{
	0xCE,0xED,0x66,0x66,0xCC,0x0D,0x00,0x0B,0x03,0x73,0x00,0x83,0x00,0x0C,0x00,0x0D,
	0x00,0x08,0x11,0x1F,0x88,0x89,0x00,0x0E,0xDC,0xCC,0x6E,0xE6,0xDD,0xDD,0xD9,0x99,
	0xBB,0xBB,0x67,0x63,0x6E,0x0E,0xEC,0xCC,0xDD,0xDC,0x99,0x9F,0xBB,0xB9,0x33,0x3E
};




/*
 * Misc. routines
 *
 */

void PrintUsage( void )
{
	printf("motorgbfix v" XGBFIX_VERSION " (part of ASMotor " ASMOTOR_VERSION ")\n\n");
	printf("Usage: rgbfix [options] image[.gb]\n");
	printf("Options:\n");
	printf("\t-h\t\tThis text\n");
	printf("\t-d\t\tDebug: Don't change image\n");
	printf("\t-p\t\tPad image to valid size\n\t\t\tPads to 32/64/128/256/512kB as appropriate\n");
	printf("\t-t<name>\tChange cartridge title field (16 characters)\n");
	printf("\t-v\t\tValidate header\n\t\t\tCorrects - Nintendo Character Area (0x0104)\n\t\t\t\t - ROM type (0x0147)\n\t\t\t\t - ROM size (0x0148)\n\t\t\t\t - Checksums (0x014D-0x014F)\n");
	exit(EXIT_SUCCESS);
}

void FatalError(char* s)
{
	printf("ERROR: %s\n\n", s);
	exit(EXIT_FAILURE);
}


/*
 * File helper routines
 *
 */

long int FileSize(FILE* f)
{
	long nPrevPos;
	long r;

	fflush(f);
	nPrevPos = ftell(f);
	fseek(f, 0, SEEK_END);
	r = ftell(f);
	fseek(f, nPrevPos, SEEK_SET);
	return r;
}

int FileExists(char* s)
{
	FILE* f;

	if((f = fopen(s, "rb")) != NULL)
	{
		fclose(f);
		return 1;
	}
	else
		return 0;
}


/*
 * ROM image validation routines
 *
 */

void ValidateNintendoCharacterArea(FILE* f)
{
	int i;
	int nBytesChanged = 0;

	fflush(f);
	fseek(f, POS_NINTENDO_LOGO, SEEK_SET);

	for(i = 0; i < 48; ++i)
	{
		int ch = fgetc( f );
		if(ch == EOF)
			ch = 0x00;
		if(ch != g_NintendoChar[i])
		{
			++nBytesChanged;

			if(!IsOptionSet(OPTF_DEBUG))
			{
				fflush(f);
				fseek(f, POS_NINTENDO_LOGO + i, SEEK_SET);
				fwrite(&g_NintendoChar[i], 1, 1, f);
				fflush(f);
			}
		}
	}

	if(IsOptionSet(OPTF_DEBUG))
	{
		if(nBytesChanged != 0)
			printf("\tChanged %d bytes in the Nintendo Character Area\n", nBytesChanged);
		else
			printf("\tNintendo Character Area is OK\n");
	}
}

void ValidateRomSize(FILE* f)
{
	int nCartRomSize;
	long nFilesize;
	unsigned char nCalcRomSize = 0;

	fflush(f);
	fseek(f, POS_ROM_SIZE, SEEK_SET);

	nCartRomSize = fgetc(f);
	if(nCartRomSize == EOF)
		nCartRomSize = 0x00;

	nFilesize = FileSize(f);
	while(nFilesize > (0x8000L << nCalcRomSize))
		++nCalcRomSize;

	if(nCalcRomSize == nCartRomSize)
	{
		if(IsOptionSet(OPTF_DEBUG))
			printf("\tROM size byte is OK\n");
		return;
	}

	if(IsOptionSet(OPTF_DEBUG))
	{
		printf("\tChanged ROM size byte from 0x%02X (%ldkB) to 0x%02X (%ldkB)\n",
			nCartRomSize, (0x8000L << nCartRomSize) / 1024,
			nCalcRomSize, (0x8000L << nCalcRomSize) / 1024);
	}
	else
	{
		fseek(f, POS_ROM_SIZE, SEEK_SET);
		fwrite(&nCalcRomSize, 1, 1, f);
		fflush(f);
	}
}

void ValidateCartridgeType(FILE* f)
{
	int nCartType;

	fflush(f);
	fseek(f, POS_CARTRIDGE_TYPE, SEEK_SET);

	nCartType = fgetc( f );
	if(nCartType == EOF)
		nCartType = 0x00;

	if(FileSize(f) <= 0x8000L || nCartType != 0x00)
	{
		/* carttype byte can be anything? */
		if(IsOptionSet(OPTF_DEBUG))
			printf("\tCartridge type byte is OK\n");
		return;
	}

	if(IsOptionSet(OPTF_DEBUG))
	{
		printf("\tCartridge type byte changed to 0x01\n");
		return;
	}

	nCartType = 0x01;
	fseek(f, POS_CARTRIDGE_TYPE, SEEK_SET);
	fwrite(&nCartType, 1, 1, f);
	fflush(f);
}

void ValidateChecksum(FILE* f)
{
	long i;
	long nRomSize = FileSize(f);
	unsigned short nCartChecksum = 0;
	unsigned short nCalcChecksum = 0;
	unsigned char nCartCompChecksum = 0;
	unsigned char nCalcCompChecksum = 0;

	fflush(f);
	fseek(f, 0, SEEK_SET);

	for(i = 0; i < nRomSize; ++i)
	{
		int ch = fgetc(f);
		if(ch == EOF)
			ch = 0;

		if(i < 0x0134L)
			nCalcChecksum += ch;
		else if(i < 0x014DL)
		{
			nCalcCompChecksum += ch;
			nCalcChecksum += ch;
		}
		else if(i == 0x014DL)
			nCartCompChecksum = ch;
		else if(i == 0x014EL)
			nCartChecksum = ch << 8;
		else if(i == 0x014FL)
			nCartChecksum |= ch;
		else
			nCalcChecksum += ch;
	}

	nCalcCompChecksum = 0xE7 - nCalcCompChecksum;
	nCalcChecksum += nCalcCompChecksum;

	if(nCartChecksum == nCalcChecksum)
	{
		if(IsOptionSet(OPTF_DEBUG))
			printf("\tChecksum is OK\n");
	}
	else
	{
		if(!IsOptionSet(OPTF_DEBUG))
		{
			fflush(f);
			fseek(f, POS_CHECKSUM, SEEK_SET);
			fputc(nCalcChecksum >> 8, f);
			fputc(nCalcChecksum & 0xFF, f);
			fflush(f);
		}
		else
			printf("\tChecksum changed from 0x%04lX to 0x%04lX\n", (long)nCartChecksum, (long)nCalcChecksum);
	}

	if(nCartCompChecksum == nCalcCompChecksum)
	{
		if(IsOptionSet(OPTF_DEBUG))
			printf("\tCompChecksum is OK\n");
	}
	else if(!IsOptionSet(OPTF_DEBUG))
	{
		fflush(f);
		fseek(f, POS_COMP_CHECKSUM, SEEK_SET);
	 	fwrite(&nCalcCompChecksum, 1, 1, f);
		fflush(f);
	}
	else
		printf("\tCompChecksum changed from 0x%02lX to 0x%02lX\n", (long)nCartCompChecksum, (long)nCalcCompChecksum);
}


void PadRomImage(FILE* f)
{
	long size = FileSize(f);
	long padto;

	padto = 0x8000L;
	while(size > padto)
		padto *= 2;

	if(IsOptionSet(OPTF_DEBUG))
		printf("Padding to %ldkB:\n", padto / 1024);

	if(size == padto)
	{
		printf("\tNo padding needed\n");
		return;
	}

	if(IsOptionSet(OPTF_DEBUG))
	{
		printf("\tAdded %ld bytes\n", padto - size);
		return;
	}

	fseek(f, 0, SEEK_END);
	while(size < padto)
	{
		++size;
		fputc(0, f);
	}
	fflush(f);
}


void SetCartridgeTitle(FILE* f, char* pszTitle)
{
	char szCartname[16];

	if(IsOptionSet(OPTF_DEBUG))
	{
		printf("Setting cartridge title:\n");
		printf("\tTitle set to %s\n", pszTitle);
		return;
	}

	memset(szCartname, 0, 16);
	strncpy(szCartname, pszTitle, 16);

	fflush(f);
	fseek(f, POS_CARTRIDGE_TITLE, SEEK_SET);
	fwrite(szCartname, 16, 1, f);
	fflush(f);
}


/*
 * Das main
 *
 */

int main( int argc, char *argv[] )
{
	int argn=1;
	char filename[512];
	char cartname[32];
	FILE* f;

	g_nOptions = 0;

	if(--argc == 0)
		PrintUsage();

	while(*argv[argn] == '-')
	{
		--argc;
		switch(argv[argn++][1])
		{
			case '?':
			case 'h':
				PrintUsage();
				break;
			case 'd':
				g_nOptions |= OPTF_DEBUG;
				break;
			case 'p':
				g_nOptions |= OPTF_PAD;
				break;
			case 'v':
				g_nOptions |= OPTF_VALIDATE;
				break;
			case 't':
				strncpy(cartname, argv[argn - 1] + 2, 16);
				g_nOptions |= OPTF_TITLE;
				break;
		}
	}

	strcpy( filename, argv[argn++] );

	if(!FileExists(filename))
		strcat(filename, ".gb");

	if((f=fopen(filename,"rb+")) != NULL)
	{
		/*
		 * -d (Debug) option code
		 *
		 */

		if(IsOptionSet(OPTF_DEBUG))
			printf("-d (Debug) option enabled...\n");

		/*
		 * -p (Pad) option code
		 *
		 */

		if(IsOptionSet(OPTF_PAD))
			PadRomImage(f);

		/*
		 * -t (Set carttitle) option code
		 *
		 */

		if(IsOptionSet(OPTF_TITLE))
			SetCartridgeTitle(f, cartname);

		/*
		 * -v (Validate header) option code
		 *
		 */

		if(IsOptionSet(OPTF_VALIDATE))
		{
			if(IsOptionSet(OPTF_DEBUG))
				printf("Validating header:\n");

			/* Nintendo Character Area */
			ValidateNintendoCharacterArea(f);

			/* ROM size */
			ValidateRomSize(f);

			/* Cartridge type */
			ValidateCartridgeType(f);

			/* Checksum */
			ValidateChecksum(f);
		}
		fclose( f );
	}
	else
	{
		FatalError( "Unable to open file" );
	}

	return EXIT_SUCCESS;
}
