//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/transform.h"

#include "core/math/vec3.inl"

#include <math.h>

inline M4 look_forward_lh(const V3& eye, const V3& forward, const V3& up) {
  V3 z_axis = v3_normalize(forward);
  V3 x_axis = v3_normalize(v3_cross(z_axis, up));
  V3 y_axis = v3_cross(x_axis, z_axis);

  // M * [x_asix, 0] = [1, 0, 0, 0]
  // M * [y_asix, 0] = [0, 1, 0, 0]
  // M * [z_asix, 0] = [0, 0, 1, 0]
  // M * [eye, 1] = [0, 0, 0, 1]
  return {V4{x_axis.x, x_axis.y, x_axis.z, -v3_dot(x_axis, eye)},
          V4{y_axis.x, y_axis.y, y_axis.z, -v3_dot(y_axis, eye)},
          V4{z_axis.x, z_axis.y, z_axis.z, -v3_dot(z_axis, eye)},
          V4{0.0f, 0.0f, 0.0f, 1.0f}};
}

inline M4 look_at_lh(const V3& eye, const V3& target, const V3& up) {
  return look_forward_lh(eye, eye - target, up);
}

// Left-handed.
// Near plane -> z = 0, far plane -> z = 1
inline M4 perspective(F32 fovy, F32 aspect, F32 z_near, F32 z_far) {
  M4 result;
  result.a[0][0] = 1 / tanf(fovy / 2);
  result.a[1][1] = result.a[0][0] * aspect;
  result.a[2][2] = -z_far / (z_near - z_far);
  result.a[2][3] = z_near*z_far / (z_near - z_far);
  result.a[3][2] = 1.f;
  return result;
}

// Split vector v that is being rotated into othorgonal vectors v1 and v2 (v1 lies on this axis).
// v' is the rotated vector and v' =  v1 + v2'
// v2' = v * cos(a) + u * sin(a) (u = cross(axis, v))
// Finally we have: v' = cos(a)*v + (1-cos(a))(dot(v, axis) * axis) + sin(a) * cross(axis, v)
inline M4 rotate(const V3& axis, F32 angle) {
  V3 n = v3_normalize(axis);
  F32 x = n.x;
  F32 y = n.y;
  F32 z = n.z;
  F32 c = cosf(angle);
  F32 s = sinf(angle);
  return {V4{c + x * x * (1 - c), y * x * (1 - c) + z * s, z * x * (1 - c) - y * s, 0.f},
          V4{x * y * (1 - c) - z * s, c + y * y * (1 - c), z * y * (1 - c) + x * s, 0.f},
          V4{x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, c + z * z * (1 - c), 0.f},
          V4{0.f, 0.f, 0.f, 1.f}};
}

inline M4 scale(F32 sx, F32 sy, F32 sz) {
  M4 m;
  m.a[0][0] = sx;
  m.a[1][1] = sy;
  m.a[2][2] = sz;
  m.a[3][3] = 1.f;
  return m;
}

inline M4 translate(const V3& v) {
  return {V4{1.f, 0.f, 0.f, v.x},
          V4{0.f, 1.f, 0.f, v.y},
          V4{0.f, 0.f, 1.f, v.z},
          V4{0.f, 0.f, 0.f, 1.f}};
}
