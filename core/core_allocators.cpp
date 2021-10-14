//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/core_allocators.h"

#include "core/free_list_allocator.h"
#include "core/linear_allocator.h"

static LinearAllocator<> g_internal_persistent_allocator("persistent_allocator");
static FreeListAllocator g_internal_general_allocator("general_allocator", 10 * 1024 * 1024);

ngAllocator* g_persistent_allocator = &g_internal_persistent_allocator;
ngAllocator* g_general_allocator = &g_internal_general_allocator;

bool core_allocators_init() {
  bool rv = true;
  rv &= g_internal_persistent_allocator.la_init();
  rv &= g_internal_general_allocator.fla_init();
  return rv;
}
