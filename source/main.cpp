#include <windows.h>
#include <stdint.h>
#include <xinput.h>
// https://www.youtube.com/watch?v=qGC3xiliJW8

static auto Global_GameRunning = true;

struct win32_bitmap_buffer {
  // pixels are alywas 32 bit, memory order BB GG RR XX (padding)
  BITMAPINFO info;
  void* memory;
  int width;
  int height;
  int pitch;
  int bytes_per_pixel;
};

struct win32_window_dimensions {
  int width;
  int height;
};

static win32_bitmap_buffer Global_backbuffer;

static HBITMAP Global_BitmapHandle;
static HDC Global_BitmapDeviceContext;

// making sure that if we don't have links to functions we don't crash because we use stubs
typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
static x_input_get_state* XInputGetState_;
static x_input_set_state* XInputSetState_;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

static bool win32_load_xinput() {
  // looks locally, looks in windows
  auto xinput_lib = LoadLibraryA("xinput1_3.dll");

  if (xinput_lib) {
    XInputGetState = (x_input_get_state*)GetProcAddress(xinput_lib, "XInputGetState");
    XInputSetState = (x_input_set_state*)GetProcAddress(xinput_lib, "XInputSetState");

    if (!XInputGetState || !XInputSetState)
      return false;
  } else {
    return false;
  }

  return true;
}

static win32_window_dimensions get_window_dimensions(HWND window) {

  win32_window_dimensions result;
  
  RECT clientRect;
  GetClientRect(window, &clientRect);

  result.height = clientRect.bottom - clientRect.top;
  result.width = clientRect.right - clientRect.left;

  return result;
}

static void render_255_gradient(win32_bitmap_buffer* bitmap_buffer, int blue_offset, int green_offset) {

  auto row = (uint8_t*)bitmap_buffer->memory;

  for (int y = 0; y < bitmap_buffer->height; y++) {
    auto pixel = (uint32_t*)row;
    for (int x = 0; x < bitmap_buffer->width; x++) {
      // pixel bytes	   1  2	 3  4
      // pixel in memory:  BB GG RR xx (so it looks in registers 0x xxRRGGBB)
      // little endian

      uint8_t blue = x + blue_offset;
      uint8_t green = y + green_offset;

      // 0x 00 00 00 00 -> 0x xx rr gg bb
      // | composites bytes
      // green << 8 - shifts by 8 bits
      // other stay 00
      // * dereference pixel
      // pixel++ - pointer arithmetic - jumps by it's size (32 bits in this case)
      *pixel++ = (green << 8) | blue;
    }

    row += bitmap_buffer->pitch;
  }
}

// DIB - device independant section
static void win32_resize_dib_section(win32_bitmap_buffer* bitmap_buffer, int width, int height) {

  if (bitmap_buffer->memory) {
    VirtualFree(bitmap_buffer->memory, 0, MEM_RELEASE);
  }

  bitmap_buffer->width = width;
  bitmap_buffer->height = height;
  bitmap_buffer->bytes_per_pixel = 4;
  bitmap_buffer->pitch = bitmap_buffer->bytes_per_pixel * width;

  bitmap_buffer->info.bmiHeader.biSize = sizeof(bitmap_buffer->info.bmiHeader);
  bitmap_buffer->info.bmiHeader.biWidth = width;
  bitmap_buffer->info.bmiHeader.biHeight = -height; // draw left-to-right, top-to-bottom
  bitmap_buffer->info.bmiHeader.biPlanes = 1;
  bitmap_buffer->info.bmiHeader.biBitCount = 32;
  bitmap_buffer->info.bmiHeader.biCompression = BI_RGB;

  // we are taking 4 bytes (8 bits for each color (rgb) + aligment 8 bits = 32 bits) for each pixel
  auto bitmap_memory_size = width * height * bitmap_buffer->bytes_per_pixel;
  // virtual alloc allocates region of page which, one page size is 1 mb? GetSystemInfo can output that info
  bitmap_buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

inline static void win32_display_buffer_to_window(win32_bitmap_buffer* bitmap_buffer, HDC deviceContext, int window_width, int window_height, int x, int y, int width, int height) {
  // StretchDIBits(deviceContext, x, y, width, height, x, y, width, height, Global_BitmapMemory, &Global_BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  StretchDIBits(deviceContext, 0, 0, window_width, window_height, 0, 0, bitmap_buffer->width, bitmap_buffer->height, bitmap_buffer->memory, &bitmap_buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK win32_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
  case WM_KEYUP:
  case WM_KEYDOWN:
  case WM_SYSKEYUP:
  case WM_SYSKEYDOWN: {
    auto vk_key = wParam;
    auto previous_state = lParam & (1 << 30); // will return 0 or bit 30
    bool was_down = lParam & (1 << 30) != 0; // if I get 0 I get true if I get something besides zero I compare it to zero and I will get false
    bool is_down = lParam & (1 << 31) == 0;

    if (vk_key == 'W') {
    } else if (vk_key == 'S') {
    } else if (vk_key == 'A') {
    } else if (vk_key == 'D') {
    } else if (vk_key == VK_UP) {
    } else if (vk_key == VK_LEFT) {
    } else if (vk_key == VK_DOWN) {
    } else if (vk_key == VK_RIGHT) {
    } else if (vk_key == VK_ESCAPE) {
    } else if (vk_key == VK_SPACE) {
    }
  } break;
  case WM_SIZE: {
  } break;
    
  case WM_DESTROY: {
    Global_GameRunning = false;
  } break;
    
  case WM_CLOSE: {
    Global_GameRunning = false;
  } break;
    
  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;
    
  case WM_PAINT: {
    PAINTSTRUCT paint;
    auto deviceContext = BeginPaint(window, &paint);

    auto x = paint.rcPaint.left;
    auto y = paint.rcPaint.top;
    auto width = paint.rcPaint.right - paint.rcPaint.left;
    auto height = paint.rcPaint.bottom - paint.rcPaint.top;

    auto dimensions = get_window_dimensions(window);
    win32_display_buffer_to_window(&Global_backbuffer, deviceContext, dimensions.width, dimensions.height, x, y, width, height);
    EndPaint(window, &paint);
  } break;
    
  default: {
    result = DefWindowProc(window, message, wParam, lParam);
  } break;
  }

  return result;
}

int main(HINSTANCE currentInstance, HINSTANCE previousInstance, LPSTR commandLineParams, int nothing) {

  auto xinput_ready = win32_load_xinput();

  WNDCLASSA window_class = {};

  win32_resize_dib_section(&Global_backbuffer, 1280, 720);

  window_class.style = CS_HREDRAW | CS_HREDRAW; // redraw full window (vertical/horizontal) (streches)
  window_class.lpfnWndProc = win32_window_proc;
  window_class.hInstance = currentInstance;
  //WindowClass.hIcon = 0;
  window_class.lpszClassName = "GG";

  if (RegisterClass(&window_class) != 0)
    OutputDebugStringA("RegisterClass failed");

  auto windowHandle = CreateWindowEx(0, window_class.lpszClassName, "GG", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, currentInstance, 0);

  if (windowHandle == 0)
    OutputDebugStringA("CreateWindow failed");

  MSG message;
  BOOL windowMessage;
  HDC deviceContext;
  RECT clientRect;
  int height, width, x_offset = 0, y_offset = 0;

  while (Global_GameRunning) {
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {

      if (message.message == WM_QUIT) Global_GameRunning = false;

      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    // managing controller
    if (xinput_ready) {
      for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	// Simply get the state of the controller from XInput.
	auto dwResult = XInputGetState(i, &state);

	if (dwResult == ERROR_SUCCESS) {
	  // Controller is connected
	  auto* pad = &state.Gamepad;

	  // button is enabled if pad->wButton dword (32 bits - 4 bytes) and (&) with some bytes (0x0001 0x0002 ..)
	  auto button_up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
	  auto button_down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
	  auto button_left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
	  auto button_right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
	  auto button_start = pad->wButtons & XINPUT_GAMEPAD_START;
	  auto button_back = pad->wButtons & XINPUT_GAMEPAD_BACK;
	  auto button_left_thumb  = pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
	  auto button_right_thumb = pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
	  auto button_left_shoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
	  auto button_right_shoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
	  auto button_a = pad->wButtons & XINPUT_GAMEPAD_A;
	  auto button_b = pad->wButtons & XINPUT_GAMEPAD_B;
	  auto button_x = pad->wButtons & XINPUT_GAMEPAD_X;
	  auto button_y = pad->wButtons & XINPUT_GAMEPAD_Y;

	  auto stick_x = pad->sThumbLX;
	  auto stick_y = pad->sThumbLY;

	  if (button_up) y_offset++;
	  XINPUT_VIBRATION vibration;
	  vibration.wLeftMotorSpeed = 60000;
	  vibration.wRightMotorSpeed = 60000;
	  XInputSetState(i, &vibration); 
	}
	else {
	  // Controller is not connected
	}
      }
    }
    
    render_255_gradient(&Global_backbuffer, x_offset++, y_offset);
      
    deviceContext = GetDC(windowHandle);
    
    auto dimensions = get_window_dimensions(windowHandle);
    win32_display_buffer_to_window(&Global_backbuffer, deviceContext, dimensions.width, dimensions.height, 0, 0, dimensions.width, dimensions.height);
    
    ReleaseDC(windowHandle, deviceContext);
  }
  
  return 0;
}
