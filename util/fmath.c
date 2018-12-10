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

#include <math.h>

#include "asmotor.h"
#include "types.h"

#ifndef PI
# define PI acos(-1.0)
#endif

double
fixedToDouble(int32_t a) {
    return a * (1.0 / 65536.0);
}

int32_t
doubleToFixed(double a) {
    return (int32_t) (a * 65536);
}

int32_t
imuldiv(int32_t a, int32_t b, int32_t c) {
    return (int32_t) ((int64_t) a * b / c);
}

int32_t
fmul(int32_t a, int32_t b) {
    return (int32_t) (((int64_t) a * b) / 65536);
}

int32_t
fdiv(int32_t a, int32_t b) {
    return (int32_t) (((int64_t) a * 65536) / b);
}

int32_t
fsin(int32_t a) {
    return doubleToFixed(sin(fixedToDouble(a) * 2 * PI));
}

int32_t
fasin(int32_t a) {
    return doubleToFixed(asin(fixedToDouble(a)) / (2 * PI));
}

int32_t
fcos(int32_t a) {
    return doubleToFixed(cos(fixedToDouble(a) * 2 * PI));
}

int32_t
facos(int32_t a) {
    return doubleToFixed(acos(fixedToDouble(a)) / (2 * PI));
}

int32_t
ftan(int32_t a) {
    return doubleToFixed(tan(fixedToDouble(a) * 2 * PI));
}

int32_t
fatan(int32_t a) {
    return doubleToFixed(atan(fixedToDouble(a)) / (2 * PI));
}

int32_t
fatan2(int32_t a, int32_t b) {
    return doubleToFixed(atan2(fixedToDouble(a), fixedToDouble(b)) / (2 * PI));
}

uint32_t
log2n(size_t value) {
    uint32_t r = sizeof(value) * 8 - 1;
    size_t mask = (size_t) 1 << (sizeof(value) * 8 - 1);
    while (r > 0) {
        if (value & mask)
            return r;

        r -= 1;
        mask = mask >> 1u;
    }

    return 0;
}

int32_t
asr(int32_t lhs, int32_t rhs) {
    if (lhs < 0)
        return (uint32_t) lhs >> (int32_t) rhs | ~(UINT32_MAX >> (int32_t) rhs);
    else
        return lhs >> rhs;
}

bool
isPowerOfTwo(int32_t d) {
    return ((uint32_t) d & (uint32_t) -d) == (uint32_t) d && d != 0;
}
