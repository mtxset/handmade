#include <windows.h>
// https://youtu.be/GAi_nTx1zG8?t=1842
#define static_function static
#define static_global static
#define static_local static

static_global auto GameRunning = true;

// DIB - device independant section
static void Win32ResizeDIBSection(int width, int height) {
  
}

static void Win32UpdateWindow(HWND window, int x, int y, int width, int height) {
  
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
    GameRunning = false;
  } break;
  case WM_CLOSE: {
    GameRunning = false;
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
    
    Win32UpdateWindow(window, x, y, width, height);
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
  while (GameRunning) {
    windowMessage = GetMessage(&message, 0, 0, 0);

    if (windowMessage < 0)
      break;

    TranslateMessage(&message);
    DispatchMessage(&message);
  }
  
  return 0;
}
