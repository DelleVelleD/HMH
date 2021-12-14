#include <math.h> //sinf
#include "handmade.h"

local void
GameOutputSound(GameState* game_state, GameSoundOutputBuffer* sound_buffer){
	s16 toneVolume = 3000;
	int wavePeriod = sound_buffer->samplesPerSecond / game_state->toneHz;
	
	s16* sampleOut = sound_buffer->samples;
	for(int sampleIndex = 0; sampleIndex < sound_buffer->sampleCount; ++sampleIndex){
		f32 sineValue = sinf(game_state->tSine);
		s16 sampleValue = (s16)(sineValue *   toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;
		
		game_state->tSine += M_2PI / (f32)wavePeriod;
		if(game_state->tSine > M_2PI) game_state->tSine -= M_2PI;
	}
}

local void
RenderGradient(GameOffscreenBuffer* buffer, int xOffset, int yOffset){
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

external GAME_UPDATE_AND_RENDER(GameUpdateAndRender){
	Assert((&input->controllers[0].TERMINATOR - &input->controllers[0].buttons[0]) == 
		   ArrayCount(input->controllers[0].buttons));
	Assert(sizeof(GameState) <= memory->permanentStorageSize);
	
	GameState* game_state = (GameState*)memory->permanentStorage;
	if(!memory->initialized) {
		char* filename = __FILE__;
		DEBUGReadFileResult test = memory->DEBUGPlatformReadEntireFile(filename);
		if(test.memory){
			memory->DEBUGPlatformWriteEntireFile("data/test.out", test.memory, test.memorySize);
			memory->DEBUGPlatformFreeFileMemory(test.memory);
		}
		
		game_state->toneHz = 64;
		game_state->tSine = 0.0f;
		memory->initialized = true;
	}
	
	forX(controller_idx, ArrayCount(input->controllers)){
		GameControllerInput* controller = GetController(input, controller_idx);
		if(controller->analog){
			game_state->xOffset += (int)(4.f*(controller->leftStickAverageX));
			game_state->toneHz = 64 + (int)(128.f*(controller->leftStickAverageY));
		}else{
			if(controller->leftStickUp.  endedDown) game_state->toneHz = ClampMax(game_state->toneHz + 4, 1024);
			if(controller->leftStickDown.endedDown) game_state->toneHz = ClampMin(game_state->toneHz - 4, 8);
			if(controller->buttonLeft.   endedDown) game_state->xOffset -= 1;
			if(controller->buttonRight.  endedDown) game_state->xOffset += 1;
			if(controller->buttonUp.     endedDown) game_state->yOffset -= 1;
			if(controller->buttonDown.   endedDown) game_state->yOffset += 1;
		}
	}
	
	RenderGradient(render_buffer, game_state->xOffset, game_state->yOffset);
}

external GAME_GET_SOUND_SAMPLES(GameGetSoundSamples){
	GameState* game_state = (GameState*)memory->permanentStorage;
	GameOutputSound(game_state, sound_buffer);
}

#if HANDMADE_WIN64
#undef function
#include "windows.h"
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdReason, LPVOID lpvReserved){
	return TRUE;
}
#endif //HANDMADE_WIN64