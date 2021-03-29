#include <windows.h>


int main(HINSTANCE currentInstance, HINSTANCE previousInstance, LPSTR commandLineParams, int nothing) {

  WNDCLASS WindowClass = {};

  WindowClass.style = 0x0020; // CS_OWNDC
  WindowClass.lpfnWndProc = 0;
  WindowClass.hInstance = currentInstance;
  //WindowClass.hIcon = 0;
  WindowClass.lpszClassName = "GG";

  /*
  typedef struct tagWNDCLASSA {
    UINT      style;
    WNDPROC   lpfnWndProc;
    int       cbClsExtra;
    int       cbWndExtra;
    HINSTANCE hInstance;
    HICON     hIcon;
    HCURSOR   hCursor;
    HBRUSH    hbrBackground;
    LPCSTR    lpszMenuName;
    LPCSTR    lpszClassName;
  } WNDCLASSA, *PWNDCLASSA, *NPWNDCLASSA, *LPWNDCLASSA;
  */

  MessageBoxA(0, "Test", "GG", MB_OK|MB_ICONINFORMATION);
  return 0;
}
