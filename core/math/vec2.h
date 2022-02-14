//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"

struct V2_t {
  union {
    struct {
      F32 x;
      F32 y;
    };
    F32 a[2] = {};
  };
};

V2_t operator+(const V2_t& v1, const V2_t& v2);
V2_t operator-(const V2_t& v1, const V2_t& v2);
V2_t operator*(const V2_t& v1, const V2_t& v2);
V2_t operator/(const V2_t& v1, const V2_t& v2);

V2_t operator*(const V2_t& v, F32 f);
V2_t operator/(const V2_t& v, F32 f);

V2_t& operator+=(V2_t& v1, const V2_t& v2);
V2_t& operator-=(V2_t& v1, const V2_t& v2);
V2_t& operator*=(V2_t& v1, const V2_t& v2);
V2_t& operator/=(V2_t& v1, const V2_t& v2);

V2_t& operator*=(V2_t& v, F32 f);
V2_t& operator/=(V2_t& v, F32 f);

F32 v2_dot(const V2_t& v1, const V2_t& v2);
V2_t v2_normalize(const V2_t& v);
