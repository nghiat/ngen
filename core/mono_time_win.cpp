//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/mono_time.h"

#include <Windows.h>

static LARGE_INTEGER g_performance_freq_;

bool mono_time_init() {
  return QueryPerformanceFrequency(&g_performance_freq_);
}

S64 mono_time_from_s(F64 s) {
  return s * g_performance_freq_.QuadPart;
}

S64 mono_time_now() {
  LARGE_INTEGER pc;
  QueryPerformanceCounter(&pc);
  return pc.QuadPart;
}

F64 mono_time_to_s(S64 t) {
  return (F64)t / g_performance_freq_.QuadPart;
}

F64 mono_time_to_ms(S64 t) {
  return mono_time_to_s(t) * 1000;
}

F64 mono_time_to_us(S64 t) {
  return mono_time_to_s(t) * 1000000;
}
