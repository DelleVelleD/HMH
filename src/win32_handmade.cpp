//windows entry point
#include "defines.h"

#include <math.h>
#include "handmade.cpp"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

//TODO temporary globals
global_variable b32 global_running;
global_variable Win32OffscreenBuffer global_backbuffer;
global_variable LPDIRECTSOUNDBUFFER global_secondary_buffer;

//XInputGetState, this initializes to a stub pointer
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(x_input_get_state);
XINPUT_GET_STATE(XInputGetStateStub){return ERROR_DEVICE_NOT_CONNECTED;}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//XInputSetState
#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pState)
typedef XINPUT_SET_STATE(x_input_set_state);
XINPUT_SET_STATE(XInputSetStateStub){return ERROR_DEVICE_NOT_CONNECTED;}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

//DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) DWORD WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static_internal DEBUGReadFileResult
DEBUGPlatformReadEntireFile(char* filename){
	DEBUGReadFileResult result{};
	
	HANDLE file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(file_handle != INVALID_HANDLE_VALUE){
		LARGE_INTEGER file_size;
		if(GetFileSizeEx(file_handle, &file_size)){
			u32 file_size_32 = SafeTruncateU64(file_size.QuadPart); //NOTE ReadFile only takes up to a u32 value
			result.memory = VirtualAlloc(0, file_size.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(result.memory){
				DWORD bytes_read;
				if(ReadFile(file_handle, result.memory, file_size_32, &bytes_read, 0) &&
				   file_size_32 == bytes_read){
					//NOTE file read successfully
					result.memory_size = file_size_32;
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

static_internal void 
DEBUGPlatformFreeFileMemory(void* memory){
	if(memory){
		VirtualFree(memory, 0, MEM_RELEASE);
		memory = 0;
	}
}

static_internal b32 
DEBUGPlatformWriteEntireFile(char* filename, void* memory, u32 memory_size){
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

static_internal void
Win32LoadXInput(){
	HMODULE xinput_library = LoadLibraryA("xinput1_4.dll"); //TODO diagnostic
	if(!xinput_library) xinput_library = LoadLibraryA("xinput9_1_0.dll"); //TODO diagnostic
	if(!xinput_library) xinput_library = LoadLibraryA("xinput1_3.dll"); //TODO diagnostic
	if(xinput_library){
		XInputGetState = (x_input_get_state*)GetProcAddress(xinput_library, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(xinput_library, "XInputSetState");
		//TODO diagnostic
	}else{ /*TODO diagnostic*/ }
}

static_internal void
Win32InitDSound(HWND Window, s32 samples_per_second, s32 buffer_size){
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
		wave_format.nSamplesPerSec  = samples_per_second;
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
		buffer_desc.dwFlags       = 0;
		buffer_desc.dwBufferBytes = buffer_size;
		buffer_desc.lpwfxFormat   = &wave_format;
		
		result = direct_sound->CreateSoundBuffer(&buffer_desc, &global_secondary_buffer, 0);
		if(SUCCEEDED(result)){
			OutputDebugStringA("Secondary buffer created successfully\n");
		}else{ /*TODO diagnostic*/ }
		
		//start it playing
		
	}else{ /*TODO diagnostic*/ }
}

static_internal Win32WindowDimensions
Win32GetWindowDimensions(HWND window){
	Win32WindowDimensions result;
	
	RECT client_rect;
	GetClientRect(window, &client_rect);
	result.height = client_rect.bottom - client_rect.top;
	result.width = client_rect.right - client_rect.left;
	
	return result;
}

static_internal void
Win32ResizeDIBSection(Win32OffscreenBuffer* buffer, int width, int height){
	//free memory if it exists
	if(buffer->memory){
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}
	
	buffer->width = width;
	buffer->height = height;
	int bytes_per_pixel = 4;
	
	//setup bitmap info
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height; //negative to make it top-down
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;
	
	//allocate memory
	int bitmap_memory_size = bytes_per_pixel * width * height;
	buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = buffer->width * bytes_per_pixel;
}

static_internal void
Win32DisplayBufferInWindow(Win32OffscreenBuffer* buffer, HDC device_context, int window_width, int window_height){
	StretchDIBits(device_context, 
				  0, 0, window_width, window_height,
				  0, 0, buffer->width, buffer->height,
				  buffer->memory,
				  &buffer->info,
				  DIB_RGB_COLORS, SRCCOPY);
}

static_internal LRESULT CALLBACK
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
			Assert(!"Keyboard input came in through a non-dispatch message");
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

static_internal void
Win32ClearSoundBuffer(Win32SoundOutput* sound_output){
	void* region1; void* region2;
	DWORD region1_size, region2_size;
	if(SUCCEEDED(global_secondary_buffer->Lock(0, sound_output->secondary_buffer_size, &region1, &region1_size, &region2, &region2_size, 0))){
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

static_internal void 
Win32FillSoundBuffer(Win32SoundOutput* sound_output, GameSoundOutputBuffer* source_buffer, DWORD byte_to_lock, DWORD bytes_to_write){
	void* region1; void* region2;
	DWORD region1_size, region2_size;
	if(SUCCEEDED(global_secondary_buffer->Lock(byte_to_lock, bytes_to_write, &region1, &region1_size, &region2, &region2_size, 0))){
		//TODO assert region1_size/region2_size is valid
		DWORD region1_sample_count = region1_size/sound_output->bytes_per_sample;
		s16* dst_sample = (s16*)region1;
		s16* src_sample = source_buffer->samples;
		for(DWORD sample_idx = 0; sample_idx < region1_sample_count; ++sample_idx){
			*dst_sample++ = *src_sample++;
			*dst_sample++ = *src_sample++;
			++sound_output->running_sample_index;
		}
		
		DWORD region2_sample_count = region2_size/sound_output->bytes_per_sample;
		dst_sample = (s16*)region2;
		for(DWORD sample_idx = 0; sample_idx < region2_sample_count; ++sample_idx){
			*dst_sample++ = *src_sample++;
			*dst_sample++ = *src_sample++;
			++sound_output->running_sample_index;
		}
		
		global_secondary_buffer->Unlock(region1, region1_size, region2, region2_size);
	}
}

static_internal void
Win32ProcessXInputDigitalButton(DWORD xinput_button_state, GameButtonState* old_state, GameButtonState* new_state, DWORD button_bit){
	new_state->ended_down = (xinput_button_state & button_bit);
	new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;
}

static_internal void
Win32ProcessKeyboardMessage(GameButtonState* new_state, b32 is_down){
	new_state->ended_down = is_down;
	new_state->half_transition_count += 1;
}

static_internal void
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
						
					}else if(VKCode == 'A'){
						
					}else if(VKCode == 'S'){
						
					}else if(VKCode == 'D'){
						
					}else if(VKCode == 'Q'){
						Win32ProcessKeyboardMessage(&keyboard_controller->shoulder_left, is_down);
					}else if(VKCode == 'E'){
						Win32ProcessKeyboardMessage(&keyboard_controller->shoulder_right, is_down);
					}else if(VKCode == VK_UP){
						Win32ProcessKeyboardMessage(&keyboard_controller->button_up, is_down);
					}else if(VKCode == VK_LEFT){
						Win32ProcessKeyboardMessage(&keyboard_controller->button_left, is_down);
					}else if(VKCode == VK_DOWN){
						Win32ProcessKeyboardMessage(&keyboard_controller->button_down, is_down);
					}else if(VKCode == VK_RIGHT){
						Win32ProcessKeyboardMessage(&keyboard_controller->button_right, is_down);
					}else if(VKCode == VK_ESCAPE){
						Win32ProcessKeyboardMessage(&keyboard_controller->button_start, is_down);
					}else if(VKCode == VK_SPACE){
						
					}
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

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_code){
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
	if(!window){ /*TODO error logging */ }
	
	//NOTE since we specify CS_OWNDC, we can just get one device context 
	//and use it forever b/c we dont need to share it with anyone
	HDC device_context = GetDC(window);
	
	//// init sound ////
	Win32SoundOutput sound_output{};
	sound_output.samples_per_second = 48000;
	sound_output.bytes_per_sample = sizeof(s16)*2;
	sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
	sound_output.latency_sample_count = sound_output.samples_per_second;
	
	Win32InitDSound(window, sound_output.samples_per_second, sound_output.secondary_buffer_size);
	Win32ClearSoundBuffer(&sound_output);
	global_secondary_buffer->Play(0, 0, DSBPLAY_LOOPING);
	
	//// init memory ////
	s16* samples = (s16*)VirtualAlloc(0, sound_output.secondary_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	
#if HANDMADE_INTERNAL
	LPVOID base_address = (LPVOID)Terabytes(2);
#else
	LPVOID base_address = 0;
#endif //HANDMADE_INTERNAL
	
	GameMemory game_memory{};
	game_memory.permanent_storage_size = Megabytes(64);
	game_memory.transient_storage_size = Gigabytes(1);
	u64 total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
	
	game_memory.permanent_storage = VirtualAlloc(base_address, (size_t)total_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	game_memory.transient_storage = (u8*)game_memory.permanent_storage + game_memory.permanent_storage_size;
	
	if(!samples || !game_memory.permanent_storage || !game_memory.transient_storage){ /*TODO error logging */ }
	
	//// init timers ////
	LARGE_INTEGER perf_count_frequency_result;
	QueryPerformanceFrequency(&perf_count_frequency_result);
	s64 perf_count_frequency = perf_count_frequency_result.QuadPart;
	
	u64 last_cycle_count = __rdtsc();
	LARGE_INTEGER last_counter;
	QueryPerformanceCounter(&last_counter);
	
	//// start windows loop ////
	global_running = true;
	while(global_running){
		GameControllerInput* keyboard_controller = &new_input->controllers[0];
		*keyboard_controller = {};
		
		//handle a message when there is one, keep going otherwise
		Win32ProcessPendingMessages(keyboard_controller);
		
		//get input from XInput
		DWORD max_controller_count = XUSER_MAX_COUNT;
		if(max_controller_count > ArrayCount(new_input->controllers)) max_controller_count = ArrayCount(new_input->controllers);
		
		for(DWORD controller_idx = 0; controller_idx < max_controller_count; ++controller_idx){
			GameControllerInput* old_controller = &old_input->controllers[controller_idx];
			GameControllerInput* new_controller = &new_input->controllers[controller_idx];
			
			XINPUT_STATE controller_state;
			if(XInputGetState(controller_idx, &controller_state) == ERROR_SUCCESS){
				//controller plugged in
				XINPUT_GAMEPAD* pad = &controller_state.Gamepad;
				
				//dpad
				b32 up    = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
				b32 down  = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
				b32 left  = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
				b32 right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
				
				new_controller->analog = true;
				{//left stick
					new_controller->stick_left.start_x = old_controller->stick_left.start_x;
					new_controller->stick_left.start_y = old_controller->stick_left.start_y;
					f32 x;
					if(pad->sThumbLX < 0){
						x = (f32)pad->sThumbLX / 32768.f;
					}else{
						x = (f32)pad->sThumbLX / 32767.f;
					}
					f32 y;
					if(pad->sThumbLY < 0){
						y = (f32)pad->sThumbLY / 32768.f;
					}else{
						y = (f32)pad->sThumbLY / 32767.f;
					}
					new_controller->stick_left.min_x = new_controller->stick_left.max_x = new_controller->stick_left.end_x = x;
					new_controller->stick_left.min_y = new_controller->stick_left.max_y = new_controller->stick_left.end_y = y;
				}
				
				//buttons
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->button_down, &new_controller->button_down, XINPUT_GAMEPAD_A);
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->button_right, &new_controller->button_right, XINPUT_GAMEPAD_B);
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->button_left, &new_controller->button_left, XINPUT_GAMEPAD_X);
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->button_up, &new_controller->button_up, XINPUT_GAMEPAD_Y);
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->shoulder_left, &new_controller->shoulder_left, XINPUT_GAMEPAD_LEFT_SHOULDER);
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->shoulder_right, &new_controller->shoulder_right, XINPUT_GAMEPAD_RIGHT_SHOULDER);
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->button_start, &new_controller->button_start, XINPUT_GAMEPAD_START);
				Win32ProcessXInputDigitalButton(pad->wButtons, &old_controller->button_select, &new_controller->button_select, XINPUT_GAMEPAD_BACK);
				
				//controller rumble
				//XINPUT_VIBRATION vibration;
				//vibration.wLeftMotorSpeed = 60000;
				//vibration.wRightMotorSpeed = 60000;
				//XInputSetState(controller_idx, &vibration);
			}else{
				//controller not available
			}
		}
		
		//update the sound information
		DWORD byte_to_lock = 0; 
		DWORD bytes_to_write = 0;
		DWORD play_cursor = 0;
		DWORD write_cursor = 0;
		DWORD target_cursor = 0;
		b32 sound_is_valid = false;
		if(SUCCEEDED(global_secondary_buffer->GetCurrentPosition(&play_cursor, &write_cursor))){
			byte_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
			target_cursor = (play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.secondary_buffer_size;
			if(byte_to_lock > play_cursor){
				bytes_to_write = sound_output.secondary_buffer_size - byte_to_lock;
				bytes_to_write += target_cursor;
			}else{
				bytes_to_write = target_cursor - byte_to_lock;
			}
			
			sound_is_valid = true;
		}
		
		//create render buffer for game
		GameOffscreenBuffer render_buffer{};
		render_buffer.memory = global_backbuffer.memory;
		render_buffer.width  = global_backbuffer.width;
		render_buffer.height = global_backbuffer.height;
		render_buffer.pitch  = global_backbuffer.pitch;
		
		//create sound buffer for game
		GameSoundOutputBuffer sound_buffer{};
		sound_buffer.samples_per_second = sound_output.samples_per_second;
		sound_buffer.sample_count       = bytes_to_write / sound_output.bytes_per_sample;
		sound_buffer.samples            = samples;
		
		//run the game tick
		GameUpdateAndRender(&game_memory, new_input, &render_buffer, &sound_buffer);
		
		//fill the sound buffer for playing
		if(sound_is_valid){
			Win32FillSoundBuffer(&sound_output, &sound_buffer, byte_to_lock, bytes_to_write);
		}
		
		//display render buffer
		Win32WindowDimensions dimensions = Win32GetWindowDimensions(window);
		Win32DisplayBufferInWindow(&global_backbuffer, device_context, dimensions.width, dimensions.height);
		
		//update timers and clock counters
		u64 end_cycle_count = __rdtsc();
		LARGE_INTEGER end_counter;
		QueryPerformanceCounter(&end_counter);
		
		u64 cycles_elapsed = end_cycle_count - last_cycle_count;
		s64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
		f64 mspf = (1000.0*(f64)counter_elapsed) / (f64)perf_count_frequency;
		f64 fps = (f64)perf_count_frequency / (f64)counter_elapsed;
		f64 mcpf = (f64)cycles_elapsed / (1000.0*1000.0);
		
#if 0
		//print timing info to Output
		char buffer[256];
		sprintf(buffer, "%.2fmspf, %.2ffps, %.2fmcpf\n", mspf, fps, mcpf);
		OutputDebugStringA(buffer);
#endif
		
		last_counter = end_counter;
		last_cycle_count = end_cycle_count;
		
		//reset game input
		GameInput* temp = new_input;
		new_input = old_input;
		old_input = temp;
		//Swap(new_input, old_input);
		//TODO clear these here?
	}
	return 0;
}

