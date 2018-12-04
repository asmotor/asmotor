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

#ifdef __VBCC__

#include <devices/timer.h>
#include <proto/timer.h>
#include <proto/exec.h>
#include <stdlib.h>

#include "amitime.h"

static struct timerequest time_Request;
static struct Library* TimerBase = NULL;

void time_Shutdown(void) {
	CloseDevice((struct IORequest*) &time_Request);
}

void time_Init(void) {
	if (TimerBase == NULL) {
		if (OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest*) &time_Request, 0) == 0) {
			TimerBase = (struct Library*) time_Request.tr_node.io_Device;
			atexit(time_Shutdown);
		}
	}
}

uint32_t time_GetMicroSeconds(void) {
	struct timeval tv;

	time_Init();

	GetSysTime(&tv);
	return tv.tv_secs * 1000000 + tv.tv_micro;
}

#endif
