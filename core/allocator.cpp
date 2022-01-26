//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/allocator.h"

#include <string.h>

void* Allocator_t::alloc(Sip size) {
  return aligned_alloc(size, 16);
}

void* Allocator_t::alloc_zero(Sip size) {
  void* p = aligned_alloc(size, 16);
  memset(p, 0, size);
  return p;
}
