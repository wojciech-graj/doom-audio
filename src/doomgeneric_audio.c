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

#include "doomgeneric.h"
#include "doomkeys.h"

#include <math.h>
#include <stdbool.h>
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

#define bin_idx_from_freq(frequency) (round((frequency)*INPUT_FRAMES_PER_BUFFER * 2 / (double)INPUT_SAMPLE_RATE))

#define OUTPUT_SAMPLE_RATE 44100
#define INPUT_SAMPLE_RATE 44100
#define INPUT_FRAMES_PER_BUFFER 128
#define FRAMETIME_MS 1001
#define INPUT_MAGNITUDE_THRESH 1.0
#define N_KEYS 15

#define PI 3.14159265

struct key {
	uint8_t doomkey;
	bool pressed;
};

struct color_t {
	uint32_t b : 8;
	uint32_t g : 8;
	uint32_t r : 8;
	uint32_t a : 8;
};

struct keymap {
	size_t bin_idx;
	struct key key;
} keys[N_KEYS];

struct key_event_queue {
	size_t read_idx;
	size_t write_idx;
	struct key events[N_KEYS];
} key_events = {
	.read_idx = 1,
	.write_idx = 0,
};

struct timespec ts_init;
struct timespec ts_prev_frame;
PaStream *output_stream;
PaStream *input_stream;

extern void rdft(int, int, double *);

void DG_Init(void)
{
	keys[0] = (struct keymap){ .bin_idx = bin_idx_from_freq(2000), .key = { .doomkey = KEY_RIGHTARROW } };
	keys[1] = (struct keymap){ .bin_idx = bin_idx_from_freq(3000), .key = { .doomkey = KEY_LEFTARROW } };
	keys[2] = (struct keymap){ .bin_idx = bin_idx_from_freq(4000), .key = { .doomkey = KEY_UPARROW } };
	keys[3] = (struct keymap){ .bin_idx = bin_idx_from_freq(5000), .key = { .doomkey = KEY_DOWNARROW } };
	keys[4] = (struct keymap){ .bin_idx = bin_idx_from_freq(6000), .key = { .doomkey = KEY_USE } };
	keys[5] = (struct keymap){ .bin_idx = bin_idx_from_freq(7000), .key = { .doomkey = KEY_FIRE } };
	keys[6] = (struct keymap){ .bin_idx = bin_idx_from_freq(8000), .key = { .doomkey = KEY_ESCAPE } };
	keys[7] = (struct keymap){ .bin_idx = bin_idx_from_freq(9000), .key = { .doomkey = KEY_ENTER } };
	keys[8] = (struct keymap){ .bin_idx = bin_idx_from_freq(10000), .key = { .doomkey = KEY_TAB } };
	keys[9] = (struct keymap){ .bin_idx = bin_idx_from_freq(11000), .key = { .doomkey = '1' } };
	keys[10] = (struct keymap){ .bin_idx = bin_idx_from_freq(12000), .key = { .doomkey = '2' } };
	keys[11] = (struct keymap){ .bin_idx = bin_idx_from_freq(13000), .key = { .doomkey = '3' } };
	keys[12] = (struct keymap){ .bin_idx = bin_idx_from_freq(14000), .key = { .doomkey = '4' } };
	keys[13] = (struct keymap){ .bin_idx = bin_idx_from_freq(15000), .key = { .doomkey = '5' } };
	keys[14] = (struct keymap){ .bin_idx = bin_idx_from_freq(16000), .key = { .doomkey = '6' } };

	call_pa(Pa_Initialize());

	PaStreamParameters output_params;
	output_params.device = Pa_GetDefaultOutputDevice();
	error_check(output_params.device != paNoDevice, "Error: No default output device.\n");
	output_params.channelCount = 1;
	output_params.sampleFormat = paFloat32;
	output_params.suggestedLatency = Pa_GetDeviceInfo(output_params.device)->defaultLowOutputLatency;
	output_params.hostApiSpecificStreamInfo = NULL;

	PaStreamParameters input_params;
	input_params.device = Pa_GetDefaultInputDevice();
	error_check(output_params.device != paNoDevice, "Error: No default input device.\n");
	input_params.channelCount = 1;
	input_params.sampleFormat = paInt16;
	input_params.suggestedLatency = Pa_GetDeviceInfo(input_params.device)->defaultLowInputLatency;
	input_params.hostApiSpecificStreamInfo = NULL;

	call_pa(Pa_OpenStream(&output_stream, NULL, &output_params, OUTPUT_SAMPLE_RATE, OUTPUT_SAMPLE_RATE, paClipOff, NULL, NULL));
	call_pa(Pa_OpenStream(&input_stream, &input_params, NULL, OUTPUT_SAMPLE_RATE, INPUT_FRAMES_PER_BUFFER, paClipOff, NULL, NULL));

	call_pa(Pa_StartStream(output_stream));

	clock_gettime(CLOCK_REALTIME, &ts_init);
	clock_gettime(CLOCK_REALTIME, &ts_prev_frame);
}

void DG_DrawFrame(void)
{
	float buffer[OUTPUT_SAMPLE_RATE];
	struct color_t *pixels = (struct color_t *)DG_ScreenBuffer;

	/* Fill buffer */
	unsigned i = 0;
	unsigned x, y;
	for (x = 0; x < OUTPUT_SAMPLE_RATE; x++) {
		float sample = 0;
		for (y = 0; y < DOOMGENERIC_RESY; y++) {
			struct color_t pixel = pixels[y * DOOMGENERIC_RESX + (x * DOOMGENERIC_RESX / OUTPUT_SAMPLE_RATE)];
			sample += (pixel.b + pixel.g + pixel.r) * sinf((DOOMGENERIC_RESY + 1 - y) * (20000.f / DOOMGENERIC_RESY) * PI * 2.f * x / OUTPUT_SAMPLE_RATE);
		}
		buffer[i++] = sample / 153600.f;
	}

	/* Wait for last frame to finish playing */
	struct timespec ts_now;
	clock_gettime(CLOCK_REALTIME, &ts_now);
	uint32_t elapsed_ms = (ts_now.tv_sec - ts_prev_frame.tv_sec) * 1000 + (ts_now.tv_nsec - ts_prev_frame.tv_nsec) / 1000000;
	if (elapsed_ms < FRAMETIME_MS)
		DG_SleepMs(FRAMETIME_MS - elapsed_ms);

	/* Play frame */
	Pa_WriteStream(output_stream, buffer, OUTPUT_SAMPLE_RATE);
	clock_gettime(CLOCK_REALTIME, &ts_prev_frame);
}

void DG_SleepMs(const uint32_t ms)
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
	/* Read input if hasn't been read this frame */
	if (key_events.read_idx > key_events.write_idx) {
		key_events.read_idx = 0;
		key_events.write_idx = 0;

		/* Get audio sample */
		int16_t input_buffer[INPUT_FRAMES_PER_BUFFER];
		call_pa(Pa_StartStream(input_stream));
		call_pa(Pa_ReadStream(input_stream, input_buffer, INPUT_FRAMES_PER_BUFFER));
		call_pa(Pa_StopStream(input_stream));

		/* Convert to double */
		double buffer[INPUT_FRAMES_PER_BUFFER];
		unsigned i;
		for (i = 0; i < INPUT_FRAMES_PER_BUFFER; i++)
			buffer[i] = input_buffer[i] / 32768.;

		/* Perform fourier transform */
		rdft(INPUT_FRAMES_PER_BUFFER, 1, buffer);

		/* Generate key events */
		for (i = 0; i < N_KEYS; i++) {
			if ((fabs(buffer[keys[i].bin_idx]) > INPUT_MAGNITUDE_THRESH) != keys[i].key.pressed) {
				keys[i].key.pressed = !keys[i].key.pressed;
				key_events.events[key_events.write_idx++] = keys[i].key;
			}
		}
	}

	if (key_events.read_idx == key_events.write_idx) {
		key_events.read_idx++;
		return 0;
	}

	struct key event = key_events.events[key_events.read_idx++];
	*pressed = event.pressed;
	*doomKey = event.doomkey;
	return 1;
}

void DG_SetWindowTitle(const char *title)
{
	(void)title;
}
