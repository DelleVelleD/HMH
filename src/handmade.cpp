#include "handmade.h"

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
GameUpdateAndRender(game_offscreen_buffer* renderBuffer, int xOffset, int yOffset){
	RenderGradient(renderBuffer, xOffset, yOffset);
}