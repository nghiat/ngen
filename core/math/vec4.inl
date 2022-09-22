//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/vec4.h"

#include "core/math/vec3.h"

#include <math.h>
#include <string.h>

inline V4_t V3o_v4(const V3_t& v, F32 w) {
  return V4_t{v.x, v.y, v.z, w};
}

inline V4_t operator+(const V4_t& v1, const V4_t& v2) {
  return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

inline V4_t operator-(const V4_t& v1, const V4_t& v2) {
  return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

inline V4_t operator*(const V4_t& v, F32 f) {
  return {v.x * f, v.y * f, v.z * f, v.w * f};
}

inline V4_t operator/(const V4_t& v, F32 f) {
  return {v.x / f, v.y / f, v.z / f, v.w / f};
}

inline V4_t& operator+=(V4_t& v1, const V4_t& v2) {
  v1 = v1 + v2;
  return v1;
}

inline V4_t& operator-=(V4_t& v1, const V4_t& v2) {
  v1 = v1 - v2;
  return v1;
}

inline V4_t& operator*=(V4_t& v, F32 f) {
  v = v * f;
  return v;
}

inline V4_t& operator/=(V4_t& v, F32 f) {
  v = v / f;
  return v;
}

inline bool operator==(const V4_t& v1, const V4_t& v2) {
  return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
}

inline F32 v4_dot(const V4_t& v1, const V4_t& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

inline F32 v4_len(const V4_t& v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline V4_t v4_normalize(const V4_t& v) {
  return v / v4_len(v);
}

inline V4_t vec4_lerp(const V4_t& v1, const V4_t& v2, float f) {
  return v1*(1 - f) + v2*f;
}

inline V4_t vec4_lerp(const V4_t& v1, const V4_t& v2, float from, float to, float f) {
  return vec4_lerp(v1, v2, (f - from)/(to - from));
}
