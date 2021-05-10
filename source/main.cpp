#include <windows.h>
#include <stdint.h>
// https://www.youtube.com/watch?v=J3y1x54vyIQ

static auto Global_GameRunning = true;

struct Win32_bitmap_buffer {
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

static Win32_bitmap_buffer Global_backbuffer;

static HBITMAP Global_BitmapHandle;
static HDC Global_BitmapDeviceContext;

// pointer aliasing

win32_window_dimensions get_window_dimensions(HWND window) {

  win32_window_dimensions result;
  
  RECT clientRect;
  GetClientRect(window, &clientRect);
  
  result.height = clientRect.bottom - clientRect.top;
  result.width = clientRect.right - clientRect.left;

  return result;
}
						    
static void render_255_gradient(Win32_bitmap_buffer bitmap_buffer, int blue_offset, int green_offset) {

  auto row = (uint8_t*)bitmap_buffer.memory;
  
  for (int y = 0; y < bitmap_buffer.height; y++) {
    auto pixel = (uint32_t*)row;
    for (int x = 0; x < bitmap_buffer.width; x++) {
      // pixel bytes       1  2  3  4
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
    
    row += bitmap_buffer.pitch;
  }
}

// DIB - device independant section
static void Win32ResizeDIBSection(Win32_bitmap_buffer *bitmap_buffer, int width, int height) {

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
  bitmap_buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

inline static void Win32_display_buffer_to_window(Win32_bitmap_buffer bitmap_buffer, HDC deviceContext, int window_width, int window_height, int x, int y, int width, int height) {
  // StretchDIBits(deviceContext, x, y, width, height, x, y, width, height, Global_BitmapMemory, &Global_BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  StretchDIBits(deviceContext, 0, 0, window_width, window_height, 0, 0, bitmap_buffer.width, bitmap_buffer.height, bitmap_buffer.memory, &bitmap_buffer.info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
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
    Win32_display_buffer_to_window(Global_backbuffer, deviceContext, dimensions.width, dimensions.height, x, y, width, height);
    EndPaint(window, &paint);
  } break;
  default: {
    result = DefWindowProc(window, message, wParam, lParam);
  } break;
  }

  return result;
}

int main(HINSTANCE currentInstance, HINSTANCE previousInstance, LPSTR commandLineParams, int nothing) {

  WNDCLASS windowClass = {};

  Win32ResizeDIBSection(&Global_backbuffer, 1280, 720);

  windowClass.style = CS_HREDRAW | CS_HREDRAW; // redraw full window (vertical/horizontal) (streches)
  windowClass.lpfnWndProc = Win32WindowProc;
  windowClass.hInstance = currentInstance;
  //WindowClass.hIcon = 0;
  windowClass.lpszClassName = "GG";

  if (RegisterClass(&windowClass) != 0)
    OutputDebugStringA("RegisterClass failed");
  
  auto windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "GG", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, currentInstance, 0);

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
    
    render_255_gradient(Global_backbuffer, x_offset++, y_offset++);
      
    deviceContext = GetDC(windowHandle);
    
    auto dimensions = get_window_dimensions(windowHandle);
    Win32_display_buffer_to_window(Global_backbuffer, deviceContext, dimensions.width, dimensions.height, 0, 0, dimensions.width, dimensions.height);
    
    ReleaseDC(windowHandle, deviceContext);
  }
  
  return 0;
}
