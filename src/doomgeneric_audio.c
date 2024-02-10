//
// Copyright(C) 2024 Wojciech Graj
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//     Nil
//

#include "doomkeys.h"
//#include "i_system.h"
#include "doomgeneric.h"

#include <time.h>

struct timespec ts_init;

void DG_Init()
{
	 clock_gettime(CLOCK_REALTIME, &ts_init);
}

void DG_DrawFrame()
{

}

void DG_SleepMs(uint32_t ms)
{
	struct timespec ts = (struct timespec) {
		.tv_sec = ms / 1000,
		.tv_nsec = (ms % 1000ul) * 1000000,
	};
	nanosleep(&ts, NULL);
}

uint32_t DG_GetTicksMs()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	return (ts.tv_sec - ts_init.tv_sec) * 1000 + (ts.tv_nsec - ts_init.tv_nsec) / 1000000;
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	return 0;
}

void DG_SetWindowTitle(const char *title)
{
	(void)title;
}
