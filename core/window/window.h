//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#ifndef CORE_WINDOW_WINDOW_H
#define CORE_WINDOW_WINDOW_H

#include "core/os_string.h"
#include "core/os.h"
#include "core/window/input.h"
#include "core/windows_lite.h"

struct ngAllocator;

#if OS_WIN()
struct WindowPlatform {
  HWND hwnd;
};
#endif

class ngWindow {
public:
  ngWindow(const OSChar* title, int width, int height) : m_title(title), m_width(width), m_height(height) {}
  bool init();
  virtual void destroy();

  void os_loop();
  void show_cursor(bool show);
  void set_cursor_pos(int x, int y);
  virtual void loop() {}
  virtual void on_mouse_event(EMouse mouse, int x, int y, bool is_down) {}
  virtual void on_mouse_move(int x, int y) {}
  virtual void on_key_event(EKey key, bool is_down) {}
  virtual void on_char_event(wchar_t c) {}

  bool m_key_down[EKEY_COUNT] = {};
  bool m_mouse_down[EMOUSE_COUNT] = {};
  int m_old_mouse_x[EMOUSE_COUNT] = {};
  int m_old_mouse_y[EMOUSE_COUNT] = {};
  bool m_is_cursor_visible = true;

  const OSChar* m_title;
  int m_width;
  int m_height;

  WindowPlatform m_platform_data;
};

#endif // CORE_WINDOW_WINDOW_H
