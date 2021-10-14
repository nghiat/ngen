//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "sample/cam.h"

#include "core/log.h"
#include "core/math/quat.inl"
#include "core/math/transform.inl"
#include "core/math/vec3.inl"
#include "core/window/window.h"

bool ngCam::cam_init(const V3& eye, const V3& target, ngWindow* w) {
  m_w = w;
  m_eye = eye;
  m_forward = target - m_eye;
  m_dist = v3_len(m_forward);
  m_forward = v3_normalize(m_forward);
  m_up = {0.f, 1.f, 0.f};
  m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  return true;
}

bool ngCam::cam_update() {
  bool need_update_view = false;
  V3 cam_right = v3_normalize(v3_cross(m_forward, m_up));
  if (m_w->m_key_down[EKEY_W]) {
    V3 forward = m_forward * 0.1f;
    m_eye += forward;
    need_update_view = true;
  }
  if (m_w->m_key_down[EKEY_S]) {
    V3 backward = m_forward * -0.1f;
    m_eye += backward;
    need_update_view = true;
  }
  if (m_w->m_key_down[EKEY_D]) {
    V3 right = cam_right * 0.1f;
    m_eye += right;
    need_update_view = true;
  }
  if (m_w->m_key_down[EKEY_A]) {
    V3 left = cam_right * -0.1f;
    m_eye += left;
    need_update_view = true;
  }
  if (need_update_view) {
    m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  }
  return need_update_view;
}

void ngCam::cam_mouse_move(int x, int y) {
  if (m_w->m_mouse_down[EMOUSE_LEFT]) {
    m_w->set_cursor_pos(m_w->m_old_mouse_x[EMOUSE_LEFT], m_w->m_old_mouse_y[EMOUSE_LEFT]);

    int delta_x = m_w->m_old_mouse_x[EMOUSE_LEFT] - x;
    int delta_y = m_w->m_old_mouse_y[EMOUSE_LEFT] - y;
    float angle_x = delta_x * 0.002f;
    float angle_y = delta_y * 0.002f;
    V3 right = v3_cross(m_forward, m_up);
    M4 rotate_vert = quat_to_m4(quat_rotate_v3(right, angle_y));
    M4 rotate_hori = quat_to_m4(quat_rotate_v3(m_up, angle_x));
    M4 rotate_mat = rotate_hori * rotate_vert;
    m_forward = v4_normalize(rotate_mat * (V4){m_forward.x, m_forward.y, m_forward.z, 1.f});
    m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  }

  if (m_w->m_mouse_down[EMOUSE_MIDDLE]) {
    int delta_x = m_w->m_old_mouse_x[EMOUSE_MIDDLE] - x;
    int delta_y = m_w->m_old_mouse_y[EMOUSE_MIDDLE] - y;
    float angle_x = delta_x * 0.002f;
    float angle_y = delta_y * 0.002f;
    V3 backward = m_forward * -1.0f;
    V3 full_backward = backward * m_dist;
    V3 target = m_eye - full_backward;
    V3 right = v3_cross(backward, m_up);
    M4 rotate_vert = quat_to_m4(quat_rotate_v3(right, angle_y));
    M4 rotate_hori = quat_to_m4(quat_rotate_v3(m_up, -angle_x));
    M4 rotate_mat = rotate_vert * rotate_hori;
    backward = rotate_mat * (V4){backward.x, backward.y, backward.z, 1.f};
    full_backward = backward * m_dist;
    m_eye = target + full_backward;
    m_forward = backward * -1.0f;
    m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  }
}

void ngCam::cam_mouse_event(EMouse mouse, int x, int y, bool is_down) {
  if (mouse == EMOUSE_LEFT) {
    m_w->show_cursor(!is_down);
  }
}
