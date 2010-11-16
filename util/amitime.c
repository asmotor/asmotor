#ifdef __VBCC__
#include <devices/timer.h>
#include <proto/timer.h>
#include <proto/exec.h>
#include <stdlib.h>

#include "amitime.h"

static struct timerequest time_Request;
struct Library* TimerBase = NULL;

void time_Shutdown(void)
{
	CloseDevice((struct IORequest*)&time_Request);
}

void time_Init(void)
{
	if(TimerBase == NULL)
	{
		if(OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest*)&time_Request, 0) == 0)
		{
			TimerBase = (struct Library*)time_Request.tr_node.io_Device;
			atexit(time_Shutdown);
		}
	}
}

uint32_t time_GetMicroSeconds(void)
{
	struct timeval tv;

	time_Init();

	GetSysTime(&tv);
	return tv.tv_secs * 1000000 + tv.tv_micro;
}
#endif
