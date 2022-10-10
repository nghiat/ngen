//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"

struct V3_t;

struct V4_t {
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

V4_t V3o_v4(const V3_t& v, F32 w);

V4_t operator+(const V4_t& v1, const V4_t& v2);
V4_t operator-(const V4_t& v1, const V4_t& v2);
V4_t operator*(const V4_t& v, F32 f);
V4_t operator/(const V4_t& v, F32 f);

V4_t& operator+=(V4_t& v1, const V4_t& v2);
V4_t& operator-=(V4_t& v1, const V4_t& v2);
V4_t& operator*=(V4_t& v, F32 f);
V4_t& operator/=(V4_t& v, F32 f);

bool operator==(const V4_t& v1, const V4_t& v2);

F32 vec4_dot(const V4_t& lhs, const V4_t& rhs);
F32 vec4_len(const V4_t& v);
V4_t vec4_normalize(const V4_t& v);
V4_t vec4_lerp(const V4_t& v1, const V4_t& v2, float f);
V4_t vec4_lerp(const V4_t& v1, const V4_t& v2, float from, float to, float f);

#include "core/math/vec4.inl"
