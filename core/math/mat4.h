//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/vec4.h"

// Row-major
struct M4 {
  union {
    V4 v[4] = {};
    F32 a[4][4];
  };
};

M4 m4_identity();

V4 operator*(const M4& m, const V4& v);
M4 operator*(const M4& m1, const M4& m2);
bool operator==(const M4& m1, const M4& m2);

M4& operator*=(M4& m1, const M4& m2);
