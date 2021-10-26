//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"
#include "core/math/vec3.h"
#include "core/window/input.h"

class ngWindow;

class ngCam {
public:
  bool cam_init(const V3& eye, const V3& target, ngWindow* w);
  bool cam_update();
  void cam_mouse_move(int x, int y);
  void cam_mouse_event(E_mouse mouse, int x, int y, bool is_down);

  M4 m_view_mat;
  V3 m_eye;
  V3 m_forward;
  V3 m_up;

  ngWindow* m_w;
  F32 m_dist;
  bool m_is_mouse_down;
};
