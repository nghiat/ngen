//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

struct V4_t;

struct V3_t {
  V3_t& operator=(const V4_t& v);

  union {
    struct {
      F32 x;
      F32 y;
      F32 z;
    };
    F32 a[3] = {};
  };
};

V3_t operator+(const V3_t& v1, const V3_t& v2);
V3_t operator-(const V3_t& v1, const V3_t& v2);
V3_t operator*(const V3_t& v, F32 f);
V3_t operator/(const V3_t& v, F32 f);

V3_t& operator+=(V3_t& v1, const V3_t& v2);
V3_t& operator-=(V3_t& v1, const V3_t& v2);
V3_t& operator*=(V3_t& v, F32 f);
V3_t& operator/=(V3_t& v, F32 f);

bool operator==(const V3_t& v1, const V3_t& v2);

V3_t v3_cross(const V3_t& v1, const V3_t& v2);
F32 v3_dot(const V3_t& v1, const V3_t& v2);
F32 v3_len(const V3_t& v);
V3_t v3_normalize(const V3_t& v);
