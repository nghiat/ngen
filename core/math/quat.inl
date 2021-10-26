//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/quat.h"

#include "core/math/mat4.inl"
#include "core/math/vec3.inl"

#include <math.h>

// res * q = 1
Quat quat_inverse(const Quat& q) {
  F32 sum = q.a * q.a + q.b * q.b + q.c * q.c + q.d * q.d;
  return {q.a / sum, -q.b / sum, -q.c / sum, -q.d / sum};
}

F32 quat_norm(const Quat& q) {
  return sqrtf(q.a * q.a + q.b * q.b + q.c * q.c + q.d * q.d);
}

Quat quat_normalize(const Quat& q) {
  F32 norm = quat_norm(q);
  return {q.a / norm, q.b / norm, q.c / norm, q.d / norm};
}

M4 quat_to_m4(const Quat& q) {
  F32 a = q.a;
  F32 b = q.b;
  F32 c = q.c;
  F32 d = q.d;
  F32 a2 = q.a * q.a;
  F32 b2 = q.b * q.b;
  F32 c2 = q.c * q.c;
  F32 d2 = q.d * q.d;
  return {V4{a2 + b2 - c2 - d2, 2 * (b * c + a * d), 2 * (b * d - a * c), 0.f},
          V4{2 * (b * c - a * d), a2 - b2 + c2 - d2, 2 * (c * d + a * b), 0.f},
          V4{2 * (b * d + a * c), 2 * (c * d - a * b), a2 - b2 - c2 + d2, 0.f},
          V4{0.f, 0.f, 0.f, 1.f}};
}

// https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Proof_of_the_quaternion_rotation_identity
// q = cos(angle/2) + v*sin(angle/2)
Quat quat_rotate_v3(const V3& v, F32 angle) {
  F32 cos_half = cosf(angle / 2);
  F32 sin_half = sinf(angle / 2);
  return {cos_half, sin_half * v.x, sin_half * v.y, sin_half * v.z};
}
