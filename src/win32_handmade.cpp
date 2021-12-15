//windows entry point
#include <stdio.h> //sprintf_s

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include "defines.h"
#include "handmade.h"
#include "win32_handmade.h"

//TODO temporary globals
global_ b32 global_running;
global_ b32 global_pause;
global_ Win32OffscreenBuffer global_backbuffer;
global_ LPDIRECTSOUNDBUFFER global_secondary_buffer;
global_ s64 global_perf_count_frequency;

//XInputGetState, this initializes to a stub pointer
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(x_input_get_state);
XINPUT_GET_STATE(XInputGetStateStub){return ERROR_DEVICE_NOT_CONNECTED;}
global_ x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//XInputSetState
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pState)
typedef XINPUT_SET_STATE(x_input_set_state);
XINPUT_SET_STATE(XInputSetStateStub){return ERROR_DEVICE_NOT_CONNECTED;}
global_ x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

//DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) DWORD WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory){
	if(memory){
		VirtualFree(memory, 0, MEM_RELEASE);
		memory = 0;
	}
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile){
	DEBUGReadFileResult result{};
	
	HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(file_handle != INVALID_HANDLE_VALUE){
		LARGE_INTEGER file_size;
		if(GetFileSizeEx(file_handle, &file_size)){
			u32 file_size_32 = safeTruncateU64(file_size.QuadPart); //NOTE ReadFile only takes up to a u32 value
			result.memory = VirtualAlloc(0, file_size.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(result.memory){
				DWORD bytes_read;
				if(ReadFile(file_handle, result.memory, file_size_32, &bytes_read, 0) &&
				   file_size_32 == bytes_read){
					//NOTE file read successfully
					result.memorySize = file_size_32;
				}else{
					/*TODO error logging*/
					DEBUGPlatformFreeFileMemory(result.memory);
				}
			}else{ /*TODO error logging*/ }
		}else{ /*TODO error logging*/  }
		
		CloseHandle(file_handle);
	}else{ /*TODO error logging*/  }
	
	return result; 
}

DEBUG_PLATFORM_WRITE_FILE_MEMORY(DEBUGPlatformWriteEntireFile){
	b32 result = false;
	
	HANDLE file_handle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(file_handle != INVALID_HANDLE_VALUE){
		DWORD bytes_written;
		if(WriteFile(file_handle, memory, memory_size, &bytes_written, 0)){
			//NOTE file write successful
			result = (bytes_written == memory_size);
		}else{
			/*TODO error logging*/
		}
		CloseHandle(file_handle);
	}else{ /*TODO error logging*/  }
	
	return result;
}

local FILETIME Win32GetLastWriteTime(char* filepath){
	FILETIME last_write_time = {};
	WIN32_FIND_DATA find_data;
	HANDLE find_handle = FindFirstFileA(filepath, &find_data);
	if(find_handle != INVALID_HANDLE_VALUE){
		last_write_time = find_data.ftLastWriteTime;
		FindClose(find_handle);
	}
	return last_write_time;
}

struct Win32GameCode{
	HMODULE dll;
	FILETIME dllLastWriteTime;
	game_update_and_render* UpdateAndRender;
	game_get_sound_samples* GetSoundSamples;
	b32 isValid;
};
function Win32GameCode
Win32LoadGameCode(char* source_dll_filepath, char* temp_dll_filepath){
	Win32GameCode result = {};
	result.dllLastWriteTime = Win32GetLastWriteTime(source_dll_filepath);
	CopyFile(source_dll_filepath, temp_dll_filepath, FALSE);
	result.dll = LoadLibraryA(temp_dll_filepath);
	if(result.dll){
		result.UpdateAndRender = (game_update_and_render*)GetProcAddress(result.dll, "GameUpdateAndRender");
		result.GetSoundSamples = (game_get_sound_samples*)GetProcAddress(result.dll, "GameGetSoundSamples");
		result.isValid = (result.UpdateAndRender && result.GetSoundSamples);
	}
	if(!result.isValid){
		result.UpdateAndRender = GameUpdateAndRenderStub;
		result.GetSoundSamples = GameGetSoundSamplesStub;
	}
	return result;
}

function void
Win32UnloadGameCode(Win32GameCode* game_code){
	if(game_code->dll){
		FreeLibrary(game_code->dll);
		game_code->dll = 0;
	}
	game_code->isValid = false;
	game_code->UpdateAndRender = GameUpdateAndRenderStub;
	game_code->GetSoundSamples = GameGetSoundSamplesStub;
}

function void
Win32LoadXInput(){
	HMODULE xinput_library = 0;
	if(xinput_library = LoadLibraryA("xinput1_4.dll")){
		OutputDebugStringA("Loaded xinput1_4.dll\n");
	}else{
		OutputDebugStringA("Failed to load xinput1_4.dll; trying to load xinput9_1_0.dll\n");
		if(xinput_library = LoadLibraryA("xinput9_1_0.dll")){
			OutputDebugStringA("Loaded xinput9_1_0.dll\n");
		}else{
			OutputDebugStringA("Failed to load xinput9_1_0.dll; trying to load xinput1_3.dll\n");
			if(xinput_library = LoadLibraryA("xinput1_3.dll")){
				OutputDebugStringA("Loaded xinput1_3.dll\n");
			}
		}
	}
	
	if(xinput_library){
		XInputGetState = (x_input_get_state*)GetProcAddress(xinput_library, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(xinput_library, "XInputSetState");
	}else{ 
		OutputDebugStringA("Failed to load any XInput dlls\n");
	}
}

function void
Win32InitDSound(HWND Window, s32 samplesPerSecond, s32 buffer_size){
	HRESULT result;
	
	//load the library
	HMODULE dsound_library = LoadLibraryA("dsound.dll");
	if(!dsound_library){ /*TODO diagnostic*/ }
	
	//get a DirectSound object
	direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(dsound_library, "DirectSoundCreate");
	
	LPDIRECTSOUND direct_sound;
	if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &direct_sound, 0))){
		//setup sound format
		WAVEFORMATEX wave_format{};
		wave_format.wFormatTag      = WAVE_FORMAT_PCM;
		wave_format.nChannels       = 2;
		wave_format.nSamplesPerSec  = samplesPerSecond;
		wave_format.wBitsPerSample  = 16;
		wave_format.nBlockAlign     = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
		wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
		wave_format.cbSize          = 0;
		
		//create a primary buffer and setup format
		if(SUCCEEDED(direct_sound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
			DSBUFFERDESC buffer_desc{};
			buffer_desc.dwSize  = sizeof(buffer_desc);
			buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
			
			LPDIRECTSOUNDBUFFER primary_buffer;
			result = direct_sound->CreateSoundBuffer(&buffer_desc, &primary_buffer, 0);
			if(SUCCEEDED(result)){
				result = primary_buffer->SetFormat(&wave_format);
				if(SUCCEEDED(result)){
					OutputDebugStringA("Primary buffer format was set\n");
				}else{ /*TODO diagnostic*/ }
			}else{ /*TODO diagnostic*/ }
		}else{ /*TODO diagnostic*/ }
		
		//create a secondary buffer
		DSBUFFERDESC buffer_desc{};
		buffer_desc.dwSize        = sizeof(buffer_desc);
		buffer_desc.dwFlags       = DSBCAPS_GETCURRENTPOSITION2;
		buffer_desc.dwBufferBytes = buffer_size;
		buffer_desc.lpwfxFormat   = &wave_format;
		
		result = direct_sound->CreateSoundBuffer(&buffer_desc, &global_secondary_buffer, 0);
		if(SUCCEEDED(result)){
			OutputDebugStringA("Secondary buffer created successfully\n");
		}else{ /*TODO diagnostic*/ }
		
		//start it playing
		
	}else{ /*TODO diagnostic*/ }
}

function Win32WindowDimensions
Win32GetWindowDimensions(HWND window){
	Win32WindowDimensions result;
	
	RECT client_rect;
	GetClientRect(window, &client_rect);
	result.height = client_rect.bottom - client_rect.top;
	result.width = client_rect.right - client_rect.left;
	
	return result;
}

function void
Win32ResizeDIBSection(Win32OffscreenBuffer* backbuffer, int width, int height){
	//free memory if it exists
	if(backbuffer->memory){
		VirtualFree(backbuffer->memory, 0, MEM_RELEASE);
	}
	
	backbuffer->width = width;
	backbuffer->height = height;
	int bytes_per_pixel = 4;
	backbuffer->bytesPerPixel = bytes_per_pixel;
	
	//setup bitmap info
	backbuffer->info.bmiHeader.biSize = sizeof(backbuffer->info.bmiHeader);
	backbuffer->info.bmiHeader.biWidth = backbuffer->width;
	backbuffer->info.bmiHeader.biHeight = -backbuffer->height; //negative to make it top-down
	backbuffer->info.bmiHeader.biPlanes = 1;
	backbuffer->info.bmiHeader.biBitCount = 32;
	backbuffer->info.bmiHeader.biCompression = BI_RGB;
	
	//allocate memory
	int bitmap_memory_size = bytes_per_pixel * width * height;
	backbuffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	backbuffer->pitch = backbuffer->width * bytes_per_pixel;
}

function void
Win32DisplayBufferInWindow(Win32OffscreenBuffer* backbuffer, HDC device_context, int window_width, int window_height){
	StretchDIBits(device_context, 
				  0, 0, window_width, window_height,
				  0, 0, backbuffer->width, backbuffer->height,
				  backbuffer->memory,
				  &backbuffer->info,
				  DIB_RGB_COLORS, SRCCOPY);
}

function LRESULT CALLBACK
Win32MainWindowCallback(HWND window, UINT message, WPARAM w_param, LPARAM l_param){
	LRESULT result = 0;
	
	switch(message){
		case WM_DESTROY:{
			//TODO handle with a message to the user?
			global_running = false;
		} break;
		
		case WM_CLOSE:{
			//TODO handle this as an error - recreate window?
			global_running = false;
		} break;
		
		case WM_ACTIVATEAPP:{
			OutputDebugStringA("WM_ACTIVEAPP\n");
		} break;
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:{
			Assert(!"Keyboard input came in through a non-dispatch message; it should have been captured already");
		}break;
		
		case WM_PAINT:{
			PAINTSTRUCT paint;
			HDC device_context = BeginPaint(window, &paint);
			
			Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
			Win32DisplayBufferInWindow(&global_backbuffer, device_context, 
									   dimensions.width, dimensions.height);
			EndPaint(window, &paint);
		} break;
		
		default:{
			//OutputDebugStringA("default\n");
			result = DefWindowProcA(window, message, w_param, l_param);
		} break;
	}
	
	return(result);
}

function void
Win32ClearSoundBuffer(Win32SoundOutput* sound_output){
	void* region1; void* region2;
	DWORD region1_size, region2_size;
	if(SUCCEEDED(global_secondary_buffer->Lock(0, sound_output->secondaryBufferSize, &region1, &region1_size, &region2, &region2_size, 0))){
		u8* dst_sample = (u8*)region1;
		for(DWORD byteIndex = 0; byteIndex < region1_size; ++byteIndex){
			*dst_sample++ = 0;
		}
		
		dst_sample = (u8*)region2;
		for(DWORD byteIndex = 0; byteIndex < region2_size; ++byteIndex){
			*dst_sample++ = 0;
		}
		
		global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
	}
}

function void 
Win32FillSoundBuffer(Win32SoundOutput* sound_output, GameSoundOutputBuffer* source_buffer, DWORD byte_to_lock, DWORD bytes_to_write){
	void* region1; void* region2;
	DWORD region1_size, region2_size;
	if(SUCCEEDED(global_secondary_buffer->Lock(byte_to_lock, bytes_to_write, &region1, &region1_size, &region2, &region2_size, 0))){
		//TODO assert region1_size/region2_size is valid
		DWORD region1_sample_count = region1_size/sound_output->bytesPerSample;
		s16* dst_sample = (s16*)region1;
		s16* src_sample = source_buffer->samples;
		for(DWORD sample_idx = 0; sample_idx < region1_sample_count; ++sample_idx){
			*dst_sample++ = *src_sample++;
			*dst_sample++ = *src_sample++;
			++sound_output->runningSampleIndex;
		}
		
		DWORD region2_sample_count = region2_size/sound_output->bytesPerSample;
		dst_sample = (s16*)region2;
		for(DWORD sample_idx = 0; sample_idx < region2_sample_count; ++sample_idx){
			*dst_sample++ = *src_sample++;
			*dst_sample++ = *src_sample++;
			++sound_output->runningSampleIndex;
		}
		
		global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
	}
}

function void
Win32ProcessXInputDigitalButton(DWORD xinput_button_state, GameButtonState* old_state, GameButtonState* new_state, DWORD button_bit){
	new_state->endedDown = (xinput_button_state & button_bit);
	new_state->halfTransitionCount = (old_state->endedDown != new_state->endedDown) ? 1 : 0;
}

function f32 
Win32ProcessXInputStickValue(SHORT value, SHORT deadzone_threshold){
	f32 result = 0;
	if(value < -deadzone_threshold){
		result = (f32)((value + deadzone_threshold) / (32768.f - deadzone_threshold));
	}else if(value > deadzone_threshold){
		result = (f32)((value - deadzone_threshold) / (32767.f - deadzone_threshold));
	}
	return result;
}

function void
Win32ProcessKeyboardMessage(GameButtonState* new_state, b32 is_down){
	Assert(new_state->endedDown != is_down);
	new_state->endedDown = is_down;
	new_state->halfTransitionCount += 1;
}

function void
Win32ProcessPendingMessages(GameControllerInput* keyboard_controller){
	MSG message;
	while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)){
		switch(message.message){
			//keyboard input
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:{
				u32 VKCode = (u32)message.wParam;
				b32 was_down = ((message.lParam & (1 << 30)) != 0);
				b32 is_down = ((message.lParam & (1 << 31)) == 0);
				if(was_down != is_down){
					if      (VKCode == 'W'){
						Win32ProcessKeyboardMessage(&keyboard_controller->leftStickUp, is_down);
					}else if(VKCode == 'A'){
						Win32ProcessKeyboardMessage(&keyboard_controller->leftStickLeft, is_down);
					}else if(VKCode == 'S'){
						Win32ProcessKeyboardMessage(&keyboard_controller->leftStickDown, is_down);
					}else if(VKCode == 'D'){
						Win32ProcessKeyboardMessage(&keyboard_controller->leftStickRight, is_down);
					}else if(VKCode == 'Q'){
						Win32ProcessKeyboardMessage(&keyboard_controller->shoulderLeft, is_down);
					}else if(VKCode == 'E'){
						Win32ProcessKeyboardMessage(&keyboard_controller->shoulderRight, is_down);
					}else if(VKCode == VK_UP){
						Win32ProcessKeyboardMessage(&keyboard_controller->buttonUp, is_down);
					}else if(VKCode == VK_LEFT){
						Win32ProcessKeyboardMessage(&keyboard_controller->buttonLeft, is_down);
					}else if(VKCode == VK_DOWN){
						Win32ProcessKeyboardMessage(&keyboard_controller->buttonDown, is_down);
					}else if(VKCode == VK_RIGHT){
						Win32ProcessKeyboardMessage(&keyboard_controller->buttonRight, is_down);
					}else if(VKCode == VK_ESCAPE){
						Win32ProcessKeyboardMessage(&keyboard_controller->buttonStart, is_down);
					}else if(VKCode == VK_SPACE){
						Win32ProcessKeyboardMessage(&keyboard_controller->buttonBack, is_down);
					}
#if HANDMADE_INTERNAL
					else if(VKCode == 'P'){
						if(is_down){
							ToggleBool(global_pause);
						}
					}
#endif //HANDMADE_INTERNAL
				}
				b32 alt_key_was_down = message.lParam & (1 << 29); //checks if the 29th bit is set
				if(VKCode == VK_F4 && alt_key_was_down){
					global_running = false;
				}
			}break;
			
			//quit message
			case WM_QUIT:{
				global_running = false;
			}break;
			
			//other messages
			default:{
				TranslateMessage(&message);
				DispatchMessageA(&message);
			}break;
		}
	}
}

inline LARGE_INTEGER
Win32GetWallClock(){
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

inline f32
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end){
	return (f32)(end.QuadPart - start.QuadPart) / (f32)global_perf_count_frequency;
}

function void
DEBUGWin32DrawVertical(Win32OffscreenBuffer* backbuffer, int x, int top, int bottom, u32 color){
	top = ClampMin(top, 0);
	bottom = ClampMax(bottom, backbuffer->height);
	if((x >= 0) && (x < backbuffer->width)){
		u8* pixel = (u8*)backbuffer->memory + (x * backbuffer->bytesPerPixel) + (top * backbuffer->pitch);
		for(int y = top; y < bottom; ++y){
			*(u32*)pixel = color;
			pixel += backbuffer->pitch;
		}
	}
}

inline function void 
DEBUGWin32DrawSoundBufferMarker(Win32OffscreenBuffer* backbuffer, Win32SoundOutput* sound,
								f32 coef, int pad_x, int top, int bottom, DWORD value, u32 color){
	int x = pad_x + (int)(coef * (f32)value);
	DEBUGWin32DrawVertical(backbuffer, x, top, bottom, color);
}

function void
DEBUGWin32SyncDisplay(Win32OffscreenBuffer* backbuffer, Win32SoundOutput* sound, DEBUGWin32TimeMarker* markers, 
					  int marker_count, int current_marker_idx, f32 target_seconds_per_frame){
	int pad_x = 16;
	int pad_y = 16;
	int line_height = 64;
	f32 coef = (f32)(backbuffer->width - 2*pad_x) / (f32)sound->secondaryBufferSize;
	
	forX(marker_idx, marker_count){
		DEBUGWin32TimeMarker* marker = &markers[marker_idx];
		Assert(marker->outputPlayCursor < sound->secondaryBufferSize);
		Assert(marker->outputWriteCursor < sound->secondaryBufferSize);
		Assert(marker->outputLocation < sound->secondaryBufferSize);
		Assert(marker->outputByteCount < sound->secondaryBufferSize);
		Assert(marker->flipPlayCursor < sound->secondaryBufferSize);
		Assert(marker->flipWriteCursor < sound->secondaryBufferSize);
		
		DWORD play_color = 0xFFFFFFFF;
		DWORD write_color = 0xFFFF0000;
		DWORD expected_flip_color = 0xFFFFFF00;
		DWORD play_window_color = 0xFFFF00FF;
		int top = pad_y;
		int bottom = pad_y + line_height;
		if(marker_idx == current_marker_idx){
			top += line_height + pad_y;
			bottom += line_height + pad_y;
			int first_top = top;
			
			DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,top,bottom, marker->outputPlayCursor,  play_color);
			DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,top,bottom, marker->outputWriteCursor, write_color);
			top += line_height + pad_y;
			bottom += line_height + pad_y;
			
			DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,top,bottom, marker->outputLocation, play_color);
			DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,top,bottom, marker->outputLocation + marker->outputByteCount, write_color);
			top += line_height + pad_y;
			bottom += line_height + pad_y;
			DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,first_top,bottom, marker->expectedFlipPlayCursor, expected_flip_color);
		}
		
		DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,top,bottom, marker->flipPlayCursor,  play_color);
		DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,top,bottom, marker->flipPlayCursor + 480*sound->bytesPerSample,  play_window_color);
		DEBUGWin32DrawSoundBufferMarker(backbuffer,sound, coef, pad_x,top,bottom, marker->flipWriteCursor, write_color);
	}
}

function void
ConcatStrings(upt src1_size, char* src1, upt src2_size, char* src2, upt dest_size, char* dest){
	forI(src1_size) *dest++ = *src1++;
	forI(src2_size) *dest++ = *src2++;
	*dest++ = 0;
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code){
	//// setup directory ////
	char exe_filepath[MAX_PATH];
	DWORD filepath_size = GetModuleFileNameA(0, exe_filepath, sizeof(exe_filepath));
	char* exe_filename = exe_filepath;
	for(char* scan = exe_filepath; *scan != 0; ++scan){
		if(*scan == '\\'){
			exe_filename = scan+1;
		}
	}
	
	char source_game_code_dll_filename[] = "handmade.dll";
	char source_game_code_dll_filepath[MAX_PATH];
	ConcatStrings(exe_filename-exe_filepath, exe_filepath, 
				  sizeof(source_game_code_dll_filename)-1, source_game_code_dll_filename,
				  sizeof(source_game_code_dll_filepath), source_game_code_dll_filepath);
	
	char temp_game_code_dll_filename[] = "handmade_temp.dll";
	char temp_game_code_dll_filepath[MAX_PATH];
	ConcatStrings(exe_filename-exe_filepath, exe_filepath, 
				  sizeof(temp_game_code_dll_filename)-1, temp_game_code_dll_filename,
				  sizeof(temp_game_code_dll_filepath), temp_game_code_dll_filepath);
	
	//// setup timers ////
	LARGE_INTEGER perf_count_frequency_result;
	QueryPerformanceFrequency(&perf_count_frequency_result);
	global_perf_count_frequency = perf_count_frequency_result.QuadPart;
	
#define monitor_refresh_hz 60
#define game_update_hz (monitor_refresh_hz / 2)
	f32 target_seconds_per_frame = 1.0f / (f32)game_update_hz;
	
	//set the Windows scheduler granularity to 1ms so that our Sleep() can be more granular
	UINT desired_scheduler_ms = 1;
	b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);
	
	LARGE_INTEGER last_counter = Win32GetWallClock();
	LARGE_INTEGER flip_wall_clock = Win32GetWallClock();
	u64 last_cycle_count = __rdtsc();
	
	//// init input ////
	Win32LoadXInput();
	
	GameInput input[2] = {};
	GameInput* new_input = &input[0];
	GameInput* old_input = &input[1];
	
	//// init window ////
	Win32ResizeDIBSection(&global_backbuffer, 1280, 720);
	
    WNDCLASSA window_class{};
	window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	window_class.lpfnWndProc = Win32MainWindowCallback;
	window_class.hInstance = instance;
	//window_class.hIcon;
	window_class.lpszClassName = "HandmadeHero Window";
	
	if(!RegisterClassA(&window_class)){ /*TODO error logging */ }
	HWND window = CreateWindowExA(0, window_class.lpszClassName,
								  "Handmade Hero",
								  WS_OVERLAPPEDWINDOW|WS_VISIBLE,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  0, 0, instance, 0);
	if(!window){ Assert(!"Failed to open window :( thanks windows"); }
	
	//NOTE since we specify CS_OWNDC, we can just get one device context 
	//and use it forever b/c we dont need to share it with anyone
	HDC device_context = GetDC(window);
	
	//// init sound ////
	Win32SoundOutput sound_output{};
	sound_output.samplesPerSecond    = 48000;
	sound_output.bytesPerSample      = 2*sizeof(s16);
	sound_output.secondaryBufferSize = sound_output.samplesPerSecond * sound_output.bytesPerSample;
	sound_output.latencySampleCount  = 3*(sound_output.samplesPerSecond / game_update_hz);
	sound_output.safetyBytes          = ((sound_output.samplesPerSecond * sound_output.bytesPerSample) / game_update_hz) / 3;
	
	Win32InitDSound(window, sound_output.samplesPerSecond, sound_output.secondaryBufferSize);
	Win32ClearSoundBuffer(&sound_output);
	global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
	
	DWORD audio_latency_bytes = 0;
	f32 audio_latency_seconds = 0;
	b32 sound_is_valid = false;
	
	int DEBUG_marker_idx = 0;
	DEBUGWin32TimeMarker DEBUG_time_markers[game_update_hz/2] = {};
	
#if 0
	//this tests the play_cursor/write_cursor update frequency
	while(true){
		DWORD play_cursor;
		DWORD write_cursor;
		global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor);
		
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "PC:%u WC:%u\n", play_cursor, write_cursor);
		OutputDebugStringA(buffer);
	}
#endif
	
	//// init memory ////
	s16* samples = (s16*)VirtualAlloc(0, sound_output.secondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	
#if HANDMADE_INTERNAL
	LPVOID base_address = (LPVOID)Terabytes(2);
#else
	LPVOID base_address = 0;
#endif //HANDMADE_INTERNAL
	
	GameMemory game_memory{};
	game_memory.permanentStorageSize = Megabytes(256);
	game_memory.transientStorageSize = Gigabytes(1);
	u64 total_size = game_memory.permanentStorageSize + game_memory.transientStorageSize;
	game_memory.permanentStorage = VirtualAlloc(base_address, (size_t)total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	game_memory.transientStorage = (u8*)game_memory.permanentStorage + game_memory.permanentStorageSize;
	game_memory.DEBUGPlatformFreeFileMemory  = DEBUGPlatformFreeFileMemory;
	game_memory.DEBUGPlatformReadEntireFile  = DEBUGPlatformReadEntireFile;
	game_memory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
	
	if(!samples || !game_memory.permanentStorage || !game_memory.transientStorage){ /*TODO error logging */ }
	
	//// start windows loop ////
	Win32GameCode game = Win32LoadGameCode(source_game_code_dll_filepath, temp_game_code_dll_filepath);
	last_counter = Win32GetWallClock();
	flip_wall_clock = Win32GetWallClock();
	last_cycle_count = __rdtsc();
	global_running = true;
	while(global_running){
		FILETIME new_dll_write_time = Win32GetLastWriteTime(source_game_code_dll_filepath);
		if(CompareFileTime(&new_dll_write_time, &game.dllLastWriteTime) != 0){
			Win32UnloadGameCode(&game);
			game = Win32LoadGameCode(source_game_code_dll_filepath, temp_game_code_dll_filepath);
		}
		
		GameControllerInput* old_keyboard_controller = GetController(old_input, 0);
		GameControllerInput* new_keyboard_controller = GetController(new_input, 0);
		*new_keyboard_controller = {};
		new_keyboard_controller->connected = true;
		forX(button_idx, ArrayCount(new_keyboard_controller->buttons)){
			new_keyboard_controller->buttons[button_idx].endedDown = old_keyboard_controller->buttons[button_idx].endedDown;
		}
		
		//handle a message when there is one, keep going otherwise
		Win32ProcessPendingMessages(new_keyboard_controller);
		
		if(!global_pause){
			//get input from XInput
			DWORD max_controller_count = XUSER_MAX_COUNT;
			//one extra b/c keyboard
			if(max_controller_count > (ArrayCount(new_input->controllers) - 1)) {
				max_controller_count = (ArrayCount(new_input->controllers) - 1);
			}
			
			for(DWORD controller_idx = 0; controller_idx < max_controller_count; ++controller_idx){
				DWORD our_controller_idx = controller_idx + 1;
				GameControllerInput* old_controller = GetController(old_input, our_controller_idx);
				GameControllerInput* new_controller = GetController(new_input, our_controller_idx);
				
				XINPUT_STATE controller_state;
				if(XInputGetState(controller_idx, &controller_state) == ERROR_SUCCESS){
					//controller plugged in
					XINPUT_GAMEPAD* pad = &controller_state.Gamepad;
					new_controller->connected = true;
					
					//sticks as analogs
					new_controller->leftStickAverageX = Win32ProcessXInputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
					new_controller->leftStickAverageY = Win32ProcessXInputStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
					if(new_controller->leftStickAverageX != 0.0f || new_controller->leftStickAverageY != 0.0f){
						new_controller->analog = true;
					}
					
					//dpad (currently overrides left stick analogs)
					if(pad->wButtons & XINPUT_GAMEPAD_DPAD_UP){
						new_controller->leftStickAverageY = 1.f;
						new_controller->analog = false;
					}
					if(pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN){
						new_controller->leftStickAverageY = -1.f;
						new_controller->analog = false;
					}
					if(pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT){
						new_controller->leftStickAverageX = 1.f;
						new_controller->analog = false;
					}
					if(pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT){
						new_controller->leftStickAverageX = -1.f;
						new_controller->analog = false;
					}
					
					//sticks as buttons
					Win32ProcessXInputDigitalButton((new_controller->leftStickAverageX < -0.5f) ? 1 : 0,
													&old_controller->leftStickLeft, &new_controller->leftStickLeft, 1);
					Win32ProcessXInputDigitalButton((new_controller->leftStickAverageX > 0.5f) ? 1 : 0,
													&old_controller->leftStickRight, &new_controller->leftStickRight, 1);
					Win32ProcessXInputDigitalButton((new_controller->leftStickAverageY < -0.5f) ? 1 : 0,
													&old_controller->leftStickUp, &new_controller->leftStickUp, 1);
					Win32ProcessXInputDigitalButton((new_controller->leftStickAverageY > 0.5f) ? 1 : 0,
													&old_controller->leftStickDown, &new_controller->leftStickDown, 1);
					
					//buttons
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->buttonDown, &new_controller->buttonDown, XINPUT_GAMEPAD_A);
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->buttonRight, &new_controller->buttonRight, XINPUT_GAMEPAD_B);
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->buttonLeft, &new_controller->buttonLeft, XINPUT_GAMEPAD_X);
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->buttonUp, &new_controller->buttonUp, XINPUT_GAMEPAD_Y);
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->shoulderLeft, &new_controller->shoulderLeft, XINPUT_GAMEPAD_LEFT_SHOULDER);
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->shoulderRight, &new_controller->shoulderRight, XINPUT_GAMEPAD_RIGHT_SHOULDER);
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->buttonStart, &new_controller->buttonStart, XINPUT_GAMEPAD_START);
					Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->buttonBack, &new_controller->buttonBack, XINPUT_GAMEPAD_BACK);
					
					//controller rumble
					//XINPUT_VIBRATION vibration;
					//vibration.wLeftMotorSpeed = 60000;
					//vibration.wRightMotorSpeed = 60000;
					//XInputSetState(controller_idx, &vibration);
				}else{
					//controller not available
					new_controller->connected = false;
				}
			}
			
			//create render buffer for game
			GameOffscreenBuffer render_buffer{};
			render_buffer.memory = global_backbuffer.memory;
			render_buffer.width  = global_backbuffer.width;
			render_buffer.height = global_backbuffer.height;
			render_buffer.pitch  = global_backbuffer.pitch;
			
			//run the game tick
			game.UpdateAndRender(&game_memory, new_input, &render_buffer);
			
			LARGE_INTEGER audio_wall_clock = Win32GetWallClock();
			f32 from_being_to_audio_seconds = 1000.0f*Win32GetSecondsElapsed(flip_wall_clock, audio_wall_clock);
			
			//fill the sound buffer for playing
			DWORD play_cursor;
			DWORD write_cursor;
			if(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor) == DS_OK){
				/* NOTE(casey): Here is how sound output computation works.
					We define a safety value that is the number of samples we think our game update loop 
					may vary by (let's say up to 2ms)
			  
					When we wake up to write audio, we will look and see what the play cursor position is 
					and we will forecast ahead where we think the play cursor will be on the next frame boundary.
	
					We will then look to see if the write cursor is before that by at least our safety value. 
					If it is, the target fill position is that frame boundary plus one frame. This gives us 
					perfect audio sync in the case of a card that has low enough latency.
	
					If the write cursor is _after_ that safety margin, then we assume we can never sync the audio 
					perfectly, so we will write one frame's worth of audio plus the safety margin's worth of guard samples.
				*/
				
				if(!sound_is_valid){
					sound_output.runningSampleIndex = write_cursor / sound_output.bytesPerSample;
					sound_is_valid = true;
				}
				
				DWORD expected_sound_bytes_per_frame = (sound_output.samplesPerSecond * sound_output.bytesPerSample) / game_update_hz;
				f32 seconds_left_until_flip = target_seconds_per_frame - from_being_to_audio_seconds;
				DWORD expected_bytes_until_flip = (DWORD)((seconds_left_until_flip / target_seconds_per_frame) * (f32)expected_sound_bytes_per_frame);
				DWORD expected_frame_boundary_byte = play_cursor + expected_sound_bytes_per_frame;
				
				DWORD safe_write_cursor = write_cursor;
				if(safe_write_cursor < play_cursor){
					safe_write_cursor += sound_output.secondaryBufferSize;
				}
				Assert(safe_write_cursor >= play_cursor);
				safe_write_cursor += sound_output.safetyBytes;
				
				DWORD target_cursor = 0;
				b32 audio_card_is_low_latency = (safe_write_cursor < expected_frame_boundary_byte);
				if(audio_card_is_low_latency){
					target_cursor = expected_frame_boundary_byte + expected_sound_bytes_per_frame;
				}else{
					target_cursor = write_cursor + expected_sound_bytes_per_frame + sound_output.safetyBytes;
				}
				target_cursor %= sound_output.secondaryBufferSize;
				
				DWORD byte_to_lock = (sound_output.runningSampleIndex * sound_output.bytesPerSample) % sound_output.secondaryBufferSize;
				DWORD bytes_to_write = 0;
				if(byte_to_lock > target_cursor){
					bytes_to_write = sound_output.secondaryBufferSize - byte_to_lock;
					bytes_to_write += target_cursor;
				}else{
					bytes_to_write = target_cursor - byte_to_lock;
				}
				
				//create sound buffer for game
				GameSoundOutputBuffer sound_buffer{};
				sound_buffer.samplesPerSecond = sound_output.samplesPerSecond;
				sound_buffer.sampleCount      = bytes_to_write / sound_output.bytesPerSample;
				sound_buffer.samples          = samples;
				game.GetSoundSamples(&game_memory, &sound_buffer);
				
#if HANDMADE_INTERNAL
				DEBUGWin32TimeMarker* marker = &DEBUG_time_markers[DEBUG_marker_idx];
				marker->outputPlayCursor        = play_cursor;
				marker->outputWriteCursor       = write_cursor;
				marker->outputLocation           = byte_to_lock;
				marker->outputByteCount         = bytes_to_write;
				marker->expectedFlipPlayCursor = expected_frame_boundary_byte;
				
				DWORD unwrapped_write_cursor = write_cursor;
				if(unwrapped_write_cursor < play_cursor) unwrapped_write_cursor += sound_output.secondaryBufferSize;
				audio_latency_bytes = unwrapped_write_cursor - play_cursor;
				audio_latency_seconds = ((f32)audio_latency_bytes / (f32)sound_output.bytesPerSample) / (f32)sound_output.samplesPerSecond;
				
				char buffer[256];
				sprintf_s(buffer, sizeof(buffer), "BTL:%6u TC:%6u BTW:%u - PC:%6u WC:%6u DELTA:%u (%fms)\n", 
						  byte_to_lock, target_cursor, bytes_to_write, 
						  play_cursor, write_cursor, audio_latency_bytes, audio_latency_seconds*1000.f);
				OutputDebugStringA(buffer);
#endif //HANDMADE_INTERNAL
				Win32FillSoundBuffer(&sound_output, &sound_buffer, byte_to_lock, bytes_to_write);
			}else{
				sound_is_valid = false;
			}
			
			//update timers and clock counters
			LARGE_INTEGER work_counter = Win32GetWallClock();
			f32 work_seconds_elapsed = Win32GetSecondsElapsed(last_counter, work_counter);
			
			f32 frame_seconds_elapsed = work_seconds_elapsed;
			if(frame_seconds_elapsed < target_seconds_per_frame){
				if(sleep_is_granular){
					DWORD sleep_ms = (DWORD)(1000.0f*(target_seconds_per_frame - frame_seconds_elapsed));
					if(sleep_ms) Sleep(sleep_ms);
				}
				
				f32 test_frame_seconds_elapsed = Win32GetSecondsElapsed(last_counter, Win32GetWallClock());
				if(test_frame_seconds_elapsed < target_seconds_per_frame){
					//TODO log missed sleep here
				}
				
				while(frame_seconds_elapsed < target_seconds_per_frame){
					frame_seconds_elapsed = Win32GetSecondsElapsed(last_counter, Win32GetWallClock());
				}
			}else{
				OutputDebugStringA("Missed target framerate of 33.3ms!\n");
			}
			
			LARGE_INTEGER end_counter = Win32GetWallClock();
			f32 mspf = 1000.0f * Win32GetSecondsElapsed(last_counter, end_counter);
			last_counter = end_counter;
			
			u64 end_cycle_count = __rdtsc();
			u64 cycles_elapsed = end_cycle_count - last_cycle_count;
			last_cycle_count = end_cycle_count;
			
			//display render buffer
			Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
#if HANDMADE_INTERNAL
			DEBUGWin32SyncDisplay(&global_backbuffer, &sound_output, DEBUG_time_markers, 
								  ArrayCount(DEBUG_time_markers), DEBUG_marker_idx-1, target_seconds_per_frame);
#endif //HANDMADE_INTERNAL
			Win32DisplayBufferInWindow(&global_backbuffer, device_context, dimensions.width, dimensions.height);
			
			flip_wall_clock = Win32GetWallClock();
			
#if HANDMADE_INTERNAL
			if(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor) == DS_OK){
				Assert(DEBUG_marker_idx < ArrayCount(DEBUG_time_markers));
				DEBUGWin32TimeMarker* marker = &DEBUG_time_markers[DEBUG_marker_idx];
				marker->flipPlayCursor  = play_cursor;
				marker->flipWriteCursor = write_cursor;
			}
#endif //HANDMADE_INTERNAL
			
			//reset game input
			GameInput* temp = new_input;
			new_input = old_input;
			old_input = temp;
			//Swap(new_input, old_input);
			//TODO clear these here?
			
			//@Debug
			//print timing info to Output
			f32 fps  = 0.0f;
			f32 mcpf = (f32)cycles_elapsed / (1000.0f*1000.0f);
			
			char fps_buffer[256];
			sprintf_s(fps_buffer, sizeof(fps_buffer), "%.2fmspf, %.2ffps, %.2fmcpf\n", mspf, fps, mcpf);
			OutputDebugStringA(fps_buffer);
			
#if HANDMADE_INTERNAL
			if(++DEBUG_marker_idx == ArrayCount(DEBUG_time_markers)){
				DEBUG_marker_idx = 0;
			}
#endif //HANDMADE_INTERNAL
		}
	}
	//// end windows loop ////
	return 0;
}
