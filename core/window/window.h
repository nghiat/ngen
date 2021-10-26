//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os_string.h"
#include "core/os.h"
#include "core/window/input.h"
#include "core/windows_lite.h"

struct Allocator;

#if M_os_is_win()
struct Platform_data_ {
  HWND hwnd;
};
#endif

class ngWindow {
public:
  ngWindow(const Os_char* title, int width, int height) : m_title(title), m_width(width), m_height(height) {}
  bool init();
  virtual void destroy();

  void os_loop();
  void show_cursor(bool show);
  void set_cursor_pos(int x, int y);
  virtual void loop() {}
  virtual void on_mouse_event(E_mouse mouse, int x, int y, bool is_down) {}
  virtual void on_mouse_move(int x, int y) {}
  virtual void on_key_event(E_key key, bool is_down) {}
  virtual void on_char_event(wchar_t c) {}

  bool m_key_down[e_key_count] = {};
  bool m_mouse_down[e_mouse_count] = {};
  int m_old_mouse_x[e_mouse_count] = {};
  int m_old_mouse_y[e_mouse_count] = {};
  bool m_is_cursor_visible = true;

  const Os_char* m_title;
  int m_width;
  int m_height;

  Platform_data_ m_platform_data;
};
