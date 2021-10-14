//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

bool mono_time_init();
S64 mono_time_now();
S64 mono_time_from_s(F64 s);
F64 mono_time_to_s(S64 t);
F64 mono_time_to_ms(S64 t);
F64 mono_time_to_us(S64 t);
