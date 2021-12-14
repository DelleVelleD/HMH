#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

#include "defines.h"

inline u32 
safeTruncateU64(u64 value){
	Assert(value <= 0xFFFFFFFF);
	return (u32)value;
}

//-
//// services provided from the platform layer to the game ////

#if HANDMADE_INTERNAL
//@Important
//NOTE these are not for doing anything in the release build: they are blocking
// and the write doesnt protect against lost data
struct DEBUGReadFileResult{
	void* memory;
	u32   memorySize;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void* memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DEBUGReadFileResult name(char* filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_FILE_MEMORY(name) b32 name(char* filename, void* memory, u32 memory_size)
typedef DEBUG_PLATFORM_WRITE_FILE_MEMORY(debug_platform_write_file_memory);
#endif //HANDMADE_INTERNAL

//-
//// services provided from the game to the platform layer ////

struct GameOffscreenBuffer{
	//pixels are 32-bits, memory order: BB GG RR XX
	void* memory;
	int   width;
	int   height;
	int   pitch; //possible row offset
};

struct GameSoundOutputBuffer{
	int  samplesPerSecond;
	int  sampleCount;
	s16* samples;
};

struct GameButtonState{
	int halfTransitionCount; //twice the number of times the button's state changed
	b32 endedDown;
};

struct GameControllerInput{
	b32 connected;
	b32 analog;
	
	f32 leftStickAverageX;
	f32 leftStickAverageY;
	
	union{
		GameButtonState buttons[12];
		struct{
			GameButtonState leftStickUp;
			GameButtonState leftStickDown;
			GameButtonState leftStickRight;
			GameButtonState leftStickLeft;
			
			GameButtonState buttonStart;
			GameButtonState buttonBack;
			GameButtonState buttonUp;
			GameButtonState buttonDown;
			GameButtonState buttonRight;
			GameButtonState buttonLeft;
			
			GameButtonState shoulderRight;
			GameButtonState shoulderLeft;
			
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
	
	u64   permanentStorageSize;
	void* permanentStorage; //NOTE required to be init to zero at startup
	u64   transientStorageSize;
	void* transientStorage; //NOTE required to be init to zero at startup
	
	debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
	debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
	debug_platform_write_file_memory* DEBUGPlatformWriteEntireFile;
};

//timing, keyboard input, bitmap buffer to use, sound buffer to use
#define GAME_UPDATE_AND_RENDER(name) void name(GameMemory* memory, GameInput* input, GameOffscreenBuffer* render_buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub){}

//NOTE at the moment, this has to be a very fast function, it cannot be more than a millisecond or so
//TODO reduce the pressure on this functions perf by measuring it or etc
#define GAME_GET_SOUND_SAMPLES(name) void name(GameMemory* memory, GameSoundOutputBuffer* sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub){}

//-

struct GameState{
	int xOffset;
	int yOffset;
	int toneHz;
	f32 tSine;
};

#endif //HANDMADE_H
