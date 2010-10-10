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


extern ULONG time_GetMicroSeconds(void);

#else

#include <time.h>

#endif

#endif
