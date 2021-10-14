//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/window/window.h"

#include "core/allocator.h"
#include "core/log.h"

#include <Windows.h>
#include <windowsx.h>

#include <string.h>

#define WIN_KEY_MAX 256

static EKey g_vk_to_key[WIN_KEY_MAX];

static void init_key_codes_map() {
  static_assert(EKEY_NONE == 0, "g_vk_to_key is default initialized to 0s");
  g_vk_to_key[0x41] = EKEY_A;
  g_vk_to_key[0x44] = EKEY_D;
  g_vk_to_key[0x53] = EKEY_S;
  g_vk_to_key[0x57] = EKEY_W;
  g_vk_to_key[VK_OEM_3] = EKEY_BELOW_ESC;
}

static void update_mouse_val(ngWindow* w, LPARAM l_param, EMouse mouse, bool is_down) {
  int x = GET_X_LPARAM(l_param);
  int y = GET_Y_LPARAM(l_param);
  w->m_mouse_down[mouse] = is_down;
  w->on_mouse_event(mouse, x, y, is_down);
  w->m_old_mouse_x[mouse] = x;
  w->m_old_mouse_y[mouse] = y;
}

static LRESULT CALLBACK wnd_proc(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM w_param, _In_ LPARAM l_param) {
  ngWindow* w = (ngWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (msg) {
  case WM_CLOSE:
    DestroyWindow(hwnd);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_KEYUP:
  case WM_KEYDOWN:
    w->m_key_down[g_vk_to_key[w_param]] = msg == WM_KEYDOWN;
    w->on_key_event(g_vk_to_key[w_param], msg == WM_KEYDOWN);
    break;
  case WM_CHAR:
    w->on_char_event((wchar_t)w_param);
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
    update_mouse_val(w, l_param, EMOUSE_LEFT, msg == WM_LBUTTONDOWN);
    break;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
    update_mouse_val(w, l_param, EMOUSE_MIDDLE, msg == WM_MBUTTONDOWN);
    break;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
    update_mouse_val(w, l_param, EMOUSE_RIGHT, msg == WM_MBUTTONDOWN);
    break;
  case WM_MOUSEMOVE: {
    int x = GET_X_LPARAM(l_param);
    int y = GET_Y_LPARAM(l_param);
    w->on_mouse_move(x, y);
    if (w->m_is_cursor_visible) {
      for (int i = 0; i < EMOUSE_COUNT; ++i) {
        w->m_old_mouse_x[i] = x;
        w->m_old_mouse_y[i] = y;
      }
    }
  } break;
  default:
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
  return 0;
}

bool ngWindow::init() {
  HINSTANCE hinstance;
  WNDCLASSEX wcex;
  HWND hwnd;
  init_key_codes_map();
  hinstance = GetModuleHandle(NULL);
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_OWNDC;
  wcex.lpfnWndProc = &wnd_proc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hinstance;
  wcex.hIcon = LoadIcon(hinstance, MAKEINTRESOURCE(IDI_APPLICATION));
  wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = NULL;
  wcex.lpszClassName = m_title;
  wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

  CHECK_LOG_RETURN_VAL(RegisterClassEx(&wcex), false, "Can't register WNDCLASSEX");
  hwnd = CreateWindow(m_title, m_title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height, NULL, NULL, hinstance, NULL);
  CHECK_LOG_RETURN_VAL(hwnd, false, "Can't create HWND");
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
  ShowWindow(hwnd, SW_SHOWNORMAL);
  UpdateWindow(hwnd);
  m_platform_data.hwnd = hwnd;
  return true;
}

void ngWindow::destroy() {
  DestroyWindow(m_platform_data.hwnd);
}

void ngWindow::os_loop() {
  MSG msg;
  while (true) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT)
        break;
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    } else {
      loop();
    }
  }
}

void ngWindow::show_cursor(bool show) {
  ShowCursor(show);
  this->m_is_cursor_visible = show;
  if (show) {
    ClipCursor(NULL);
  }
  else {
    RECT rect;
    GetClientRect(m_platform_data.hwnd, &rect);
    POINT p1 = {rect.left, rect.top};
    POINT p2 = {rect.right, rect.bottom};
    ClientToScreen(m_platform_data.hwnd, &p1);
    ClientToScreen(m_platform_data.hwnd, &p2);
    SetRect(&rect, p1.x, p1.y, p2.x, p2.y);
    ClipCursor(&rect);
  }
}

void ngWindow::set_cursor_pos(int x, int y) {
  POINT p{x, y};
  ClientToScreen(m_platform_data.hwnd, &p);
  SetCursorPos(p.x, p.y);
}
