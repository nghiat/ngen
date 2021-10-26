//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"

// Maximum number of traces.
#define M_max_traces_ 64

// Maximum char for each trace.
// Hopfully we don't use more than |gc_max_symbol_length| characters which means
// we have to use heap allocation.
#define M_max_symbol_length_ 1024

// Maximum char for all traces.
#define M_max_stack_trace_length_ (M_max_traces_ * M_max_symbol_length_)

#if M_os_is_win()
#define M_debug_break_debugger() __debugbreak()
#elif M_os_is_linux()
#define M_debug_break_debugger() __asm__("int $3")
#else
#error "?"
#endif

bool debug_init();
void debug_get_stack_trace(char* buffer, int len);
bool debug_is_debugger_attached();
