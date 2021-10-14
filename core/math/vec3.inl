//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/vec3.h"

#include "core/math/vec4.h"

#include <math.h>

inline V3& V3::operator=(const V4& v) {
  x = v.x;
  y = v.y;
  z = v.z;
  return *this;
}

inline V3 operator+(const V3& v1, const V3& v2) {
  return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
}

inline V3 operator-(const V3& v1, const V3& v2) {
  return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
}

inline V3 operator*(const V3& v, F32 f) {
  return {v.x * f, v.y * f, v.z * f};
}

inline V3 operator/(const V3& v, F32 f) {
  return {v.x / f, v.y / f, v.z / f};
}

inline V3& operator+=(V3& v1, const V3& v2) {
  v1 = v1 + v2;
  return v1;
}

inline V3& operator-=(V3& v1, const V3& v2) {
  v1 = v1 - v2;
  return v1;
}

inline V3& operator*=(V3& v, F32 f) {
  v = v * f;
  return v;
}

inline V3& operator/=(V3& v, F32 f) {
  v = v / f;
  return v;
}

inline bool operator==(const V3& v1, const V3& v2) {
  return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

inline V3 v3_cross(const V3& v1, const V3& v2) {
  return {v1.y * v2.z - v1.z * v2.y,
          v1.z * v2.x - v1.x * v2.z,
          v1.x * v2.y - v1.y * v2.x};
}

inline F32 v3_dot(const V3& v1, const V3& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline F32 v3_len(const V3& v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline V3 v3_normalize(const V3& v) {
  return v / v3_len(v);
}
