//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include <math.h>

inline bool float_equal(float a, float b) {
  return fabs(a - b) < M_epsilon_f;
}

inline bool float_equal_0(float a) {
  return fabs(a) < M_epsilon_f;
}

inline float degree_to_rad(float deg) {
  return deg / 180.0f * M_pi_f;
}

inline float max(float a, float b) {
  if (a > b) {
    return a;
  }
  return b;
}
