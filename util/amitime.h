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

#ifndef ASMOTOR_AMITIME_H_
#define ASMOTOR_AMITIME_H_


#if defined(__VBCC__)
#define __CLOCK_T 1
typedef unsigned long clock_t;
#include <time.h>
#include "../../amitime.h"
#define clock time_GetMicroSeconds
#undef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000


#include <exec/types.h>


extern uint32_t time_GetMicroSeconds(void);

#else

#include <time.h>

#endif

#endif
