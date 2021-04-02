#include <windows.h>
// https://youtu.be/GAi_nTx1zG8?t=4344

static auto Global_GameRunning = true;

static BITMAPINFO Global_BitmapInfo;
static void *Global_BitmapMemory;
static HBITMAP Global_BitmapHandle;
static HDC Global_BitmapDeviceContext;

// DIB - device independant section
static void Win32ResizeDIBSection(int width, int height) {

  if (Global_BitmapHandle)
    DeleteObject(Global_BitmapHandle);

  if (!Global_BitmapDeviceContext)
    Global_BitmapDeviceContext = CreateCompatibleDC(0);

  Global_BitmapInfo.bmiHeader.biSize = sizeof(Global_BitmapInfo.bmiHeader);
  Global_BitmapInfo.bmiHeader.biWidth = width;
  Global_BitmapInfo.bmiHeader.biHeight = height;
  Global_BitmapInfo.bmiHeader.biPlanes = 1;
  Global_BitmapInfo.bmiHeader.biBitCount = 32;
  Global_BitmapInfo.bmiHeader.biCompression = BI_RGB;

  Global_BitmapHandle = CreateDIBSection(Global_BitmapDeviceContext, &Global_BitmapInfo, DIB_RGB_COLORS, &Global_BitmapMemory, 0, 0);
}

static void Win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height) {
  StretchDIBits(deviceContext, x, y, width, height, x, y, width, height, Global_BitmapMemory, &Global_BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
    
    Win32UpdateWindow(deviceContext, x, y, width, height);
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
  while (Global_GameRunning) {
    windowMessage = GetMessage(&message, 0, 0, 0);

    if (windowMessage < 0)
      break;

    TranslateMessage(&message);
    DispatchMessage(&message);
  }
  
  return 0;
}
