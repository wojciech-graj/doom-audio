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
#include "doomgeneric.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <portaudio.h>

#define unlikely(x) __builtin_expect((x), 0)
#define error(...)                            \
	do {                                  \
		fprintf(stderr, __VA_ARGS__); \
		Pa_Terminate();               \
		exit(1);                      \
	} while (0)

#define error_check(cond, ...)              \
	do {                                \
		if (unlikely(!(cond))) {    \
			error(__VA_ARGS__); \
		}                           \
	} while (0)

#define call_pa(stmt)                                                                          \
	do {                                                                                   \
		PaError err = stmt;                                                            \
		error_check(err == paNoError, "Error: Portaudio: %s\n", Pa_GetErrorText(err)); \
	} while (0)

#define SAMPLE_RATE 96000

#define PI 3.14159265

struct timespec ts_init;
PaStream *stream;

struct color_t {
	uint32_t b : 8;
	uint32_t g : 8;
	uint32_t r : 8;
	uint32_t a : 8;
};

void DG_Init(void)
{
	call_pa(Pa_Initialize());

	PaStreamParameters output_params;
	output_params.device = Pa_GetDefaultOutputDevice();
	error_check(output_params.device != paNoDevice, "Error: No default output device.\n");
	output_params.channelCount = 1;
	output_params.sampleFormat = paFloat32;
	output_params.suggestedLatency = Pa_GetDeviceInfo(output_params.device)->defaultLowOutputLatency;
	output_params.hostApiSpecificStreamInfo = NULL;

	call_pa(Pa_OpenStream(&stream, NULL, &output_params, SAMPLE_RATE, SAMPLE_RATE, paClipOff, NULL, NULL));

	call_pa(Pa_StartStream(stream));

	clock_gettime(CLOCK_REALTIME, &ts_init);
}

void DG_DrawFrame(void)
{
	float buffer[SAMPLE_RATE];
	struct color_t *pixels = (struct color_t *)DG_ScreenBuffer;

	unsigned i = 0;
	unsigned x, y;
	for (x = 0; x < SAMPLE_RATE; x++) {
		float rez = 0;
		for (y = 0; y < DOOMGENERIC_RESY; y++) {
			struct color_t pixel = pixels[y * DOOMGENERIC_RESX + (x * DOOMGENERIC_RESX / SAMPLE_RATE)];
			rez += (pixel.b + pixel.g + pixel.r) * sinf((DOOMGENERIC_RESY + 1 - y) * (20000.f / DOOMGENERIC_RESY) * PI * 2.f * x / SAMPLE_RATE);
		}
		buffer[i++] = rez / 153600.f;
	}

	Pa_WriteStream(stream, buffer, SAMPLE_RATE);
}

void DG_SleepMs(uint32_t ms)
{
	struct timespec ts = (struct timespec){
		.tv_sec = ms / 1000,
		.tv_nsec = (ms % 1000ul) * 1000000,
	};
	nanosleep(&ts, NULL);
}

uint32_t DG_GetTicksMs(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	return (ts.tv_sec - ts_init.tv_sec) * 1000 + (ts.tv_nsec - ts_init.tv_nsec) / 1000000;
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
	return 0;
}

void DG_SetWindowTitle(const char *title)
{
	(void)title;
}
