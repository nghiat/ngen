//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"
#include "core/math/vec3.h"

M4_t look_forward_lh(const V3_t& eye, const V3_t& forward, const V3_t& up);
M4_t look_at_lh(const V3_t& eye, const V3_t& target, const V3_t& up);
M4_t perspective(F32 fovy, F32 aspect, F32 z_near, F32 z_far);
M4_t rotate(const V3_t& axis, F32 angle);
M4_t scale(F32 sx, F32 sy, F32 sz);
M4_t translate(const V3_t& v);

#include "core/math/transform.inl"
