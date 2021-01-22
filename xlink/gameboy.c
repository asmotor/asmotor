/*  Copyright 2008-2021 Carsten Elton Sorensen and contributors

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

#include "file.h"
#include "types.h"

#include "gameboy.h"
#include "image.h"
#include "xlink.h"

#define POS_NINTENDO_LOGO   0x0104L
#define POS_CARTRIDGE_TITLE 0x0134L
#define POS_CARTRIDGE_TYPE  0x0147L
#define POS_ROM_SIZE        0x0148L
#define POS_COMP_CHECKSUM   0x014DL
#define POS_CHECKSUM        0x014EL

static uint8_t g_nintendoChar[48] = {
    0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08,
    0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
    0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

static void
updateNintendoCharacterArea(FILE* fileHandle) {
    fseek(fileHandle, POS_NINTENDO_LOGO, SEEK_SET);
    fwrite(g_nintendoChar, sizeof(uint8_t), sizeof(g_nintendoChar), fileHandle);
}

static void
updateRomSize(FILE* fileHandle) {
    fflush(fileHandle);
    fseek(fileHandle, POS_ROM_SIZE, SEEK_SET);

    int cartRomSize = fgetc(fileHandle);
    if (cartRomSize == EOF)
        cartRomSize = 0x00;

    uint8_t calculatedRomSize = 0;
    size_t romSize = fsize(fileHandle);
    while (romSize > (0x8000UL << calculatedRomSize))
        ++calculatedRomSize;

    if (calculatedRomSize != cartRomSize) {
        fseek(fileHandle, POS_ROM_SIZE, SEEK_SET);
        fwrite(&calculatedRomSize, 1, 1, fileHandle);
    }
}

static void
updateCartridgeType(FILE* fileHandle) {
    fseek(fileHandle, POS_CARTRIDGE_TYPE, SEEK_SET);

    int cartType = fgetc(fileHandle);
    if (cartType == EOF)
        cartType = 0x00;

    if (fsize(fileHandle) <= 0x8000UL || cartType != 0x00) {
        // cart type byte can be anything
        return;
    }

    cartType = 0x01;
    fseek(fileHandle, POS_CARTRIDGE_TYPE, SEEK_SET);
    fwrite(&cartType, 1, 1, fileHandle);
}

static void
updateChecksum(FILE* fileHandle) {
    size_t romSize = fsize(fileHandle);

    uint16_t cartChecksum = 0;
    uint16_t calculatedChecksum = 0;
    uint8_t cartCompChecksum = 0;
    uint8_t calculatedCompChecksum = 0;

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

    if (cartChecksum != calculatedChecksum) {
        fseek(fileHandle, POS_CHECKSUM, SEEK_SET);
        fputc(calculatedChecksum >> 8U, fileHandle);
        fputc(calculatedChecksum & 0xFFU, fileHandle);
    }

    if (cartCompChecksum != calculatedCompChecksum) {
        fseek(fileHandle, POS_COMP_CHECKSUM, SEEK_SET);
        fwrite(&calculatedCompChecksum, 1, 1, fileHandle);
    }
}

static void
updateGameBoyHeader(FILE* fileHandle) {
    updateNintendoCharacterArea(fileHandle);
    updateRomSize(fileHandle);
    updateCartridgeType(fileHandle);
    updateChecksum(fileHandle);
}

void
gameboy_WriteImage(const char* outputFilename) {
    FILE* fileHandle = fopen(outputFilename, "w+b");
    if (fileHandle == NULL)
        error("Unable to open \"%s\" for writing", outputFilename);

    image_WriteBinaryToFile(fileHandle, 0);

    updateGameBoyHeader(fileHandle);

    fclose(fileHandle);
}
