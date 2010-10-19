#include "asmotor.h"
#include "types.h"

#include <math.h>

#ifndef	PI
#define	PI	acos(-1.0)
#endif

double f2d(SLONG a)
{
	return a * (1.0 / 65536.0);
}

SLONG d2f(double a)
{
	return (SLONG)(a * 65536);
}

SLONG imuldiv(SLONG a, SLONG b, SLONG c)
{
	return (SLLONG)a * b / c;
}

SLONG fmul(SLONG a, SLONG b)
{
	return (SLONG)(((SLLONG)a * b) >> 16);
}

SLONG fdiv(SLONG a, SLONG b)
{
	return (SLONG)(((SLLONG)a << 16) / b);
}

SLONG fsin(SLONG a)
{
	return d2f(sin(f2d(a) * 2 * PI));
}

SLONG fasin(SLONG a)
{
	return d2f(asin(f2d(a)) / (2 * PI));
}

SLONG fcos(SLONG a)
{
	return d2f(cos(f2d(a) * 2 * PI));
}

SLONG facos(SLONG a)
{
	return d2f(acos(f2d(a)) / (2 * PI));
}

SLONG ftan(SLONG a)
{
	return d2f(tan(f2d(a) * 2 * PI));
}

SLONG fatan(SLONG a)
{
	return d2f(atan(f2d(a)) / (2 * PI));
}

SLONG fatan2(SLONG a, SLONG b)
{
	return d2f(atan2(f2d(a), f2d(b)) / (2 * PI));
}

