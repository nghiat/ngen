//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"
#include "core/math/vec3.h"
#include "core/window/input.h"

class Window_t;

class Cam_t {
public:
  bool init(const V3_t& eye, const V3_t& target, Window_t* w);
  bool update();
  void mouse_move(int x, int y);
  void mouse_event(E_mouse mouse, int x, int y, bool is_down);

  M4_t m_view_mat;
  V3_t m_eye;
  V3_t m_forward;
  V3_t m_up;

  Window_t* m_w;
  F32 m_dist;
  bool m_is_mouse_down;
};
