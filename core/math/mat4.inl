//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"

#include "core/math/vec4.inl"
#include "core/utils.h"

#include <string.h>

inline M4_t m4_identity() {
  M4_t m;
  m.m[0][0] = 1.0f;
  m.m[1][1] = 1.0f;
  m.m[2][2] = 1.0f;
  m.m[3][3] = 1.0f;
  return m;
}

inline M4_t m4_transpose(const M4_t& m) {
  M4_t rv = m;
  for (int i = 1; i < 4; ++i) {
    for (int j = 0; j < i; ++j) {
      swap(&rv.m[i][j], &rv.m[j][i]);
    }
  }
  return rv;
}

inline V4_t operator*(const M4_t& m, const V4_t& v) {
  V4_t result;
  for (int i = 0; i < 4; ++i) {
    result.a[i] = v4_dot(m.v[i], v);
  }
  return result;
}

inline M4_t operator*(const M4_t& m1, const M4_t& m2) {
  M4_t result;
  // Multiply |each row of m1| with m2.
  for (int i = 0; i < 4; ++i) {
    // Each row of result is the sum of products of m1 row with every m2 row.
    for (int j = 0; j < 4; ++j) {
      for (int k = 0; k < 4; ++k) {
        result.m[i][k] += m1.m[i][j] * m2.m[j][k];
      }
    }
  }
  return result;
}

inline bool operator==(const M4_t& m1, const M4_t& m2) {
  return !memcmp(m1.a, m2.a, sizeof(M4_t));
}

inline M4_t& operator*=(M4_t& m1, const M4_t& m2) {
  m1 = m1 * m2;
  return m1;
}
