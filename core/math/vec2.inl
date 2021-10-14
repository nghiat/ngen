//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/vec2.h"

#include <math.h>

inline V2 operator+(const V2& v1, const V2& v2) {
  return {v1.x + v2.x, v1.y + v2.y};
}

inline V2 operator-(const V2& v1, const V2& v2) {
  return {v1.x - v2.x, v1.y - v2.y};
}

inline V2 operator*(const V2& v1, const V2& v2) {
  return {v1.x * v2.x, v1.y * v2.y};
}

inline V2 operator/(const V2& v1, const V2& v2) {
  return {v1.x / v2.x, v1.y / v2.y};
}

inline V2 operator*(const V2& v, F32 f) {
  return {v.x * f, v.y * f};
}

inline V2 operator/(const V2& v, F32 f) {
  return {v.x / f, v.y / f};
}

inline V2& operator+=(V2& v1, const V2& v2) {
  v1 = v1 + v2;
  return v1;
}

inline V2& operator-=(V2& v1, const V2& v2) {
  v1 = v1 - v2;
  return v1;
}

inline V2& operator*=(V2& v1, const V2& v2) {
  v1 = v1 * v2;
  return v1;
}

inline V2& operator/=(V2& v1, const V2& v2) {
  v1 = v1 / v2;
  return v1;
}

inline V2& operator*=(V2& v, F32 f) {
  v = v * f;
  return v;
}

inline V2& operator/=(V2& v, F32 f) {
  v = v / f;
  return v;
}

inline F32 v2_dot(const V2& v1, const V2& v2) {
  return v1.x * v2.x + v1.y * v2.y;
}

inline V2 v2_normalize(const V2& v) {
  F32 len = sqrt(v.x * v.x + v.x * v.x);
  return v / len;
}
