//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/vec4.h"

// Row-major
struct M4_t {
  union {
    V4_t v[4] = {};
    F32 a[4][4];
  };
};

M4_t m4_identity();

V4_t operator*(const M4_t& m, const V4_t& v);
M4_t operator*(const M4_t& m1, const M4_t& m2);
bool operator==(const M4_t& m1, const M4_t& m2);

M4_t& operator*=(M4_t& m1, const M4_t& m2);
