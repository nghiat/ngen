//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/window/window.h"

#include "core/allocator.h"
#include "core/log.h"

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include <stdlib.h>
#include <string.h>

struct WindowPlatform {
  Display* xdisplay;
  xcb_connection_t* xcb_connection;
  xcb_intern_atom_reply_t* reply2;
  uint32_t xcb_window_id;
  xcb_key_symbols_t* key_symbols;
};

static EKey g_xcb_key_code_to_key[EKEY_COUNT];
static EMouse g_xcb_button_to_mouse[EMOUSE_COUNT];

static void init_input(xjWindow* w) {
  // key
  static_assert (XKB_KEY_NoSymbol == EKEY_NONE && EKEY_NONE == 0, "g_xcb_key_code_to_key is default initialized to 0s");
  int key_to_xkb[EKEY_COUNT] = {};

  key_to_xkb[EKEY_A] = XKB_KEY_A;
  key_to_xkb[EKEY_D] = XKB_KEY_D;
  key_to_xkb[EKEY_S] = XKB_KEY_S;
  key_to_xkb[EKEY_W] = XKB_KEY_W;
  key_to_xkb[EKEY_BELOW_ESC] = XKB_KEY_grave;

  for (int i = KEY_NONE + 1; i < KEY_COUNT; i++) {
    EKey code = (EKey)i;
    xcb_keycode_t* kc = xcb_key_symbols_get_keycode(w->platform_data->key_symbols, key_to_xkb[i]);
    if (kc) {
      g_xcb_key_code_to_key[*kc] = code;
      free(kc);
    } else {
      LOGF("xcb_keycode_t* for key code %d is NULL", code);
    }
  }

  // mouse
  static_assert(EMOUSE_NONE == 0, "g_xcb_button_to_mouse is default initialized to 0s");
  g_xcb_button_to_mouse[XCB_BUTTON_INDEX_1] = EMOUSE_LEFT;
  g_xcb_button_to_mouse[XCB_BUTTON_INDEX_2] = EMOUSE_RIGHT;
  g_xcb_button_to_mouse[XCB_BUTTON_INDEX_3] = EMOUSE_MIDDLE;
}

static void update_mouse_val(xjWindow* w, EMouse mouse, int x, int y, bool is_down) {
  w->mouse_down[mouse] = is_down;
  w->on_mouse_event(mouse, x, y, is_down);
  w->old_mouse_x[mouse] = x;
  w->old_mouse_y[mouse] = y;
}

bool xjWindow::init() {
  Display* xdisplay = XOpenDisplay(0);
  CHECK_LOG_RETURN_VAL(xdisplay, false, "XOpenDisplay failed");
  xcb_connection_t* xcb_connection = XGetXCBConnection(xdisplay);
  CHECK_LOG_RETURN_VAL(xcb_connection, false, "XGetXCBConnection failed");
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
  xcb_create_window(xcb_connection, XCB_COPY_FROM_PARENT, xcb_window_id, screen->root, 0, 0, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);
  xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window_id, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
  xcb_intern_atom_cookie_t cookie = xcb_intern_atom(xcb_connection, 1, 12, "WM_PROTOCOLS");
  xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(xcb_connection, cookie, 0);

  xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(xcb_connection, 0, 16, "WM_DELETE_WINDOW");
  xcb_intern_atom_reply_t* reply2 = xcb_intern_atom_reply(xcb_connection, cookie2, 0);

  xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window_id, (*reply).atom, 4, 32, 1, &(*reply2).atom);
  xcb_map_window(xcb_connection, xcb_window_id);

  xcb_flush(xcb_connection);
  xcb_key_symbols_t* key_symbols = xcb_key_symbols_alloc(xcb_connection);
  platform_data = (WindowPlatform*)allocator->alloc(sizeof(WindowPlatform));
  platform_data->xdisplay = xdisplay;
  platform_data->xcb_connection = xcb_connection;
  platform_data->reply2 = reply2;
  platform_data->xcb_window_id = xcb_window_id;
  platform_data->key_symbols = key_symbols;
  init_input(this);
  return true;
}

void xjWindow::destroy() {
  xcb_destroy_window(platform_data->xcb_connection, platform_data->xcb_window_id);
  xcb_key_symbols_free(platform_data->key_symbols);
  allocator->free(platform_data);
}

void xjWindow::os_loop() {
  bool running = true;
  while (running) {
    xcb_generic_event_t* event;
    while ((event = xcb_poll_for_event(platform_data->xcb_connection))) {
      if (event) {
        switch (event->response_type & ~0x80) {
        case XCB_BUTTON_PRESS: {
          xcb_button_press_event_t* bp = (xcb_button_press_event_t*)event;
          update_mouse_val(this, g_xcb_button_to_mouse[bp->detail], bp->event_x, bp->event_y, true);
        } break;
        case XCB_BUTTON_RELEASE: {
          xcb_button_release_event_t* br = (xcb_button_release_event_t*)event;
          update_mouse_val(this, g_xcb_button_to_mouse[br->detail], br->event_x, br->event_y, false);
        } break;
        case XCB_KEY_PRESS: {
          xcb_key_press_event_t* kp = (xcb_key_press_event_t*)event;
          EKey code = g_xcb_key_code_to_key[kp->detail];
          key_down[code] = true;
          this->on_key_event(code, true);
        } break;
        case XCB_KEY_RELEASE: {
          xcb_key_release_event_t* kr = (xcb_key_release_event_t*)event;
          EKey code = g_xcb_key_code_to_key[kr->detail];
          key_down[code] = true;
          this->on_key_event(code, false);
        } break;
        case XCB_MOTION_NOTIFY: {
          xcb_motion_notify_event_t* m = (xcb_motion_notify_event_t*)event;
          this->on_mouse_move(m->event_x, m->event_y);
          if (w->is_cursor_visible) {
            for (int i = 0; i < EMOUSE_COUNT; ++i) {
              if (mouse_down[i]) {
                old_mouse_x[i] = m->event_x;
                old_mouse_y[i] = m->event_y;
              }
            }
          }
        } break;
        case XCB_CLIENT_MESSAGE:
          if ((*(xcb_client_message_event_t*)event).data.data32[0] == (*platform_data->reply2).atom)
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
