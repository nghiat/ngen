//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_allocators.h"

#include "core/free_list_allocator.h"
#include "core/linear_allocator.h"

static Linear_allocator_t<> g_persistent_allocator_("persistent_allocator");
static Free_list_allocator_t g_general_allocator_("general_allocator", 10 * 1024 * 1024);

Allocator_t* g_persistent_allocator = &g_persistent_allocator_;
Allocator_t* g_general_allocator = &g_general_allocator_;

bool core_allocators_init() {
  bool rv = true;
  rv &= g_general_allocator_.init();
  return rv;
}

void core_allocators_destroy() {
  g_persistent_allocator_.destroy();
  g_general_allocator_.destroy();
}
