#include "../Header/winproc.h"
#include "../Header/app.h"
#include "../Header/keyboard.h"
#include "../Header/mouse.h"
#include "../Header/render.h"
#include "../Header/search.h"
#include "../Header/scroll.h"
#include "../Header/main.h"

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  // Pastikan uFindReplaceMsg tidak bernilai 0 (sudah diinisialisasi)
  if (g_searchState.uFindReplaceMsg != 0 &&
      message == g_searchState.uFindReplaceMsg)
  {
    AppState *appState = App_GetState(hWnd);
    if (appState)
    {
      Search_HandleMessage(hWnd, appState, lParam);
    }
    return 0;
  }

  switch (message)
  {
  case WM_CREATE:
    return App_OnCreate(hWnd);
  case WM_CLOSE:
    return App_OnClose(hWnd);
  case WM_DESTROY:
    return App_OnDestroy(hWnd);

  case WM_SETFOCUS:
    return App_OnSetFocus(hWnd);
  case WM_KILLFOCUS:
    return App_OnKillFocus(hWnd);
  case WM_TIMER:
    return App_OnTimer(hWnd, wParam);

  case WM_SIZE:
    {
      AppState *s = App_GetState(hWnd);
      if (s)
      {
          if (s->wordWrapEnabled)
          {
              RECT rc;
              GetClientRect(hWnd, &rc);
              int clientWidth = rc.right - rc.left;
              s->textBuffer.wrapCols = CalcWrapCols(clientWidth, s->charWidth);
              Buffer_ReflowAll(&s->textBuffer);
          }
          Scroll_UpdateScrollbars(hWnd);
          Scroll_EnsureCursorVisible(hWnd);
      }
      else
      {
          Scroll_UpdateScrollbars(hWnd);
      }
      return 0;
    }

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
  case WM_LBUTTONDBLCLK:
    return Mouse_OnLButtonDblClk(hWnd, wParam, lParam);

  case WM_VSCROLL:
    Scroll_OnVerticalScroll(hWnd, wParam);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;

  case WM_HSCROLL:
    Scroll_OnHorizontalScroll(hWnd, wParam);
    InvalidateRect(hWnd, NULL, FALSE);
    return 0;

  case WM_COMMAND:
    return App_OnCommand(hWnd, wParam, lParam);

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
}