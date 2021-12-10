#include "handmade.h"

local void
gameOutputSound(GameSoundOutputBuffer* sound_buffer, int tone_hz){
	persist f32 tSine;
	s16 toneVolume = 3000;
	int wavePeriod = sound_buffer->samples_per_second / tone_hz;
	
	s16* sampleOut = sound_buffer->samples;
	for(int sampleIndex = 0; sampleIndex < sound_buffer->sample_count; ++sampleIndex){
		f32 sineValue = sinf(tSine);
		s16 sampleValue = (s16)(sineValue *   toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;
		
		tSine += M_2PI / (f32)wavePeriod;
		if(tSine > M_2PI) tSine -= M_2PI;
	}
}

local void
renderGradient(GameOffscreenBuffer* buffer, int xOffset, int yOffset){
	u8* row = (u8*)buffer->memory;
	for(int y = 0; y < buffer->height; ++y){
		u32* pixel = (u32*)row;
		for(int x = 0; x < buffer->width; ++x){
			//b,g,r,pad     because windows
			u8 b = (u8)(x + xOffset);
			u8 g = (u8)(y + yOffset);
			//u8 r = ;
			//u8 p = ;
			
			*pixel++ = (/*((r << 16) |*/ (g << 8) | (b << 0)); //shift red and green, increment the pointer
		}
		row += buffer->pitch;
	}
}

local void 
gameUpdateAndRender(GameMemory* memory, GameInput* input, GameOffscreenBuffer* render_buffer){
	Assert((&input->controllers[0].TERMINATOR - &input->controllers[0].buttons[0]) == 
		   ArrayCount(input->controllers[0].buttons));
	Assert(sizeof(GameState) <= memory->permanent_storage_size);
	
	GameState* game_state = (GameState*)memory->permanent_storage;
	if(!memory->initialized) {
		char* filename = __FILE__;
		DEBUGReadFileResult test = debugPlatformReadEntireFile(filename);
		if(test.memory){
			debugPlatformWriteEntireFile("data/test.out", test.memory, test.memory_size);
			debugPlatformFreeFileMemory(test.memory);
		}
		
		game_state->tone_hz = 64;
		memory->initialized = true;
	}
	
	forX(controller_idx, ArrayCount(input->controllers)){
		GameControllerInput* controller = getController(input, controller_idx);
		if(controller->analog){
			game_state->x_offset += (int)(4.f*(controller->left_stick_average_x));
			game_state->tone_hz = 64 + (int)(128.f*(controller->left_stick_average_y));
		}else{
			if(controller->left_stick_up.   ended_down) game_state->tone_hz = ClampMax(game_state->tone_hz + 4, 1024);
			if(controller->left_stick_down. ended_down) game_state->tone_hz = ClampMin(game_state->tone_hz - 4, 8);
			if(controller->button_left.     ended_down) game_state->x_offset -= 1;
			if(controller->button_right.    ended_down) game_state->x_offset += 1;
			if(controller->button_up.       ended_down) game_state->y_offset -= 1;
			if(controller->button_down.     ended_down) game_state->y_offset += 1;
		}
	}
	
	renderGradient(render_buffer, game_state->x_offset, game_state->y_offset);
}

local void 
gameGetSoundSamples(GameMemory* memory, GameSoundOutputBuffer* sound_buffer){
	GameState* game_state = (GameState*)memory->permanent_storage;
	gameOutputSound(sound_buffer, game_state->tone_hz);
}