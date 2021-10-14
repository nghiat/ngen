//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/mat4.h"
#include "core/math/vec3.h"

struct ngQuat {
  F32 a;
  F32 b;
  F32 c;
  F32 d;
};

ngQuat quat_inverse(const ngQuat& q);
F32 quat_norm(const ngQuat& q);
ngQuat quat_normalize(const ngQuat& q);
M4 quat_to_m4(const ngQuat& q);
ngQuat quat_rotate_v3(const V3& v, F32 angle);
