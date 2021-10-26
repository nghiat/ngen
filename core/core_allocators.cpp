//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/core_allocators.h"

#include "core/free_list_allocator.h"
#include "core/linear_allocator.h"

static Linear_allocator<> g_persistent_allocator_("persistent_allocator");
static FreeList_allocator g_general_allocator_("general_allocator", 10 * 1024 * 1024);

Allocator* g_persistent_allocator = &g_persistent_allocator_;
Allocator* g_general_allocator = &g_general_allocator_;

bool core_allocators_init() {
  bool rv = true;
  rv &= g_persistent_allocator_.la_init();
  rv &= g_general_allocator_.fla_init();
  return rv;
}
