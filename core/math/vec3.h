//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

struct V4;

struct V3 {
  V3& operator=(const V4& v);

  union {
    struct {
      F32 x;
      F32 y;
      F32 z;
    };
    F32 a[3] = {};
  };
};

V3 operator+(const V3& v1, const V3& v2);
V3 operator-(const V3& v1, const V3& v2);
V3 operator*(const V3& v, F32 f);
V3 operator/(const V3& v, F32 f);

V3& operator+=(V3& v1, const V3& v2);
V3& operator-=(V3& v1, const V3& v2);
V3& operator*=(V3& v, F32 f);
V3& operator/=(V3& v, F32 f);

bool operator==(const V3& v1, const V3& v2);

V3 v3_cross(const V3& v1, const V3& v2);
F32 v3_dot(const V3& v1, const V3& v2);
F32 v3_len(const V3& v);
V3 v3_normalize(const V3& v);
