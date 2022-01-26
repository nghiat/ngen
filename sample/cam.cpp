//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "sample/cam.h"

#include "core/log.h"
#include "core/math/quat.inl"
#include "core/math/transform.inl"
#include "core/math/vec3.inl"
#include "core/window/window.h"

bool Cam_t::init(const V3_t& eye, const V3_t& target, Window_t* w) {
  m_w = w;
  m_eye = eye;
  m_forward = target - m_eye;
  m_dist = v3_len(m_forward);
  m_forward = v3_normalize(m_forward);
  m_up = {0.f, 1.f, 0.f};
  m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  return true;
}

bool Cam_t::update() {
  bool need_update_view = false;
  V3_t cam_right = v3_normalize(v3_cross(m_forward, m_up));
  if (m_w->m_key_down[e_key_w]) {
    V3_t forward = m_forward * 0.1f;
    m_eye += forward;
    need_update_view = true;
  }
  if (m_w->m_key_down[e_key_s]) {
    V3_t backward = m_forward * -0.1f;
    m_eye += backward;
    need_update_view = true;
  }
  if (m_w->m_key_down[e_key_d]) {
    V3_t right = cam_right * 0.1f;
    m_eye += right;
    need_update_view = true;
  }
  if (m_w->m_key_down[e_key_a]) {
    V3_t left = cam_right * -0.1f;
    m_eye += left;
    need_update_view = true;
  }
  if (need_update_view) {
    m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  }
  return need_update_view;
}

void Cam_t::mouse_move(int x, int y) {
  if (m_w->m_mouse_down[e_mouse_left]) {
    m_w->set_cursor_pos(m_w->m_old_mouse_x[e_mouse_left], m_w->m_old_mouse_y[e_mouse_left]);

    int delta_x = m_w->m_old_mouse_x[e_mouse_left] - x;
    int delta_y = m_w->m_old_mouse_y[e_mouse_left] - y;
    float angle_x = delta_x * 0.002f;
    float angle_y = delta_y * 0.002f;
    V3_t right = v3_cross(m_forward, m_up);
    M4_t rotate_vert = quat_to_m4(quat_rotate_v3(right, angle_y));
    M4_t rotate_hori = quat_to_m4(quat_rotate_v3(m_up, angle_x));
    M4_t rotate_mat = rotate_hori * rotate_vert;
    m_forward = v4_normalize(rotate_mat * (V4_t){m_forward.x, m_forward.y, m_forward.z, 1.f});
    m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  }

  if (m_w->m_mouse_down[e_mouse_middle]) {
    int delta_x = m_w->m_old_mouse_x[e_mouse_middle] - x;
    int delta_y = m_w->m_old_mouse_y[e_mouse_middle] - y;
    float angle_x = delta_x * 0.002f;
    float angle_y = delta_y * 0.002f;
    V3_t backward = m_forward * -1.0f;
    V3_t full_backward = backward * m_dist;
    V3_t target = m_eye - full_backward;
    V3_t right = v3_cross(backward, m_up);
    M4_t rotate_vert = quat_to_m4(quat_rotate_v3(right, angle_y));
    M4_t rotate_hori = quat_to_m4(quat_rotate_v3(m_up, -angle_x));
    M4_t rotate_mat = rotate_vert * rotate_hori;
    backward = rotate_mat * (V4_t){backward.x, backward.y, backward.z, 1.f};
    full_backward = backward * m_dist;
    m_eye = target + full_backward;
    m_forward = backward * -1.0f;
    m_view_mat = look_forward_lh(m_eye, m_forward, m_up);
  }
}

void Cam_t::mouse_event(E_mouse mouse, int x, int y, bool is_down) {
  if (mouse == e_mouse_left) {
    m_w->show_cursor(!is_down);
  }
}
