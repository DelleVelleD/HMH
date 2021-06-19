#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

#include "defines.h"

//// services provided from the platform layer to the game ////


//// services provided from the game to the platform layer ////

struct GameOffscreenBuffer{
	//pixels are 32-bits, memory order: BB GG RR XX
	void* memory;
	int   width;
	int   height;
	int   pitch; //possible row offset
};

struct GameSoundOutputBuffer{
	int  samples_per_second;
	int  sample_count;
	s16* samples;
};

struct GameButtonState{
	int half_transition_count; //twice the number of times the button's state changed
	b32 ended_down;
};

struct GameStickState{
	f32 start_x;
	f32 start_y;
	f32 end_x;
	f32 end_y;
	f32 min_x;
	f32 min_y;
	f32 max_x;
	f32 max_y;
};

struct GameControllerInput{
	b32 analog;
	
	union{
		GameStickState sticks[2];
		struct{
			GameStickState stick_right;
			GameStickState stick_left;
		};
	};
	
	union{
		GameButtonState buttons[8];
		struct{
			GameButtonState button_start;
			GameButtonState button_select;
			GameButtonState button_up;
			GameButtonState button_down;
			GameButtonState button_right;
			GameButtonState button_left;
			GameButtonState shoulder_right;
			GameButtonState shoulder_left;
		};
	};
};

struct GameInput{
	GameControllerInput controllers[4];
};

struct GameMemory{
	b32   initialized;
	u64   permanent_storage_size;
	void* permanent_storage; //NOTE required to be init to zero at startup
	u64   transient_storage_size;
	void* transient_storage; //NOTE required to be init to zero at startup
};

//timing, keyboard input, bitmap buffer to use, sound buffer to use
static_internal void GameUpdateAndRender(GameMemory* memory, GameInput* input, GameOffscreenBuffer* render_buffer, GameSoundOutputBuffer* sound_buffer);



//-

struct GameState{
	int x_offset;
	int y_offset;
	int tone_hz;
};

#endif //HANDMADE_H
