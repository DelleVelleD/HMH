//windows entry point

#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static 
#define global_variable static


typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer{
	BITMAPINFO info;
	void* memory;
	int width;
	int height;
	int pitch; //possible row offset
	int bytesPerPixel;
};

//TODO temporary globals
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimensions{
	int width;
	int height;
};

//XInputGetState, this initializes to a stub pointer
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub){return 0;}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pState)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub){return 0;}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(){
	HMODULE XInputLibrary =  LoadLibraryA("xinput1_3.dll");
	if(XInputLibrary){
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal win32_window_dimensions
Win32GetWindowDimensions(HWND Window){
	win32_window_dimensions result;
	
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	result.height = ClientRect.bottom - ClientRect.top;
	result.width = ClientRect.right - ClientRect.left;
	
	return result;
}

internal void
RenderGradient(win32_offscreen_buffer* buffer, int xOffset, int yOffset){
	uint8* row = (uint8*)buffer->memory;
	for(int y = 0; y < buffer->height; ++y){
		uint32* pixel = (uint32*)row;
		for(int x = 0; x < buffer->width; ++x){
			//b,g,r,pad     because windows
			uint8 b = (x + xOffset);
			uint8 g = (y + yOffset);
			//uint8 r = ;
			//uint8 p = ;
			
			*pixel++ = (/*((r << 16) |*/ (g << 8) | b); //shift red and green, increment the pointer
		}
		row += buffer->pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer* buffer, int width, int height){
	//free memory if it exists
	if(buffer->memory){
		VirtualFree(buffer->memory, 0, MEM_RELEASE);
	}
	
	buffer->width = width;
	buffer->height = height;
	buffer->bytesPerPixel = 4;
	
	//setup bitmap info
	buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
	buffer->info.bmiHeader.biWidth = buffer->width;
	buffer->info.bmiHeader.biHeight = -buffer->height; //negative to make it top-down
	buffer->info.bmiHeader.biPlanes = 1;
	buffer->info.bmiHeader.biBitCount = 32;
	buffer->info.bmiHeader.biCompression = BI_RGB;
	
	//allocate memory
	int BitmapMemorySize = buffer->bytesPerPixel * width * height;
	buffer->memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	buffer->pitch = buffer->width * buffer->bytesPerPixel;
	
	//draw the pixels
	//RenderGradient(0, 0); //TODO remove this possibly
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer* buffer, HDC DeviceContext,
						   int windowWidth, int windowHeight){
	StretchDIBits(DeviceContext, 
				  /*
				  x, y, width, height,
					  x, y, width, height,*/
				  0, 0, windowWidth, windowHeight,
				  0, 0, buffer->width, buffer->height,
				  buffer->memory,
				  &buffer->info,
				  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
						UINT Message,
						WPARAM wParam,
						LPARAM lParam){
	LRESULT Result = 0;
	
	switch(Message){
		case WM_DESTROY:{
			//TODO handle with a message to the user?
			GlobalRunning = false;
		} break;
		
		case WM_CLOSE:{
			//TODO handle this as an error - recreate window?
			GlobalRunning = false;
		} break;
		
		case WM_ACTIVATEAPP:{
			OutputDebugStringA("WM_ACTIVEAPP\n");
		} break;
		
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:{
			uint32 VKCode = wParam;
			bool wasDown = ((lParam & (1 << 30)) != 0);
			bool isDown = ((lParam & (1 << 31)) == 0);
			if(wasDown != isDown){
				if(VKCode == 'W'){
					
				}else if(VKCode == 'A'){
					
				}else if(VKCode == 'S'){
					
				}else if(VKCode == 'D'){
					
				}else if(VKCode == 'Q'){
					
				}else if(VKCode == 'E'){
					
				}else if(VKCode == VK_UP){
					
				}else if(VKCode == VK_LEFT){
					
				}else if(VKCode == VK_DOWN){
					
				}else if(VKCode == VK_RIGHT){
					
				}else if(VKCode == VK_ESCAPE){
					
				}else if(VKCode == VK_SPACE){
					
				}
			}
		}break;
		
		case WM_PAINT:{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			
			win32_window_dimensions dimensions = Win32GetWindowDimensions(Window);
			Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, 
									   dimensions.width, dimensions.height);
			EndPaint(Window, &Paint);
		} break;
		
		default:{
			//OutputDebugStringA("default\n");
			Result = DefWindowProc(Window, Message, wParam, lParam);
		} break;
	}
	
	return(Result);
}

internal int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode){
	Win32LoadXInput();
	
    WNDCLASSA WindowClass = {};
	
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
	
	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	//WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";
	
	if(RegisterClassA(&WindowClass)){
		HWND Window = CreateWindowExA(0, WindowClass.lpszClassName,
									  "HandmadeHeroWindowClass",
									  WS_OVERLAPPEDWINDOW|WS_VISIBLE,
									  CW_USEDEFAULT, CW_USEDEFAULT,
									  CW_USEDEFAULT, CW_USEDEFAULT,
									  0, 0, Instance, 0);
		//get a message when there is one, keep going otherwise
		if(Window){
			GlobalRunning = true;
			int xOffset = 0;
			int yOffset = 0;
			
			while(GlobalRunning){
				MSG Message;
				
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
					if(Message.message == WM_QUIT){
						GlobalRunning = false;
					}
					
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				//input
				for(DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex){
					XINPUT_STATE controllerState;
					if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS){
						//controller plugged in
						XINPUT_GAMEPAD* pad = &controllerState.Gamepad;
						
						bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
						bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool lShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool rShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
						bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
						bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
						bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);
						
						int16 stickX = pad->sThumbLX; 
						int16 stickY = pad->sThumbLY; 
						
						xOffset += stickX >> 12;
						yOffset += stickY >> 12;
					}
				}else{
					//contoller not available
					
				}
			}
			//controller rumble
			//XINPUT_VIBRATION vibration;
			//vibration.wLeftMotorSpeed = 60000;
			//vibration.wRightMotorSpeed = 60000;
			//XInputSetState(0, &vibration);
			
			
			
			//render
			RenderGradient(&GlobalBackbuffer, xOffset, yOffset);
			HDC DeviceContext = GetDC(Window);
			//draw window
			win32_window_dimensions dimensions = Win32GetWindowDimensions(Window);
			Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, 
									   dimensions.width, dimensions.height);
			ReleaseDC(Window, DeviceContext);
		}
	}else{
		//TODO logging 
	}
}else{
	//TODO logging 
}
return 0;
}

