#include "../Header/winproc.h"
#include "../Header/app.h"
#include "../Header/keyboard.h"
#include "../Header/mouse.h"
#include "../Header/render.h"

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
  case WM_CREATE:
    return App_OnCreate(hWnd);
  case WM_DESTROY:
    return App_OnDestroy(hWnd);

  case WM_SETFOCUS:
    return App_OnSetFocus(hWnd);
  case WM_KILLFOCUS:
    return App_OnKillFocus(hWnd);
  case WM_TIMER:
    return App_OnTimer(hWnd, wParam);

  case WM_PAINT:
    return Render_OnPaint(hWnd);

  case WM_CHAR:
    return Keyboard_OnChar(hWnd, wParam, lParam);
  case WM_KEYDOWN:
    return Keyboard_OnKeyDown(hWnd, wParam, lParam);

  case WM_LBUTTONDOWN:
    return Mouse_OnLButtonDown(hWnd, wParam, lParam);
  case WM_MOUSEWHEEL:
    return Mouse_OnMouseWheel(hWnd, wParam, lParam);
  case WM_MOUSEMOVE:
    return Mouse_OnMouseMove(hWnd, wParam, lParam);
  case WM_LBUTTONUP:
    return Mouse_OnLButtonUp(hWnd, wParam, lParam);

  case WM_COMMAND:
    return App_OnCommand(hWnd, wParam, lParam);

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}