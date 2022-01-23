//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/mono_time.h"

#include <time.h>

bool mono_time_init() { return true; }

S64 mono_time_now() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

S64 mono_time_from_s(F64 s) {
  return s * 1000000000.0;
}

F64 mono_time_to_s(S64 t) {
  return t / 1000000000.0;
}

F64 mono_time_to_ms(S64 t) {
  return t / 1000000.0;
}

F64 mono_time_to_us(S64 t) {
  return t / 1000.0;
}
