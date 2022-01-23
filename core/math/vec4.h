//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

struct V3;

struct V4 {
  union {
    struct {
      F32 x;
      F32 y;
      F32 z;
      F32 w;
    };
    F32 a[4] = {};
  };
};

V4 V3o_v4(const V3& v, F32 w);

V4 operator+(const V4& v1, const V4& v2);
V4 operator-(const V4& v1, const V4& v2);
V4 operator*(const V4& v, F32 f);
V4 operator/(const V4& v, F32 f);

V4& operator+=(V4& v1, const V4& v2);
V4& operator-=(V4& v1, const V4& v2);
V4& operator*=(V4& v, F32 f);
V4& operator/=(V4& v, F32 f);

bool operator==(const V4& v1, const V4& v2);

F32 vec4_dot(const V4& lhs, const V4& rhs);
F32 vec4_len(const V4& v);
V4 vec4_normalize(const V4& v);
