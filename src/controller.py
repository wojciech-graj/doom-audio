"""
Copyright(C) 2024 Wojciech Graj

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
"""

import pygame
import numpy as np

KEYMAP = {
    pygame.K_RIGHT: 2000,
    pygame.K_LEFT: 3000,
    pygame.K_UP: 4000,
    pygame.K_DOWN: 5000,
    pygame.K_e: 6000,
    pygame.K_SPACE: 7000,
    pygame.K_ESCAPE: 8000,
    pygame.K_RETURN: 9000,
    pygame.K_TAB: 10000,
    pygame.K_1: 11000,
    pygame.K_2: 12000,
    pygame.K_3: 13000,
    pygame.K_4: 14000,
    pygame.K_5: 15000,
    pygame.K_6: 16000,
}

SAMPLE_RATE = 44100
BITS = 16

def sine_wave(frequency, duration):
    samples = int(SAMPLE_RATE * duration)
    period = int(SAMPLE_RATE / frequency)
    amplitude = 2 ** (BITS - 1) - 1
    mono_wave = amplitude * np.sin(2.0 * np.pi * frequency * np.arange(samples) / SAMPLE_RATE)
    stereo_wave = np.column_stack((mono_wave, mono_wave))
    return stereo_wave.astype(np.int16)

def mainloop():
    window = pygame.display.set_mode((320, 200))
    notes_playing = {}
    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                return
            elif event.type == pygame.KEYDOWN:
                if event.key in KEYMAP:
                    frequency = KEYMAP[event.key]
                    if event.key not in notes_playing:
                        sound = pygame.sndarray.make_sound(sine_wave(frequency, 0.5))
                        sound.play(loops=-1)
                        notes_playing[event.key] = sound
            elif event.type == pygame.KEYUP:
                if event.key in notes_playing:
                    notes_playing[event.key].stop()
                    del notes_playing[event.key]
        window.fill((255, 255, 255))
        pygame.display.flip()

if __name__ == "__main__":
    pygame.init()
    pygame.mixer.init(frequency=SAMPLE_RATE, size=-16)
    mainloop()
    pygame.quit()
