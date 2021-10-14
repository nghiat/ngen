//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"
#include "core/math/vec3.h"

M4 look_forward_lh(const V3& eye, const V3& forward, const V3& up);
M4 look_at_lh(const V3& eye, const V3& target, const V3& up);
M4 perspective(F32 fovy, F32 aspect, F32 z_near, F32 z_far);
M4 rotate(const V3& axis, F32 angle);
M4 scale(F32 sx, F32 sy, F32 sz);
M4 translate(const V3& v);
