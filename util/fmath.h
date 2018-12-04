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

#ifndef UTIL_FMATH_H_INCLUDED_
#define UTIL_FMATH_H_INCLUDED_

#include "types.h"

extern int32_t imuldiv(int32_t a, int32_t b, int32_t c);

extern int32_t fmul(int32_t a, int32_t b);
extern int32_t fdiv(int32_t a, int32_t b);
extern int32_t fsin(int32_t a);
extern int32_t fcos(int32_t a);
extern int32_t ftan(int32_t a);
extern int32_t fasin(int32_t a);
extern int32_t facos(int32_t a);
extern int32_t fatan(int32_t a);
extern int32_t fatan2(int32_t a, int32_t b);

extern uint32_t log2n(size_t value);

extern int32_t asr(int32_t lhs, int32_t rhs);

#endif /* UTIL_FMATH_H_INCLUDED_ */
