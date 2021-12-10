#pragma once
#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

struct Win32OffscreenBuffer{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int bytes_per_pixel;
	int pitch; //possible row offset
};

struct Win32WindowDimensions{
	int width;
	int height;
};

struct DEBUGWin32TimeMarker{
	DWORD output_play_cursor;
	DWORD output_write_cursor;
	DWORD output_location;
	DWORD output_byte_count;
	DWORD expected_flip_play_cursor;
	DWORD flip_play_cursor;
	DWORD flip_write_cursor;
};

struct Win32SoundOutput{
	int samples_per_second;
	u32 running_sample_index;
	int bytes_per_sample;
	DWORD secondary_buffer_size;
	DWORD safety_bytes;
	f32 t_sine;
	int latency_sample_count;
};

#endif //WIN32_HANDMADE_H
