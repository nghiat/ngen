//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/quat.h"

#include "core/math/float.h"
#include "core/math/mat4.inl"
#include "core/math/vec3.inl"

#include <math.h>

// res * q = 1
inline Quat_t quat_inverse(const Quat_t& q) {
  F32 sum = q.a * q.a + q.b * q.b + q.c * q.c + q.d * q.d;
  return {q.a / sum, -q.b / sum, -q.c / sum, -q.d / sum};
}

inline F32 quat_norm(const Quat_t& q) {
  return sqrtf(q.a * q.a + q.b * q.b + q.c * q.c + q.d * q.d);
}

inline Quat_t quat_normalize(const Quat_t& q) {
  F32 norm = quat_norm(q);
  return {q.a / norm, q.b / norm, q.c / norm, q.d / norm};
}

inline Quat_t quat_lerp(const Quat_t& q1, const Quat_t& q2, float v) {
  Quat_t q;
  q.v = vec4_lerp(q1.v, q2.v, v);
  return q;
}

inline Quat_t quat_lerp(const Quat_t& q1, const Quat_t& q2, float from, float to, float v) {
  Quat_t q;
  q.v = vec4_lerp(q1.v, q2.v, from, to, v);
  return q;
}

inline M4_t quat_to_m4(const Quat_t& q) {
  F32 a = q.a;
  F32 b = q.b;
  F32 c = q.c;
  F32 d = q.d;
  F32 b2 = q.b * q.b;
  F32 c2 = q.c * q.c;
  F32 d2 = q.d * q.d;
  return {V4_t{1 - 2*(c2 + d2), 2 * (b*c + a*d), 2*(b*d - a*c), 0.f},
          V4_t{2*(b*c - a*d), 1 - 2*(b2 + d2), 2*(c*d + a*b), 0.f},
          V4_t{2*(b*d + a*c), 2*(c*d - a*b), 1 - 2*(b2 + c2), 0.f},
          V4_t{0.f, 0.f, 0.f, 1.f}};
}

inline Quat_t quat_from_m4(const M4_t& m) {
  Quat_t q;
  float a = m.m[0][0] + m.m[1][1] + m.m[2][2]; // 4a^2 - 1
  float b = m.m[0][0] - m.m[1][1] - m.m[2][2]; // 4b^2 - 1
  float c = m.m[1][1] - m.m[0][0] - m.m[2][2]; // 4c^2 - 1
  float d = m.m[2][2] - m.m[0][0] - m.m[1][1]; // 4d^2 - 1
  float max = a;
  if (b > max) {
    max = b;
  }
  if (c > max) {
    max = c;
  }
  if (d > max) {
    max = d;
  }
  if(max == a) {
    float s = sqrt(a + 1.f)/2;
    float o = 0.25f/s;
    q.a = s;
    q.b = (m.m[1][2] - m.m[2][1])*o;
    q.c = (m.m[2][0] - m.m[0][2])*o;
    q.d = (m.m[0][1] - m.m[1][0])*o;
  } else if (max == b) {
    float s = sqrtf(b + 1.f)/2;
    float o = 0.25/s;
    q.a = (m.m[1][2] - m.m[2][1])*o;
    q.b = s;
    q.c = (m.m[1][0] + m.m[0][1])*o;
    q.d = (m.m[2][0] + m.m[0][2])*o;
  } else if (max == c) {
    float s = sqrtf(c + 1.f)/2;
    float o = 0.25f/s;
    q.a = (m.m[2][0] - m.m[0][2])*o;
    q.b = (m.m[1][0] + m.m[0][1])*o;
    q.c = s;
    q.d = (m.m[2][1] + m.m[1][2])*o;
  } else {
    float s = sqrtf(d + 1.0f)/2;
    float o = 0.25/s;
    q.a = (m.m[0][1] - m.m[1][0])*o;
    q.b = (m.m[2][0] + m.m[0][2])*o;
    q.c = (m.m[2][1] + m.m[1][2])*o;
    q.d = s;
  }
  return q;
}

// https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Proof_of_the_quaternion_rotation_identity
// q = cos(angle/2) + v*sin(angle/2)
inline Quat_t quat_rotate_v3(const V3_t& v, F32 angle) {
  F32 cos_half = cosf(angle / 2);
  F32 sin_half = sinf(angle / 2);
  return {cos_half, sin_half * v.x, sin_half * v.y, sin_half * v.z};
}
