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
#include <stdint.h>
#include <stdbool.h>

#include "asmotor.h"

#define XGBFIX_VERSION "1.0.0"

#define POS_NINTENDO_LOGO   0x0104L
#define POS_CARTRIDGE_TITLE 0x0134L
#define POS_CARTRIDGE_TYPE  0x0147L
#define POS_ROM_SIZE        0x0148L
#define POS_COMP_CHECKSUM   0x014DL
#define POS_CHECKSUM        0x014EL


/* Option handling
 */

#define OPTF_DEBUG    0x01L
#define OPTF_PAD      0x02L
#define OPTF_VALIDATE 0x04L
#define OPTF_TITLE    0x08L

uint8_t g_OptionFlags;

static bool isOptionSet(uint8_t optionFlag) {
	return (g_OptionFlags & optionFlag) != 0;
}

/* Misc. variables */

static uint8_t g_NintendoChar[48] = {
		0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08,
		0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
		0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

/* Diagnostic functions */

static void printUsage(void) {
	printf("xgbfix v" XGBFIX_VERSION " (part of ASMotor " ASMOTOR_VERSION ")\n\n"
		   "Usage: xgbfix [options] image[.gb]\n"
		   "Options:\n"
		   "\t-h\t\tThis text\n"
		   "\t-d\t\tDebug: Don't change image\n"
		   "\t-p\t\tPad image to valid size\n\t\t\tPads to 32/64/128/256/512kB as appropriate\n"
		   "\t-t<name>\tChange cartridge title field (16 characters)\n"
		   "\t-v\t\tValidate header\n"
		   "\t\t\tCorrects - Nintendo Character Area (0x0104)\n"
		   "\t\t\t\t - ROM type (0x0147)\n"
		   "\t\t\t\t - ROM size (0x0148)\n"
		   "\t\t\t\t - Checksums (0x014D-0x014F)\n");
	exit(EXIT_SUCCESS);
}

static void fatalError(const char* error) {
	printf("ERROR: %s\n\n", error);
	exit(EXIT_FAILURE);
}

/* File helper functions */

static size_t fileSize(FILE* fileHandle) {
	fflush(fileHandle);
	off_t prevPos = ftello(fileHandle);
	fseek(fileHandle, 0, SEEK_END);
	off_t r = ftello(fileHandle);
	fseek(fileHandle, prevPos, SEEK_SET);

	return (size_t) r;
}

static bool fileExists(const char* filename) {
	FILE* fileHandle;

	if ((fileHandle = fopen(filename, "rb")) != NULL) {
		fclose(fileHandle);
		return true;
	} else
		return false;
}

/* ROM image validation functions */

static void validateNintendoCharacterArea(FILE* fileHandle) {
	uint32_t bytesChangedCount = 0;

	fflush(fileHandle);
	fseek(fileHandle, POS_NINTENDO_LOGO, SEEK_SET);

	for (int i = 0; i < 48; ++i) {
		int ch = fgetc(fileHandle);
		if (ch == EOF)
			ch = 0x00;
		if (ch != g_NintendoChar[i]) {
			++bytesChangedCount;

			if (!isOptionSet(OPTF_DEBUG)) {
				fflush(fileHandle);
				fseek(fileHandle, POS_NINTENDO_LOGO + i, SEEK_SET);
				fwrite(&g_NintendoChar[i], 1, 1, fileHandle);
				fflush(fileHandle);
			}
		}
	}

	if (isOptionSet(OPTF_DEBUG)) {
		if (bytesChangedCount != 0)
			printf("\tChanged %d bytes in the Nintendo Character Area\n", bytesChangedCount);
		else
			printf("\tNintendo Character Area is OK\n");
	}
}

static void validateRomSize(FILE* fileHandle) {
	fflush(fileHandle);
	fseek(fileHandle, POS_ROM_SIZE, SEEK_SET);

	int cartRomSize = fgetc(fileHandle);
	if (cartRomSize == EOF)
		cartRomSize = 0x00;

	uint8_t calculatedRomSize = 0;
	size_t romSize = fileSize(fileHandle);
	while (romSize > (0x8000UL << calculatedRomSize))
		++calculatedRomSize;

	if (calculatedRomSize == cartRomSize) {
		if (isOptionSet(OPTF_DEBUG))
			printf("\tROM size byte is OK\n");
		return;
	}

	if (isOptionSet(OPTF_DEBUG)) {
		printf("\tChanged ROM size byte from 0x%02X (%ldkB) to 0x%02X (%ldkB)\n", cartRomSize,
			   (0x8000UL << (uint8_t) cartRomSize) / 1024, calculatedRomSize, (0x8000UL << calculatedRomSize) / 1024);
	} else {
		fseek(fileHandle, POS_ROM_SIZE, SEEK_SET);
		fwrite(&calculatedRomSize, 1, 1, fileHandle);
		fflush(fileHandle);
	}
}

static void validateCartridgeType(FILE* fileHandle) {
	fflush(fileHandle);
	fseek(fileHandle, POS_CARTRIDGE_TYPE, SEEK_SET);

	int cartType = fgetc(fileHandle);
	if (cartType == EOF)
		cartType = 0x00;

	if (fileSize(fileHandle) <= 0x8000UL || cartType != 0x00) {
		/* cart type byte can be anything? */
		if (isOptionSet(OPTF_DEBUG))
			printf("\tCartridge type byte is OK\n");
		return;
	}

	if (isOptionSet(OPTF_DEBUG)) {
		printf("\tCartridge type byte changed to 0x01\n");
		return;
	}

	cartType = 0x01;
	fseek(fileHandle, POS_CARTRIDGE_TYPE, SEEK_SET);
	fwrite(&cartType, 1, 1, fileHandle);
	fflush(fileHandle);
}

static void validateChecksum(FILE* fileHandle) {
	size_t romSize = fileSize(fileHandle);

	uint16_t cartChecksum = 0;
	uint16_t calculatedChecksum = 0;
	uint8_t cartCompChecksum = 0;
	uint8_t calculatedCompChecksum = 0;

	fflush(fileHandle);
	fseek(fileHandle, 0, SEEK_SET);

	for (size_t i = 0; i < romSize; ++i) {
		int ch = fgetc(fileHandle);
		if (ch == EOF)
			ch = 0;

		if (i < 0x0134L) {
			calculatedChecksum += (uint16_t) ch;
		} else if (i < 0x014DL) {
			calculatedCompChecksum += (uint8_t) ch;
			calculatedChecksum += (uint16_t) ch;
		} else if (i == 0x014DL) {
			cartCompChecksum = (uint8_t) ch;
		} else if (i == 0x014EL) {
			cartChecksum = (uint16_t) ch << 8U;
		} else if (i == 0x014FL) {
			cartChecksum |= (uint16_t) ch;
		} else {
			calculatedChecksum += (uint16_t) ch;
		}
	}

	calculatedCompChecksum = (uint8_t) (0xE7U - calculatedCompChecksum);
	calculatedChecksum += calculatedCompChecksum;

	if (cartChecksum == calculatedChecksum) {
		if (isOptionSet(OPTF_DEBUG))
			printf("\tChecksum is OK\n");
	} else {
		if (!isOptionSet(OPTF_DEBUG)) {
			fflush(fileHandle);
			fseek(fileHandle, POS_CHECKSUM, SEEK_SET);
			fputc(calculatedChecksum >> 8U, fileHandle);
			fputc(calculatedChecksum & 0xFFU, fileHandle);
			fflush(fileHandle);
		} else
			printf("\tChecksum changed from 0x%04lX to 0x%04lX\n", (long) cartChecksum, (long) calculatedChecksum);
	}

	if (cartCompChecksum == calculatedCompChecksum) {
		if (isOptionSet(OPTF_DEBUG))
			printf("\tCompChecksum is OK\n");
	} else if (!isOptionSet(OPTF_DEBUG)) {
		fflush(fileHandle);
		fseek(fileHandle, POS_COMP_CHECKSUM, SEEK_SET);
		fwrite(&calculatedCompChecksum, 1, 1, fileHandle);
		fflush(fileHandle);
	} else
		printf("\tCompChecksum changed from 0x%02lX to 0x%02lX\n", (long) cartCompChecksum,
			   (long) calculatedCompChecksum);
}

static void padRomImage(FILE* fileHandle) {
	size_t romSize = fileSize(fileHandle);
	size_t padToSize;

	padToSize = 0x8000UL;
	while (romSize > padToSize)
		padToSize *= 2;

	if (isOptionSet(OPTF_DEBUG))
		printf("Padding to %ldkB:\n", padToSize / 1024);

	if (romSize == padToSize) {
		printf("\tNo padding needed\n");
		return;
	}

	if (isOptionSet(OPTF_DEBUG)) {
		printf("\tAdded %ld bytes\n", padToSize - romSize);
		return;
	}

	fseek(fileHandle, 0, SEEK_END);
	while (romSize < padToSize) {
		++romSize;
		fputc(0, fileHandle);
	}
	fflush(fileHandle);
}

static void setCartridgeTitle(FILE* fileHandle, char* newTitle) {
	char cartTitle[16];

	if (isOptionSet(OPTF_DEBUG)) {
		printf("Setting cartridge title:\n");
		printf("\tTitle set to %s\n", newTitle);
		return;
	}

	memset(cartTitle, 0, 16);
	strncpy(cartTitle, newTitle, 16);

	fflush(fileHandle);
	fseek(fileHandle, POS_CARTRIDGE_TITLE, SEEK_SET);
	fwrite(cartTitle, 16, 1, fileHandle);
	fflush(fileHandle);
}

int main(int argc, char* argv[]) {
	if (--argc == 0)
		printUsage();

	g_OptionFlags = 0;

	char newCartTitle[32];
	int argn = 1;

	while (*argv[argn] == '-') {
		--argc;
		switch (argv[argn++][1]) {
			default:
				printf("Unknown option \"%s\"\n", argv[argn - 1]);
				exit(EXIT_FAILURE);
			case '?':
			case 'h':
				printUsage();
				break;
			case 'd':
				g_OptionFlags |= OPTF_DEBUG;
				break;
			case 'p':
				g_OptionFlags |= OPTF_PAD;
				break;
			case 'v':
				g_OptionFlags |= OPTF_VALIDATE;
				break;
			case 't':
				strncpy(newCartTitle, argv[argn - 1] + 2, 16);
				g_OptionFlags |= OPTF_TITLE;
				break;
		}
	}

	char filename[512];
	strcpy(filename, argv[argn++]);

	FILE* fileHandle;

	if (!fileExists(filename))
		strcat(filename, ".gb");

	if ((fileHandle = fopen(filename, "rb+")) != NULL) {
		/* -d (Debug) option */

		if (isOptionSet(OPTF_DEBUG))
			printf("-d (Debug) option enabled...\n");

		/* -p (Pad) option */

		if (isOptionSet(OPTF_PAD))
			padRomImage(fileHandle);

		/* -t (Set cart title) option */

		if (isOptionSet(OPTF_TITLE))
			setCartridgeTitle(fileHandle, newCartTitle);

		/* -v (Validate header) option */

		if (isOptionSet(OPTF_VALIDATE)) {
			if (isOptionSet(OPTF_DEBUG))
				printf("Validating header:\n");

			/* Nintendo Character Area */
			validateNintendoCharacterArea(fileHandle);

			/* ROM size */
			validateRomSize(fileHandle);

			/* Cartridge type */
			validateCartridgeType(fileHandle);

			/* Checksum */
			validateChecksum(fileHandle);
		}
		fclose(fileHandle);
	} else {
		fatalError("Unable to open file");
	}

	return EXIT_SUCCESS;
}
