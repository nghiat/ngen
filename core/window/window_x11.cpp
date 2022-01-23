//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/window/window.h"

#include "core/allocator.h"
#include "core/log.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <stdlib.h>
#include <string.h>

static E_key g_xcb_key_code_to_key_[e_key_count];
static E_mouse g_xcb_button_to_mouse_[e_mouse_count];

static void init_input_(ngWindow* w) {
  // key
  static_assert (XKB_KEY_NoSymbol == e_key_none && e_key_none == 0, "g_xcb_key_code_to_key_ is default initialized to 0s");
  int key_to_xkb[e_key_count] = {};

  key_to_xkb[e_key_a] = XKB_KEY_A;
  key_to_xkb[e_key_d] = XKB_KEY_D;
  key_to_xkb[e_key_s] = XKB_KEY_S;
  key_to_xkb[e_key_w] = XKB_KEY_W;
  key_to_xkb[e_key_below_esc] = XKB_KEY_grave;

  for (int i = e_key_none + 1; i < e_key_count; i++) {
    E_key code = (E_key)i;
    xcb_keycode_t* kc = xcb_key_symbols_get_keycode(w->m_platform_data.key_symbols, key_to_xkb[i]);
    if (kc) {
      g_xcb_key_code_to_key_[*kc] = code;
      free(kc);
    } else {
      M_logf("xcb_keycode_t* for key code %d is NULL", code);
    }
  }

  // mouse
  static_assert(e_mouse_none == 0, "g_xcb_button_to_mouse_ is default initialized to 0s");
  g_xcb_button_to_mouse_[XCB_BUTTON_INDEX_1] = e_mouse_left;
  g_xcb_button_to_mouse_[XCB_BUTTON_INDEX_2] = e_mouse_right;
  g_xcb_button_to_mouse_[XCB_BUTTON_INDEX_3] = e_mouse_middle;
}

static void update_mouse_val_(ngWindow* w, E_mouse mouse, int x, int y, bool is_down) {
  w->m_mouse_down[mouse] = is_down;
  w->on_mouse_event(mouse, x, y, is_down);
  w->m_old_mouse_x[mouse] = x;
  w->m_old_mouse_y[mouse] = y;
}

bool ngWindow::init() {
  Display* xdisplay = XOpenDisplay(0);
  M_check_log_return_val(xdisplay, false, "XOpenDisplay failed");
  xcb_connection_t* xcb_connection = XGetXCBConnection(xdisplay);
  M_check_log_return_val(xcb_connection, false, "XGetXCBConnection failed");
  XSetEventQueueOwner(xdisplay, XCBOwnsEventQueue);
  xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(xcb_connection)).data;

  U32 xcb_window_id = xcb_generate_id(xcb_connection);
  U32 value_mask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
  U32 event_mask = XCB_EVENT_MASK_BUTTON_1_MOTION | XCB_EVENT_MASK_BUTTON_2_MOTION |
                     XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
                     XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE;
  U32 colormap = xcb_generate_id(xcb_connection);
  xcb_create_colormap(xcb_connection, XCB_COLORMAP_ALLOC_NONE, colormap, screen->root, screen->root_visual);
  uint32_t value_list[3] = {event_mask, colormap, 0};
  xcb_create_window(xcb_connection, XCB_COPY_FROM_PARENT, xcb_window_id, screen->root, 0, 0, m_width, m_height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);
  xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window_id, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(m_title), m_title);
  xcb_intern_atom_cookie_t cookie = xcb_intern_atom(xcb_connection, 1, 12, "WM_PROTOCOLS");
  xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(xcb_connection, cookie, 0);

  xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(xcb_connection, 0, 16, "WM_DELETE_WINDOW");
  xcb_intern_atom_reply_t* reply2 = xcb_intern_atom_reply(xcb_connection, cookie2, 0);

  xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window_id, (*reply).atom, 4, 32, 1, &(*reply2).atom);
  xcb_map_window(xcb_connection, xcb_window_id);

  xcb_flush(xcb_connection);
  xcb_key_symbols_t* key_symbols = xcb_key_symbols_alloc(xcb_connection);
  m_platform_data.xdisplay = xdisplay;
  m_platform_data.xcb_connection = xcb_connection;
  m_platform_data.reply2 = reply2;
  m_platform_data.xcb_window_id = xcb_window_id;
  m_platform_data.key_symbols = key_symbols;
  init_input_(this);
  return true;
}

void ngWindow::destroy() {
  xcb_destroy_window(m_platform_data.xcb_connection, m_platform_data.xcb_window_id);
  xcb_key_symbols_free(m_platform_data.key_symbols);
}

void ngWindow::os_loop() {
  bool running = true;
  while (running) {
    xcb_generic_event_t* event;
    while ((event = xcb_poll_for_event(m_platform_data.xcb_connection))) {
      if (event) {
        switch (event->response_type & ~0x80) {
        case XCB_BUTTON_PRESS: {
          xcb_button_press_event_t* bp = (xcb_button_press_event_t*)event;
          update_mouse_val_(this, g_xcb_button_to_mouse_[bp->detail], bp->event_x, bp->event_y, true);
        } break;
        case XCB_BUTTON_RELEASE: {
          xcb_button_release_event_t* br = (xcb_button_release_event_t*)event;
          update_mouse_val_(this, g_xcb_button_to_mouse_[br->detail], br->event_x, br->event_y, false);
        } break;
        case XCB_KEY_PRESS: {
          xcb_key_press_event_t* kp = (xcb_key_press_event_t*)event;
          E_key code = g_xcb_key_code_to_key_[kp->detail];
          m_key_down[code] = true;
          this->on_key_event(code, true);
        } break;
        case XCB_KEY_RELEASE: {
          xcb_key_release_event_t* kr = (xcb_key_release_event_t*)event;
          E_key code = g_xcb_key_code_to_key_[kr->detail];
          m_key_down[code] = true;
          this->on_key_event(code, false);
        } break;
        case XCB_MOTION_NOTIFY: {
          xcb_motion_notify_event_t* m = (xcb_motion_notify_event_t*)event;
          this->on_mouse_move(m->event_x, m->event_y);
          if (m_is_cursor_visible) {
            for (int i = 0; i < e_mouse_count; ++i) {
              if (m_mouse_down[i]) {
                m_old_mouse_x[i] = m->event_x;
                m_old_mouse_y[i] = m->event_y;
              }
            }
          }
        } break;
        case XCB_CLIENT_MESSAGE:
          if ((*(xcb_client_message_event_t*)event).data.data32[0] == m_platform_data.reply2->atom)
            running = false;
          break;
        default:
          break;
        }
        free(event);
      }
    }
    this->loop();
  }
}
