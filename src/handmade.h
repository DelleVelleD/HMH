#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

//// services provided from the platform layer to the game ////


//// services provided from the game to the platform layer ////

struct game_offscreen_buffer{
	//pixels are 32-bits, memory order: BB GG RR XX
	void* memory;
	int width;
	int height;
	int pitch; //possible row offset
};

//timing, keyboard input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(game_offscreen_buffer* renderBuffer, int xOffset, int yOffset);

#endif //HANDMADE_H
