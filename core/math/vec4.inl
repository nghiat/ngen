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

inline V4 V3o_v4(const V3& v, F32 w) {
  return V4{v.x, v.y, v.z, w};
}

inline V4 operator+(const V4& v1, const V4& v2) {
  return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

inline V4 operator-(const V4& v1, const V4& v2) {
  return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

inline V4 operator*(const V4& v, F32 f) {
  return {v.x * f, v.y * f, v.z * f, v.w * f};
}

inline V4 operator/(const V4& v, F32 f) {
  return {v.x / f, v.y / f, v.z / f, v.w / f};
}

inline V4& operator+=(V4& v1, const V4& v2) {
  v1 = v1 + v2;
  return v1;
}

inline V4& operator-=(V4& v1, const V4& v2) {
  v1 = v1 - v2;
  return v1;
}

inline V4& operator*=(V4& v, F32 f) {
  v = v * f;
  return v;
}

inline V4& operator/=(V4& v, F32 f) {
  v = v / f;
  return v;
}

inline bool operator==(const V4& v1, const V4& v2) {
  return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
}

inline F32 v4_dot(const V4& v1, const V4& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

inline F32 v4_len(const V4& v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline V4 v4_normalize(const V4& v) {
  return v / v4_len(v);
}
