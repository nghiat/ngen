//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"
#include "core/math/vec3.h"

struct Quat_t {
  union {
    struct {
      F32 a;
      F32 b;
      F32 c;
      F32 d;
    };
    V4_t v = {};
  };
};

Quat_t quat_inverse(const Quat_t& q);
F32 quat_norm(const Quat_t& q);
Quat_t quat_normalize(const Quat_t& q);
Quat_t quat_lerp(const Quat_t& q1, const Quat_t& q2, float v);
Quat_t quat_lerp(const Quat_t& q1, const Quat_t& q2, float from, float to, float v);
M4_t quat_to_m4(const Quat_t& q);
Quat_t quat_from_m4(const M4_t& m);
Quat_t quat_rotate_v3(const V3_t& v, F32 angle);
