#pragma once
#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

#include "defines.h"

struct Win32OffscreenBuffer{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int bytesPerPixel;
	int pitch; //possible row offset
};

struct Win32WindowDimensions{
	int width;
	int height;
};

struct DEBUGWin32TimeMarker{
	DWORD outputPlayCursor;
	DWORD outputWriteCursor;
	DWORD outputLocation;
	DWORD outputByteCount;
	DWORD expectedFlipPlayCursor;
	DWORD flipPlayCursor;
	DWORD flipWriteCursor;
};

struct Win32SoundOutput{
	int samplesPerSecond;
	u32 runningSampleIndex;
	int bytesPerSample;
	DWORD secondaryBufferSize;
	DWORD safetyBytes;
	f32 tSine;
	int latencySampleCount;
};

#endif //WIN32_HANDMADE_H
