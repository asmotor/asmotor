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

#ifndef XLINK_HC800_H_INCLUDED_
#define XLINK_HC800_H_INCLUDED_

#include "types.h"

extern uint8_t hc800_ConfigSmall[];
extern uint8_t hc800_ConfigSmallHarvard[];
extern uint8_t hc800_ConfigMedium[];
extern uint8_t hc800_ConfigMediumHarvard[];
extern uint8_t hc800_ConfigLarge[];

extern void
hc800_WriteExecutable(const char* name, uint8_t* configuration);

extern void
hc800_WriteKernal(const char* outputFilename);

#endif
