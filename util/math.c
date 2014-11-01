#include "asmotor.h"
#include "types.h"

#include <math.h>

#ifndef	PI
#define	PI	acos(-1.0)
#endif

double f2d(int32_t a)
{
	return a * (1.0 / 65536.0);
}

int32_t d2f(double a)
{
	return (int32_t)(a * 65536);
}

int32_t imuldiv(int32_t a, int32_t b, int32_t c)
{
	return (int32_t)((int64_t)a * b / c);
}

int32_t fmul(int32_t a, int32_t b)
{
	return (int32_t)(((int64_t)a * b) >> 16);
}

int32_t fdiv(int32_t a, int32_t b)
{
	return (int32_t)(((int64_t)a << 16) / b);
}

int32_t fsin(int32_t a)
{
	return d2f(sin(f2d(a) * 2 * PI));
}

int32_t fasin(int32_t a)
{
	return d2f(asin(f2d(a)) / (2 * PI));
}

int32_t fcos(int32_t a)
{
	return d2f(cos(f2d(a) * 2 * PI));
}

int32_t facos(int32_t a)
{
	return d2f(acos(f2d(a)) / (2 * PI));
}

int32_t ftan(int32_t a)
{
	return d2f(tan(f2d(a) * 2 * PI));
}

int32_t fatan(int32_t a)
{
	return d2f(atan(f2d(a)) / (2 * PI));
}

int32_t fatan2(int32_t a, int32_t b)
{
	return d2f(atan2(f2d(a), f2d(b)) / (2 * PI));
}

