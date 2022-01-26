//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"

#include "core/math/vec4.inl"

#include <string.h>

inline M4_t m4_identity() {
  M4_t m;
  m.a[0][0] = 1.0f;
  m.a[1][1] = 1.0f;
  m.a[2][2] = 1.0f;
  m.a[3][3] = 1.0f;
  return m;
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
        result.a[i][k] += m1.a[i][j] * m2.a[j][k];
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
