#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

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
local DEBUGReadFileResult debugPlatformReadEntireFile(char* filename);
local void debugPlatformFreeFileMemory(void* memory);
local b32 debugPlatformWriteEntireFile(char* filename, void* memory, u32 memory_size);
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
inline GameControllerInput* getController(GameInput* input, u32 controller_index){
	Assert(controller_index < ArrayCount(input->controllers));
	return &input->controllers[controller_index];
}

struct GameMemory{
	b32   initialized;
	u64   permanentStorageSize;
	void* permanentStorage; //NOTE required to be init to zero at startup
	u64   transientStorageSize;
	void* transientStorage; //NOTE required to be init to zero at startup
};

//timing, keyboard input, bitmap buffer to use, sound buffer to use
local void gameUpdateAndRender(GameMemory* memory, GameInput* input, GameOffscreenBuffer* render_buffer);

//NOTE at the moment, this has to be a very fast function, it cannot be more than a millisecond or so
//TODO reduce the pressure on this functions perf by measuring it or etc
local void gameGetSoundSamples(GameMemory* memory, GameSoundOutputBuffer* sound_buffer);

//-

struct GameState{
	int xOffset;
	int yOffset;
	int toneHz;
};

#endif //HANDMADE_H
