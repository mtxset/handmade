#include <windows.h>

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
  case WM_SIZE: {
    OutputDebugStringA("WM_SIZE\n");
  } break;
  case WM_DESTROY: {
    OutputDebugStringA("WM_DESTROY\n");
  } break;
  case WM_CLOSE: {
    OutputDebugStringA("WM_CLOSE\n");
  } break;
  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;
  case WM_PAINT: {
    PAINTSTRUCT paint;
    auto deviceContext = BeginPaint(window, &paint);
    auto x = paint.rcPaint.left;
    auto y = paint.rcPaint.top;
    auto height = paint.rcPaint.bottom - paint.rcPaint.top;
    auto width = paint.rcPaint.right - paint.rcPaint.left;

    static auto operation = WHITENESS;
    PatBlt(deviceContext, x, y, width, height, operation);
    if (operation == WHITENESS)
      operation = BLACKNESS;
    else
      operation = WHITENESS;
    
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
  windowClass.lpfnWndProc = WindowProc;
  windowClass.hInstance = currentInstance;
  //WindowClass.hIcon = 0;
  windowClass.lpszClassName = "GG";

  if (RegisterClass(&windowClass) != 0)
    OutputDebugStringA("RegisterClass failed");
  
  auto windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "GG", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, currentInstance, 0);

  if (windowHandle == 0)
    OutputDebugStringA("CreateWindow failed");

  MSG message;
  while (true) {
    BOOL windowMessage = GetMessage(&message, 0, 0, 0);

    if (windowMessage < 0)
      break;

    TranslateMessage(&message);
    DispatchMessage(&message);
  }
  
  return 0;
}
