//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"

// Maximum number of traces.
#define MAX_TRACES 64

// Maximum char for each trace.
// Hopfully we don't use more than |gc_max_symbol_length| characters which means
// we have to use heap allocation.
#define MAX_SYMBOL_LENGTH 1024

// Maximum char for all traces.
#define MAX_STACK_TRACE_LENGTH (MAX_TRACES * MAX_SYMBOL_LENGTH)

#if OS_WIN()
#define DEBUG_BREAK_DEBUGGER() __debugbreak()
#elif OS_LINUX()
#define DEBUG_BREAK_DEBUGGER() __asm__("int $3")
#else
#error "?"
#endif

bool debug_init();
void debug_get_stack_trace(char* buffer, int len);
bool debug_is_debugger_attached();
