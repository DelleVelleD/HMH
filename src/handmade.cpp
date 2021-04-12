#include "handmade.h"

internal void
GameOutputSound(game_sound_output_buffer* soundBuffer, int toneHz){
	local_persist f32 tSine;
	i16 toneVolume = 3000;
	//int toneHz = 256;
	int wavePeriod = soundBuffer->samplesPerSecond / toneHz;
	
	i16* sampleOut = soundBuffer->samples;
	for(int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex){
		f32 sineValue = sinf(tSine);
		i16 sampleValue = (i16)(sineValue *   toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;
		
		tSine += 2.f*M_PI / (f32)wavePeriod;
	}
}

internal void
RenderGradient(game_offscreen_buffer* buffer, int xOffset, int yOffset){
	u8* row = (u8*)buffer->memory;
	for(int y = 0; y < buffer->height; ++y){
		u32* pixel = (u32*)row;
		for(int x = 0; x < buffer->width; ++x){
			//b,g,r,pad     because windows
			u8 b = (x + xOffset);
			u8 g = (y + yOffset);
			//u8 r = ;
			//u8 p = ;
			
			*pixel++ = (/*((r << 16) |*/ (g << 8) | b); //shift red and green, increment the pointer
		}
		row += buffer->pitch;
	}
}

internal void 
GameUpdateAndRender(game_offscreen_buffer* renderBuffer, game_sound_output_buffer* soundBuffer, int xOffset, int yOffset, int toneHz){
	GameOutputSound(soundBuffer, toneHz);
	RenderGradient(renderBuffer, xOffset, yOffset);
}