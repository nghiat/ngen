//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

struct V2 {
  union {
    struct {
      F32 x;
      F32 y;
    };
    F32 a[2] = {};
  };
};

V2 operator+(const V2& v1, const V2& v2);
V2 operator-(const V2& v1, const V2& v2);
V2 operator*(const V2& v1, const V2& v2);
V2 operator/(const V2& v1, const V2& v2);

V2 operator*(const V2& v, F32 f);
V2 operator/(const V2& v, F32 f);

V2& operator+=(V2& v1, const V2& v2);
V2& operator-=(V2& v1, const V2& v2);
V2& operator*=(V2& v1, const V2& v2);
V2& operator/=(V2& v1, const V2& v2);

V2& operator*=(V2& v, F32 f);
V2& operator/=(V2& v, F32 f);

F32 v2_dot(const V2& v1, const V2& v2);
V2 v2_normalize(const V2& v);
