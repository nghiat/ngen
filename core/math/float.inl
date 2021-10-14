//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/float.h"

#include <math.h>

inline bool float_equal(float a, float b) {
  return fabs(a - b) < NG_EPSILON_F;
}

inline bool float_equal_0(float a) {
  return fabs(a) < NG_EPSILON_F;
}

float degree_to_rad(float deg) {
  return deg / 180.0f * NG_PI_F;
}
