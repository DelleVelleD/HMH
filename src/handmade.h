#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

#include "defines.h"

//// services provided from the platform layer to the game ////


//// services provided from the game to the platform layer ////

struct game_offscreen_buffer{
	//pixels are 32-bits, memory order: BB GG RR XX
	void* memory;
	int width;
	int height;
	int pitch; //possible row offset
};

struct game_sound_output_buffer{
	int samplesPerSecond;
	int sampleCount;
	s16* samples;
};

//timing, keyboard input, bitmap buffer to use, sound buffer to use
static_internal void GameUpdateAndRender(game_offscreen_buffer* renderBuffer, int xOffset, int yOffset);

#endif //HANDMADE_H
