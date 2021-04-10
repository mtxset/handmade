#include <windows.h>
#include <stdint.h>
// https://www.youtube.com/watch?v=w7ay7QXmo_o

static auto Global_GameRunning = true;

static BITMAPINFO Global_BitmapInfo;
static void *Global_BitmapMemory;
static HBITMAP Global_BitmapHandle;
static HDC Global_BitmapDeviceContext;

static int Global_Bitmap_Width;
static int Global_Bitmap_Height;
static int Global_Bytes_Per_Pixel = 4;

static void render_255_gradient(int x_offset, int y_offset) {
  auto pitch = Global_Bitmap_Width * Global_Bytes_Per_Pixel;
  auto row = (uint8_t*)Global_BitmapMemory;
  
  for (int y = 0; y < Global_Bitmap_Height; y++) {
    auto pixel = (uint32_t*)row;
    for (int x = 0; x < Global_Bitmap_Width; x++) {
      // pixel bytes       1  2  3  4
      // pixel in memory:  BB GG RR xx (so it looks in registers 0x xxRRGGBB)
      // little endian
      
      uint8_t blue = x + x_offset;
      uint8_t green = y + y_offset;

      // 0x 00 00 00 00 -> 0x xx rr gg bb
      // | composites bytes
      // green << 8 shifts by 8 bits
      // other stay 00
      // * dereference pixel
      // pixel++ - pointer arithmetic - jumps by it's size (32 bits in this case)
      *pixel++ = (green << 8) | blue;
    }
    
    row += pitch;
  }
}

// DIB - device independant section
static void Win32ResizeDIBSection(int width, int height) {

  if (Global_BitmapMemory) {
    VirtualFree(Global_BitmapMemory, 0, MEM_RELEASE);
  }

  Global_Bitmap_Width = width;
  Global_Bitmap_Height = height;

  Global_BitmapInfo.bmiHeader.biSize = sizeof(Global_BitmapInfo.bmiHeader);
  Global_BitmapInfo.bmiHeader.biWidth = width;
  Global_BitmapInfo.bmiHeader.biHeight = -height; // so we draw left-to-right, top-to-bottom
  Global_BitmapInfo.bmiHeader.biPlanes = 1;
  Global_BitmapInfo.bmiHeader.biBitCount = 32;
  Global_BitmapInfo.bmiHeader.biCompression = BI_RGB;

  // we are taking 4 bytes (8 bits for each color (rgb) + aligment 8 bits = 32 bits) for each pixel
  auto bitmap_memory_size = width * height * Global_Bytes_Per_Pixel;
  Global_BitmapMemory = VirtualAlloc(0, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
}

static void Win32UpdateWindow(HDC deviceContext, RECT *window_rect, int x, int y, int width, int height) {
  int window_width = window_rect->right - window_rect->left;
  int window_height = window_rect->bottom - window_rect->top;
  // StretchDIBits(deviceContext, x, y, width, height, x, y, width, height, Global_BitmapMemory, &Global_BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  StretchDIBits(deviceContext, 0, 0, Global_Bitmap_Width, Global_Bitmap_Height, 0, 0, window_width, window_height, Global_BitmapMemory, &Global_BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
  case WM_SIZE: {
    RECT clientRect;
    GetClientRect(window, &clientRect);
    
    auto height = clientRect.bottom - clientRect.top;
    auto width = clientRect.right - clientRect.left;
    Win32ResizeDIBSection(width, height);
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

    RECT clientRect;
    GetClientRect(window, &clientRect);
    
    Win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
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

  windowClass.style = CS_OWNDC | CS_HREDRAW | CS_HREDRAW;
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
    
    render_255_gradient(x_offset++, y_offset++);
      
    deviceContext = GetDC(windowHandle);
    GetClientRect(windowHandle, &clientRect);

    height = clientRect.bottom - clientRect.top;
    width = clientRect.right - clientRect.left;
    
    Win32UpdateWindow(deviceContext, &clientRect, 0, 0, width, height);
    ReleaseDC(windowHandle, deviceContext);
  }
  
  return 0;
}
