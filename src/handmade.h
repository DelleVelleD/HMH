#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H



inline u32 
SafeTruncateU64(u64 value){
	Assert(value <= 0xFFFFFFFF);
	return (u32)value;
}

//// services provided from the platform layer to the game ////

#if HANDMADE_INTERNAL
//@Important
//NOTE these are not for doing anything in the release build: they are blocking
// and the write doesnt protect against lost data
struct DEBUGReadFileResult{
	void* memory;
	u32   memory_size;
};
static_internal DEBUGReadFileResult DEBUGPlatformReadEntireFile(char* filename);
static_internal void DEBUGPlatformFreeFileMemory(void* memory);
static_internal b32 DEBUGPlatformWriteEntireFile(char* filename, void* memory, u32 memory_size);
#endif //HANDMADE_INTERNAL

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

struct GameControllerInput{
	b32 connected;
	b32 analog;
	
	f32 left_stick_average_x;
	f32 left_stick_average_y;
	
	union{
		GameButtonState buttons[12];
		struct{
			GameButtonState left_stick_up;
			GameButtonState left_stick_down;
			GameButtonState left_stick_right;
			GameButtonState left_stick_left;
			
			GameButtonState button_start;
			GameButtonState button_back;
			GameButtonState button_up;
			GameButtonState button_down;
			GameButtonState button_right;
			GameButtonState button_left;
			
			GameButtonState shoulder_right;
			GameButtonState shoulder_left;
			
			//NOTE all buttons must be added above this line
			GameButtonState TERMINATOR;
		};
	};
};

struct GameInput{
	GameControllerInput controllers[5];
};
inline GameControllerInput* GetController(GameInput* input, u32 controller_index){
	Assert(controller_index < ArrayCount(input->controllers));
	return &input->controllers[controller_index];
}

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
