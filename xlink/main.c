/*  Copyright 2008-2022 Carsten Elton Sorensen and contributors

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

#include <ctype.h>
#include <stdarg.h>

#include "util.h"
#include "file.h"
#include "str.h"

#include "amiga.h"
#include "assign.h"
#include "coco.h"
#include "commodore.h"
#include "foenix.h"
#include "gameboy.h"
#include "group.h"
#include "hc800.h"
#include "image.h"
#include "mapfile.h"
#include "memorymap.h"
#include "object.h"
#include "patch.h"
#include "section.h"
#include "sega.h"
#include "smart.h"
#include "xlink.h"

#define FF_GAME_BOY			(FILE_FORMAT_BINARY | FILE_FORMAT_GAME_BOY)
#define FF_AMIGA			(FILE_FORMAT_BINARY | FILE_FORMAT_AMIGA_EXECUTABLE | FILE_FORMAT_AMIGA_LINK_OBJECT)
#define FF_CBM				(FILE_FORMAT_BINARY | FILE_FORMAT_CBM_PRG)
#define FF_MEGA_DRIVE		(FILE_FORMAT_BINARY | FILE_FORMAT_MEGA_DRIVE)
#define FF_MASTER_SYSTEM	(FILE_FORMAT_BINARY | FILE_FORMAT_MASTER_SYSTEM)
#define FF_HC800_KERNEL		(FILE_FORMAT_BINARY | FILE_FORMAT_HC800_KERNEL)
#define FF_HC800			(FILE_FORMAT_BINARY | FILE_FORMAT_HC800)
#define FF_FOENIX			(FILE_FORMAT_BINARY | FILE_FORMAT_PGZ)
#define FF_COCO				(FILE_FORMAT_BINARY | FILE_FORMAT_COCO_BIN)

FileFormat g_allowedFormats = 0;
uint16_t g_cbmBaseAddress = 0;

static FileFormat g_outputFormat = FILE_FORMAT_NONE;
static uint8_t* g_hc800Config = NULL;
static int g_binaryPad = -1;
static const char* g_outputFilename = NULL;
static const char* g_smartlink = NULL;
static const char* g_entry = NULL;
static const char* g_mapFilename = NULL;
static bool g_targetDefined = false;

static bool
format_SupportsReloc(FileFormat type) {
    return type == FILE_FORMAT_AMIGA_EXECUTABLE || type == FILE_FORMAT_AMIGA_LINK_OBJECT;
}

static bool
format_SupportsOnlySectionRelativeReloc(FileFormat type) {
    return type == FILE_FORMAT_AMIGA_EXECUTABLE;
}

static bool
format_SupportsImports(FileFormat type) {
    return type == FILE_FORMAT_AMIGA_LINK_OBJECT;
}

static void
printUsage(void) {
    printf("xlink (ASMotor v" ASMOTOR_VERSION ")\n"
           "\n"
           "Usage: xlink [options] file1 [file2 [... filen]]\n"
		   "\n"
           "Options: (a forward slash (/) can be used instead of the dash (-))\n"
		   "\n"
           "    -h  This text\n"
		   "\n"
           "    -c<config>  Memory configuration\n"
           "          -camiga     Amiga\n"
           "          -ccbm64     Commodore 64\n"
           "          -ccbm128    Commodore 128 unbanked\n"
           "          -ccbm128f   Commodore 128 Function ROM (32 KiB)\n"
           "          -ccbm128fl  Commodore 128 Function ROM Low (16 KiB)\n"
           "          -ccbm128fh  Commodore 128 Function ROM High (16 KiB)\n"
           "          -ccbm264    Commodore 264/TED series\n"
           "          -cngb       Game Boy ROM image\n"
           "          -cngbs      Game Boy small mode (32 KiB)\n"
           "          -csmd       Sega Mega Drive\n"
           "          -csms       Sega Master System (32 KiB)\n"
           "          -csmsb      Sega Master System banked (64+ KiB)\n"
           "          -chc800b    HC800 16 KiB ROM\n"
           "          -chc800s    HC800 small mode (64 KiB text + data + bss)\n"
           "          -chc800sh   HC800 small Harvard mode (64 KiB text + data, 64 KiB data + bss)\n"
           "          -chc800m    HC800 medium mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "                      banks text)\n"
           "          -chc800mh   HC800 medium Harvard executable (32 KiB text + 32 KiB\n"
		   "                      sized text banks, 64 KiB data + bss)\n"
           "          -chc800l    HC800 large mode (32 KiB text + data + bss, 32 KiB sized\n"
		   "                      banks text + data + bss)\n"
           "          -cfxa2560x  Foenix A2560X/K\n"
           "          -cfxf256jrs Foenix F256 Jr. small mode (64 KiB, I/O hole)\n"
		   "          -ccoco      Tandy TRS-80 Color Computer\n"
		   "\n"
           "    -e<symbol>  Code entry point when supported by output format.\n"
		   "                Will override \"-s\" option\n"
		   "\n"
           "    -f<format>  File format\n"
           "          -famigaexe  Amiga executable\n"
           "          -famigalink Amiga link object\n"
           "          -fbin       Binary\n"
           "          -fcbm       Commodore .PRG\n"
           "          -fngb       Game Boy ROM\n"
           "          -fsmd       Sega Mega Drive/Genesis ROM\n"
           "          -fsms       Sega Master System ROM\n"
           "          -fhc800     HC800 executable\n"
           "          -fhc800k    HC800 kernel\n"
           "          -ffxpgz     Foenix PGZ\n"
           "          -fcocobin   TRS-80 Color Computer .bin\n"
		   "\n"
           "    -m<mapfile> Write a mapfile to <mapfile>\n"
		   "\n"
           "    -o<output>  Write output to file <output>\n"
		   "\n"
           "    -s<symbol>  Strip unused sections, rooting the section containing <symbol>\n"
           "                <symbol> is used as entry point when support by output format\n"
    );
    exit(EXIT_SUCCESS);
}

static void
handleFileFormatOption(const string* target) {
	if (str_EqualConst(target, "amigaexe")) {	/* Amiga executable */
		g_outputFormat = FILE_FORMAT_AMIGA_EXECUTABLE;
	} else if (str_EqualConst(target, "amigalink")) {	/* Amiga executable */
		g_outputFormat = FILE_FORMAT_AMIGA_LINK_OBJECT;
	} else if (str_EqualConst(target, "bin")) {	/* Binary */
		g_outputFormat = FILE_FORMAT_BINARY;
	} else if (str_EqualConst(target, "cbm")) {	/* Commodore .prg */
		g_outputFormat = FILE_FORMAT_CBM_PRG;
	} else if (str_EqualConst(target, "ngb")) {	/* Gameboy ROM image */
		g_outputFormat = FILE_FORMAT_GAME_BOY;
	} else if (str_EqualConst(target, "smd")) {	/* Sega Mega Drive/Genesis */
		g_outputFormat = FILE_FORMAT_MEGA_DRIVE;
	} else if (str_EqualConst(target, "sms")) {	/* Sega Master System */
		g_outputFormat = FILE_FORMAT_MASTER_SYSTEM;
	} else if (str_EqualConst(target, "hc800k")) {	/* HC800 16 KiB text + data, 16 KiB bss */
		g_outputFormat = FILE_FORMAT_HC800_KERNEL;
	} else if (str_EqualConst(target, "hc800")) {	/* HC800, CODE: 64 KiB text + data + bss */
		g_outputFormat = FILE_FORMAT_HC800;
	} else if (str_EqualConst(target, "fxpgz")) {	/* Foenix PGZ */
		g_outputFormat = FILE_FORMAT_PGZ;
	} else if (str_EqualConst(target, "cocobin")) {	/* CoCo BIN */
		g_outputFormat = FILE_FORMAT_COCO_BIN;
	} else {
		error("Unknown format \"%s\"", str_String(target));
	}
}


static void
handleLegacyTargetOption(const string* target) {
	if (str_EqualConst(target, "a")) {			/* Amiga executable */
		group_SetupAmiga();
		g_outputFormat = FILE_FORMAT_AMIGA_EXECUTABLE;
		g_allowedFormats = FF_AMIGA;
	} else if (str_EqualConst(target, "b")) {	/* Amiga link object */
		group_SetupAmiga();
		g_outputFormat = FILE_FORMAT_AMIGA_LINK_OBJECT;
		g_allowedFormats = FF_AMIGA;
	} else if (str_EqualConst(target, "g")) {	/* Gameboy ROM image */
		group_SetupGameboy();
		g_outputFormat = FILE_FORMAT_GAME_BOY;
		g_allowedFormats = FF_GAME_BOY;
	} else if (str_EqualConst(target, "s")) {	/* Gameboy small mode ROM image */
		group_SetupSmallGameboy();
		g_outputFormat = FILE_FORMAT_GAME_BOY;
		g_allowedFormats = FF_GAME_BOY;
	} else if (str_EqualConst(target, "c64")) {	/* Commodore 64 .prg */
		commodore_SetupCommodore64();
		g_cbmBaseAddress = 0x0801;
		g_outputFormat = FILE_FORMAT_CBM_PRG;
		g_allowedFormats = FF_CBM;
	} else if (str_EqualConst(target, "c128")) {	/* Commodore 128 .prg */
		commodore_SetupUnbankedCommodore128();
		g_cbmBaseAddress = 0x1C01;
		g_outputFormat = FILE_FORMAT_CBM_PRG;
		g_allowedFormats = FF_CBM;
	} else if (str_EqualConst(target, "c128f")) {	/* Commodore 128 Function ROM */
		commodore_SetupCommodore128FunctionROM();
		g_outputFormat = FILE_FORMAT_BINARY;
		g_allowedFormats = FILE_FORMAT_BINARY;
		g_binaryPad = 0x8000;
	} else if (str_EqualConst(target, "c128fl")) {	/* Commodore 128 Function ROM Low */
		commodore_SetupCommodore128FunctionROMLow();
		g_outputFormat = FILE_FORMAT_BINARY;
		g_allowedFormats = FILE_FORMAT_BINARY;
		g_binaryPad = 0x4000;
	} else if (str_EqualConst(target, "c128fh")) {	/* Commodore 128 Function ROM High */
		commodore_SetupCommodore128FunctionROMHigh();
		g_allowedFormats = FILE_FORMAT_BINARY;
		g_binaryPad = 0x4000;
	} else if (str_EqualConst(target, "c264")) {	/* Commodore 264 series .prg */
		commodore_SetupCommodore264();
		g_cbmBaseAddress = 0x1001;
		g_outputFormat = FILE_FORMAT_CBM_PRG;
		g_allowedFormats = FF_CBM;
	} else if (str_EqualConst(target, "md")) {	/* Sega Mega Drive/Genesis */
		group_SetupSegaMegaDrive();
		g_outputFormat = FILE_FORMAT_MEGA_DRIVE;
		g_allowedFormats = FF_MEGA_DRIVE;
	} else if (str_EqualConst(target, "ms8")) {	/* Sega Master System 8 KiB */
		group_SetupSegaMasterSystem(0x2000);
		g_outputFormat = FILE_FORMAT_MASTER_SYSTEM;
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0x2000;
	} else if (str_EqualConst(target, "ms16")) {	/* Sega Master System 16 KiB */
		group_SetupSegaMasterSystem(0x4000);
		g_outputFormat = FILE_FORMAT_MASTER_SYSTEM;
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0x4000;
	} else if (str_EqualConst(target, "ms32")) {	/* Sega Master System 32 KiB */
		group_SetupSegaMasterSystem(0x8000);
		g_outputFormat = FILE_FORMAT_MASTER_SYSTEM;
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0x8000;
	} else if (str_EqualConst(target, "ms48")) {	/* Sega Master System 48 KiB */
		group_SetupSegaMasterSystem(0xC000);
		g_outputFormat = FILE_FORMAT_MASTER_SYSTEM;
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0xC000;
	} else if (str_EqualConst(target, "msb")) {		/* Sega Master System 64+ KiB */
		group_SetupSegaMasterSystemBanked();
		g_outputFormat = FILE_FORMAT_MASTER_SYSTEM;
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0;
	} else if (str_EqualConst(target, "hc8b")) {	/* HC800 16 KiB text + data, 16 KiB bss */
		group_SetupHC8XXROM();
		g_outputFormat = FILE_FORMAT_HC800_KERNEL;
		g_allowedFormats = FF_HC800_KERNEL;
		g_binaryPad = 0;
	} else if (str_EqualConst(target, "hc8s")) {	/* HC800, CODE: 64 KiB text + data + bss */
		group_SetupHC8XXSmall();
		g_outputFormat = FILE_FORMAT_HC800;
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigSmall;
	} else if (str_EqualConst(target, "hc8sh")) {	/* HC800 CODE: 64 KiB text, DATA: 64 KiB data + bss */
		group_SetupHC8XXSmallHarvard();
		g_outputFormat = FILE_FORMAT_HC800;
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigSmallHarvard;
	} else if (str_EqualConst(target, "hc8m")) {	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text */
		group_SetupHC8XXMedium();
		g_outputFormat = FILE_FORMAT_HC800;
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigMedium;
	} else if (str_EqualConst(target, "hc8mh")) {	/* HC800, CODE: 32 KiB text, CODE: 32 KiB sized text banks, DATA: 64 KiB data + bss */
		group_SetupHC8XXMediumHarvard();
		g_outputFormat = FILE_FORMAT_HC800;
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigMediumHarvard;
	} else if (str_EqualConst(target, "hc8l")) {	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text + data + bss */
		group_SetupHC8XXLarge();
		g_outputFormat = FILE_FORMAT_HC800;
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigLarge;
	} else if (str_EqualConst(target, "fxa2560x")) {	/* Foenix A2560X/K */
		foenix_SetupFoenixA2560XGroups();
		g_outputFormat = FILE_FORMAT_PGZ;
		g_allowedFormats = FF_FOENIX;
	} else if (str_EqualConst(target, "fxf256jrs")) {	/* Foenix F256 Jr */
		foenix_SetupFoenixF256JrSmallGroups();
		g_outputFormat = FILE_FORMAT_PGZ;
		g_allowedFormats = FF_FOENIX;
	} else if (str_EqualConst(target, "coco")) {	/* TRS Color Computer */
		group_SetupCoCo();
		g_outputFormat = FILE_FORMAT_COCO_BIN;
		g_allowedFormats = FF_COCO;
	} else {
		error("Unknown target \"%s\"", str_String(target));
	}
}


static void
handleMemoryConfigurationOption(const string* target) {
	g_outputFormat = FILE_FORMAT_NONE;

	if (str_EqualConst(target, "amiga")) {		/* Amiga */
		group_SetupAmiga();
		g_allowedFormats = FF_AMIGA;
	} else if (str_EqualConst(target, "c64")) {	/* Commodore 64 .prg */
		commodore_SetupCommodore64();
		g_cbmBaseAddress = 0x0801;
		g_allowedFormats = FF_CBM;
	} else if (str_EqualConst(target, "c128")) {	/* Commodore 128 .prg */
		commodore_SetupUnbankedCommodore128();
		g_cbmBaseAddress = 0x1C01;
		g_allowedFormats = FF_CBM;
	} else if (str_EqualConst(target, "c128f")) {	/* Commodore 128 Function ROM */
		commodore_SetupCommodore128FunctionROM();
		g_allowedFormats = FILE_FORMAT_BINARY;
		g_binaryPad = 0x8000;
	} else if (str_EqualConst(target, "c128fl")) {	/* Commodore 128 Function ROM Low */
		commodore_SetupCommodore128FunctionROMLow();
		g_allowedFormats = FILE_FORMAT_BINARY;
		g_binaryPad = 0x4000;
	} else if (str_EqualConst(target, "c128fh")) {	/* Commodore 128 Function ROM High */
		commodore_SetupCommodore128FunctionROMHigh();
		g_allowedFormats = FILE_FORMAT_BINARY;
		g_binaryPad = 0x4000;
	} else if (str_EqualConst(target, "c264")) {	/* Commodore 264 series .prg */
		commodore_SetupCommodore264();
		g_cbmBaseAddress = 0x1001;
		g_allowedFormats = FF_CBM;
	} else if (str_EqualConst(target, "ngb")) {	/* Gameboy ROM image */
		group_SetupGameboy();
		g_allowedFormats = FF_GAME_BOY;
	} else if (str_EqualConst(target, "ngbs")) {	/* Gameboy small mode ROM image */
		group_SetupSmallGameboy();
		g_allowedFormats = FF_GAME_BOY;
	} else if (str_EqualConst(target, "smd")) {	/* Sega Mega Drive/Genesis */
		group_SetupSegaMegaDrive();
		g_allowedFormats = FF_MEGA_DRIVE;
	} else if (str_EqualConst(target, "sms8")) {	/* Sega Master System 8 KiB */
		group_SetupSegaMasterSystem(0x2000);
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0x2000;
	} else if (str_EqualConst(target, "sms16")) {	/* Sega Master System 16 KiB */
		group_SetupSegaMasterSystem(0x4000);
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0x4000;
	} else if (str_EqualConst(target, "sms32")) {	/* Sega Master System 32 KiB */
		group_SetupSegaMasterSystem(0x8000);
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0x8000;
	} else if (str_EqualConst(target, "smsb")) {		/* Sega Master System 64+ KiB */
		group_SetupSegaMasterSystemBanked();
		g_allowedFormats = FF_MASTER_SYSTEM;
		g_binaryPad = 0;
	} else if (str_EqualConst(target, "hc800b")) {	/* HC800 16 KiB text + data, 16 KiB bss */
		group_SetupHC8XXROM();
		g_allowedFormats = FF_HC800_KERNEL;
		g_binaryPad = 0;
	} else if (str_EqualConst(target, "hc8s")) {	/* HC800, CODE: 64 KiB text + data + bss */
		group_SetupHC8XXSmall();
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigSmall;
	} else if (str_EqualConst(target, "hc8sh")) {	/* HC800 CODE: 64 KiB text, DATA: 64 KiB data + bss */
		group_SetupHC8XXSmallHarvard();
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigSmallHarvard;
	} else if (str_EqualConst(target, "hc8m")) {	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text */
		group_SetupHC8XXMedium();
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigMedium;
	} else if (str_EqualConst(target, "hc8mh")) {	/* HC800, CODE: 32 KiB text, CODE: 32 KiB sized text banks, DATA: 64 KiB data + bss */
		group_SetupHC8XXMediumHarvard();
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigMediumHarvard;
	} else if (str_EqualConst(target, "hc8l")) {	/* HC800, CODE: 32 KiB text + data + bss, CODE: 32 KiB sized banks text + data + bss */
		group_SetupHC8XXLarge();
		g_allowedFormats = FF_HC800;
		g_hc800Config = hc800_ConfigLarge;
	} else if (str_EqualConst(target, "fxa2560x")) {	/* Foenix A2560X/K */
		foenix_SetupFoenixA2560XGroups();
		g_allowedFormats = FF_FOENIX;
	} else if (str_EqualConst(target, "fxf256jrs")) {	/* Foenix F256 Jr */
		foenix_SetupFoenixF256JrSmallGroups();
		g_allowedFormats = FF_FOENIX;
	} else if (str_EqualConst(target, "coco")) {	/* TRS Color Computer */
		group_SetupCoCo();
		g_allowedFormats = FF_COCO;
	} else {
		error("Unknown target \"%s\"", str_String(target));
	}
}


static void
writeOutput(const char* g_outputFilename) {
	switch (g_outputFormat) {
		case FILE_FORMAT_NONE:
			error("Output format not specified");
			break;
		case FILE_FORMAT_MEGA_DRIVE:
			sega_WriteMegaDriveImage(g_outputFilename);
			break;
		case FILE_FORMAT_MASTER_SYSTEM:
			sega_WriteMasterSystemImage(g_outputFilename, g_binaryPad);
			break;
		case FILE_FORMAT_GAME_BOY:
			gameboy_WriteImage(g_outputFilename);
			break;
		case FILE_FORMAT_BINARY:
			image_WriteBinary(g_outputFilename, g_binaryPad);
			break;
		case FILE_FORMAT_CBM_PRG:
			commodore_WritePrg(g_outputFilename, g_entry, g_cbmBaseAddress);
			break;
		case FILE_FORMAT_AMIGA_EXECUTABLE:
			amiga_WriteExecutable(g_outputFilename, g_entry, false);
			break;
		case FILE_FORMAT_AMIGA_LINK_OBJECT:
			amiga_WriteLinkObject(g_outputFilename, false);
			break;
		case FILE_FORMAT_HC800_KERNEL:
			hc800_WriteKernal(g_outputFilename);
			break;
		case FILE_FORMAT_HC800:
			hc800_WriteExecutable(g_outputFilename, g_hc800Config);
			break;
		case FILE_FORMAT_PGZ:
			foenix_WriteExecutable(g_outputFilename, g_entry);
			break;
		case FILE_FORMAT_COCO_BIN:
			coco_WriteQuickloadBin(g_outputFilename, g_entry);
			break;
	}
}


static bool
handleOption(const char* option) {
	switch (tolower(option[0])) {
		case '?':
		case 'h':
			printUsage();
			break;
		case 'a': {	/* Memory map */
			if (g_targetDefined) error("more than one target (option \"a\", \"t\", \"c\") defined");

			g_targetDefined = true;
			string* filename = str_ToLower(str_Create(&option[1]));
			mmap_Read(filename);
			str_Free(filename);
			return true;
		}
		case 'c': {	/* Memory configuration */
			if (g_targetDefined) error("more than one target (option \"a\", \"t\", \"c\") defined");

			g_targetDefined = true;
			string* target = str_ToLower(str_Create(&option[1]));
			handleMemoryConfigurationOption(target);
			str_Free(target);
			return true;
		}
		case 'e':	/* Entry point */
			if (option[1] == 0) error("option \"e\" needs an argument");
			g_entry = &option[1];

			return true;
		case 'f': {	/* File format */
			string* target = str_ToLower(str_Create(&option[1]));
			handleFileFormatOption(target);
			str_Free(target);
			return true;
		}
		case 'm':	/* Map file */
			if (option[1] == 0) error("option \"m\" needs an argument");

			g_mapFilename = &option[1];
			return true;
		case 'o':	/* Output filename */
			if (option[1] == 0) error("option \"o\" needs an argument");

			g_outputFilename = &option[1];
			return true;
		case 's':	/* Smart linking */
			if (option[1] == 0) error("option \"s\" needs an argument");

			g_smartlink = &option[1];
			if (g_entry == NULL)
				g_entry = g_smartlink;

			return true;
		case 't': {	/* Target */
			if (g_targetDefined) error("more than one target (option \"a\", \"t\", \"c\") defined");

			fprintf(stderr, "Warning: option -t is deprecated and has been replaced with -f and -c\n");
			g_targetDefined = true;

			string* target = str_ToLower(str_Create(&option[1]));
			handleLegacyTargetOption(target);
			str_Free(target);
			return true;
		}
		default:
			break;
	}
	return false;
}

int
main(int argc, char* argv[]) {
    int argn = 1;

    if (argc == 1) {
        printUsage();
	}

    while (argn < argc && (argv[argn][0] == '-' || argv[argn][0] == '/')) {
		if (!handleOption(&argv[argn][1]))
			error("Unknown option \"%s\"", argv[argn]);
		++argn;
    }

    if (!g_targetDefined) {
        error("No target format defined");
	}

	if ((g_outputFormat & g_allowedFormats) == 0) {
		error("Memory/machine configuration does not support output format");
	}

	group_InitMemoryChunks();

    while (argn < argc && argv[argn]) {
        obj_Read(argv[argn++]);
    }

    smart_Process(g_smartlink);

    if (!format_SupportsReloc(g_outputFormat)) {
        assign_Process();
		sect_ResolveUnresolved();
	}

    patch_Process(
		format_SupportsReloc(g_outputFormat),
		format_SupportsOnlySectionRelativeReloc(g_outputFormat),
        format_SupportsImports(g_outputFormat));

	if (g_outputFilename != NULL) {
		writeOutput(g_outputFilename);
	}

    if (g_mapFilename != NULL) {
        if (!format_SupportsReloc(g_outputFormat)) {
            sect_SortSections();
            map_Write(g_mapFilename);
        } else {
            error("Output format does not support producing a mapfile");
        }
    }

    return EXIT_SUCCESS;
}
